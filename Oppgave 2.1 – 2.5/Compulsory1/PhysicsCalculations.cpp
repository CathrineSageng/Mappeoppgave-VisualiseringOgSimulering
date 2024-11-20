#include "PhysicsCalculations.h"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>

PhysicsCalculations::PhysicsCalculations(float xMin, float xMax, float yMin, float yMax, float ballRadius)
    : xMin(xMin), xMax(xMax), yMin(yMin), yMax(yMax), ballRadius(ballRadius) {}

void PhysicsCalculations::applyFriction(glm::vec3& velocity, const glm::vec3& position, float deltaTime,
    float normalFriction, float highFriction,
    float frictionAreaXMin, float frictionAreaXMax,
    float frictionAreaYMin, float frictionAreaYMax)
{
    // Beregn hastigheten
    float speed = glm::length(velocity);
    glm::vec3 direction = glm::normalize(velocity);

    // Definer krefter
    const float gravity = 9.81f;  // Gravitasjon i m/s^2
    float normalForce = gravity;  // Anta konstant normalkraft (flate uten helling)
    float frictionForce = 0.0f;

    // Sjekk om ballen er i friksjonsområdet
    if (position.x >= frictionAreaXMin && position.x <= frictionAreaXMax &&
        position.y >= frictionAreaYMin && position.y <= frictionAreaYMax)
    {
        // Friksjonskraft i området
        frictionForce = highFriction * normalForce;
    }
    else
    {
        // Friksjonskraft utenfor området
        frictionForce = normalFriction * normalForce;
    }

    // Beregn akselerasjon fra friksjonskraft
    float frictionAcceleration = frictionForce;  // F = ma, m = 1.0f
    float newSpeed = speed - frictionAcceleration * deltaTime;

    // Sett minimumsfart for å sikre at ballene ikke stopper
    float minSpeed = 0.5f;
    if (newSpeed < minSpeed)
        newSpeed = minSpeed;

    // Oppdater hastighet basert på akselerasjon
    velocity = direction * newSpeed;

}

void PhysicsCalculations::updatePhysics(std::vector<glm::vec3>& ballPositions, std::vector<glm::vec3>& ballVelocities,
    std::vector<std::vector<glm::vec3>>& ballTrajectories, Octree& octree,
    bool ballsMoving, float timeStep, float ballRadius, float xMin, float xMax,
    float yMin, float yMax, Surface& surface, float normalFriction,
    float highFriction, float frictionAreaXMin, float frictionAreaXMax,
    float frictionAreaYMin, float frictionAreaYMax) {
    if (!ballsMoving)
        return;

    octree = Octree(glm::vec3(xMin, yMin, xMin), glm::vec3(xMax, yMax, xMax), 0, 4, 4);

    for (int i = 0; i < ballPositions.size(); ++i) {
        ballPositions[i] += ballVelocities[i] * timeStep;

        if (ballPositions[i].x - ballRadius <= xMin || ballPositions[i].x + ballRadius >= xMax) {
            ballVelocities[i].x = -ballVelocities[i].x;
            ballPositions[i].x = glm::clamp(ballPositions[i].x, xMin + ballRadius, xMax - ballRadius);
        }
        if (ballPositions[i].y - ballRadius <= yMin || ballPositions[i].y + ballRadius >= yMax) {
            ballVelocities[i].y = -ballVelocities[i].y;
            ballPositions[i].y = glm::clamp(ballPositions[i].y, yMin + ballRadius, yMax - ballRadius);
        }

        float u = (ballPositions[i].x - xMin) / (xMax - xMin);
        float v = (ballPositions[i].y - yMin) / (yMax - yMin);
        glm::vec3 surfacePoint = surface.calculateSurfacePoint(u, v);
        ballPositions[i].z = surfacePoint.z + ballRadius;

        if (ballTrajectories[i].empty() || glm::distance(ballPositions[i], ballTrajectories[i].back()) > 0.01f) {
            ballTrajectories[i].push_back(ballPositions[i]);
        }

        applyFriction(ballVelocities[i], ballPositions[i], timeStep, normalFriction, highFriction,
            frictionAreaXMin, frictionAreaXMax, frictionAreaYMin, frictionAreaYMax);

        octree.insert(i, ballPositions, ballRadius);
    }

    std::vector<std::pair<int, int>> potentialCollisions;
    octree.getPotentialCollisions(potentialCollisions, ballPositions, ballRadius);

    for (const auto& pair : potentialCollisions) {
        if (checkCollision(ballPositions[pair.first], ballPositions[pair.second], ballRadius, ballRadius)) {
            whenCollisionHappens(ballPositions[pair.first], ballVelocities[pair.first],
                ballPositions[pair.second], ballVelocities[pair.second], ballRadius, 2.5f);
        }
    }
}

bool PhysicsCalculations::checkCollision(glm::vec3 posA, glm::vec3 posB, float radiusA, float radiusB) {
    double distance = glm::distance(posA, posB);
    return distance < (radiusA + radiusB);
}

void PhysicsCalculations::whenCollisionHappens(glm::vec3& p1, glm::vec3& v1, glm::vec3& p2, glm::vec3& v2,
    float radius, float ballSpeed) {
    float m1 = 1.0f; // Masse for ball 1
    float m2 = 1.0f; // Masse for ball 2

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
    float e = 1.0f; // Perfekt elastisk kollisjon
    float j = -(1 + e) * velocityAlongNormal / (1.0f / m1 + 1.0f / m2);

    glm::vec3 impulse = j * normal;

    v1 += impulse / m1;
    v2 -= impulse / m2;

    // Normaliser hastighetene til konstant verdi
    v1 = glm::normalize(v1) * ballSpeed;
    v2 = glm::normalize(v2) * ballSpeed;
}
