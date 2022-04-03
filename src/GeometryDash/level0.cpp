#include <utils.hpp>
struct DashCollider
{
    Object *obj;
    enum {AABB,TRIANGLE} type;
    bool killOnTouch;
    DashCollider(Object *obj, decltype(type) type, bool killOnTouch)
        : obj(obj), type(type), killOnTouch(killOnTouch) { }

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
struct DashLevel0
{
    Listener renderConn;
    ListenerT<TouchData> inputConn;
    AssetRef<Texture> menuTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square, triangle;

    std::unique_ptr<AudioPlayer> bgAudio;
    std::unique_ptr<Material> playerMat, otherMat, spikeMat;

    std::vector<DashCollider> colliders;
    Object *player;


    float gravity = -25.9f*2;
    float speed = 10;
    float jumpHeight = 2;
    Vector3 vel;

    bool jump=0, gameEnded=0;

    void pre_render();
    void end_game(bool won);
    void handle_touch(TouchData data);
    DashLevel0();
    ~DashLevel0();
};
void DashLevel0::end_game(bool won)
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
}
void min_max_dot(Vector2 *points, uintg len, Vector2 axis, float &min, float &max)
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
DashCollisionData test_collision(Vector2 *player, DashCollider *collider)
{
    Vector2 mtv;
    float mtvMag = Math::INF;
    float minDot[2], maxDot[2];

    Vector2 points[4];
    uintg numPoints = 0;
    
    auto testAxis = [&](Vector2 axis) -> bool
    {
        min_max_dot(player, 4,         axis, minDot[0], maxDot[0]);
        min_max_dot(points, numPoints, axis, minDot[1], maxDot[1]);

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
    // Points should be cached in DashCollider
    Vector2 halfScale = collider->obj->scale*0.5f;
    if (collider->type == DashCollider::AABB)
    {
        numPoints = 4;
        points[0] = Vector2(-1, -1);
        points[1] = Vector2( 1, -1);
        points[2] = Vector2(-1,  1);
        points[3] = Vector2( 1,  1);
        for (uintg i = 0; i < 4; ++i)
            points[i] = collider->obj->position + (halfScale*points[i]);
        
        if (!testAxis(Vector2(1, 0)) || !testAxis(Vector2(0, 1))
            || !testAxis(player[4]) || !testAxis(player[5]))
            return DashCollisionData();

        return DashCollisionData(mtv);
    }
    else if (collider->type == DashCollider::TRIANGLE)
    {
        numPoints = 3;
        points[0] = Vector2(-1, -1);
        points[1] = Vector2( 1, -1);
        points[2] = Vector2( 0,  1);
        for (uintg i = 0; i < 3; ++i)
            points[i] = collider->obj->position + (halfScale*points[i]);

        if (!testAxis(player[4]) || !testAxis(player[5])
            || !testAxis(Vector2(0,1)) || !testAxis(triangleNorm0) || !testAxis(triangleNorm1))
            return DashCollisionData();

        return DashCollisionData(mtv);
    }
    return DashCollisionData();
}

void DashLevel0::handle_touch(TouchData data)
{
    jump = data.state != TouchState::RELEASED;
}
void DashLevel0::pre_render()
{

    vel.y += gravity * Time::delta();
    if (jump)
    {
        jump = 0;
        vel.y = sqrt(jumpHeight*2*-gravity);
    }
    player->position.x += speed * Time::delta();
    player->position.y += vel.y * Time::delta();

    Vector2 playerPoints[6];
    {
        //Matrix2x2 rot = Matrix2x2::rotate(player->rotation.z);
        Matrix2x2 rot = Matrix2x2::identity();
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
        didCollide |= data.collided;

        if ((collider.killOnTouch && data.collided) || data.mtv.x < -0.0001)
        {
            end_game(0);
            return;
        }
    }
    if (didCollide)
    {
        vel.y = -0.1;
        player->rotation.z = 0;
    }
    else 
    {
        player->rotation.z -= Math::PI*1.9f*Time::delta();
    }
    Camera::main->position.x = player->position.x + 7.5f;
}
DashLevel0::DashLevel0()
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
    player = Object::create(square.get(), playerMat.get());
    player->scale = Vector2(1, 1);
    player->position = Vector2(0, -0.75);
    player->position.z = 1;

    //fakePlayerMat = std::make_unique<Material>(colShader.get());
    //fakePlayerMat->set_uniform("u_color", Vector4(1,1,1,0.5));
    //fakePlayer = Object::create(square.get(), fakePlayerMat.get());
    //fakePlayer->scale = Vector2(1, 1);
    //fakePlayer->position = Vector2(0, -0.75);
    //fakePlayer->position.z = 0.5;

    otherMat = std::make_unique<Material>(colShader.get());
    otherMat->set_uniform("u_color", Vector4(0.3,0.3,0.5,1));
    Object *other = Object::create(square.get(), otherMat.get());
    other->scale = Vector2(10000, 100);
    other->position = Vector2(0, -1 - other->scale.y/2);

    spikeMat = std::make_unique<Material>(colShader.get());
    spikeMat->set_uniform("u_color", Vector4(0.75,0.75,0.75,1));
    for (uintg i = 1; i <= 500; ++i)
    {
        Object *test = Object::create(square.get(), spikeMat.get());
        test->scale = Vector2(0.35,1);
        test->position = Vector2(i*14.25, -1.75);

        uintg numSpikes = rand()%3+1;
        for (uintg j = 0; j < numSpikes; ++j)
        {
            Object *spike = Object::create(triangle.get(), spikeMat.get());
            spike->position = Vector2(i*14.25+2+j,-0.5);
            colliders.push_back(DashCollider(spike, DashCollider::TRIANGLE, 1));
        }
    }

    colliders.push_back(DashCollider(other, DashCollider::AABB, 0));
    //for (uintG i = 0; i < 1000; ++i)
    //{
    //    Object *other = Object::create(square.get(), otherMat.get());
    //    other->scale = Vector2(0.1, 0.1);
    //    other->position = Vector2(rand()/(float)RAND_MAX*10000, rand()/(float)RAND_MAX * 11 - 1);
    //}
    

    //Math::insertion_sort<DashCollider>(colliders, [](DashCollider &a, DashCollider &b)->bool
    //{ return a.obj->position.x < b.obj->position.x; });

    Input::add_bind("jump", Key::Space, [this](bool p){jump=p;});
}
DashLevel0::~DashLevel0()
{
    Renderer::set_clear_color(0,0,0,1);
    Input::remove_bind("jump");
    Camera::main->reset();
}
ADD_SCENE_COMPONENT("Dash-Level0", DashLevel0);