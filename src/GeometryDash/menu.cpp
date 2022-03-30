#include <utils.hpp>
struct DashMenu
{
    AssetRef<Texture> menuTex;

    DashMenu();
    ~DashMenu();
};
DashMenu::DashMenu()
{
    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);
    
    UIImage *menuButton = UIImage::create(menuTex.get());
    menuButton->anchor = Vector2(1, 1);
    menuButton->scale = Vector2(0.15, 0.15);
    menuButton->pos = Vector2(-0.25f, -menuButton->scale.y/2-0.02);
    menuButton->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Start");
    };

    UIImage *startBtn = UIImage::create();
    startBtn->tint = Vector4(0.2,0.2,0.2,1);
    startBtn->anchor = Vector2(0,0);
    startBtn->scale = Vector2(1, 0.25);
    startBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Dash-Level0");
    };
    UIText *text = text_for_img("Start!", startBtn);
}
DashMenu::~DashMenu()
{
    
}
ADD_SCENE_COMPONENT("Dash", DashMenu);