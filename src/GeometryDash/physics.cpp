#include "dash.hpp"
#include "physics.hpp"


//static const Vector2 triangleNorm0 = (Vector2(0, 0.5) - Vector2(-0.5, -0.5)).unit().perpendicular();
//static const Vector2 triangleNorm1 = (Vector2(0, 0.5) - Vector2( 0.5, -0.5)).unit().perpendicular();

static bool test_aabb_1D(float min0, float max0, float min1, float max1)
{
    return min1<max0 && min0<max1;
}
void DashCollider::generate()
{
    Vector2 halfScale = obj->scale*0.5f;
    if (type == ColliderType::AABB)
    {
        points[0] = Vector2(-1, -1);
        points[1] = Vector2( 1, -1);
        points[2] = Vector2(-1,  1);
        points[3] = Vector2( 1,  1);
        for (uintg i = 0; i < 4; ++i)
            points[i] = obj->position + (halfScale*points[i]);
    }
    else if (type == ColliderType::TRIANGLE)
    {
        points[0] = Vector2(-0.66, -1   );
        points[1] = Vector2( 0.66, -1   );
        points[2] = Vector2(-0.66,  0.7);
        points[3] = Vector2( 0.66,  0.7);
        for (uintg i = 0; i < 4; ++i)
            points[i] = obj->position + (halfScale*points[i]);
        
        //Object2D::create(new Mesh({points[0], points[1], }, {, , , , }), );
    }
    else
    {
        logerr("Collider not recognized: %d\n", type);
    }
}

static void min_max_dot(Vector2 *points, uintg len, Vector2 axis, float &min, float &max)
{
    min = Vector2::dot(points[0], axis);
    max = min;
    for (uintg i = 0; i < len; ++i)
    {
        float product = Vector2::dot(points[i], axis);
        if (min > product)
            min = product;
        
        if (max < product)
            max = product;
    }
}
DashCollisionData test_collision(Vector2 *player, DashCollider *collider)
{
    Vector2 mtv;
    float mtvMag = Math::INF;
    float minDot[2], maxDot[2];
    uintg numPoints = 0;
    
    auto testAxis = [&](Vector2 axis) -> bool
    {
        min_max_dot(player,           4,         axis, minDot[0], maxDot[0]);
        min_max_dot(collider->points, numPoints, axis, minDot[1], maxDot[1]);

            /*
                tests
                <-min0--max0----min1--max1----> 0   correct
                <-min1--max1----min0--max0----> 0   correct

                <-min0--min1----max0--max1----> 1   correct
                <-min1--min0----max1--max0----> 1   correct

                <-min0--min1----max1--max0----> 1   correct
                <-min1--min0----max0--max1----> 1   correct
            */
        if (minDot[1]<maxDot[0]
            && minDot[0]<maxDot[1])
        {
            // max1-min0   when 1 is less
            // min1-max0   when 0 is less
            float avg[2] = {(minDot[0]+maxDot[0])*0.5f, (minDot[1]+maxDot[1])*0.5f};
            float overlap = (avg[0] < avg[1]) ? minDot[1]-maxDot[0] : maxDot[1]-minDot[0];
            if (abs(overlap) < abs(mtvMag))
            {
                mtvMag = overlap;
                mtv = axis*mtvMag;
            }
            return 1;
        }
        return 0;
    };
    if (collider->type == ColliderType::AABB)
    {
        numPoints = 4;
        if (!testAxis(Vector2(1, 0)) || !testAxis(Vector2(0, 1)))
            return DashCollisionData();

        return DashCollisionData(mtv);
    }
    else if (collider->type == ColliderType::TRIANGLE)
    {
        numPoints = 4;
        if (!testAxis(Vector2(1, 0)) || !testAxis(Vector2(0, 1)))
            return DashCollisionData();

        return DashCollisionData(mtv);
    }
    logerr("Collider not recognized: %d\n", collider->type);
    return DashCollisionData();
}
