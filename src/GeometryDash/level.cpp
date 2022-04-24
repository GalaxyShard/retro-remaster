#include <utils.hpp>
#include <global.hpp>
#include <GeometryDash/dash.hpp>
#include <GeometryDash/physics.hpp>

/*
    DashMenu
        Level button - loads selected level and runs it
        Editor button - loads selected level and the editor
        Level selector - textfield (difficult to implement), stepper (simple, two buttons and a text), or slider (difficult to use)
        Use globalScene->add_component<DashSceneData>(); to pass data between scenes
    DashEditor
        Save button
        Execute button - Saves the level, then loads DashLevel scene if successfully saved (add a callback for asset refresh for webgl)
        Camera panning
        Buttons to place/delete/move objects
        Click to select objects
    DashLevel
    *   Pause button
    *       Resume button
    *       Exit button (goes back to previous scene, either editor or menu)
        Physics
        Load level file on startup, immediately start level after loading
*/
struct DashObjData
{
    Object2D *obj;
    std::unique_ptr<Material> mat;
    DashObjData(Object2D *obj, Shader *shader)
        : obj(obj), mat(std::make_unique<Material>(shader)) {}
};
struct DashLevel
{
    Listener renderConn;
    ListenerT<TouchData> inputConn;
    AssetRef<Texture> menuTex, bgTex;
    AssetRef<Shader> colShader, tintShader;
    AssetRef<Mesh> square, triangle;

    std::unique_ptr<AudioPlayer> bgAudio;
    std::unique_ptr<Material> playerMat, floorMat, spikeMat, bgMat;

    UIText *coordsText;

    std::vector<DashCollider> colliders;
    std::vector<DashObjData> objData;
    Object2D *player, *bg;


    // slow  8.382
    // 1x    10.386
    // 2x    12.914
    // 3x    15.6
    // 4x    19.2
    float speed = 10.386f;
    float gravity = -80.f;
    float jumpHeight = 2;
    Vector3 vel;

    bool jump=0, gameEnded=0, isGrounded=1;

    void pre_render();
    void end_game(bool won);
    void handle_touch(TouchData data);
    DashLevel();
    ~DashLevel();
};
void DashLevel::end_game(bool won)
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

    Object2D::destroy(player);
}
float move_towards(float current, float goal, float speed)
{
    float to = goal - current;
    float s = abs(speed);
    if (abs(to) <= s) return goal;
    if (current < goal)
        return current + s;
    else
        return current - s;
}
void DashLevel::handle_touch(TouchData data)
{
    jump = data.state != TouchState::RELEASED;
}
void DashLevel::pre_render()
{
    if (jump && isGrounded)
    {
        vel.y = sqrt(jumpHeight*2*-gravity);
    }
    player->position.x += speed * Time::delta();
    player->position.y += vel.y * Time::delta();
    if (!isGrounded)
        vel.y += gravity * Time::delta();

    Vector2 playerPoints[4];
    {
        Vector2 corner = player->scale*0.5f;
        playerPoints[0] = Vector2(-1, -1);
        playerPoints[1] = Vector2( 1, -1);
        playerPoints[2] = Vector2(-1,  1);
        playerPoints[3] = Vector2( 1,  1);
        for (uintg i = 0; i < 4; ++i)
            playerPoints[i] = (corner*playerPoints[i]);
    }
    Vector2 tempPlayer[4];
    bool didCollide = 0;

    for (auto &collider : colliders)
    {
        if (collider.obj->wasCulled())
            continue;
        for (uintg i = 0; i < 4; ++i)
            tempPlayer[i] = player->position + playerPoints[i];
        
        DashCollisionData mainHitbox = test_collision(tempPlayer, &collider);
        if (mainHitbox.mtv.y > 0)
            player->position.y += mainHitbox.mtv.y;

        for (uintg i = 0; i < 4; ++i)
            tempPlayer[i].y -= 0.05f;

        if (!collider.killOnTouch)
        {
            DashCollisionData groundData = test_collision(tempPlayer, &collider);
            didCollide |= groundData.collided;
        }
        for (uintg i = 0; i < 4; ++i)
            tempPlayer[i] = player->position + playerPoints[i]*0.5f;
        DashCollisionData solidHitbox = test_collision(tempPlayer, &collider);

        //if ((collider.killOnTouch && mainHitbox.collided) || mainHitbox.mtv.x < -0.1f || mainHitbox.mtv.y < -0.01f)
        if ((collider.killOnTouch && mainHitbox.collided) || solidHitbox.mtv.x < 0.f || solidHitbox.mtv.y < 0.f)
        {
            end_game(0);
            return;
        }
    }
    if (didCollide)
    {
        vel.y = 0;
        float increment = Math::PI*0.5f;
        float r = -player->rotation;
        //float toR = roundf(r/increment)*increment;
        float toR = ceilf(r/increment-0.01f)*increment;

        player->rotation = -move_towards(r, toR, std::max(abs(r-toR), 0.1f)*20.f*Time::delta());
        isGrounded = 1;
    }
    else 
    {
        player->rotation -= Math::PI*1.75f*Time::delta();
        isGrounded = 0;
    }
    player->dirtyBounds();
    Camera::main->position.x = player->position.x + 7.5f;
    bg->position.x = Camera::main->position.x*0.9;

    char buffer[16];
    snprintf(buffer, 16, "%d, %d", (int)Camera::main->position.x, (int)Camera::main->position.y);
    coordsText->str = buffer;
    coordsText->refresh();
}
DashLevel::DashLevel()
{
    Camera::main->set_bg(0.2,0.2,0.3);
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

    UIImage *menuBtn = UIImage::create(menuTex.get());
	TINT_ON_CLICK(menuBtn, (1,1,1,1), (0.75,0.75,0.75,1));
    menuBtn->anchor = Vector2(1, 1);
    menuBtn->scale = Vector2(0.15, 0.15);
    menuBtn->pos = Vector2(-0.25f, -menuBtn->scale.y/2-0.02);
    menuBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Dash");
    };
    coordsText = UIText::create("0, 0");
    coordsText->anchor = Vector2(0, -1);
    coordsText->pivot = Vector2(0, 0);
    coordsText->scale = Vector2(0.5, 0.1);
    coordsText->pos = Vector2(0, 0.05f);

    playerMat = std::make_unique<Material>(colShader.get());
    playerMat->set_uniform("u_color", Vector4(1,1,1,1));
    player = Object2D::create(square.get(), playerMat.get());
    player->scale = Vector2(1, 1);
    player->position = Vector2(0, -0.75);
    player->zIndex(1);

    floorMat = std::make_unique<Material>(colShader.get());
    floorMat->set_uniform("u_color", Vector4(0.3,0.3,0.5,1));
    Object2D *floor = Object2D::create(square.get(), floorMat.get());
    floor->scale = Vector2(10000, 100);
    floor->position = Vector2(0, -1 - floor->scale.y/2);

    spikeMat = std::make_unique<Material>(colShader.get());
    spikeMat->set_uniform("u_color", Vector4(0.75,0.75,0.75,1));

    colliders.push_back(DashCollider(floor, ColliderType::AABB, 0));

    bgTex = Texture::load(Assets::path()+"/textures/ping4-bg.png", Texture::Pixel);
    bgMat = std::make_unique<Material>(tintShader.get());
    bgMat->mainTex = bgTex.get();
    bgMat->set_uniform("u_color", Vector4(0.25,0.25,0.25,1));
    
    bg = Object2D::create(square.get(), bgMat.get());
    bg->scale = Vector2(250,250);
    bg->zIndex(-1000);
    

    // File format
    // (short) version
    // object list:
    //     (enum) mesh
    //     (vec2) pos
    //     (vec2) scale
    //     (float) rotation
    //     (float) z-index
    //     (vec3) color

    uintg level = globalScene->get_component<DashSceneData>()->level;

    BinaryFileReader reader = BinaryFileReader(Assets::data_path()+"/level"+std::to_string(level));
    if (reader)
    {
        shortg version = reader.read<shortg>();
        bool failed = 0;
        if (version != 0)
        {
            logmsg("Invalid version\n");
            failed = 1;
            goto END_READ;
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
                logmsg("Error reading\n");
                failed = 1;
                goto END_READ;
            }

            Mesh *mesh=0;
            if (meshType == MeshType::SQUARE) mesh=square.get();
            else if (meshType == MeshType::TRIANGLE) mesh = triangle.get();
            else
            {
                logmsg("Error reading\n");
                failed = 1;
                goto END_READ;
            }
            Object2D *obj = Object2D::create(mesh, nullptr);
            obj->position = pos;
            obj->scale = scale;
            obj->rotation = rotation;
            obj->zIndex(zIndex);

            DashObjData &data = objData.emplace_back(obj, colShader.get());
            data.mat->set_uniform("u_color", Vector4(col.x,col.y,col.z,1));
            obj->material = data.mat.get();

            colliders.push_back(DashCollider(obj,
                meshType == MeshType::SQUARE ? ColliderType::AABB : ColliderType::TRIANGLE,
                mesh==triangle.get()));
        }
END_READ:
        if (failed)
        {

            return;
        }
    }

    Input::add_bind("jump", Key::Space, [this](bool p){jump=p;});
}
DashLevel::~DashLevel()
{
    Input::remove_bind("jump");
    Camera::main->reset();
}
ADD_SCENE_COMPONENT("DashLevel", DashLevel);