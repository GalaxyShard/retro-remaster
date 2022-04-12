#include <utils.hpp>
#include <global.hpp>
#include <GeometryDash/dash.hpp>
struct DashMenu
{
    AssetRef<Texture> menuTex;
    uintg selectedLevel = 0;
    UIText *selectedText;

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
    startBtn->onClick = [this]()
    {
        Scene::destroy(Scene::activeScene);
        globalScene->add_component<DashSceneData>();
        auto *data = globalScene->get_component<DashSceneData>();
        data->level = selectedLevel;
        Scene::create("DashLevel");
    };
    UIText *startText = text_for_img("Start!", startBtn);

    UIImage *editorBtn = UIImage::create();
    editorBtn->tint = Vector4(0.2,0.2,0.2,1);
    editorBtn->anchor = Vector2(0,0);
    editorBtn->scale = Vector2(1, 0.25);
    editorBtn->pos = Vector2(0, -0.3);
    editorBtn->onClick = [this]()
    {
        Scene::destroy(Scene::activeScene);
        globalScene->add_component<DashSceneData>();
        auto *data = globalScene->get_component<DashSceneData>();
        data->level = selectedLevel;
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
        if (selectedLevel != 0)
            selectedLevel -= 1;
        selectedText->str = std::to_string(selectedLevel);
        selectedText->refresh();
    };
    UIImage *increaseBtn = UIImage::create(menuTex.get());
    increaseBtn->anchor = Vector2(0,0);
    increaseBtn->scale = Vector2(0.25, 0.25);
    increaseBtn->pos = Vector2(0.4, -0.6);
    increaseBtn->rotation = -Math::PI*0.5f;
    increaseBtn->onTouchDown = [this]()
    {
        selectedLevel += 1;
        selectedText->str = std::to_string(selectedLevel);
        selectedText->refresh();
    };
    auto *sceneData = globalScene->get_component<DashSceneData>();
    if (sceneData)
        selectedLevel = sceneData->level;
    selectedText = UIText::create(std::to_string(selectedLevel));
    selectedText->anchor = Vector2(0,0);
    selectedText->scale = Vector2(0.25, 0.25);
    selectedText->pos = Vector2(0, -0.6);
    selectedText->pivot = Vector2(0,0);
}
DashMenu::~DashMenu()
{
    
}
ADD_SCENE_COMPONENT("Dash", DashMenu);