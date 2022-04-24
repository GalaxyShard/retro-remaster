#include <utils.hpp>
#include <global.hpp>
#include <GeometryDash/dash.hpp>

constexpr uintg NO_OBJ = ~0u;
struct EditorObjData
{
    Object2D *obj;
    std::unique_ptr<Material> mat;
    Vector4 color;
    EditorObjData(Object2D *obj, Shader *shader)
        : obj(obj), mat(std::make_unique<Material>(shader)) {}
};
struct DashEditState { enum : ucharG { PAN,TRASH }; };

struct UIElement
{
    void *data;
    enum {UIIMAGE,UITEXT} type;
    UIElement(UIImage *img) : data(img), type(UIIMAGE){}
    UIElement(UIText *txt) : data(txt), type(UITEXT){}
};
struct DashEditorMenu
{
    UIGroup *group;
    std::vector<UIElement> ui;
};
struct DashEditor
{
    AssetRef<Texture> menuTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square, triangle;
    std::unique_ptr<Material> floorMat, spikeMat;

    ListenerT<TouchData> inputConn;

    std::vector<EditorObjData> objData;

    UIImage *trashBtn, *panBtn;
    UIText *coordsText, *saveText;

    DashEditorMenu *colorEditor = 0;

    uintg selectedObj = NO_OBJ;
    uintg saveSlot = 0;
    uintg timerID = ~0u;
    ucharG currentState = DashEditState::PAN;
    bool saveDebounce = 0;
    bool snap = 1;


    void handle_touch(TouchData data);
    void save_finished(bool success);
    void save_game(bool exitOnFinish = 0);
    void set_state(ucharG state);
    uintg find_obj(Vector2 screenPos);
    void select_obj(uintg obj);
    float get_snap();

    DashEditor();
    ~DashEditor();
};
static Vector2 roundv(Vector2 v, float increment)
{
    return Vector2(roundf(v.x/increment)*increment, roundf(v.y/increment)*increment);
}
static float dir_to_angle(Vector2 dir)
{
    return atan2f(dir.x, dir.y);
}
float DashEditor::get_snap() { return snap ? 0.5f : 0.1f; }
void DashEditor::select_obj(uintg obj)
{
    if (selectedObj != NO_OBJ)
        objData[selectedObj].mat->set_uniform("u_color", objData[selectedObj].color);
    selectedObj = obj;
    if (obj != NO_OBJ)
        objData[obj].mat->set_uniform("u_color", Vector4(0.5,0.75,1,1));
}
void DashEditor::set_state(ucharG state)
{
    const Vector4 disabled = Vector4(0.75, 0.75, 0.75, 1);
    const Vector4 enabled = Vector4(1,1,1,1);
    panBtn->tint = disabled;
    trashBtn->tint = disabled;

    if (state == currentState)
        currentState = DashEditState::PAN;
    else currentState = state;
    
    if (currentState == DashEditState::PAN) panBtn->tint = enabled;
    else if (currentState == DashEditState::TRASH) trashBtn->tint = enabled;
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
    return NO_OBJ;
}
void DashEditor::handle_touch(TouchData data)
{
    if (currentState == DashEditState::PAN)
    {
        if (data.state == TouchState::PRESSED)
        {
            if (find_obj(data.pos) != selectedObj)
                select_obj(NO_OBJ);
        }
        if(data.state == TouchState::RELEASED)
        {
            if ((data.pos-data.startPos).sqr_magnitude() < 0.01*0.01)
                select_obj(find_obj(data.pos));
            else if (find_obj(data.pos) != selectedObj)
                select_obj(NO_OBJ);
            return;
        }
        if (selectedObj == NO_OBJ)
        {
            Vector3 &camPos = Camera::main->position;
            camPos -= data.delta * 10;
            char buffer[16];
            snprintf(buffer, 16, "%d, %d", (int)camPos.x, (int)camPos.y);
            coordsText->str = buffer;
            coordsText->refresh();
        }
        else
        {
            Object2D *obj = objData[selectedObj].obj;
            Vector2 pos = (Vector2)Camera::main->position + data.pos*Camera::main->orthoSize;
            obj->position = roundv(pos, get_snap());
        }
    }
    else if (currentState == DashEditState::TRASH)
    {
        if (data.state == TouchState::PRESSED)
        {
            uintg index = find_obj(data.pos);
            if (index != NO_OBJ)
            {
                select_obj(NO_OBJ);
                Object2D::destroy(objData[index].obj);
                objData.erase(objData.begin()+index);
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
void DashEditor::save_game(bool exitOnFinish)
{
    if (saveDebounce)
        return;
    saveDebounce = 1;
    saveText->str = "Saving...";
    saveText->refresh();

    std::string path = Assets::data_path()+"/level"+std::to_string(saveSlot);
    if (objData.size() == 0)
        remove(path.c_str());
    else
    {
        std::ofstream out = std::ofstream(path, std::ios::binary);
        if (!out)
        {
            save_finished(0);
            return;
        }
        BinaryWriter writer;
        writer.write<shortg>(0);
        for (auto &data : objData)
        {
            ucharG meshType;

            if (data.obj->mesh == square.get())
                meshType = MeshType::SQUARE;
            else if (data.obj->mesh == triangle.get())
                meshType = MeshType::TRIANGLE;
            else throw("unsupported mesh");

            writer.write<ucharG>(meshType);
            writer.write<Vector2>(data.obj->position);
            writer.write<Vector2>(data.obj->scale);
            writer.write<float>(data.obj->rotation);
            writer.write<float>(data.obj->zIndex());
            writer.write<Vector3>(data.color);
        }
        out << writer.get_buffer();
        if (!out)
        {
            save_finished(0);
            return;
        }
    }
    if (exitOnFinish)
        Assets::sync_files([this](bool success)
        {
            save_finished(success);
            if (success)
            {
                Scene::destroy(Scene::activeScene);
                Scene::create("Dash");
            }
        });
    else Assets::sync_files(TYPE_LAMBDA(save_finished, bool));
}
#define TINT_ON_CLICK_D(img) TINT_ON_CLICK(img, (1,1,1,1), (0.75,0.75,0.75,1))

DashEditor::DashEditor()
{
    Camera::main->set_bg(0.2,0.2,0.3);
    Camera::main->orthoSize = 10;
    Camera::main->refresh();
    Camera::main->position.x = 7.5;

    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);

    square = Mesh::load_obj(Assets::gpath()+"/models/square.obj");
    triangle = Mesh::load_obj(Assets::path()+"/models/triangle.obj");
    colShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/color.shader");
    tintShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/tint.shader");

    inputConn = Input::touch_changed().connect(TYPE_LAMBDA(handle_touch, TouchData));

    UIImage *menuBtn = UIImage::create(menuTex.get());
	TINT_ON_CLICK(menuBtn, (1,1,1,1), (0.75,0.75,0.75,1));
    menuBtn->anchor = Vector2(1, 1);
    menuBtn->scale = Vector2(0.15, 0.15);
    menuBtn->pos = Vector2(-0.25f, -menuBtn->scale.y/2-0.02);
    menuBtn->onClick = [this]() { save_game(1); };

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

    trashBtn = UIImage::create();
    trashBtn->anchor = Vector2(-1, -1);
    trashBtn->scale = Vector2(0.35, 0.15);
    trashBtn->pos = Vector2(0.175f, 0.075*3);
    trashBtn->onClick = [this]() { set_state(DashEditState::TRASH); };
    text_for_img("Trash", trashBtn);

    set_state(DashEditState::PAN);

    UIImage *squareBtn = UIImage::create();
    squareBtn->anchor = Vector2(1, -1);
    squareBtn->scale = Vector2(0.35, 0.15);
    squareBtn->pos = Vector2(-0.175f, 0.075);
    TINT_ON_CLICK_D(squareBtn);
    squareBtn->onClick = [this]()
    {
        Object2D *obj = Object2D::create(square.get(), nullptr);
        obj->position = roundv(Camera::main->position, get_snap());
        EditorObjData &data = objData.emplace_back(obj, colShader.get());
        data.color = Vector4(1,1,1,1);
        data.mat->set_uniform("u_color", data.color);
        obj->material = data.mat.get();
    };
    text_for_img("New Square", squareBtn);

    UIImage *spikeBtn = UIImage::create();
    spikeBtn->anchor = Vector2(1, -1);
    spikeBtn->scale = Vector2(0.35, 0.15);
    spikeBtn->pos = Vector2(-0.175f, 0.075*3);
    TINT_ON_CLICK_D(spikeBtn);
    spikeBtn->onClick = [this]()
    {
        Object2D *obj = Object2D::create(triangle.get(), nullptr);
        obj->position = roundv(Camera::main->position, get_snap());
        EditorObjData &data = objData.emplace_back(obj, colShader.get());
        data.color = Vector4(1,1,1,1);
        data.mat->set_uniform("u_color", data.color);
        obj->material = data.mat.get();

    };
    text_for_img("New Spike", spikeBtn);

    UIImage *snapBtn = UIImage::create();
    snapBtn->anchor = Vector2(1, -1);
    snapBtn->scale = Vector2(0.35, 0.15);
    snapBtn->pos = Vector2(-0.175f*3, 0.075);
    snapBtn->onClick = [this]()
    {
        snap = !snap;
        UIImage::get_held()->tint = snap ? Vector4(1,1,1,1) : Vector4(0.75,0.75,0.75,1);
    };
    text_for_img("Snap", snapBtn);

    coordsText = UIText::create("7, 0");
    coordsText->anchor = Vector2(0, -1);
    coordsText->pivot = Vector2(0, 0);
    coordsText->pos = Vector2(0, 0);
    coordsText->scale = Vector2(0.5, 0.1);
    coordsText->pos = Vector2(0, 0.05f);


    floorMat = std::make_unique<Material>(colShader.get());
    floorMat->set_uniform("u_color", Vector4(0.3,0.3,0.5,1));
    Object2D *floor = Object2D::create(square.get(), floorMat.get());
    floor->scale = Vector2(10000, 100);
    floor->position = Vector2(0, -1 - floor->scale.y/2);

    spikeMat = std::make_unique<Material>(colShader.get());
    spikeMat->set_uniform("u_color", Vector4(0.75,0.75,0.75,1));
    saveSlot = globalScene->get_component<DashSceneData>()->level;

    BinaryFileReader reader = BinaryFileReader(Assets::data_path()+"/level"+std::to_string(saveSlot));
    if (reader)
    {
        bool errored = 0;
        shortg version = reader.read<shortg>();
        if (version != 0)
        {
            logmsg("Invalid version\n");
            return;
        }

        while(1)
        {
            ucharG meshType = reader.read<ucharG>();
            if (!reader)
                break;
            Vector2 pos = reader.read<Vector2>();
            Vector2 scale = reader.read<Vector2>();
            float rotation = reader.read<float>();
            float zIndex = reader.read<float>();
            Vector3 col = reader.read<Vector3>();
            if (!reader)
            {
                errored = 1;
                break;
            }

            Mesh *mesh=0;
            if (meshType == MeshType::SQUARE) mesh=square.get();
            else if (meshType == MeshType::TRIANGLE) mesh = triangle.get();
            else { errored=1; break; }
            Object2D *obj = Object2D::create(mesh, nullptr);
            obj->position = pos;
            obj->scale = scale;
            obj->rotation = rotation;
            obj->zIndex(zIndex);

            EditorObjData &data = objData.emplace_back(obj, colShader.get());
            obj->material = data.mat.get();
            data.color = Vector4(col.x,col.y,col.z,1);
            data.mat->set_uniform("u_color", data.color);
        }
        if (errored)
        {
            logmsg("Error reading\n");
            for (auto &d : objData)
                Object2D::destroy(d.obj);
            objData.clear();
        }
    }
}
DashEditor::~DashEditor()
{
    if (timerID != ~0u)
        Timer::cancel(timerID);
    Camera::main->reset();
}
ADD_SCENE_COMPONENT("DashEditor", DashEditor);