#pragma once
class Scene;

extern Scene *globalScene;


struct Header { enum { OBJ2D }; };
struct MeshType { enum { SQUARE,TRIANGLE }; };
struct ColliderType { enum { NONE,AABB,TRIANGLE }; };