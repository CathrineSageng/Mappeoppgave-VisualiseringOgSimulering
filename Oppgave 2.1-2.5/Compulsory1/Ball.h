#ifndef BALL_H
#define BALL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include<iostream>
#include <vector>


class Ball
{
public:
    Ball(float radius, int sectors, int stacks, glm::vec3 color);
    void DrawBall();

    void UpdateRotation(const glm::vec3& velocity, float deltaTime, bool ballsMoving);

    //Stores the rotation of the ball
    //Every ball has its own rotation
    glm::mat4 rotationMatrix = glm::mat4(1.0f);

private:
    GLuint VAO, VBO, EBO;
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    std::vector<GLfloat> colors;
    std::vector<float> texCoords;

    float ballRadius;
};

#endif


