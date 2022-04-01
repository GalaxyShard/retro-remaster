#include <utils.hpp>
struct DashLevel0
{
    Listener renderConn;
    ListenerT<TouchData> inputConn;
    AssetRef<Texture> menuTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square;

    std::unique_ptr<AudioPlayer> bgAudio;
    std::unique_ptr<Material> playerMat, otherMat;

    float gravity = -25.9f;
    float speed = 10;
    float jumpHeight = 2;
    Object *player;
    Vector3 vel;

    bool jump=0;

    void pre_render();
    void handle_touch(TouchData data);
    DashLevel0();
    ~DashLevel0();
};

bool test_aabb(Vector2 aabbMin0, Vector2 aabbMax0, Vector2 aabbMin1, Vector2 aabbMax1)
{
    return !(((aabbMin1.x >= aabbMax0.x) || (aabbMin0.x >= aabbMax1.x))
        && ((aabbMin1.y >= aabbMax0.y) || (aabbMin0.y >= aabbMax1.y)));
}
bool test_obb()
{
    return 0;
}

void DashLevel0::handle_touch(TouchData data)
{
    jump = data.state == TouchState::PRESSED;
}
void DashLevel0::pre_render()
{
    if (jump)
    {
        jump = 0;
        vel.y = sqrt(jumpHeight*2*-gravity);
    }
    player->position.x += speed * Time::delta();
    player->position.y += vel.y * Time::delta();
    vel.y += gravity * Time::delta();
    if (player->position.y <= -1+player->scale.x/2)
    {
        player->position.y = -1+player->scale.x/2;
        vel.y = -0.1f;
        player->rotation.z = 0;
    }
    else 
    {
        player->rotation.z -= Math::PI*2.f*Time::delta();
    }
    Camera::main->position.x = player->position.x + 7.5f;
}
DashLevel0::DashLevel0()
{
    Camera::main->orthoSize = 10;
    Camera::main->refresh();
    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);

    renderConn = Renderer::pre_render().connect(CLASS_LAMBDA(pre_render));
    inputConn = Input::touch_changed().connect(TYPE_LAMBDA(handle_touch, TouchData));

    square = Mesh::load_obj(Assets::gpath()+"/models/square.obj");
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

    otherMat = std::make_unique<Material>(colShader.get());
    otherMat->set_uniform("u_color", Vector4(0.5,0.5,0.9,1));
    Object *other = Object::create(square.get(), otherMat.get());
    other->scale = Vector2(10000, 1);
    other->position = Vector2(0, -1.5);

    //for (uintG i = 0; i < 1000; ++i)
    //{
    //    Object *other = Object::create(square.get(), otherMat.get());
    //    other->scale = Vector2(0.1, 0.1);
    //    other->position = Vector2(rand()/(float)RAND_MAX*100, rand()/(float)RAND_MAX * 11 - 1);
    //}

    Input::add_bind("jump", Key::Space, [this](bool p){jump=p;});
}
DashLevel0::~DashLevel0()
{
    Input::remove_bind("jump");
    Camera::main->reset();
}
ADD_SCENE_COMPONENT("Dash-Level0", DashLevel0);