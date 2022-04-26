#include <utils.hpp>
#include <list>
struct Snake
{
    float speed = 1.f/5;
    int width = 15, height = 15;

    float tileSize = 2.f/width;

    Listener preRenderConn, aspectConn;
    ListenerT<TouchData> touchInputConn;

    double lastMove = 0;
    Vector2Int moveDir, lastMoveDir;
    Vector2Int playerTile, humanTile;

    Vector2 touchOrigin;

    AssetRef<Mesh> square;
    AssetRef<Texture> playerTex, tileTex, menuTex, humanTex, restartTex, touchCircle;
    AssetRef<Shader> colShader, texShader, tintShader;
    std::unique_ptr<Material> snakeMat, touchIndicatorMat, playerMat, bgMat, humanMat;

    Object2D *touchIndicator = 0;

    Object2D *background, *player, *human;
    std::vector<Object2D*> snakeBody;
    std::list<Vector2Int> snakeBodyPos;

    UIText *score;

    Vector2 get_pos(Vector2Int tile)
    { return Vector2(tile.x,tile.y)*tileSize + (Vector2)background->position; }

    void set_move_dir(int x, int y);
    void touch_changed(TouchData data);
    void pre_render();
    void end_game();
    void move_human();
    void fix_board();
    Snake();
    ~Snake();
};
void Snake::fix_board()
{
    Vector2 ratio = Renderer::aspectRatio;
    float size,pos;
    if (ratio.y > 1.25f) size=1, pos=0;
    else size=1.111111f, pos=0.111111f;

    Camera::main->orthoSize = size;
    Camera::main->position.y = pos;

    Camera::main->refresh();
}

int random(int min, int max) { return rand()%(max-min)+min; }
void Snake::set_move_dir(int x, int y)
{
    if ((x != 0 && lastMoveDir.x == 0) || (y != 0 && lastMoveDir.y == 0))
        moveDir = Vector2Int(x,y);
}
void Snake::move_human()
{
    humanTile = Vector2Int(random(-7,7), random(-7,7));
    human->position = get_pos(humanTile);
}
void Snake::end_game()
{
    UIImage *image = UIImage::create();
    image->anchor = Vector2();
    constexpr float grey = 0.2f;
    image->tint = Vector4(grey,grey,grey,0.5f);
    image->scale = Vector2(1,0.3);
    UIText *text = UIText::create("Game Over");
    text->anchor = Vector2();
    text->pivot = Vector2();
    text->scale = image->scale*0.95;

    preRenderConn.disconnect();
}
void Snake::pre_render()
{
    if (moveDir == Vector2Int()) return;
    if (Time::get() > lastMove + speed)
    {
        lastMove = Time::get();
        lastMoveDir = moveDir;
        Vector2Int lastTile = playerTile;

        playerTile += moveDir;
        player->position = get_pos(playerTile);
        
        if (playerTile == humanTile)
        {
            move_human();
            snakeBodyPos.push_front(Vector2Int());
            Object2D *obj = Object2D::create(square.get(), snakeMat.get());
            snakeBody.push_back(obj);
            score->str = std::to_string(snakeBody.size());
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
void Snake::touch_changed(TouchData data)
{
    if (UIImage::get_held(data.id))
        return;

    constexpr float activateDist = 0.075f;
    if (data.state == PRESSED)
    {
        touchOrigin = data.pos;
        if (!touchIndicator)
        {
            touchIndicatorMat = std::make_unique<Material>(tintShader.get());
            touchIndicatorMat->set_uniform("u_color", Vector4(0.75,0.75,0.75,0.75));
            touchIndicator = Object2D::create(square.get(), touchIndicatorMat.get());
            touchIndicator->zIndex(1000);
            touchCircle = Texture::load(Assets::path()+"/textures/4-circle.png", Texture::Pixel);
            touchIndicatorMat->mainTex = touchCircle.get();
        }
        touchIndicator->position = touchOrigin*Camera::main->orthoSize+Camera::main->position;
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
Snake::Snake()
{
    //Camera::main->orthoSize = 1;
    //Camera::main->refresh();
    fix_board();
    Input::add_bind("up", Key::W, [this](bool pressed)
    { if (pressed) set_move_dir(0,1); });
    
    Input::add_bind("down", Key::S, [this](bool pressed)
    { if (pressed) set_move_dir(0,-1); });
    
    Input::add_bind("left", Key::A, [this](bool pressed)
    { if (pressed) set_move_dir(-1,0); });
    
    Input::add_bind("right", Key::D, [this](bool pressed)
    { if (pressed) set_move_dir(1,0); });

    touchInputConn = Input::touch_changed().connect(TYPE_LAMBDA(touch_changed, auto));
    preRenderConn = Renderer::pre_render().connect(CLASS_LAMBDA(pre_render));
    aspectConn = Renderer::aspect_ratio_changed().connect(CLASS_LAMBDA(fix_board));

    square = Mesh::load_obj(Assets::gpath()+"/models/square.obj");
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
    background = Object2D::create(square.get(), bgMat.get());
    background->scale = Vector2(2,2);
    background->zIndex(-1000);

    playerMat = std::make_unique<Material>(texShader.get());
    playerMat->mainTex = playerTex.get();
    player = Object2D::create(square.get(), playerMat.get());
    player->scale = Vector2(tileSize, tileSize);
    player->position = get_pos(Vector2Int()); // center UFO
    player->zIndex(10);

    humanMat = std::make_unique<Material>(texShader.get());
    humanMat->mainTex = humanTex.get();
    human = Object2D::create(square.get(), humanMat.get());
    human->scale = Vector2(tileSize, tileSize);
    move_human();
    human->zIndex(1);

    UIImage *menuBtn = UIImage::create(menuTex.get());
    menuBtn->group = UIGroup::safeArea;
	TINT_ON_CLICK(menuBtn, (1,1,1,1), (0.75,0.75,0.75,1));
    menuBtn->anchor = Vector2(1, 1);
    menuBtn->scale = Vector2(0.15, 0.15);

    menuBtn->pos = -menuBtn->scale/2-0.02;
    menuBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Start");
    };
    UIImage *restartBtn = UIImage::create(restartTex.get());
    restartBtn->group = UIGroup::safeArea;
    restartBtn->anchor = Vector2(-1, 1);
    restartBtn->scale = Vector2(0.15, 0.15);

    restartBtn->pos = Vector2(0.35f, -restartBtn->scale.y/2-0.02);
    restartBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Snake");
    };
    score = UIText::create("0");
    score->group = UIGroup::safeArea;
    score->anchor = Vector2(-1,1);
    score->pivot = Vector2(0,0);
    score->scale = Vector2(0.15, 0.15);
    score->pos = Vector2(0.2f, -0.1f);
}
Snake::~Snake()
{
    Camera::main->reset();
    Input::remove_bind("up");
    Input::remove_bind("down");
    Input::remove_bind("left");
    Input::remove_bind("right");
}
ADD_SCENE_COMPONENT("Snake", Snake);