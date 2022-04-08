#include <utils.hpp>
struct DashCollider
{
    Object2D *obj;
    Vector2 points[4];
    enum {NONE,AABB,TRIANGLE} type;
    bool killOnTouch;
    DashCollider(Object2D *obj, decltype(type) type, bool killOnTouch)
        : obj(obj), type(type), killOnTouch(killOnTouch) { generate(); }

    void generate();
};
static const Vector2 triangleNorm0 = (Vector2(0, 0.5) - Vector2(-0.5, -0.5)).unit().perpendicular();
static const Vector2 triangleNorm1 = (Vector2(0, 0.5) - Vector2( 0.5, -0.5)).unit().perpendicular();
struct DashCollisionData
{
    Vector2 mtv;
    bool collided;
    DashCollisionData(Vector2 mtv) : mtv(mtv), collided(1) {}
    DashCollisionData() : mtv(), collided(0) {}
};
struct DashLevel
{
    Listener renderConn;
    ListenerT<TouchData> inputConn;
    AssetRef<Texture> menuTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square, triangle;

    std::unique_ptr<AudioPlayer> bgAudio;
    std::unique_ptr<Material> playerMat, otherMat, spikeMat;

    std::vector<DashCollider> colliders;
    Object2D *player;


    float gravity = -25.9f*2;
    float speed = 10;
    float jumpHeight = 2;
    Vector3 vel;

    bool jump=0, gameEnded=0, isGrounded=1;

    void pre_render();
    void end_game(bool won);
    void handle_touch(TouchData data);
    DashLevel();
    ~DashLevel();
};
static bool test_aabb_1D(float min0, float max0, float min1, float max1)
{
    return min1<max0 && min0<max1;
}
void DashCollider::generate()
{
    Vector2 halfScale = obj->scale*0.5f;
    if (type == DashCollider::AABB)
    {
        points[0] = Vector2(-1, -1);
        points[1] = Vector2( 1, -1);
        points[2] = Vector2(-1,  1);
        points[3] = Vector2( 1,  1);
        for (uintg i = 0; i < 4; ++i)
            points[i] = obj->position + (halfScale*points[i]);
    }
    else if (type == DashCollider::TRIANGLE)
    {
        points[0] = Vector2(-1, -1);
        points[1] = Vector2( 1, -1);
        points[2] = Vector2( 0,  1);
        for (uintg i = 0; i < 3; ++i)
            points[i] = obj->position + (halfScale*points[i]);
    }
    else
    {
        logerr("Collider not recognized: %d\n", type);
    }
}
void DashLevel::end_game(bool won)
{
    if (gameEnded)
        return;
    gameEnded = 1;

    renderConn.disconnect();
    UIImage *endBg = UIImage::create();
    constexpr float grey = 0.2f;
    endBg->tint = Vector4(grey,grey,grey,0.5f);
    endBg->render_order(1);
    endBg->scale = Vector2(1, 0.25f);
    endBg->anchor = Vector2(0,0);
    UIText *endText = text_for_img(won ? "You Win!" : "Game Over", endBg);

    Object2D::destroy(player);
}
static void min_max_dot(Vector2 *points, uintg len, Vector2 axis, float &min, float &max)
{
    min = Vector2::dot(points[0], axis);
    max = min;
    for (uintg i = 0; i < len; ++i)
    {
        float product = Vector2::dot(points[i], axis);
        if (min > product)
            min = product;
        
        if (max < product)
            max = product;
    }
}
static DashCollisionData test_collision(Vector2 *player, DashCollider *collider)
{
    Vector2 mtv;
    float mtvMag = Math::INF;
    float minDot[2], maxDot[2];
    uintg numPoints = 0;
    
    auto testAxis = [&](Vector2 axis) -> bool
    {
        min_max_dot(player,           4,         axis, minDot[0], maxDot[0]);
        min_max_dot(collider->points, numPoints, axis, minDot[1], maxDot[1]);

            /*
                tests
                <-min0--max0----min1--max1----> 0   correct
                <-min1--max1----min0--max0----> 0   correct

                <-min0--min1----max0--max1----> 1   correct
                <-min1--min0----max1--max0----> 1   correct

                <-min0--min1----max1--max0----> 1   correct
                <-min1--min0----max0--max1----> 1   correct
            */
        if (minDot[1]<maxDot[0]
            && minDot[0]<maxDot[1])
        {
            // max1-min0   when 1 is less
            // min1-max0   when 0 is less
            float avg[2] = {(minDot[0]+maxDot[0])*0.5f, (minDot[1]+maxDot[1])*0.5f};
            float overlap = (avg[0] < avg[1]) ? minDot[1]-maxDot[0] : maxDot[1]-minDot[0];
            if (abs(overlap) < abs(mtvMag))
            {
                mtvMag = overlap;
                mtv = axis*mtvMag;
            }
            return 1;
        }
        return 0;
    };
    if (collider->type == DashCollider::AABB)
    {
        numPoints = 4;

        if (!testAxis(Vector2(1, 0)) || !testAxis(Vector2(0, 1))
            || !testAxis(player[4]) || !testAxis(player[5]))
            return DashCollisionData();

        return DashCollisionData(mtv);
    }
    else if (collider->type == DashCollider::TRIANGLE)
    {
        numPoints = 3;

        if (!testAxis(player[4]) || !testAxis(player[5])
            || !testAxis(Vector2(0,1)) || !testAxis(triangleNorm0) || !testAxis(triangleNorm1))
            return DashCollisionData();

        return DashCollisionData(mtv);
    }
    logerr("Collider not recognized: %d\n", collider->type);
    return DashCollisionData();
}
float move_towards(float current, float goal, float speed)
{
    float to = goal - current;
    if (to <= speed) return goal;
    return current + speed;
}
void DashLevel::handle_touch(TouchData data)
{
    jump = data.state != TouchState::RELEASED;
}
void DashLevel::pre_render()
{
    vel.y += gravity * Time::delta();
    if (jump && isGrounded)
    {
        vel.y = sqrt(jumpHeight*2*-gravity);
    }
    player->position.x += speed * Time::delta();
    player->position.y += vel.y * Time::delta();

    Vector2 playerPoints[6];
    {
        Matrix2x2 rot = Matrix2x2::rotate(player->rotation);
        Vector2 corner = player->scale*0.5f;
        playerPoints[0] = Vector2(-1, -1);
        playerPoints[1] = Vector2( 1, -1);
        playerPoints[2] = Vector2(-1,  1);
        playerPoints[3] = Vector2( 1,  1);
        for (uintg i = 0; i < 4; ++i)
            playerPoints[i] = rot*(corner*playerPoints[i]);

        playerPoints[4] = rot * Vector2(1,0);
        playerPoints[5] = rot * Vector2(0,1);
    }
    Vector2 tempPlayer[6];
    tempPlayer[4] = playerPoints[4];
    tempPlayer[5] = playerPoints[5];
    bool didCollide = 0;

    for (auto &collider : colliders)
    {
        for (uintg i = 0; i < 4; ++i)
            tempPlayer[i] = player->position + playerPoints[i];
        
        DashCollisionData data = test_collision(tempPlayer, &collider);
        player->position += data.mtv;

        for (uintg i = 0; i < 4; ++i)
            tempPlayer[i].y -= 0.05f;
        DashCollisionData groundData = test_collision(tempPlayer, &collider);
        
        if (!collider.killOnTouch) didCollide |= groundData.collided;

        if ((collider.killOnTouch && data.collided) || data.mtv.x < -0.1)
        {
            end_game(0);
            return;
        }
    }
    if (didCollide)
    {
        vel.y = -0.1;
        float increment = Math::PI*0.5f;
        player->rotation = -move_towards(-player->rotation, roundf(-player->rotation/increment)*increment, 10*Time::delta());
        isGrounded = 1;
    }
    else 
    {
        player->rotation -= Math::PI*1.75f*Time::delta();
        isGrounded = 0;
    }
    player->dirtyBounds();
    Camera::main->position.x = player->position.x + 7.5f;
}
DashLevel::DashLevel()
{
    Renderer::set_clear_color(0.2,0.2,0.3,1);
    Camera::main->orthoSize = 10;
    Camera::main->refresh();
    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);

    renderConn = Renderer::pre_render().connect(CLASS_LAMBDA(pre_render));
    inputConn = Input::touch_changed().connect(TYPE_LAMBDA(handle_touch, TouchData));

    square = Mesh::load_obj(Assets::gpath()+"/models/square.obj");
    triangle = Mesh::load_obj(Assets::path()+"/models/triangle.obj");
    colShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/color.shader");
    tintShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/tint.shader");

    AssetRef<AudioData> soundtrack = AudioData::load_wav(Assets::path()+"/audio/peaceful.wav");
    bgAudio = std::make_unique<AudioPlayer>(soundtrack.get());
    bgAudio->loop(1);
    bgAudio->play();

    UIImage *menuButton = UIImage::create(menuTex.get());
    menuButton->anchor = Vector2(1, 1);
    menuButton->scale = Vector2(0.15, 0.15);
    menuButton->pos = Vector2(-0.25f, -menuButton->scale.y/2-0.02);
    menuButton->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Dash");
    };

    playerMat = std::make_unique<Material>(colShader.get());
    playerMat->set_uniform("u_color", Vector4(1,1,1,1));
    player = Object2D::create(square.get(), playerMat.get());
    player->scale = Vector2(1, 1);
    player->position = Vector2(0, -0.75);
    player->zIndex(1);

    otherMat = std::make_unique<Material>(colShader.get());
    otherMat->set_uniform("u_color", Vector4(0.3,0.3,0.5,1));
    Object2D *other = Object2D::create(square.get(), otherMat.get());
    other->scale = Vector2(10000, 100);
    other->position = Vector2(0, -1 - other->scale.y/2);

    spikeMat = std::make_unique<Material>(colShader.get());
    spikeMat->set_uniform("u_color", Vector4(0.75,0.75,0.75,1));

    colliders.push_back(DashCollider(other, DashCollider::AABB, 0));

    // header -> ucharG enum
    // mesh type -> ucharG enum?
    // collider type -> ucharG enum
    // scale -> vec2
    // pos -> vec2
    // rotation -> float
    // z-index -> float

    std::ifstream stream = std::ifstream(Assets::data_path()+"/level0", std::ios::binary);
    // todo: loop over stream and deserialize objects

    //for (uintg i = 1; i <= 2000; ++i)
    //{
    //    Object2D *test = Object2D::create(square.get(), spikeMat.get());
    //    test->scale = Vector2(0.35,1);
    //    test->position = Vector2(i*14.25, -1.75);
    //
    //    uintg numSpikes = rand()%3+1;
    //    for (uintg j = 0; j < numSpikes; ++j)
    //    {
    //        bool isSpike = (rand()%2==0);
    //        Object2D *spike = Object2D::create(isSpike ? triangle.get() : square.get(), spikeMat.get());
    //        spike->position = Vector2(i*14.25+2+j,-0.5);
    //        colliders.push_back(DashCollider(spike, isSpike ? DashCollider::TRIANGLE : DashCollider::AABB, isSpike));
    //    }
    //}


    Input::add_bind("jump", Key::Space, [this](bool p){jump=p;});
}
DashLevel::~DashLevel()
{
    Renderer::set_clear_color(0,0,0,1);
    Input::remove_bind("jump");
    Camera::main->reset();
}
ADD_SCENE_COMPONENT("DashLevel", DashLevel);