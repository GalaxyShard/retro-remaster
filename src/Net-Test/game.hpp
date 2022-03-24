#pragma once
#include <Galaxy/engine.hpp>
#include <utils.hpp>
// instead of a universal NetTest struct, have multiple structs for each component
// eg NetTestController, or just Controller
struct NetTest
{
    std::unique_ptr<Listener> projectionConn, inputConn, physicsConn, aspectConn;
    std::unique_ptr<ListenerT<TouchData>> touchConn;

    int joystickID = -1;
    void controller_pre_render();
    void post_physics();
    void touch_changed(TouchData data);
    void move_joystick(Vector2 screenPos);
    void set_move_up(bool);
    void play_audio();
    void grab_item();
    void push_item(bool);
    void pull_item(bool);
    void grab_item(bool);
    NetTest();
    ~NetTest();

    Vector2 joystickDir;
    Vector2 rotateDelta;

    AssetRef<Mesh> cube, sphere;
    AssetRef<Texture> menuTex, objTex, joystickTex, handleTex;
    AssetRef<Shader> colShader, texShader, tintShader, colLightShader, texLightShader;
    std::unique_ptr<Material> cubeMat, collisionMat, floorMat, lightMat;

    Object *player, *collisionTest, *floor, *lightObj;
    Rigidbody* rigidbody;
    UIImage *joystick, *handle, *upBtn, *grabBtn, *pushBtn, *pullBtn;
    UIText *fpsText;
    std::unordered_map<int, Object*> players;

    float pushDirection = 0;
    Object *heldObject = 0;
    float distanceToHeld = 0;
    Vector3 initialCamRotation;
    Vector3 initialObjRotation;
    Vector3 objToContact;

    std::vector<Object*> spawnedObjects;

    std::unique_ptr<AudioPlayer> jumpPlayer;
    std::unique_ptr<AudioPlayer> backgroundPlayer;
};