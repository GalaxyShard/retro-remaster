#include <utils.hpp>
struct DashLevel0
{
    Listener renderConn;
    ListenerT<TouchData> inputConn;
    AssetRef<Texture> menuTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square;

    std::unique_ptr<AudioPlayer> bgAudio;
    std::unique_ptr<Material> playerMat;


    float gravity = -9.81f;
    float speed = 1;
    float jumpHeight = 2;
    Object *player;
    Vector3 vel;

    bool jump=0;

    void pre_render();
    void handle_touch(TouchData data);
    DashLevel0();
    ~DashLevel0();
};
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
    if (player->position.y <= -1)
    {
        player->position.y = -1;
        vel.y = -0.1f;
    }
    else 
    {
        player->rotation.z += Math::PI*1.5f*Time::delta();
    }
    Camera::main->position.x = (player->position.x + 5.f) * Renderer::reverseAspect.x;
}
DashLevel0::DashLevel0()
{
    Camera::main->projection = Matrix4x4::ortho(-10.f, 10.f, -10.f, 10.f, -10.f, 100000.f, 0);
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

    Input::add_bind("jump", Key::Space, [this](bool p){jump=p;});
}
DashLevel0::~DashLevel0()
{
    Input::remove_bind("jump");
    Camera::main->position = Vector2(0,0);
    Camera::main->projection = Matrix4x4::ortho(-1, 1, -1, 1, -100000, 100000, 0);
}
ADD_SCENE_COMPONENT("Dash-Level0", DashLevel0);