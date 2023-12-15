
set(OUT_NAME "Retro Remaster")
set(BUNDLE_ID "com.galaxyshard.retro-remaster-test")

include(private.cmake OPTIONAL)

set(GAME_SRC
    src/Minesweeper/minesweeper.cpp
    src/Ping-4/ping4.cpp
    src/Snake/snake.cpp
    src/start.cpp
    #src/Net-Test/nettest.cpp
    #src/Net-Test/joystick.cpp
    #src/Net-Test/controller.cpp
    src/utils.cpp
    src/GeometryDash/menu.cpp
    src/GeometryDash/level.cpp
    src/GeometryDash/editor.cpp
    src/GeometryDash/physics.cpp
)
set(GAME_HEADERS
    src/GeometryDash/dash.hpp
    src/GeometryDash/physics.hpp
    src/global.hpp
    src/utils.hpp
)