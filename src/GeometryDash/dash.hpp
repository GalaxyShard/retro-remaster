#pragma once
#include <Galaxy/engine.hpp>

struct DashSceneData
{
    uintg level = 0;
};
struct MeshType { enum : ucharG { SQUARE,TRIANGLE }; };
struct ColliderType { enum : ucharG { NONE,AABB,TRIANGLE }; };
