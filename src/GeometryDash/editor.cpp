#include <utils.hpp>
#include <global.hpp>
#include <GeometryDash/dash.hpp>

struct DashObjectData
{
    Object2D *obj;

};
struct DashEditState { enum : ucharG { PAN,MOVE,TRASH }; };

struct DashEditor
{
    AssetRef<Texture> menuTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square, triangle;
    std::unique_ptr<Material> otherMat, spikeMat;

    ListenerT<TouchData> inputConn;

    std::vector<DashObjectData> objData;

    UIImage *trashBtn, *panBtn, *moveBtn;
    UIText *coordsText, *saveText;

    uintg selectedObj = ~0u;
    uintg saveSlot = 0;
    uintg timerID = ~0u;
    ucharG currentState = DashEditState::PAN;
    bool saveDebounce = 0;

    void handle_touch(TouchData data);
    void save_finished(bool success);
    void save_game();
    void set_state(ucharG state);
    uintg find_obj(Vector2 screenPos);

    DashEditor();
    ~DashEditor();
};
void DashEditor::set_state(ucharG state)
{
    const Vector4 disabled = Vector4(0.75, 0.75, 0.75, 1);
    const Vector4 enabled = Vector4(1,1,1,1);
    panBtn->tint = disabled;
    moveBtn->tint = disabled;
    trashBtn->tint = disabled;

    if (state == currentState)
        currentState = DashEditState::PAN;
    else currentState = state;
    
    if (state == DashEditState::PAN) panBtn->tint = enabled;
    else if (state == DashEditState::MOVE) moveBtn->tint = enabled;
    else if (state == DashEditState::TRASH) trashBtn->tint = enabled;
}
uintg DashEditor::find_obj(Vector2 screenPos)
{
    Vector2 pos = (Vector2)Camera::main->position + screenPos*Camera::main->orthoSize;
    for (uintg i = 0; i < objData.size(); ++i)
    {
        Object2D *obj = objData[i].obj;
        Vector2 halfScale = obj->scale*0.5;
        Vector2 objPos = obj->position;
        Vector2 min = objPos - halfScale;
        Vector2 max = objPos + halfScale;
        if (Math::within(pos.x, min.x, max.x) && Math::within(pos.y, min.y, max.y))
            return i;
    }
    return ~0u;
}
void DashEditor::handle_touch(TouchData data)
{
    if (currentState == DashEditState::PAN)
    {
        Vector3 &camPos = Camera::main->position;
        camPos -= data.delta * 10;
        char buffer[16];
        snprintf(buffer, 16, "%d, %d", (int)camPos.x, (int)camPos.y);
        coordsText->str = buffer;
        coordsText->refresh();
    }
    else if (currentState == DashEditState::TRASH)
    {
        if (data.state == TouchState::PRESSED)
        {
            uintg index = find_obj(data.pos);
            if (index != ~0u)
            {
                Object2D::destroy(objData[index].obj);
                objData.erase(objData.begin()+index);
            }
        }
    }
    else if (currentState == DashEditState::MOVE)
    {
        if (data.state == TouchState::PRESSED)
        {
            selectedObj = find_obj(data.pos);
        }
        else if (data.state == TouchState::MOVED)
        {
            if (selectedObj != ~0u)
            {
                objData[selectedObj].obj->position += data.delta*Camera::main->orthoSize;
            }
        }
    }
}
void DashEditor::save_finished(bool success)
{
    saveText->str = success ? "Saved" : "Failed";
    saveText->refresh();
    timerID = Timer::wait(0.5f, [this]()
    {
        timerID = ~0u;
        saveText->str = "Save";
        saveText->refresh();
        saveDebounce = 0;
    });
    /*
        Possibly add some kind of animation/transition library to engine
        Animate::image(img, 0.5, [](UIImageData *out) { out.pos = Vector2(0,1); });
        or
        UIImageData data;
        data.pos = Vector2(0,1);
        Animate::text(txt, 0.5, data);
    */
}
void DashEditor::save_game()
{
    if (saveDebounce)
        return;
    saveDebounce = 1;
    saveText->str = "Saving...";
    saveText->refresh();

    std::ofstream out = std::ofstream(Assets::data_path()+"/level"+std::to_string(saveSlot), std::ios::binary);
    if (!out)
    {
        save_finished(0);
        return;
    }
    BinaryWriter writer;
    for (auto &data : objData)
    {
        ucharG meshType;
        ucharG colliderType;

        if (data.obj->mesh == square.get())
            meshType = MeshType::SQUARE, colliderType = ColliderType::AABB;
        else if (data.obj->mesh == triangle.get())
            meshType = MeshType::TRIANGLE, colliderType = ColliderType::TRIANGLE;
        else throw("unsupported mesh");

        writer.write<ucharG>(Header::OBJ2D);
        writer.write<ucharG>(meshType);
        writer.write<ucharG>(colliderType);
        writer.write<Vector2>(data.obj->position);
        writer.write<Vector2>(data.obj->scale);
        writer.write<float>(data.obj->rotation);
        writer.write<float>(data.obj->zIndex());
    }

    out << writer.get_buffer();
    if (!out)
    {
        save_finished(0);
        return;
    }

    Assets::sync_files(TYPE_LAMBDA(save_finished, bool));
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
    saveBtn->onClick = CLASS_LAMBDA(save_game);
    saveText = text_for_img("Save", saveBtn);

    panBtn = UIImage::create();
    panBtn->anchor = Vector2(-1, -1);
    panBtn->scale = Vector2(0.35, 0.15);
    panBtn->pos = Vector2(0.175f, 0.075);
    panBtn->onClick = [this]() { set_state(DashEditState::PAN); };
    text_for_img("Pan", panBtn);

    moveBtn = UIImage::create();
    moveBtn->anchor = Vector2(-1, -1);
    moveBtn->scale = Vector2(0.35, 0.15);
    moveBtn->pos = Vector2(0.175f, 0.075*3);
    moveBtn->onClick = [this]() { set_state(DashEditState::MOVE); };
    text_for_img("Move", moveBtn);

    trashBtn = UIImage::create();
    trashBtn->anchor = Vector2(-1, -1);
    trashBtn->scale = Vector2(0.35, 0.15);
    trashBtn->pos = Vector2(0.175f, 0.075*5);
    trashBtn->onClick = [this]() { set_state(DashEditState::TRASH); };
    text_for_img("Trash", trashBtn);

    set_state(DashEditState::PAN);

    UIImage *cubeBtn = UIImage::create();
    cubeBtn->anchor = Vector2(1, -1);
    cubeBtn->scale = Vector2(0.35, 0.15);
    cubeBtn->pos = Vector2(-0.175f, 0.075);
    cubeBtn->onClick = [this]()
    {
        Object2D *obj = Object2D::create(square.get(), spikeMat.get());
        obj->position = Camera::main->position;
        DashObjectData data;
        data.obj = obj;
        objData.push_back(data);
    };
    text_for_img("New Square", cubeBtn);

    UIImage *spikeBtn = UIImage::create();
    spikeBtn->anchor = Vector2(1, -1);
    spikeBtn->scale = Vector2(0.35, 0.15);
    spikeBtn->pos = Vector2(-0.175f, 0.075*2);
    spikeBtn->onClick = [this]()
    {
        Object2D *obj = Object2D::create(triangle.get(), spikeMat.get());
        obj->position = Camera::main->position;
        DashObjectData data;
        data.obj = obj;
        objData.push_back(data);
    };
    text_for_img("New Spike", spikeBtn);

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
    saveSlot = globalScene->get_component<DashSceneData>()->level;

    BinaryFileReader reader = BinaryFileReader(Assets::data_path()+"/level"+std::to_string(saveSlot));
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

                DashObjectData data;
                data.obj = obj;
                objData.push_back(data);
            }
            else assert(false);
        }
    }
}
DashEditor::~DashEditor()
{
    if (timerID != ~0u)
        Timer::cancel(timerID);

    Renderer::set_clear_color(0,0,0,1);
    Camera::main->reset();
}
ADD_SCENE_COMPONENT("DashEditor", DashEditor);