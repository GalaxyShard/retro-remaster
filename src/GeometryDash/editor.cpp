#include <utils.hpp>
#include <global.hpp>
#include <GeometryDash/dash.hpp>
struct DashEditor
{
    AssetRef<Texture> menuTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square, triangle;
    std::unique_ptr<Material> otherMat, spikeMat;

    ListenerT<TouchData> inputConn;

    UIText *coordsText, *saveText;

    void handle_touch(TouchData data);

    DashEditor();
    ~DashEditor();
};
void DashEditor::handle_touch(TouchData data)
{
    Vector3 &camPos = Camera::main->position;
    camPos -= data.delta * 10;
    char buffer[16];
    snprintf(buffer, 16, "%d, %d", (int)camPos.x, (int)camPos.y);
    coordsText->str = buffer;
    coordsText->refresh();
}
DashEditor::DashEditor()
{
    Renderer::set_clear_color(0.2,0.2,0.3,1);
    Camera::main->orthoSize = 10;
    Camera::main->refresh();
    Camera::main->position.x = 7.5;

    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);

    square = Mesh::load_obj(Assets::gpath()+"/models/square.obj");
    triangle = Mesh::load_obj(Assets::path()+"/models/triangle.obj");
    colShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/color.shader");
    tintShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/tint.shader");

    inputConn = Input::touch_changed().connect(TYPE_LAMBDA(handle_touch, TouchData));

    UIImage *menuButton = UIImage::create(menuTex.get());
    menuButton->anchor = Vector2(1, 1);
    menuButton->scale = Vector2(0.15, 0.15);
    menuButton->pos = Vector2(-0.25f, -menuButton->scale.y/2-0.02);
    menuButton->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Dash");
    };

    UIImage *saveBtn = UIImage::create();
    saveBtn->anchor = Vector2(-1, 1);
    saveBtn->scale = Vector2(0.35, 0.15);
    saveBtn->pos = Vector2(0.175f, -saveBtn->scale.y/2);
    saveBtn->onClick = [this]()
    {
        saveText->str = "Saving...";
        saveText->refresh();
        // TODO: save
        Assets::sync_files([this](bool success)
        {
            saveText->str = success ? "Saved" : "Failed";
            saveText->refresh();
            /*
                TODO: add a timer to game engine
                Timer::wait([this](){ saveText->str = "Save"; saveText->refresh(); });
                Possibly add some kind of animation/transition library as well
                Animate::image(img, 0.5, [](UIImageData *out) { out.pos = Vector2(0,1); });
                or
                UIImageData data;
                data.pos = Vector2(0,1);
                Animate::text(txt, 0.5, data);
            */
        });
    };
    saveText = text_for_img("Save", saveBtn);

    coordsText = UIText::create("0, 0");
    coordsText->anchor = Vector2(0, -1);
    coordsText->pivot = Vector2(0, 0);
    coordsText->pos = Vector2(0, 0);
    coordsText->scale = Vector2(0.5, 0.1);
    coordsText->pos = Vector2(0, 0.05f);


    otherMat = std::make_unique<Material>(colShader.get());
    otherMat->set_uniform("u_color", Vector4(0.3,0.3,0.5,1));
    Object2D *other = Object2D::create(square.get(), otherMat.get());
    other->scale = Vector2(10000, 100);
    other->position = Vector2(0, -1 - other->scale.y/2);

    spikeMat = std::make_unique<Material>(colShader.get());
    spikeMat->set_uniform("u_color", Vector4(0.75,0.75,0.75,1));
    uintg level = globalScene->get_component<DashSceneData>()->level;

    BinaryFileReader reader = BinaryFileReader(Assets::data_path()+"/level"+std::to_string(level));
    if (reader)
    {
        while(1)
        {
            ucharG header = reader.read<ucharG>();
            if (!reader)
                break;
            if (header == Header::OBJ2D)
            {
                ucharG meshType = reader.read<ucharG>();
                ucharG colliderType = reader.read<ucharG>();
                Vector2 pos = reader.read<Vector2>();
                Vector2 scale = reader.read<Vector2>();
                float rotation = reader.read<float>();
                float zIndex = reader.read<float>();
                assert(reader);

                Mesh *mesh=0;
                Material *material=0;
                if (meshType == MeshType::SQUARE) mesh=square.get(), material=spikeMat.get();
                else if (meshType == MeshType::TRIANGLE) mesh = triangle.get(), material=spikeMat.get();
                else assert(false);
                Object2D *obj = Object2D::create(mesh, material);
                obj->position = pos;
                obj->scale = scale;
                obj->rotation = rotation;
                obj->zIndex(zIndex);
            }
            else assert(false);
        }
    }
}
DashEditor::~DashEditor()
{
    Renderer::set_clear_color(0,0,0,1);
    Camera::main->reset();
}
ADD_SCENE_COMPONENT("DashEditor", DashEditor);