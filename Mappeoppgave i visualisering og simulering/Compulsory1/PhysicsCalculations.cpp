#include "PhysicsCalculations.h"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>

PhysicsCalculations::PhysicsCalculations(float xMin, float xMax, float yMin, float yMax, float ballRadius)
    : xMin(xMin), xMax(xMax), yMin(yMin), yMax(yMax), ballRadius(ballRadius) {}


void PhysicsCalculations::applyFriction(glm::vec3& velocity, const glm::vec3& position, float deltaTime,
    float normalFriction, float highFriction, float frictionAreaXMin, float frictionAreaXMax,
    float frictionAreaYMin, float frictionAreaYMax)
{
    // Regner ut hastigheten på vektoren og normaliserer vektoren 
    float speed = glm::length(velocity);
    glm::vec3 direction = glm::normalize(velocity);

    //Normalkraften = gravitasjonen. Her er det antatt at bakken er flat 
    const float gravity = 9.81f;
    float normalForce = gravity;
    float frictionForce = 0.0f;

    // Ser etter om ballen er innfor området med høyere friksjon
    if (position.x >= frictionAreaXMin && position.x <= frictionAreaXMax &&
        position.y >= frictionAreaYMin && position.y <= frictionAreaYMax)
    {
        // Regner ut friksjonskrafraften i området med høyere friksjon
        frictionForce = highFriction * normalForce;
    }
    else
    {
        // Regner ut friksjonskraften i området uten høyere friksjon 
        frictionForce = normalFriction * normalForce;
    }

    // Regner ut akselerasjonen til ballene, denne er lik friksjonskraften siden massen på ballene er 1
    // F/m=a. Regner deretter ut den nye farten til ballene 
    float frictionAcceleration = frictionForce;
    float newSpeed = speed - frictionAcceleration * deltaTime;

    // Oppdaterer både retningen og hastigheten på ballene
    velocity = direction * newSpeed;
}

bool PhysicsCalculations::checkCollision(glm::vec3 posA, glm::vec3 posB, float radiusA, float radiusB)
{
    double distance = glm::distance(posA, posB);
    return distance < (radiusA + radiusB);
}

void PhysicsCalculations::whenCollisionHappens(glm::vec3& p1, glm::vec3& v1, glm::vec3& p2, glm::vec3& v2,
    float radius, float ballSpeed)
{
    //Masse for ballene er 1 
    float m1 = 1.0f;
    float m2 = 1.0f;

    glm::vec3 normal = glm::normalize(p1 - p2);

    // Separasjon for å unngå overlapping
    float overlap = radius * 2.0f - glm::length(p1 - p2);
    if (overlap > 0.0f)
    {
        glm::vec3 separation = normal * (overlap / 2.0f);
        p1 += separation;
        p2 -= separation;
    }

    // Relativ hastighet
    glm::vec3 relativeVelocity = v1 - v2;

    // Hastighetskomponent langs normalen
    float velocityAlongNormal = glm::dot(relativeVelocity, normal);

    // Ballene beveger seg bort fra hverandre
    if (velocityAlongNormal > 0)
        return;

    // Impulsberegning
    float e = 1.0f;
    float j = -(1 + e) * velocityAlongNormal / (1.0f / m1 + 1.0f / m2);

    glm::vec3 impulse = j * normal;

    v1 += impulse / m1;
    v2 -= impulse / m2;

    // Begrens hastigheten for å unngå unødvendig akselerasjon
    float maxSpeed = 2.0f;
    v1 = glm::normalize(v1) * glm::min(glm::length(v1), maxSpeed);
    v2 = glm::normalize(v2) * glm::min(glm::length(v2), maxSpeed);
}

void PhysicsCalculations::updatePhysics(vector<glm::vec3>& ballPositions, vector<glm::vec3>& ballVelocities,
    vector<vector<glm::vec3>>& ballTrack, Octree& octree,
    bool ballsMoving, float timeStep, float ballRadius, float xMin, float xMax,
    float yMin, float yMax, Surface& surface, float normalFriction,
    float highFriction, float frictionAreaXMin, float frictionAreaXMax,
    float frictionAreaYMin, float frictionAreaYMax)
{
    if (!ballsMoving)
        return;

    octree = Octree(glm::vec3(xMin, yMin, xMin), glm::vec3(xMax, yMax, xMax), 0, 4, 4);

    for (int i = 0; i < ballPositions.size(); ++i) 
    {
        ballPositions[i] += ballVelocities[i] * timeStep;

        if (ballPositions[i].x - ballRadius <= xMin || ballPositions[i].x + ballRadius >= xMax) 
        {
            ballVelocities[i].x = -ballVelocities[i].x;
            ballPositions[i].x = glm::clamp(ballPositions[i].x, xMin + ballRadius, xMax - ballRadius);
        }
        if (ballPositions[i].y - ballRadius <= yMin || ballPositions[i].y + ballRadius >= yMax) 
        {
            ballVelocities[i].y = -ballVelocities[i].y;
            ballPositions[i].y = glm::clamp(ballPositions[i].y, yMin + ballRadius, yMax - ballRadius);
        }

        float u = (ballPositions[i].x - xMin) / (xMax - xMin);
        float v = (ballPositions[i].y - yMin) / (yMax - yMin);
        glm::vec3 surfacePoint = surface.calculateSurfacePoint(u, v);
        ballPositions[i].z = surfacePoint.z + ballRadius;

        if (ballTrack[i].empty() || glm::distance(ballPositions[i], ballTrack[i].back()) > 0.01f) 
        {
            ballTrack[i].push_back(ballPositions[i]);
        }

        applyFriction(ballVelocities[i], ballPositions[i], timeStep, normalFriction, highFriction,
            frictionAreaXMin, frictionAreaXMax, frictionAreaYMin, frictionAreaYMax);

        octree.insert(i, ballPositions, ballRadius);
    }

    vector<pair<int, int>> potentialCollisions;
    octree.getPotentialCollisions(potentialCollisions, ballPositions, ballRadius);

    for (const auto& pair : potentialCollisions)
    {
        if (checkCollision(ballPositions[pair.first], ballPositions[pair.second], ballRadius, ballRadius))
        {
            whenCollisionHappens(ballPositions[pair.first], ballVelocities[pair.first],
                ballPositions[pair.second], ballVelocities[pair.second], ballRadius, 2.5f);
        }
    }
}