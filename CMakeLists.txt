# OSX
# cmake -B bin/osx
# make -C bin/osx -j8

# IOS
# cmake -B bin/ios -DIOS=1 -GXcode
# open bin/ios/program.xcodeproj

# IOS Intellisense
# cmake -B bin/ios-vsc -DIOS_VSC=1

# WebGL
# emcmake cmake -B bin/web -DRELEASE=1
# emmake make -C bin/web -j8

if (IOS OR IOS_VSC) # Set on command line -DIOS=1 or -DIOS_VSC=1 (for generating vs code compile-commands.json)
    set(CMAKE_SYSTEM_NAME "iOS")
    set(IOS 1)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
    set(CMAKE_OSX_SYSROOT "iphoneos" CACHE STRING "")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
    add_compile_options(-arch arm64)
    add_link_options(-arch arm64)
endif()

project("program" LANGUAGES C CXX OBJC OBJCXX)
cmake_minimum_required(VERSION 3.19.7, FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

if (APPLE AND NOT IOS)
    set(MACOS 1)
endif()

set(proj_dir ${CMAKE_CURRENT_SOURCE_DIR})
set(bin_dir ${CMAKE_CURRENT_BINARY_DIR})
set(src_dir ${proj_dir}/src)
file(RELATIVE_PATH relative_bin_dir "${proj_dir}/bin" "${bin_dir}")

include(initialize.cmake)

macro(checkvar var)
    if (NOT DEFINED "${var}")
        message(FATAL_ERROR "${var} not found")
    endif()
endmacro()
checkvar(OUT_NAME)
checkvar(GAME_SRC)
checkvar(BUNDLE_ID)

if (NOT DEFINED ENGINE_ROOT)
    set(ENGINE_ROOT "$ENV{HOME}/Projects/Galaxy-Engine")
endif()

include_directories(${ENGINE_ROOT}/include)
include_directories(${src_dir})
include_directories(${proj_dir}/include)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(RELEASE)
    add_compile_options(-O3)
    if (EMSCRIPTEN)
        add_link_options(-O1)
    else()
        add_link_options(-O3)
    endif()
    set(CMAKE_BUILD_TYPE Release)
else()
    if (EMSCRIPTEN) # WebGL doesnt support Og
        add_compile_options(-O3)
        add_link_options(-O1)
        add_link_options(-sASSERTIONS=1)
    else()
        add_compile_options(-Og)
        add_link_options(-Og)
    endif()
    set(CMAKE_BUILD_TYPE Debug)
    add_compile_definitions(DEBUG)
    add_compile_options(-g)
    add_link_options(-g)
endif()

# Remove comment to enable clang UBsan & Asan, works on WebGL
#add_link_options(-fsanitize=undefined,address -fno-omit-frame-pointer)
#add_compile_options(-fsanitize=undefined,address -fno-omit-frame-pointer)

add_compile_options(-Wall -pedantic -Wextra)
add_compile_options(-Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-unused-lambda-capture)

if (APPLE)
    # https://stackoverflow.com/questions/822404/how-to-set-up-cmake-to-build-an-app-for-the-iphone
    
    # The icon file name in the plist
    set(MACOSX_BUNDLE_ICON_FILE appIcon)
    set(CMAKE_MACOSX_BUNDLE 1)

    set(icon_path "${proj_dir}/assets/appIcon.png")
    if (EXISTS "${icon_path}")
        # Duplicates the appIcon into "Resources"
        set_source_files_properties(
            "${icon_path}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
        )
    endif()
    add_executable(game ${GAME_SRC} ${GAME_HEADERS} ${icon_path})
    set_target_properties(game PROPERTIES OUTPUT_NAME "${OUT_NAME}")

    macro(add_fw fwname)
        target_link_libraries(game "-framework ${fwname}")
    endmacro(add_fw)

    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${BUNDLE_ID})

    if (IOS)
        target_compile_definitions(game PUBLIC -Wno-shorten-64-to-32) # for Xcode
        set_target_properties(game PROPERTIES
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUNDLE_ID}"
            XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator"
            XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 11.0         # Version required for Galaxy Engine
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"            # iPhone, iPad
            XCODE_ATTRIBUTE_USE_HEADERMAP YES                       # Avoid header search paths
            XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES NO                 # For Archiving
            XCODE_ATTRIBUTE_OTHER_LDFLAGS ""                        # For Archiving
            XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"        # For Archiving
            XCODE_ATTRIBUTE_SKIP_INSTALL NO                         # For Archiving
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"   # For convenience
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${IOS_DEV_ID}"
            XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES                # Auto adds frameworks
        )
        # GLFM
        add_fw(UIKit)
        add_fw(CoreHaptics)
        add_fw(CoreMotion)
        add_fw(CoreGraphics)
        add_fw(MetalKit)
        add_fw(OpenGLES)
        add_fw(Foundation)
        add_fw(Metal)
        add_fw(QuartzCore)

        # Galaxy Engine
        add_fw(OpenAL)

        
        # LaunchScreen needed to allow any screen size. Don't overwrite. (GLFM)
        set(STORYBOARD_PATH "${bin_dir}/LaunchScreen.storyboard")
        if (NOT EXISTS "${STORYBOARD_PATH}")
            file(READ "$ENV{HOME}/Projects/cmake/cmake_launchscreen.txt" launchscreen)
            file(WRITE "${STORYBOARD_PATH}" "${launchscreen}")
        endif()
        set_source_files_properties("${STORYBOARD_PATH}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(game PUBLIC "${STORYBOARD_PATH}")

        # In place of MacOSXBundleInfo.plist.in (GLFM)
        file(READ "$ENV{HOME}/Projects/cmake/cmake_infoplist.txt" infoplist)
        string(REPLACE "MACOSX_BUNDLE_SHORT_VERSION_STRING" "1.0" infoplist "${infoplist}")
        string(REPLACE "MACOSX_BUNDLE_BUNDLE_VERSION" "1" infoplist "${infoplist}")
        file(WRITE "${bin_dir}/CMake-Info.plist.in" "${infoplist}")
        set_property(TARGET game PROPERTY MACOSX_BUNDLE_INFO_PLIST ${bin_dir}/CMake-Info.plist.in)
                

        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ENGINE_ROOT}/assets/" "${bin_dir}/galaxy_assets")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${proj_dir}/assets/" "${bin_dir}/assets")
        set_source_files_properties(
            "${bin_dir}/assets" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        set_source_files_properties(
            "${bin_dir}/galaxy_assets" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        # try this instead
        #set_source_files_properties("${proj_dir}/assets" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        #set_source_files_properties("${ENGINE_ROOT}/assets" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/engine")
        target_sources(game
            PUBLIC ${bin_dir}/galaxy_assets
            PUBLIC ${bin_dir}/assets
        )
    else()
        # GLFW
        add_fw(Cocoa)
        add_fw(IOKit)
        add_fw(OpenGL)

        # Engine
        add_fw(OpenAL)
        
        # TODO: Possibly replace these with the target_sources/set_source_files_properties combination

        # https://stackoverflow.com/questions/13429656/how-to-copy-contents-of-a-directory-into-build-directory-after-make-with-cmake
        add_custom_command(
            TARGET game
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${proj_dir}/assets/" "${bin_dir}/${OUT_NAME}.app/Contents/Resources/assets"
        )
        add_custom_command(
            TARGET game
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ENGINE_ROOT}/assets/" "${bin_dir}/${OUT_NAME}.app/Contents/Resources/galaxy_assets"
        )
    endif()
endif()
if (EMSCRIPTEN)

    # HACK: Make modifications to shell_minimal.html to take up the entire browser window
    file(READ ${EMSCRIPTEN_ROOT_PATH}/src/shell_minimal.html EMSCRIPTEN_SHELL_HTML)
    string(FIND "${EMSCRIPTEN_SHELL_HTML}" "<style>" HAS_STYLE)
    if (${HAS_STYLE} EQUAL -1)
        message(WARNING "<style> not found in shell_minimal.html, copying as-is")
    else()
        string(CONCAT STYLE_REPLACEMENT "<meta name=\"viewport\" content=\"width=device-width,user-scalable=no,viewport-fit=cover\">\n"
            "    <meta name=\"apple-mobile-web-app-capable\" content=\"yes\">\n"
            "    <link rel=\"apple-touch-icon\" href=\"appIcon.png\">\n"
            "    <style>\n"
            "      /* GLFM: Start changes */\n"
            "      :root {\n"
            "          --glfm-chrome-top-old: constant(safe-area-inset-top);\n"
            "          --glfm-chrome-right-old: constant(safe-area-inset-right);\n"
            "          --glfm-chrome-bottom-old: constant(safe-area-inset-bottom);\n"
            "          --glfm-chrome-left-old: constant(safe-area-inset-left);\n"
            "          --glfm-chrome-top: env(safe-area-inset-top);\n"
            "          --glfm-chrome-right: env(safe-area-inset-right);\n"
            "          --glfm-chrome-bottom: env(safe-area-inset-bottom);\n"
            "          --glfm-chrome-left: env(safe-area-inset-left);\n"
            "      }\n"
            "      body, html { border: 0px none; padding: 0px; margin: 0px; width: 100%; height: 100%; overflow: hidden; position: fixed; }\n"
            "      canvas.emscripten { background: black; width: 100%; height: 100%; }\n"
            "      .emscripten_border { width: 100%; height: 100%; border: 0px none !important;}\n"
            "      hr { display: none; }\n"
            "      /* GLFM: End changes */"
        )
        string(REPLACE "<style>" "${STYLE_REPLACEMENT}" EMSCRIPTEN_SHELL_HTML "${EMSCRIPTEN_SHELL_HTML}")
        string(REPLACE "Emscripten-Generated Code" "${OUT_NAME}" EMSCRIPTEN_SHELL_HTML "${EMSCRIPTEN_SHELL_HTML}")
    endif()
    file(WRITE ${bin_dir}/shell.html.in "${EMSCRIPTEN_SHELL_HTML}")

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_executable(game ${GAME_SRC})
    set_target_properties(game PROPERTIES OUTPUT_NAME "index")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ENGINE_ROOT}/assets/" "${bin_dir}/res/galaxy_assets"
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${proj_dir}/assets/" "${bin_dir}/res/assets"
    )
    set_target_properties(game PROPERTIES LINK_FLAGS "--shell-file \"${bin_dir}/shell.html.in\" --preload-file \"${proj_dir}/bin/web/res@bin/web/res\"")
endif()
target_precompile_headers(game PRIVATE ${ENGINE_ROOT}/include/Galaxy/engine.hpp)

# Galaxy Engine
if (MACOS)
    target_link_libraries(game "${ENGINE_ROOT}/lib/libglfw3.a")
endif()
if (EMSCRIPTEN)
    # For loading file system
    target_link_options(game PUBLIC -lidbfs.js -sINITIAL_MEMORY=33554432)
endif()

target_link_libraries(game "${ENGINE_ROOT}/bin/${relative_bin_dir}/libengine.a")