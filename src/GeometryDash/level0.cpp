
struct DashLevel0
{
    AssetRef<Texture> menuTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square;
    bool jump=0;

    std::unique_ptr<Material> playerMat;
    Object *player;

    void pre_render();
    void handle_touch(TouchData data);
    DashLevel0();
    ~DashLevel0();
};
void DashLevel0::handle_touch(TouchData data)
{

}
void DashLevel0::pre_render()
{

}
DashLevel0::DashLevel0()
{
    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);

    renderConn = Renderer::pre_render().connect(CLASS_LAMBDA(pre_render));
    inputConn = Input::touch_changed().connect(CLASS_LAMBDA(handle_touch));

    square = Mesh::from_obj(Assets::gpath()+"/models/square.obj");
    colShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/color.shader");
    tintShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/tint.shader");

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

    Input::add_bind("jump", Key::Space, [this](bool p){jump=p;});
}
DashLevel0::~DashLevel0()
{
    Input::remove_bind("jump");
}
ADD_SCENE_COMPONENT("Dash", DashLevel0);