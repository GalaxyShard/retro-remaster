#pragma once
#include <Galaxy/engine.hpp>
#include <utils.hpp>
//struct Controller
//{
//    std::unique_ptr<Listener> inputConn;
//    std::unique_ptr<ListenerT<TouchData>> touchConn;

//    int joystickID = -1;
//    void pre_render();
//    void touch_changed(TouchData data);
//    void move_joystick(Vector2 screenPos);
//    Controller();
//    ~Controller();

//    Vector2 joystickDir;
//    Vector2 rotateDelta;

//    AssetRef<Texture> joystickTex, handleTex;
//    //AssetRef<Shader> colShader, texShader;
    
//    Object *player;
//    UIImage *joystick, *handle;
//    std::unordered_map<int, Object*> players;
//};