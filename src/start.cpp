#include <utils.hpp>

const char *gameName = "Retro Collection";

//static Scene *scene;
//static std::string sceneToOpen;

// for gitpod:
// source /workspace/emsdk/emsdk_env.sh
// emrun bin/web/Retro\ Collection.html
// emcmake cmake -B bin/web -DPLATFORM=web -DENGINE_ROOT=/workspace/Galaxy-Engine -DRELEASE_BUILD=1
// emmake make -C bin/web -j8

struct StartScene
{
    Scene *scene;
    std::string sceneToOpen = "";
    void init_menu(std::string name, std::string sceneName, std::string tutorial);
    StartScene();
    ~StartScene();
};

void StartScene::init_menu(std::string name, std::string sceneName, std::string tutorial)
{
    sceneToOpen = name;
    UIImage *background = UIImage::create();
    background->anchor = Vector2(0,0);
    background->scale = Vector2(1.5, 1);
    background->tint = Vector4(0.6,0.6,0.6,1);

    UIText *title = UIText::create(name);
    title->anchor = Vector2(0,0);
    title->scale = Vector2(1, 0.25);
    title->pivot = Vector2(0, 0);
    title->pos = Vector2(0.1, 0.4);

    UIImage *exitBtn = UIImage::create();
    exitBtn->tint = Vector4(1,0,0,1);
    exitBtn->anchor = Vector2(0,0);
    exitBtn->scale = Vector2(0.25,0.25);
    exitBtn->pos = Vector2(-0.75,0.5) + exitBtn->scale/2*Vector2(1,-1);

    exitBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Start");
    };

    UIImage *playBtn = UIImage::create();
    playBtn->tint = Vector4(0.9,0.9,0.9,1);
    playBtn->anchor = Vector2(0,0);
    playBtn->scale = Vector2(0.5,0.25);
    playBtn->pos = Vector2(0.75,-0.5) + playBtn->scale/2*Vector2(-1,1);
    
    text_for_img("Play", playBtn);

    playBtn->onClick = [this](){
        Scene::create(sceneToOpen);
        Scene::destroy(scene);
    };
}

StartScene::StartScene()
{
    scene = Scene::activeScene;
    UIImage *topBar = UIImage::create();
    topBar->anchor = Vector2(0, 1);
    topBar->scale = Vector2(20, 0.35);
    topBar->tint = Vector4(0.85,0.85,0.85,1);
    UIText *title = UIText::create("Retro Collection");
    title->anchor = Vector2(0, 1);
    title->pivot = Vector2(0, 1);
    title->scale = Vector2(1,1);
    title->pos.y = -title->scale.y/2;

    Vector2 gameSize = Vector2(0.5, 0.25);

    const Vector4 buttonColor = Vector4(0.6,0.6,0.6,1);

    UIImage *minesweeperBtn = UIImage::create();
    minesweeperBtn->tint = buttonColor;
    minesweeperBtn->scale = gameSize;
    minesweeperBtn->pos = Vector2(0.1,0.1) + minesweeperBtn->scale/2;
    UIText *minesweeperText = text_for_img("Minesweeper", minesweeperBtn);
    minesweeperText->scale.y *= 0.5f;
    minesweeperBtn->onClick = [this](){
        init_menu("Minesweeper", "Minesweeper",
            "A logic game where the goal is to find all of the bombs without clicking on any of them.\n"
            "The numbers that appear on each tile represent the amount of bombs that are adjacent to it\n"
            "To start, click randomly on tiles until it is possible to determine which tiles are safe and which ones have bombs\n"
            "Flags can be placed by holding Shift or clicking the flag button, they prevent accidentally clicking on bombs"
        );
    };
    UIImage *snakeBtn = UIImage::create();
    snakeBtn->tint = buttonColor;
    snakeBtn->anchor = Vector2(0,-1);
    snakeBtn->scale = gameSize;
    snakeBtn->pos = Vector2(0, 0.1 + snakeBtn->scale.y/2);
    UIText *snakeText = text_for_img("Snake", snakeBtn);
    snakeText->scale.y *= 0.5f;
    snakeBtn->onClick = [this](){
        init_menu("Snake", "Snake",
            "A recreation of the classic game, the goal is to collect as many people as possible with a UFO.\n"
            "Avoid running into the walls or the UFO's beam that appears behind you.\n"
            "Swipe or use WASD to change direction of the UFO"
        );
    };
    UIImage *pong4Btn = UIImage::create();
    pong4Btn->tint = buttonColor;
    pong4Btn->anchor = Vector2(1,-1);
    pong4Btn->scale = gameSize;
    pong4Btn->pos = Vector2(-0.1-pong4Btn->scale.x/2, 0.1+pong4Btn->scale.y/2);
    UIText *pong4Text = text_for_img("Ping 4", pong4Btn);
    pong4Text->scale.y *= 0.5f;
    pong4Btn->onClick = [this](){
        init_menu("Ping 4", "Pong4",
            "A singleplayer retro-style version of table tennis.\n"
            "The goal is to keep the ball in play for as long as possible. Hitting the ball gives one point. Losing the ball restarts the game\n"
            "Swipe or use WASD to move the paddles"
        );
    };

    UIImage *dashBtn = UIImage::create();
    dashBtn->tint = buttonColor;
    dashBtn->anchor = Vector2(-1,-1);
    dashBtn->scale = gameSize;
    dashBtn->pos = Vector2(0.1+dashBtn->scale.x/2, 0.15+dashBtn->scale.y/2+dashBtn->scale.y);
    UIText *dashText = text_for_img("Dash", dashBtn);
    dashText->scale.y *= 0.5f;
    dashBtn->onClick = [this](){
        init_menu("Dash", "Dash", "");
    };

    UIImage *testBtn = UIImage::create();
    testBtn->tint = buttonColor;
    testBtn->anchor = Vector2(0,-1);
    testBtn->scale = gameSize;
    testBtn->pos = Vector2(0, 0.15+testBtn->scale.y/2+testBtn->scale.y);
    UIText *testText = text_for_img("Test", testBtn);
    testText->scale.y *= 0.5f;
    testBtn->onClick = [this](){
        init_menu("Test", "NetTest", "");
    };
    Renderer::set_clear_color(0.75,0.75,0.75,1);
}
StartScene::~StartScene()
{
    Renderer::set_clear_color(0,0,0,1);
}
static void init()
{
    Font::defaultFont = new Font(Assets::gpath()+"/VT323");
    Scene::create("Start");
}
INIT_FUNC(init);
ADD_SCENE_COMPONENT("Start", StartScene);