#include <utils.hpp>
#include <global.hpp>
#include <GeometryDash/dash.hpp>

const char *gameName = "Retro Collection";
Scene *globalScene;

// for gitpod:
// source /workspace/emsdk/emsdk_env.sh
// emcmake cmake -B bin/web -DENGINE_ROOT=/workspace/Galaxy-Engine -DRELEASE=1
// emmake make -C bin/web -j8

struct StartScene
{
    Scene *scene;
    std::string sceneToOpen = "";
    void init_menu(std::string name, std::string sceneName, std::string tutorial);
    StartScene();
    ~StartScene();
};
#define TINT_ON_CLICK_D(img) TINT_ON_CLICK(img, (0.6,0.6,0.6,1), (0.5,0.5,0.5,1))

void StartScene::init_menu(std::string name, std::string sceneName, std::string tutorial)
{
    sceneToOpen = sceneName;
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
    exitBtn->anchor = Vector2(0,0);
    exitBtn->scale = Vector2(0.25,0.25);
    exitBtn->pos = Vector2(-0.75,0.5) + exitBtn->scale/2.f*Vector2(1,-1);
    TINT_ON_CLICK(exitBtn, (1,0,0,1), (0.75,0,0,1));

    exitBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Start");
    };

    UIImage *playBtn = UIImage::create();
    playBtn->anchor = Vector2(0,0);
    playBtn->scale = Vector2(0.5,0.25);
    playBtn->pos = Vector2(0.75,-0.5) + playBtn->scale/2.f*Vector2(-1,1);
    TINT_ON_CLICK(playBtn, (0.9,0.9,0.9,1), (0.8,0.8,0.8,1));
    
    text_for_img("Play", playBtn);

    playBtn->onClick = [this](){
        std::string open = sceneToOpen;
        Scene::destroy(scene);
        Scene::create(open);
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

    UIImage *minesweeperBtn = UIImage::create();
    TINT_ON_CLICK_D(minesweeperBtn);
    minesweeperBtn->scale = gameSize;
    minesweeperBtn->pos = Vector2(0.1,0.1) + minesweeperBtn->scale/2.f;
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
    TINT_ON_CLICK_D(snakeBtn);
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
    TINT_ON_CLICK_D(pong4Btn);
    pong4Btn->anchor = Vector2(1,-1);
    pong4Btn->scale = gameSize;
    pong4Btn->pos = Vector2(-0.1-pong4Btn->scale.x/2, 0.1+pong4Btn->scale.y/2);
    UIText *pong4Text = text_for_img("Ping 4", pong4Btn);
    pong4Text->scale.y *= 0.5f;
    pong4Btn->onClick = [this](){
        init_menu("Ping 4", "Ping4",
            "A singleplayer retro-style version of table tennis.\n"
            "The goal is to keep the ball in play for as long as possible. Hitting the ball gives one point. Losing the ball restarts the game\n"
            "Swipe or use WASD to move the paddles"
        );
    };

    UIImage *dashBtn = UIImage::create();
    TINT_ON_CLICK_D(dashBtn);
    dashBtn->anchor = Vector2(-1,-1);
    dashBtn->scale = gameSize;
    dashBtn->pos = Vector2(0.1+dashBtn->scale.x/2, 0.15+dashBtn->scale.y/2+dashBtn->scale.y);
    UIText *dashText = text_for_img("Dash", dashBtn);
    dashText->scale.y *= 0.5f;
    dashBtn->onClick = [this](){
        init_menu("Dash", "Dash", "");
    };

    UIImage *testBtn = UIImage::create();
    TINT_ON_CLICK_D(testBtn);
    testBtn->anchor = Vector2(0,-1);
    testBtn->scale = gameSize;
    testBtn->pos = Vector2(0, 0.15+testBtn->scale.y/2+testBtn->scale.y);
    UIText *testText = text_for_img("Test", testBtn);
    testText->scale.y *= 0.5f;
    testBtn->onClick = [this](){
        init_menu("Test", "NetTest", "");
    };
    //Renderer::set_clear_color(0.75,0.75,0.75,1);
    Camera::main->set_bg(0.75, 0.75, 0.75);
}
StartScene::~StartScene()
{
    Camera::main->reset();
    //Renderer::set_clear_color(0,0,0,1);
}
struct FPSCounter
{
    Listener renderConn;
    std::deque<float> lastMS;
    UIText *fpsText;
    void pre_render();
    FPSCounter();
};
void FPSCounter::pre_render()
{
    lastMS.push_back(Time::frame_time());
    float ms = 0;
    for (float last : lastMS)
        ms += last;
    ms /= lastMS.size();
    unsigned int fps = (unsigned int)(1.f/ms);

    while (lastMS.size() > std::min(fps, 20u)/2u)
        lastMS.pop_front();
    char buffer[10];
    snprintf(buffer, 10, "MS: %.2f", ms*1000);
    fpsText->str = buffer;
    fpsText->refresh();
}
FPSCounter::FPSCounter()
{
    renderConn = Renderer::pre_render().connect(CLASS_LAMBDA(pre_render));
    fpsText = UIText::create("MS: 0.0");
    fpsText->group = UIGroup::safeArea;
    fpsText->anchor = Vector2(0, 1);
    fpsText->pos = Vector2(0, -0.0325);
    fpsText->pivot = Vector2(0,0);
    fpsText->scale = Vector2(0.35, 0.075);
    fpsText->render_order(1000);
    UIImage *img = img_for_text(fpsText);
    img->tint = Vector4(0,0,0,0.5);
    img->render_order(999);
}
static void init()
{
    // linear: t
    // ease in/out (bezier): t*t*(3.f - 2.f*t)
    // ease in: t*t
    // ease out: 1-(1-t)*(1-t)

    Font::defaultFont = new Font(Assets::gpath()+"/VT323");
    globalScene = Scene::create("Global");
    Scene::create("Start");
    
    BinaryWriter writer;
    for (uintg i = 1; i <= 2000; ++i)
    {
        uintg numSpikes = rand()%3+1;
        for (uintg j = 0; j < numSpikes; ++j)
        {
            bool isSpike = (rand()%2==0);
            writer.write<ucharG>(Header::OBJ2D);
            writer.write<ucharG>(isSpike ? MeshType::TRIANGLE : MeshType::SQUARE);
            writer.write<ucharG>(isSpike ? ColliderType::TRIANGLE : ColliderType::AABB);
            writer.write<Vector2>(Vector2(roundf((14.25*i+2+j)/0.1)*0.1, -0.5));
            writer.write<Vector2>(Vector2(1,1));
            writer.write<float>(0);
            writer.write<float>(0);
        }
    }
    std::ofstream output = std::ofstream(Assets::data_path()+"/level0", std::ios::binary);
    if (!output) return;
    output << writer.get_buffer();
    if (!output) return;
    Assets::sync_files();


}
INIT_FUNC(init);
ADD_SCENE_COMPONENT("Start", StartScene);
ADD_SCENE_COMPONENT("Global", FPSCounter);