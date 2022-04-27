#include "game.hpp"
#include <sstream>
#include <map>
#include <queue>
#include <set>
#include <array>
#include <tuple>
/*
    ENGINE pch, -j4:  12.153, 13.186
    ENGINE -j4:       12.354, 15.173
    ENGINE pch:       41.256

    GAME pch, -j4:  5.570, 5.590
    GAME -j4:       5.673, 5.633
    GAME pch:       9.660
//*/

//struct RigidbodyECS
//{
//    Vector3 position;
//    Vector3 velocity;
//    float drag = .5f;
//};
//struct AABBColliderECS
//{
//    Vector3 min, max;
//};
//class PhysicsSystem : public System<PhysicsSystem, RigidbodyECS, AABBColliderECS>
//{
//public:
//    void func(RigidbodyECS &rb, AABBColliderECS &aabb)
//    {
//        rb.velocity += Vector3(0,-9.81f,0) * Time::delta();
//        rb.position += rb.velocity * Time::delta();
//        rb.velocity *= 1 - (rb.drag * Time::delta());
//        logmsg("test %.3o\n", rb.velocity);
//    }
//};

// OpenAL
//https://indiegamedev.net/2020/02/15/the-complete-guide-to-openal-with-c-part-1-playing-a-sound/
//https://www.youtube.com/watch?v=kWQM1iQ1W0E

/*
    UTILITY IDEA:
        Have an app that takes a file and decodes it based off of information given
        something like "interpret next 4 bytes as an unsigned integer"
    
    PUZZLE GAME IDEA:
        Grab onto items (like fling things & people), drag them around to solve puzzles
*/
// guide: https://devforum.roblox.com/t/rotations-with-quaternions/13209

static void remove_binds(const char *first, ...)
{
    va_list args;
    va_start(args, first);
    const char *current = first;
    while (current != nullptr)
    {
        Input::remove_bind(current);
        current = va_arg(args, const char *);
    }
    va_end(args);
}

// Possibly turn into a mario kart 64 game
Scene *thisScene;
NetTest* comp() { return thisScene->get_component<NetTest>(); }

void cmd_move(NetworkReader reader, Connection conn)
{
    NetworkWriter writer = NetworkWriter();
    writer.write<int>(conn.getID());
    writer.write<Vector3>(reader.read<Vector3>());
    writer.write<float>(reader.read<float>());
    Server::send_all("moveplr", writer);
}
void rpc_move(NetworkReader reader)
{
    int id = reader.read<int>();
    Vector3 pos = reader.read<Vector3>();
    float rotation = reader.read<float>();
    
    if (comp()->players.count(id))
    {
        comp()->players[id]->position = pos;
        comp()->players[id]->rotation = Vector3(0, rotation, 0);
    }
}
void spawn_player(NetworkReader reader)
{
    int id = reader.read<int>();
    comp()->players[id] = new Object(comp()->cube.get(), comp()->cubeMat.get());
    comp()->players[id]->add_component<CubeCollider>();
    comp()->players[id]->scale = Vector3(0.2,1,0.2);
}
void despawn_player(NetworkReader reader)
{
    int id = reader.read<int>();
    if (comp()->players.count(id))
    {
        delete comp()->players[id];
        comp()->players.erase(id);
    }
}
static void fix_projection()
{
    Camera::main->projection = Matrix4x4::perspective(
        60.f*Math::to_rad,
        ((float)Renderer::screenWidth)/Renderer::screenHeight, 
        0.01, 1000, 0);
}
void NetTest::play_audio()
{
    if (!jumpPlayer)
    {
        AssetRef<AudioData> soundtrack = AudioData::load_wav(Assets::path()+"/audio/successSound.wav");
        jumpPlayer = std::make_unique<AudioPlayer>(soundtrack.get());
    }
    jumpPlayer->play();
}
void NetTest::grab_item()
{
    if (heldObject)
    {
        if (Rigidbody *body = heldObject->get_component<Rigidbody>())
        {
            body->drag /= 20;
        }
        heldObject = 0;
        return;
    }
    RayResult result = Physics::raycast(Ray(Camera::main->position, Matrix3x3::rotate(Camera::main->rotation)*Vector3(0,0,-1)));
    if (result.collided && result.dist <= 7.5)
    {
        heldObject = result.object;
        if (Rigidbody *body = heldObject->get_component<Rigidbody>())
        {
            body->drag *= 20;
        }
        distanceToHeld = result.dist;
        initialCamRotation = Camera::main->rotation;
        initialObjRotation = result.object->rotation;
        objToContact = -result.pos + result.object->position;
    }
}
static Vector3 dir_to_angle(Vector3 dir, Vector3 up = Vector3(0,1,0))
{
    dir = dir.unit();
    Vector3 s = Vector3::cross(dir, up).unit();
    Vector3 u = Vector3::cross(s, dir);
    Matrix3x3 r = Matrix3x3(
        s.x,s.y,s.z,
        u.x,u.y,u.z,
        -dir.x,-dir.y,-dir.z
    );
    return Vector3(
        atan2f(r[2][1], r[2][2])*Math::to_deg,
        atan2f(-r[2][0], sqrtf(Math::sqr(r[2][1])+Math::sqr(r[2][2])))*Math::to_deg,
        atan2f(r[1][0], r[0][0])*Math::to_deg
    );

}
NetTest::NetTest()
{
    //Vector3 objectPosition = Vector3(0,0,0);
    //Matrix3x3 rotation = Matrix3x3::rotate(0, 0, 45*Math::to_rad);
    //Vector3 point = Vector3(1, 0, 0);
    ////Vector3 worldSpacePoint = (rotation * (point-objectPosition)) + objectPosition;
    //Vector3 worldSpacePoint = rotation * point;
    //logmsg("%o\n", worldSpacePoint);
    //// if rotation is -90 degrees on the Y axis and objectPosition is zero, worldSpacePoint will be Vector3(0, 0, 1)


    thisScene = Scene::activeScene;
    fix_projection();
    aspectConn = Renderer::aspect_ratio_changed().connect(&fix_projection);

    {
        AssetRef<AudioData> soundtrack = AudioData::load_wav(Assets::path()+"/audio/peaceful.wav");
        backgroundPlayer = std::make_unique<AudioPlayer>(soundtrack.get());
    }
    backgroundPlayer->loop(1);
    backgroundPlayer->play();

    inputConn = Renderer::pre_render().connect(MK_LAMBDA(controller_pre_render));
    touchConn = Input::touch_changed().connect(ARG_LAMBDA(touch_changed));
    physicsConn = Renderer::post_simulation().connect(MK_LAMBDA(post_physics));

    Input::add_bind("grab", Key::F, ARG_LAMBDA(grab_item));
    Input::add_bind("jump", Key::Space, ARG_LAMBDA(set_move_up));
    Input::add_bind("forward", Key::W);
    Input::add_bind("back", Key::S);
    Input::add_bind("left", Key::A);
    Input::add_bind("right", Key::D);
    Input::add_bind("closer", Key::Q, ARG_LAMBDA(pull_item));
    Input::add_bind("further", Key::E, ARG_LAMBDA(push_item));
    
    cube = Mesh::from_obj(Assets::gpath()+"/models/cube.obj");
    sphere = Mesh::from_obj(Assets::gpath()+"/models/smooth_sphere.obj");
    colShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/color.shader");
    texShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/texture.shader");
    tintShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/tint.shader");
    colLightShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/lighting_color.shader");
    texLightShader = Shader::load(Assets::gpath()+SHADER_FOLDER+"/lighting_tex.shader");

    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);
    objTex = Texture::load(Assets::path()+"/textures/cubemap.png", Texture::Pixel);
    joystickTex = Texture::load(Assets::path()+"/textures/joystick.png", Texture::Pixel);
    handleTex = Texture::load(Assets::path()+"/textures/handle.png", Texture::Pixel);

    const Vector3 lightPos = Vector3(0, 2, 0);
    //const float lightRadius = 15;
    lightMat = std::make_unique<Material>(colShader.get());
    lightMat->set_uniform("u_color", Vector4(1,1,1,1));
    lightObj = new Object(sphere.get(), lightMat.get());
    lightObj->position = lightPos;
    lightObj->scale = Vector3(0.2,0.2,0.2);
    lightObj->add_component<SphereCollider>();


    cubeMat = std::make_unique<Material>(texLightShader.get());
    cubeMat->mainTex = objTex.get();
    cubeMat->set_uniform("u_lightColor", Vector4(1,1,1,1));
    cubeMat->set_uniform("u_lightPos", lightPos);
    //cubeMat->set_uniform("u_lightRadius", lightRadius);
    cubeMat->set_uniform("ambient", 0.2f);

    collisionMat = std::make_unique<Material>(colLightShader.get());
    //collisionMat->mainTex = handleTex.get();
    collisionMat->set_uniform("u_color", Vector4(1,1,1,1));
    collisionMat->set_uniform("u_lightColor", Vector4(1,1,1,1));
    collisionMat->set_uniform("u_lightPos", lightPos);
    //collisionMat->set_uniform("u_lightRadius", lightRadius);
    collisionMat->set_uniform("ambient", 0.1f);


    floorMat = std::make_unique<Material>(texLightShader.get());
    floorMat->mainTex = objTex.get();
    floor = new Object(cube.get(), floorMat.get());
    floor->position.y = -0.5-0.05;
    floor->scale = Vector3(10, 0.1, 10);
    floor->add_component<CubeCollider>();
    

    Camera::main->isPerspective = 1;
    Camera::main->position = Vector3(0,0,3);
    player = new Object(cube.get(), cubeMat.get());
    player->scale = Vector3(0.2, 1, 0.2);
    player->position = Vector3(0,0,3);
    rigidbody = player->add_component<Rigidbody>();
    rigidbody->drag = .5f;
    rigidbody->mass = 65;
    rigidbody->freezeRotation = 1;
    player->add_component<CubeCollider>();

    collisionTest = new Object(sphere.get(), collisionMat.get());
    collisionTest->position = Vector3(-1, 1, 1);
    collisionTest->add_component<SphereCollider>();
    collisionTest->add_component<Rigidbody>()->drag = 1.f;
    //collisionTest->render_order(-1);


    //Entity testEntity = Entity::create(*ECSManager::main);
    //testEntity.add_comp(ObjRendererECS{.mesh = cube.get(), .shader = texLightShader.get(), .mainTex = handleTex.get()}, *ECSManager::main);
    //testEntity.add_comp(TransformECS(), *ECSManager::main);


    UIImage *menuBtn = new UIImage(menuTex.get());
    menuBtn->anchor = Vector2(1, 1);
    menuBtn->scale = Vector2(0.15, 0.15);

    menuBtn->pos = Vector2(-0.1f, -menuBtn->scale.y/2-0.02);
    menuBtn->onClick = []()
    {
        delete thisScene;
        new Scene("Start");
    };
    joystick = new UIImage(joystickTex.get());
    joystick->group = UIGroup::safeArea;
    joystick->scale = Vector2(0.5,0.5);
    joystick->pos = Vector2(0.35, 0.35);

    // TODO replace raw function pointers w Callbacks
    upBtn = new UIImage(menuTex.get());
    upBtn->tint = Vector4(0.5,0.5,1,1);
    upBtn->group = UIGroup::safeArea;
    upBtn->scale = Vector2(0.25,0.25);
    upBtn->pos = Vector2(-0.3,0.3);
    upBtn->anchor = Vector2(1, -1);
    upBtn->onTouchDown = []() { Input::trigger("jump", 1); };
    upBtn->onTouchUp = []() { Input::trigger("jump", 0); };
    //upBtn->onTouchDown = []() { comp()->set_move_up(1); };
    //upBtn->onTouchUp = []() { comp()->set_move_up(0); };

    grabBtn = new UIImage(handleTex.get());
    grabBtn->tint = Vector4(0.5,0.5,1,1);
    grabBtn->group = UIGroup::safeArea;
    grabBtn->scale = Vector2(0.25,0.25);
    grabBtn->pos = Vector2(-0.3,0.55);
    grabBtn->anchor = Vector2(1, -1);
    grabBtn->onTouchDown = []() { Input::trigger("grab",1); };
    grabBtn->onTouchUp = []() { Input::trigger("grab",0); };

    pushBtn = new UIImage(handleTex.get());
    pushBtn->tint = Vector4(0.5,0.5,1,1);
    pushBtn->group = UIGroup::safeArea;
    pushBtn->scale = Vector2(0.2,0.2);
    pushBtn->pos = Vector2(-0.15,1.2);
    pushBtn->anchor = Vector2(1, -1);
    pushBtn->onTouchDown = []() { Input::trigger("further",1); };
    pushBtn->onTouchUp = []() { Input::trigger("further",0); };

    pullBtn = new UIImage(handleTex.get());
    pullBtn->tint = Vector4(0.5,0.5,1,1);
    pullBtn->group = UIGroup::safeArea;
    pullBtn->scale = Vector2(0.2,0.2);
    pullBtn->pos = Vector2(-0.15,1);
    pullBtn->anchor = Vector2(1, -1);
    pullBtn->onTouchDown = []() { Input::trigger("closer",1); };
    pullBtn->onTouchUp = []() { Input::trigger("closer",0); };

    handle = new UIImage(handleTex.get());
    handle->group = UIGroup::safeArea;
    handle->scale = Vector2(0.2, 0.2);
    handle->pos = joystick->pos;

    Vector2 btnScale = Vector2(0.4,0.15);
    UIImage *startServerBtn = new UIImage();
    startServerBtn->anchor = Vector2(-1, 1);
    startServerBtn->scale = btnScale;
    startServerBtn->pos = Vector2(btnScale.x, -btnScale.y)/2;
    startServerBtn->onClick = []()
    {
        Server::start(4096);
        Server::register_rpc("moveplr", &cmd_move);

        Server::set_join_callback([](const Connection &conn)
        {
            NetworkWriter newConnWriter;
            newConnWriter.write<int>(conn.getID());
            for (auto &client : Server::get_clients())
            {
                if (client.getID() != conn.getID())
                {
                    Server::send(client, "spawnplr", newConnWriter);
                    NetworkWriter writer;
                    writer.write<int>(client.getID());
                    Server::send(conn, "spawnplr", writer);
                }
            }
        });
        Server::set_leave_callback([](const Connection &conn)
        {
            NetworkWriter writer;
            writer.write<int>(conn.getID());
            Server::send_all("despawnplr", writer);
        });
    };
    text_for_img("Start Server", startServerBtn)->scale *= 0.9f;
    UIImage *startClientBtn = new UIImage();
    startClientBtn->anchor = Vector2(-1, 1);
    startClientBtn->scale = btnScale;
    startClientBtn->pos = Vector2(btnScale.x*3, -btnScale.y)/2;
    startClientBtn->onClick = []()
    {
        if (Server::is_active())
            Client::start_as_host();
        else
        {
            //const char *ip = "68.199.1.204";
            //const char *ip = "47.16.217.8";
            const char *ip = "localhost";
            if (!Client::is_active() && !Client::start(ip, 4096))
            {
                logerr("error %o: %o\n", errno, strerror(errno));
                return; // todo: move this into a seperate function for errors
            }
        }
        Client::set_shutdown_callback([]()
        {
            for (auto &player : comp()->players)
                delete player.second;
            comp()->players.clear();
        });
        Client::register_rpc("moveplr", &rpc_move);
        Client::register_rpc("spawnplr", &spawn_player);
        Client::register_rpc("despawnplr", &despawn_player);
    };
    text_for_img("Start Client", startClientBtn)->scale *= 0.9f;

    UIImage *shutdownBtn = new UIImage();
    shutdownBtn->anchor = Vector2(-1, 1);
    shutdownBtn->scale = btnScale;
    shutdownBtn->pos = Vector2(btnScale.x*5, -btnScale.y)/2;
    shutdownBtn->onClick = []()
    {
        Client::shutdown();
        Server::shutdown();
    };
    text_for_img("Disconnect", shutdownBtn)->scale *= 0.9f;

    UIImage *soundBtn = new UIImage();
    soundBtn->anchor = Vector2(-1, 1);
    soundBtn->scale = btnScale;
    soundBtn->pos = Vector2(btnScale.x*7, -btnScale.y)/2;
    soundBtn->onClick = MK_LAMBDA(play_audio);

    text_for_img("Play Sound", soundBtn)->scale *= 0.9f;
    UIImage *crossheir = new UIImage();
    crossheir->group = UIGroup::aspectRatio;
    crossheir->anchor = Vector2(0,0);
    crossheir->scale = Vector2(0.01,0.01);
    crossheir->pos = Vector2(0,0);

    fpsText = new UIText("FPS: 00, 00");
    fpsText->group = UIGroup::aspectRatio;
    fpsText->anchor = Vector2(1, 1);
    fpsText->pos = Vector2(-0.25, -0.25);
    fpsText->scale = Vector2(0.5, 0.25);

    const int cubes = 5000;
    const int spread = 40;
    for (int i = 0; i < cubes; ++i)
    {
        //   10 + 12-3 - 1 + 1 / 2+1-3 _ 12 a -1
        // MAKE SURE TO REPLACE OBJ WITH ENTITIES FOR CULLING

        Entity testEntity = Entity::create(*ECSManager::main);
        ObjRendererECS render;
        render.mesh = cube.get();
        render.mat = collisionMat.get();
        testEntity.add_comp(render, *ECSManager::main);
        TransformECS transform;
        for (int j = 0; j < 3; ++j)
            transform.i_pos[j] = ((float)rand() / (float)RAND_MAX * spread - spread/2);
        for (int j = 0; j < 3; ++j)
            transform.i_rotation[j] = ((float)rand() / (float)RAND_MAX)*Math::PI*2;
        if (rand()%100 != 0)
            transform.i_pos.z += spread/2;
        testEntity.add_comp(transform, *ECSManager::main);


        //Object *object = new Object(cube.get(), collisionMat.get());
        //for (int j = 0; j < 3; ++j)
        //    object->position[j] = (float)rand() / (float)RAND_MAX * spread - spread/2;
        //for (int j = 0; j < 3; ++j)
        //    object->rotation[j] = ((float)rand() / (float)RAND_MAX)*Math::PI*2;
        //if (rand()%100 != 0)
        //    object->position.z += spread/2;
        //if (i < 5)
        //    object->add_component<Rigidbody>()->drag = 1.f;
        //object->add_component<CubeCollider>();
        //spawnedObjects.push_back(object);
    }
}
NetTest::~NetTest()
{
    Client::shutdown();
    Server::shutdown();
    Camera::main->isPerspective = 0;
    Camera::main->projection = Matrix4x4::ortho(-1, 1, -1, 1, -10000, 10000, 0);
    Camera::main->rotation = Vector3();
    Camera::main->position = Vector3();

    remove_binds("forward", "back",
        "left", "right",
        "up", "down",
        "jump", "grab",
        "closer", "further", 
        nullptr);
}
ADD_SCENE_COMPONENT("NetTest", NetTest);