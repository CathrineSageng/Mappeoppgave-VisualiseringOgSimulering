#ifndef PHYSICSCALCULATIONS_H
#define PHYSICSCALCULATIONS_H

#include <glm/glm.hpp>
#include <vector>
#include "Octree.h"
#include "Surface.h"

class PhysicsCalculations
{
public:
    PhysicsCalculations(float xMin, float xMax, float yMin, float yMax, float ballRadius);

    //Regner ut friksjonen i et område på B-spline flaten 
    void applyFriction(glm::vec3& velocity, const glm::vec3& position, float deltaTime, float normalFriction, float highFriction,
        float frictionAreaXMin, float frictionAreaXMax, float frictionAreaYMin, float frictionAreaYMax);

    //Reger ut om 2 baller kolliderer. 
    bool checkCollision(glm::vec3 posA, glm::vec3 posB, float radiusA, float radiusB);

    //Renger ut posiajoner og hastigheter når en elastisk kollisjon skjer mellom 2 baller 
    void whenCollisionHappens(glm::vec3& p1, glm::vec3& v1, glm::vec3& p2, glm::vec3& v2, float radius, float ballSpeed);

    //Regner ut og oppdaterer hastigheten og posisjonen til ballene, Holder ballene innenfor B-spline flaten, tar inn friksjonen, undersøker 
    //kollisjon mellom ballene og oppadaterer ballens spor. 
    void updatePhysics(vector<glm::vec3>& ballPositions, vector<glm::vec3>& ballVelocities,
        vector<vector<glm::vec3>>& ballTrack, Octree& octree, bool ballsMoving,
        float timeStep, float ballRadius, float xMin, float xMax, float yMin, float yMax,
        Surface& surface, float normalFriction, float highFriction,
        float frictionAreaXMin, float frictionAreaXMax, float frictionAreaYMin, float frictionAreaYMax);

private:
    //B-spline flaten sine grenser og ballens størrelse 
    float xMin;
    float xMax;
    float yMin;
    float yMax;
    float ballRadius;
};

#endif


