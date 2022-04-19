#pragma once

struct DashCollider
{
    Object2D *obj;
    Vector2 points[4];
    ucharG type;
    bool killOnTouch;
    DashCollider(Object2D *obj, ucharG type, bool killOnTouch)
        : obj(obj), type(type), killOnTouch(killOnTouch) { generate(); }

    void generate();
};
struct DashCollisionData
{
    Vector2 mtv;
    bool collided;
    DashCollisionData(Vector2 mtv) : mtv(mtv), collided(1) {}
    DashCollisionData() : mtv(), collided(0) {}
};
DashCollisionData test_collision(Vector2 *player, DashCollider *collider);