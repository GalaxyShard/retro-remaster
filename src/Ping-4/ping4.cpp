#include <Galaxy/engine.hpp>
namespace {

struct Game
{
    Scene *scene;

    AssetRef<Mesh> square;
    std::unique_ptr<Material> ballMat, paddleMat, bgMat;
    AssetRef<Texture> menuTex, playTex, restartTex, bgTex;
    AssetRef<Shader> gradientShader, colShader, tintShader;

    int score = 0;
    UIText *scoreText;

    UIImage *toggleGameBtn;

    Object *ball;
    Object *upPaddle, *downPaddle, *leftPaddle, *rightPaddle;

    Object *lastCollision = 0, *secondLastCollision = 0;

    std::unique_ptr<Listener> preRenderConn;
    std::unique_ptr<Listener> inputUpdateConn;
    std::unique_ptr<ListenerT<TouchData>> touchInputConn;
    Vector2 velocity;

    void clamp_paddles();
    bool check_collision(Object *obj);
    void end_game();
    void pre_render();
    void input_update();
    void handle_touch(TouchData data);
    Game();
    ~Game();
};
//std::unique_ptr<Game> game;
Game *game = nullptr;
float remap(float value, float min, float max, float newMin, float newMax)
{
    return (value-min)/(max-min)*(newMax-newMin)+newMin;
}
Vector2 random_unit()
{
/*
    cos(a) = adjacent / hypotenouse
    sin(a) = opposite / hypotenouse
    hyp == 1
    cos(a) = adjacent
    sin(a) = opposite

            |\
            | \
    opp (y) |  \ hyp (1)
            |   \
            |    \
            |_ _ _\ center
            adj (x)
*/
    float angle = rand()/((float)RAND_MAX);
    angle = angle * (Math::PI*2) - Math::PI;
    return Vector2(cos(angle), sin(angle));
}
void Game::end_game()
{
    preRenderConn->disconnect();

    UIImage *image = new UIImage();
    image->anchor = Vector2();
    image->tint = Vector4(0.75,0.75,0.75,0.5);
    image->scale = Vector2(1, 0.3);
    UIText *text = new UIText("Game over");
    text->scale = image->scale * 0.95f;
    text->anchor = Vector2();
    text->pivot = Vector2();

}
float get_dir(const char *bind0, const char *bind1)
{
    float dir = 0;
    if (Input::is_held(bind0)) dir += 1;
    if (Input::is_held(bind1)) dir -= 1;
    return dir;
}
void Game::clamp_paddles()
{
    float halfWidth = 0.3f * (0.5f);
    upPaddle->position.x = Math::clamp(upPaddle->position.x, -1+halfWidth, 1-halfWidth);
    downPaddle->position.x = Math::clamp(downPaddle->position.x, -1+halfWidth, 1-halfWidth);
    leftPaddle->position.y = Math::clamp(leftPaddle->position.y, -1+halfWidth, 1-halfWidth);
    rightPaddle->position.y = Math::clamp(rightPaddle->position.y, -1+halfWidth, 1-halfWidth);
}
bool Game::check_collision(Object *obj)
{
    constexpr float minVel = 0.7f;
    constexpr int xAxis = 0, yAxis = 1;
    int axis = xAxis;
    int mul = -1;

    if (obj==upPaddle || obj==downPaddle) axis = yAxis;
    if (obj==downPaddle || obj==leftPaddle) mul = 1;

    float ballRadius = ball->scale.x*(0.5f);
    float halfWidth = 0.3f * (0.5f);
    float halfThickness = 0.1f * (0.5f*mul);

    // lastCollision prevents hitting it twice, and hitting after it goes past for 1 frame
    if (lastCollision != obj && -(ball->position[axis]*mul-ballRadius) >= -(obj->position[axis]+halfThickness)*mul)
    {
        Object *prevLastCollision = lastCollision;
        lastCollision = obj;
        if (Math::within(ball->position[!axis], obj->position[!axis]-halfWidth-ballRadius, obj->position[!axis]+halfWidth+ballRadius))
        {
            velocity[axis] = -velocity[axis];
            velocity[!axis] = (ball->position[!axis] - obj->position[!axis]);
            float maxValue = (halfWidth+ballRadius);
            
            // Reverse velocity
            if (velocity[!axis] > 0) velocity[!axis] = remap(velocity[!axis], 0, maxValue, maxValue, 0);
            else velocity[!axis] = remap(velocity[!axis], -maxValue, 0, 0, -maxValue);

            velocity[!axis] *= 7;

            if (Math::within(velocity[!axis], -minVel, minVel))
                velocity[!axis] = (velocity[!axis] > 0) ? minVel : -minVel;
            
            if (secondLastCollision != obj)
            {
                score++;
                scoreText->text = std::to_string(score);
                scoreText->refresh();
            }
        }
        secondLastCollision = prevLastCollision;
        return 1;
    }
    return 0;
}
void Game::pre_render()
{
    ball->position += velocity*Time::delta();

    float radius = ball->scale.x * 0.5;
    float top = ball->position.y+radius;
    float left = ball->position.x-radius;
    float bottom = ball->position.y-radius;
    float right = ball->position.x+radius;
    if (right > 1 || left < -1 || top > 1 || bottom < -1)
        end_game();

    if (check_collision(upPaddle)) return;
    if (check_collision(downPaddle)) return;
    if (check_collision(rightPaddle)) return;
    if (check_collision(leftPaddle)) return;
}
void Game::handle_touch(TouchData data)
{
    if (data.state == TouchState::MOVED)
    {
        Vector2 delta = data.delta * Renderer::aspectRatio * 1.25f;
        upPaddle->position.x += delta.x;
        downPaddle->position.x += delta.x;
        leftPaddle->position.y += delta.y;
        rightPaddle->position.y += delta.y;
    }
}
void Game::input_update()
{
    float yDir = get_dir("up", "down")*Time::delta()*1.25f;
    float xDir = get_dir("right", "left")*Time::delta()*1.25f;
    upPaddle->position.x += xDir;
    downPaddle->position.x += xDir;
    leftPaddle->position.y += yDir;
    rightPaddle->position.y += yDir;
    clamp_paddles();
}
Game::Game()
{
    scene = Scene::activeScene;
    Input::add_bind("up", Key::W);
    Input::add_bind("down", Key::S);
    Input::add_bind("left", Key::A);
    Input::add_bind("right", Key::D);

    touchInputConn = Input::touch_changed().connect([](auto data) { game->handle_touch(data); });
    inputUpdateConn = Renderer::pre_render().connect([]() { game->input_update(); });
    
    square = Mesh::from_obj(Assets::gpath()+"/models/square.obj");

    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);
    playTex = Texture::load(Assets::path()+"/textures/continue.png", Texture::Pixel);
    restartTex = Texture::load(Assets::path()+"/textures/restart.png", Texture::Pixel);
    bgTex = Texture::load(Assets::path()+"/textures/ping4-bg.png", Texture::Linear);
    
    gradientShader = Shader::load(Assets::path()+SHADER_FOLDER+"/ping_gradient.shader");
    colShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/color.shader");
    tintShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/tint.shader");

    paddleMat = std::make_unique<Material>(gradientShader.get());
    ballMat = std::make_unique<Material>(colShader.get());
    ballMat->set_uniform("u_color", Vector4(1,1,1,1));

    bgMat = std::make_unique<Material>(tintShader.get());
    bgMat->mainTex = bgTex.get();
    bgMat->set_uniform("u_color", Vector4(1,1,1,0.2));

    Object *bg = new Object(square.get(), bgMat.get());
    bg->position.z = 1000;
    bg->scale = Vector2(2,2);

    upPaddle = new Object(square.get(), paddleMat.get());
    downPaddle = new Object(square.get(), paddleMat.get());
    leftPaddle = new Object(square.get(), paddleMat.get());
    rightPaddle = new Object(square.get(), paddleMat.get());

    ball = new Object(square.get(), ballMat.get());
    ball->scale = Vector3(0.1,0.1);

    upPaddle->scale = Vector2(0.3, 0.1);
    downPaddle->scale = Vector2(0.3, 0.1);
    leftPaddle->scale = Vector2(0.1, 0.3);
    rightPaddle->scale = Vector2(0.1, 0.3);

    upPaddle->position.y = 0.9;
    downPaddle->position.y = -0.9;
    leftPaddle->position.x = -0.9;
    rightPaddle->position.x = 0.9;


    UIImage *menuButton = new UIImage(menuTex.get());
    menuButton->anchor = Vector2(1, 1);
    menuButton->scale = Vector2(0.15, 0.15);

    menuButton->pos = Vector2(-0.1f, -menuButton->scale.y/2-0.02);
    menuButton->onClick = []()
    {
        delete game;
        game = nullptr;
        new Scene("Start");
    };
    toggleGameBtn = new UIImage(playTex.get());
    toggleGameBtn->anchor = Vector2(1, 1);
    toggleGameBtn->scale = Vector2(0.15, 0.15);

    toggleGameBtn->pos = Vector2(-0.3f, -toggleGameBtn->scale.y/2-0.02);
    toggleGameBtn->onClick = []()
    {
        static bool gameInProgress = 0;
        if (gameInProgress)
        {
            delete game;
            game = nullptr;
            new Scene("Pong4");
        }
        else
        {
            game->preRenderConn = Renderer::pre_render().connect([]() { game->pre_render(); });
            game->toggleGameBtn->texture = game->restartTex.get();
            game->velocity = random_unit();
        }
        gameInProgress = !gameInProgress;
    };

    scoreText = new UIText("0");
    scoreText->anchor = Vector2(-1,1);
    scoreText->pivot = Vector2(0,0);
    scoreText->scale = Vector2(0.15, 0.15);
    scoreText->pos = Vector2(0.2f, -0.1f);
}
Game::~Game()
{
    Input::remove_bind("up");
    Input::remove_bind("down");
    Input::remove_bind("left");
    Input::remove_bind("right");
    delete scene;
}

void first()
{
    //Scene::set_create_callback("Pong4", []() { game = std::make_unique<Game>(); });
    Scene::set_create_callback("Pong4", []() { game = new Game(); });
}
FIRST_INIT_FUNC(first);
//void clean()
//{
//    game = nullptr;
//}
//CLEANUP_FUNC(clean);
}