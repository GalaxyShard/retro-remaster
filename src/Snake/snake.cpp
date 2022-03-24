#include <Galaxy/engine.hpp>
#include <list>
namespace {

struct Game
{
    Scene *scene;

    float speed = 1.f/5;
    int width = 15, height = 15;
    
#if OS_MOBILE
    Vector2 minBounds = Vector2(-1,-1);
    Vector2 maxBounds = Vector2(1,1);
    float tileSize = 2.f/width;
#else
    Vector2 minBounds = Vector2(-0.9,-1);
    Vector2 maxBounds = Vector2(0.9,0.8);
    float tileSize = 1.8f/width;
#endif
    std::unique_ptr<Listener> preRenderConn;
    std::unique_ptr<ListenerT<TouchData>> touchInputConn;

    double lastMove = 0;
    Vector2Int moveDir, lastMoveDir;
    Vector2Int playerTile, humanTile;

    Vector2 touchOrigin;

    AssetRef<Mesh> square;
    AssetRef<Texture> playerTex, tileTex, menuTex, humanTex, restartTex, touchCircle;
    AssetRef<Shader> colShader, texShader, tintShader;
    std::unique_ptr<Material> snakeMat, touchIndicatorMat, playerMat, bgMat, humanMat;

    Object *touchIndicator = 0;

    Object *background, *player, *human;
    std::vector<Object*> snakeBody;
    std::list<Vector2Int> snakeBodyPos;

    UIText *score;

    Vector2 get_pos(Vector2Int tile)
    { return Vector2(tile.x,tile.y)*tileSize + (Vector2)background->position; }

    void set_move_dir(int x, int y);
    void touch_changed(TouchData data);
    void pre_render();
    void end_game();
    void move_human();
    Game();
    ~Game();
};
//std::unique_ptr<Game> game;
Game *game = 0;

int random(int min, int max) { return rand()%(max-min)+min; }
void Game::set_move_dir(int x, int y)
{
    if ((x != 0 && lastMoveDir.x == 0) || (y != 0 && lastMoveDir.y == 0))
        moveDir = Vector2Int(x,y);
}
void Game::move_human()
{
    humanTile = Vector2Int(random(-7,7), random(-7,7));
    human->position = get_pos(humanTile);
    human->position.z = 1;
}
void Game::end_game()
{
    UIImage *image = new UIImage();
    image->anchor = Vector2();
    image->tint = Vector4(.5,.5,.5,.75);
    image->scale = Vector2(1,0.3);
    UIText *text = new UIText("Game over");
    text->anchor = Vector2();
    text->pivot = Vector2();
    text->scale = image->scale*0.95;

    preRenderConn->disconnect();
}
void Game::pre_render()
{
    if (moveDir == Vector2Int()) return;
    if (Time::get() > lastMove + speed)
    {
        lastMove = Time::get();
        lastMoveDir = moveDir;
        Vector2Int lastTile = playerTile;

        playerTile += moveDir;
        player->position = get_pos(playerTile);
        player->position.z = 10;
        
        if (playerTile == humanTile)
        {
            move_human();
            snakeBodyPos.push_front(Vector2Int());
            Object *obj = new Object(square.get(), snakeMat.get());
            snakeBody.push_back(obj);
            score->text = std::to_string(snakeBody.size());
            score->refresh();
        }
        if (snakeBody.size() != 0)
        {
            snakeBodyPos.pop_front();
            snakeBodyPos.push_back(lastTile);
            auto iter = snakeBodyPos.begin();
            for (unsigned int i = 0; i < snakeBody.size(); ++i, ++iter)
            {
                Vector2Int &thisPos = *iter;
                snakeBody[i]->position = get_pos(thisPos);
                Vector2Int prevDir;
                Vector2Int nextDir;

                if (i == snakeBody.size()-1) nextDir = thisPos-playerTile;
                else {++iter; nextDir = thisPos-(*iter); --iter;}

                if (i == 0) prevDir = nextDir;
                else {--iter; prevDir = thisPos-(*iter); ++iter;}
                
                // horizontal
                if (prevDir.y == 0 && nextDir.y == 0) snakeBody[i]->scale = Vector2(tileSize, tileSize*0.75f);
                // vertical
                else if (prevDir.x == 0 && nextDir.x == 0) snakeBody[i]->scale = Vector2(tileSize*0.75f, tileSize);
                // mixed
                else snakeBody[i]->scale = Vector2(tileSize, tileSize);
            }
        }

        if (!Math::within(playerTile.x, -7, 7) || !Math::within(playerTile.y, -7, 7))
        {
            end_game();
            return;
        }
        else
        {
            for (auto pos : snakeBodyPos)
            {
                if (playerTile == pos)
                {
                    end_game();
                    return;
                }
            }
        }
    }
}
void Game::touch_changed(TouchData data)
{
    constexpr float activateDist = 0.075f;

    if (data.state == PRESSED)
    {
        touchOrigin = data.pos;
        if (!touchIndicator)
        {
            touchIndicatorMat = std::make_unique<Material>(tintShader.get());
            touchIndicatorMat->set_uniform("u_color", Vector4(0.75,0.75,0.75,0.75));
            touchIndicator = new Object(square.get(), touchIndicatorMat.get());
            touchCircle = Texture::load(Assets::path()+"/textures/4-circle.png", Texture::Pixel);
            touchIndicatorMat->mainTex = touchCircle.get();
        }
        touchIndicator->position = touchOrigin*Renderer::aspectRatio;
        touchIndicator->position.z = -1;
        touchIndicator->scale = Vector2(activateDist, activateDist)*4;
    }
    else if (data.state == MOVED)
    {
        Vector2 dir = (data.pos - touchOrigin);
        if (dir.sqr_magnitude() > (activateDist*activateDist))
        {
            /*
               __
             /    \  45
            |   ---| 0
             \ __ / -45
            */
            float angle = atan2f(dir.y, dir.x)*Math::to_deg;
            // right
            if (Math::within(angle, -45.f, 45.f)) set_move_dir(1,0);
            // left
            else if (Math::within(angle, -180.f, -135.f) || Math::within(angle, 135.f, 180.f)) set_move_dir(-1,0);
            // down
            else if (Math::within(angle, -135.f, -45.f)) set_move_dir(0,-1);
            // up
            else if (Math::within(angle, 45.f, 135.f)) set_move_dir(0,1);
        }
    }
    else if (data.state == RELEASED)
    {
        if (touchIndicator) touchIndicator->scale = Vector2();
    }
}
Game::Game()
{
    scene = Scene::activeScene;
    
    Input::add_bind("up", Key::W, (input_callback)[](bool pressed)
    { if (pressed) game->set_move_dir(0,1); });
    
    Input::add_bind("down", Key::S, (input_callback)[](bool pressed)
    { if (pressed) game->set_move_dir(0,-1); });
    
    Input::add_bind("left", Key::A, (input_callback)[](bool pressed)
    { if (pressed) game->set_move_dir(-1,0); });
    
    Input::add_bind("right", Key::D, (input_callback)[](bool pressed)
    { if (pressed) game->set_move_dir(1,0); });

    touchInputConn = Input::touch_changed().connect([](TouchData data) { game->touch_changed(data); });
    preRenderConn = Renderer::pre_render().connect([]() { game->pre_render(); });

    square = Mesh::from_obj(Assets::gpath()+"/models/square.obj");
    colShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/color.shader");
    texShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/texture.shader");
    tintShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/tint.shader");

    tileTex = Texture::load(Assets::path()+"/textures/checkers.png", Texture::Pixel);
    playerTex = Texture::load(Assets::path()+"/textures/UFO.png", Texture::Pixel);
    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);
    humanTex = Texture::load(Assets::path()+"/textures/human.png", Texture::Pixel);
    restartTex = Texture::load(Assets::path()+"/textures/restart.png", Texture::Pixel);

    snakeMat = std::make_unique<Material>(colShader.get());
    snakeMat->set_uniform("u_color", Vector4(0.5,0,1,1));

    bgMat = std::make_unique<Material>(texShader.get());
    bgMat->mainTex = tileTex.get();
    background = new Object(square.get(), bgMat.get());
    //background->render_order(-1000);
    background->scale = maxBounds - minBounds;
    background->position = (minBounds + maxBounds)/2;
    background->position.z = -1000;

    playerMat = std::make_unique<Material>(texShader.get());
    playerMat->mainTex = playerTex.get();
    player = new Object(square.get(), playerMat.get());
    player->scale = Vector2(tileSize, tileSize);
    player->position = get_pos(Vector2Int()); // center UFO
    player->position.z = 10;
    //player->render_order(10);

    humanMat = std::make_unique<Material>(texShader.get());
    humanMat->mainTex = humanTex.get();
    human = new Object(square.get(), humanMat.get());
    human->scale = Vector2(tileSize, tileSize);
    //human->render_order(1);
    move_human();
    human->position.z = 1;

    UIImage *menuButton = new UIImage(menuTex.get());
    menuButton->anchor = Vector2(1, 1);
    menuButton->scale = Vector2(0.15, 0.15);

    menuButton->pos = -menuButton->scale/2-0.02;
    menuButton->onClick = []()
    {
        delete game;
        game = nullptr;
        new Scene("Start");
    };
    UIImage *restartBtn = new UIImage(restartTex.get());
    restartBtn->anchor = Vector2(-1, 1);
    restartBtn->scale = Vector2(0.15, 0.15);

    restartBtn->pos = Vector2(0.35f, -restartBtn->scale.y/2-0.02);
    restartBtn->onClick = []()
    {
        delete game;
        game = nullptr;
        new Scene("Snake");
    };
    score = new UIText("0");
    score->anchor = Vector2(-1,1);
    score->pivot = Vector2(0,0);
    score->scale = Vector2(0.15, 0.15);
    score->pos = Vector2(0.2f, -0.1f);
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
    //Scene::set_create_callback("Snake", []() { game = std::make_unique<Game>(); });
    Scene::set_create_callback("Snake", []() { game = new Game(); });
}
FIRST_INIT_FUNC(first);
//void clean()
//{
//    game = nullptr;
//}
//CLEANUP_FUNC(clean);
}