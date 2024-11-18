#include <iostream>
#include "glm/mat4x3.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Shader.h"
#include "ShaderFileLoader.h"
#include "Camera.h"
#include "Surface.h"

using namespace std;

// 
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;


glm::vec3 sunPos(2.0f, 2.0f, 2.0f);

//Startposisjonen for kameraet
Camera camera(glm::vec3(1.5f, 1.0f, 3.0f));

GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;

float lastFrame = 0.0f;

// Kontrollpunkter fra Kapittel 12.1- B-spline flater. 
vector<glm::vec3> controlPoints =
{
    glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(3.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 2.0f), glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(3.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f, 2.0f, 0.0f), glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(3.0f, 2.0f, 0.0f),
};

// Uniform skjøtvektor horisontal retning
vector<float> knotVectorU = { 0, 0, 0, 1, 2, 2, 2 };
// Uniform skjøtvektor vertikal retning 
vector<float> knotVectorV = { 0, 0, 0, 1, 1, 1 };

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

string vfs = ShaderLoader::LoadShaderFromFile("vs.vs");
string fs = ShaderLoader::LoadShaderFromFile("fs.fs");

//PhongShader
string vfsp = ShaderLoader::LoadShaderFromFile("phong.vert");
string fsp = ShaderLoader::LoadShaderFromFile("phong.frag");


int main()
{
    cout << "vfs " << vfs.c_str() << endl;
    cout << "fs " << fs.c_str() << endl;

    cout << "vfsp " << vfs.c_str() << endl;
    cout << "fsp " << fs.c_str() << endl;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Spline-kurver", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    Shader ourShader("vs.vs", "fs.fs");
    Shader phongShader("phong.vert", "phong.frag");

    Surface surface(controlPoints, 4, 3, knotVectorU, knotVectorV);

    unsigned int surfaceVAO, surfaceVBO, normalVBO, EBO, normalVAO, normalLineVBO;

    //Hvor glatt overflaten skal være 
    int pointsOnTheSurface = 10;

    surface.setupBuffers(surfaceVAO, surfaceVBO, normalVBO, EBO, normalVAO, normalLineVBO, pointsOnTheSurface);


    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        // Time calculation for movement
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Referanse for phong shader: https://learnopengl.com/code_viewer_gh.php?code=src/2.lighting/3.1.materials/materials.cpp
        phongShader.use();

        phongShader.setVec3("light.position", sunPos);
        phongShader.setVec3("viewPos", camera.Position);

        // Egenskaper for lys
        phongShader.setVec3("light.ambient", 0.3f, 0.4f, 0.3f);
        phongShader.setVec3("light.diffuse", 0.3f, 0.7f, 0.3f);
        phongShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // Egenskaper for materialer
        phongShader.setVec3("material.ambient", 0.1f, 0.5f, 0.1f);
        phongShader.setVec3("material.diffuse", 0.2f, 0.8f, 0.8f);
        phongShader.setVec3("material.specular", 0.3f, 0.3f, 0.3f);
        phongShader.setFloat("material.shininess", 32.0f);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        phongShader.setMat4("projection", projection);
        phongShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        phongShader.setMat4("model", model);

        // Rendrer bikvadratisk b- spline tensorprodukt flate 
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(surfaceVAO);
        glDrawElements(GL_TRIANGLES, pointsOnTheSurface * pointsOnTheSurface * 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);

        //Rendrer normaler på overflaten 
        glBindVertexArray(normalVAO);
        glVertexAttrib3f(1, 1.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINES, 0, pointsOnTheSurface * pointsOnTheSurface * 2);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{

    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}











