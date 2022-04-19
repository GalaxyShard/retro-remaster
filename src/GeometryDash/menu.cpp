#include <utils.hpp>
#include <global.hpp>
#include <GeometryDash/dash.hpp>
struct DashMenu
{
    AssetRef<Texture> menuTex;
    DashSceneData *data;
    UIText *selectedText;

    DashMenu();
    ~DashMenu();
};
#define TINT_ON_CLICK_D(img) TINT_ON_CLICK(img, (0.2,0.2,0.2,1), (0.15,0.15,0.15,1))

DashMenu::DashMenu()
{
    globalScene->add_component<DashSceneData>();
    data = globalScene->get_component<DashSceneData>();

    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);
    
    UIImage *menuBtn = UIImage::create(menuTex.get());
	TINT_ON_CLICK(menuBtn, (1,1,1,1), (0.75,0.75,0.75,1));
    menuBtn->anchor = Vector2(1, 1);
    menuBtn->scale = Vector2(0.15, 0.15);
    menuBtn->pos = Vector2(-0.25f, -menuBtn->scale.y/2-0.02);
    menuBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Start");
        globalScene->remove_component<DashSceneData>();
    };

    UIImage *startBtn = UIImage::create();
    TINT_ON_CLICK_D(startBtn);
    startBtn->anchor = Vector2(0,0);
    startBtn->scale = Vector2(1, 0.25);
    startBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("DashLevel");
    };
    UIText *startText = text_for_img("Start!", startBtn);

    UIImage *editorBtn = UIImage::create();
    TINT_ON_CLICK_D(editorBtn);
    editorBtn->anchor = Vector2(0,0);
    editorBtn->scale = Vector2(1, 0.25);
    editorBtn->pos = Vector2(0, -0.3);
    editorBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("DashEditor");
    };
    UIText *editorText = text_for_img("Edit!", editorBtn);

    UIImage *decreaseBtn = UIImage::create(menuTex.get());
    decreaseBtn->anchor = Vector2(0,0);
    decreaseBtn->scale = Vector2(0.25, 0.25);
    decreaseBtn->pos = Vector2(-0.4, -0.6);
    decreaseBtn->rotation = Math::PI*0.5f;
    decreaseBtn->onTouchDown = [this]()
    {
        UIImage::get_held()->tint = Vector4(0.75,0.75,0.75, 1);
        if (data->level != 0)
            data->level -= 1;
        selectedText->str = std::to_string(data->level);
        selectedText->refresh();
    };
    decreaseBtn->onTouchUp = [](){ UIImage::get_held()->tint = Vector4(1,1,1,1); };
    UIImage *increaseBtn = UIImage::create(menuTex.get());
    increaseBtn->anchor = Vector2(0,0);
    increaseBtn->scale = Vector2(0.25, 0.25);
    increaseBtn->pos = Vector2(0.4, -0.6);
    increaseBtn->rotation = -Math::PI*0.5f;
    increaseBtn->onTouchDown = [this]()
    {
        UIImage::get_held()->tint = Vector4(0.75,0.75,0.75, 1);
        data->level += 1;
        selectedText->str = std::to_string(data->level);
        selectedText->refresh();
    };
    increaseBtn->onTouchUp = [](){ UIImage::get_held()->tint = Vector4(1,1,1,1); };
    selectedText = UIText::create(std::to_string(data->level));
    selectedText->anchor = Vector2(0,0);
    selectedText->scale = Vector2(0.25, 0.25);
    selectedText->pos = Vector2(0, -0.6);
    selectedText->pivot = Vector2(0,0);
}
DashMenu::~DashMenu()
{
    
}
ADD_SCENE_COMPONENT("Dash", DashMenu);