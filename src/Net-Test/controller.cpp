#include "game.hpp"
#include <sstream>
#include <deque>

/*
    Mass, drag, angular drag, constraints (freeze rotation)
*/
float parse_bools(bool positive, bool negative)
{
    float val = 0;
    if (positive)
        val += 1;
    if (negative)
        val -= 1;
    return val;

}
float parse_input(const char *positive, const char *negative)
{
    return parse_bools(Input::is_held(positive), Input::is_held(negative));
}
void NetTest::set_move_up(bool v)
{
    // move 2 meters high
    if (v)
    {
        if (rigidbody->velocity.y < 0)
            rigidbody->velocity.y = 0;

        rigidbody->add_acceleration(Vector3(0, sqrtf(1 * (2 * 9.81f)), 0));
    }
    upBtn->tint = v ? Vector4(1,0.5,0.5,1) : Vector4(0.5,0.5,1,1);
}
void NetTest::grab_item(bool p)
{
    if (p) grab_item();
    grabBtn->tint = heldObject ? Vector4(1,0.5,0.5,1) : Vector4(0.5,0.5,1,1);
}
void NetTest::push_item(bool p)
{
    pushDirection += p ? 1 : -1;
    pushBtn->tint = p ? Vector4(1,0.5,0.5,1) : Vector4(0.5,0.5,1,1);
}
void NetTest::pull_item(bool p)
{
    pushDirection += p ? -1 : 1;
    pullBtn->tint = p ? Vector4(1,0.5,0.5,1) : Vector4(0.5,0.5,1,1);
}
std::deque<float> lastMS;
void NetTest::controller_pre_render()
{
    lastMS.push_back(Time::frame_time());
    float ms = 0;
    for (float last : lastMS)
        ms += last;
    ms /= lastMS.size();
    unsigned int fps = (unsigned int)(1.f/ms);

    while (lastMS.size() >= std::min(fps, 60u)/2u)
        lastMS.pop_front();
    fpsText->text = "MS: " + std::to_string((int)(ms*1000));
    fpsText->refresh();

    const float speed = 8;
    distanceToHeld += parse_input("further", "closer") * speed * Time::delta();
    distanceToHeld = Math::clamp(distanceToHeld, 0.2, 7.5);
    

    Vector3 dir = Vector3(parse_input("right", "left"), 0, -parse_input("forward", "back"));
    float sqrMag = dir.sqr_magnitude();
    if (sqrMag > 1)
        dir /= sqrtf(sqrMag);
    dir += Vector3(joystickDir.x, 0, -joystickDir.y);
    sqrMag = dir.sqr_magnitude();
    if (sqrMag > 1)
        dir /= sqrtf(sqrMag);

    /*
    top = tanf(fovy/2)*near
    m11 = near/top
    m11 = tanf(fovy/2)
    fovy = atan(m11)*2
    */
    Vector3 rot = Camera::main->rotation;
    player->position += Matrix3x3::rotate(0, rot.y, 0) * dir * (Time::delta() * 2.5f);

    player->rotation = Vector3(0, rot.y, 0);
/*
    https://gamedev.stackexchange.com/questions/194575/what-is-the-logic-behind-of-screenpointtoray
*/  
    Camera::main->position = player->position+Vector3(0,0.4,0);

    if (heldObject)
    {
        if (Rigidbody *body = heldObject->get_component<Rigidbody>())
        {
            body->add_acceleration(Vector3(0,9.81f,0)*Time::delta());
        }
    }

    if (heldObject)
    {
        Matrix3x3 rot = Matrix3x3::rotate(Camera::main->rotation);
        Matrix3x3 delta = Matrix3x3::rotate(0,initialCamRotation.y,0).transpose()
            * Matrix3x3::rotate(0,Camera::main->rotation.y,0);
        
        Vector3 targetPos = rot*Vector3(0,0, -distanceToHeld) + Camera::main->position + delta*objToContact;
        if (Rigidbody *body = heldObject->get_component<Rigidbody>())
        {
            body->velocity = Vector3();
            body->add_acceleration((targetPos - heldObject->position)*10);
        }
        else
        {
            heldObject->position = heldObject->position + (targetPos - heldObject->position) * 10 * Time::delta();
            //if ((targetPos-heldObject->position).sqr_magnitude() > 0.001f)
            //    heldObject->position += (targetPos-heldObject->position).unit()*10*Time::delta();
            //heldObject->position += (targetPos-heldObject->position)*0.2;
            //heldObject->position = rot*Vector3(0,0, -distanceToHeld) + Camera::main->position + delta*objToContact;
        }

        
        Matrix3x3 r = Matrix3x3::rotate(0,initialCamRotation.y,0).transpose()
            * Matrix3x3::rotate(0,Camera::main->rotation.y,0)
            * Matrix3x3::rotate(initialObjRotation);
        
        heldObject->rotation = Vector3(
            atan2f(r[2][1], r[2][2]),
            atan2f(-r[2][0], sqrtf(Math::sqr(r[2][1])+Math::sqr(r[2][2]))),
            atan2f(r[1][0], r[0][0]));
        
        heldObject->get_component<Collider>()->refresh();
        if (heldObject == lightObj)
        {
            cubeMat->set_uniform("u_lightPos", lightObj->position);
            collisionMat->set_uniform("u_lightPos", lightObj->position);
        }
    }

    if (Client::is_active())
    {
        NetworkWriter writer;
        writer.write<Vector3>(player->position);
        writer.write<float>(rot.y);

        Client::send("moveplr", writer);
    }
}
void NetTest::post_physics()
{
    Camera::main->position = player->position+Vector3(0,0.4,0);
    const Vector3 threshold = Vector3()+100;
    Object *objects[12] = {player,collisionTest};
    for (unsigned int i = 0; i < 10; ++i)
    {
        if (spawnedObjects.size() > i)
            objects[i+2] = spawnedObjects[i];
        else
            objects[i+2] = 0;
    }
    for (Object *obj : objects)
    {
        if (!obj)
            continue;
        for (int i = 0; i < 3; ++i)
        {
            if (obj->position[i] > threshold[i])
            {
                obj->position[i] -= threshold[i]*2;
            }
            else if (obj->position[i] < -threshold[i])
            {
                obj->position[i] += threshold[i]*2;
            }
        }
    }

}

void NetTest::move_joystick(Vector2 screenPos)
{
    joystickDir = (screenPos - joystick->calc_world_pos()) / (joystick->scale.x / 2) * Renderer::aspectRatio;
    if (joystickDir.sqr_magnitude() > 1)
        joystickDir = joystickDir / joystickDir.magnitude();
    handle->pos = joystickDir * joystick->scale / 2 * 0.8f + joystick->pos;
}
void NetTest::touch_changed(TouchData data)
{
    if (data.id == joystickID)
    {
        move_joystick(data.pos);
    }
    if (data.state == TouchState::PRESSED)
    {
        if (joystick->is_within(data.pos))
        {
            joystickID = data.id;
            move_joystick(data.pos);
        }
    }
    else if (data.state == TouchState::RELEASED)
    {
        if (data.id == joystickID)
        {
            joystickID = -1;
            joystickDir = Vector2();
            handle->pos = joystick->pos;
        }
    }
    else
    {
        if (!UIImage::get_held(data.id))
        {
#if OS_MOBILE
            const float speed = 3.75f;
#else
            const float speed = 7.5f;
#endif
            Vector2 delta = data.delta * Renderer::aspectRatio * speed;
            Vector3 rot = Camera::main->rotation;
            rot.y -= delta.x;
            rot.x += delta.y;

            rot.x = Math::clamp(rot.x, -80 * Math::to_rad, 80 * Math::to_rad);
            Camera::main->rotation = rot;
        }
    }
}