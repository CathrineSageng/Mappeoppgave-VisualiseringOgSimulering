#ifndef PHYSICSCALCULATIONS_H
#define PHYSICSCALCULATIONS_H

#include <glm/glm.hpp>
#include <vector>
#include "Octree.h"
#include "Surface.h"

class PhysicsCalculations {
public:
    PhysicsCalculations(float xMin, float xMax, float yMin, float yMax, float ballRadius);

    void applyFriction(glm::vec3& velocity, const glm::vec3& position, float deltaTime, float normalFriction, float highFriction,
        float frictionAreaXMin, float frictionAreaXMax, float frictionAreaYMin, float frictionAreaYMax);

    void updatePhysics(std::vector<glm::vec3>& ballPositions, std::vector<glm::vec3>& ballVelocities,
        std::vector<std::vector<glm::vec3>>& ballTrajectories, Octree& octree, bool ballsMoving,
        float timeStep, float ballRadius, float xMin, float xMax, float yMin, float yMax,
        Surface& surface, float normalFriction, float highFriction,
        float frictionAreaXMin, float frictionAreaXMax, float frictionAreaYMin, float frictionAreaYMax);

    bool checkCollision(glm::vec3 posA, glm::vec3 posB, float radiusA, float radiusB);

    void whenCollisionHappens(glm::vec3& p1, glm::vec3& v1, glm::vec3& p2, glm::vec3& v2, float radius, float ballSpeed);

private:
    float xMin, xMax, yMin, yMax, ballRadius;
};

#endif

