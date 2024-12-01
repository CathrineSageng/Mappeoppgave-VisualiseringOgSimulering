#include<iostream>
#include <fstream>
#include <filesystem>
#include<vector>
#include <limits>
#include <algorithm>
#include <unordered_map> 
#include <utility>  

#include "glm/mat4x3.hpp"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "ShaderFileLoader.h"
#include "Camera.h"
#include "BilinearSurface.h"

using namespace std;

//----------------------------------------------------------------------------------------------------------------------------------------------------//

const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;


Camera camera(glm::vec3(2.13f, 11.7f, 0.2f));
GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//Posisjonen på lyset til teksturen og phong-shaderen 
glm::vec3 sunPos(2.0f, 11.0f, 2.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
vector<glm::vec3> calculateControlPoints(const vector<glm::vec3>& points, const vector<glm::ivec3>& triangles);
vector<float> generateKnotVector(int numControlPoints);
glm::vec3 evaluateBSpline(float u, float v, const vector<glm::vec3>& controlPoints, const vector<float>& knotsU, const vector<float>& knotsV);
float basisFunction(int i, int k, float u, const vector<float>& knots);

void setupControlPointBuffers(GLuint& VAO, GLuint& VBO, const std::vector<glm::vec3>& controlPoints);
void cleanupBuffers(const std::vector<GLuint>& VAOs, const std::vector<GLuint>& VBOs);

string vfs = ShaderLoader::LoadShaderFromFile("vs.vs");
string fs = ShaderLoader::LoadShaderFromFile("fs.fs");
string vfsp = ShaderLoader::LoadShaderFromFile("phong.vert");
string fsp = ShaderLoader::LoadShaderFromFile("phong.frag");

int main()
{
    std::cout << "vfs " << vfs.c_str() << std::endl;
    std::cout << "fs " << fs.c_str() << std::endl;

    std::cout << "vfsp " << vfs.c_str() << std::endl;
    std::cout << "fsp " << fs.c_str() << std::endl;


    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Terreng", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("vs.vs", "fs.fs"); // you can name your shader files however you like
    Shader phongShader("phong.vert", "phong.frag");

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

BilinearSurface surface;
surface.loadAndInitialize("32-2-517-155-12.txt", 0.0008f);


  while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Referanse for phong shader og tekstur: https://learnopengl.com/code_viewer_gh.php?code=src/2.lighting/3.1.materials/materials.cpp
        phongShader.use();

        phongShader.setVec3("light.position", sunPos);
        phongShader.setVec3("viewPos", camera.Position);

        // Egenskaper for lys
        phongShader.setVec3("light.ambient", 0.2f, 0.3f, 0.2f);   // Myk grønn tone i bakgrunnsbelysning
        phongShader.setVec3("light.diffuse", 0.3f, 0.5f, 0.3f);   // Mer intens grønnfarge fra lys
        phongShader.setVec3("light.specular", 0.4f, 0.4f, 0.4f);  // Dempet lysrefleksjon

        // Egenskaper for materialer
        phongShader.setVec3("material.ambient", 0.1f, 0.3f, 0.1f); // Mørk grønn i bakgrunnsbelysning
        phongShader.setVec3("material.diffuse", 0.2f, 0.6f, 0.2f); // Frisk grønn for hovedfargen
        phongShader.setVec3("material.specular", 0.1f, 0.2f, 0.1f); // Veldig svak glans
        phongShader.setFloat("material.shininess", 16.0f);          // Moderat glansspredning

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        phongShader.setMat4("projection", projection);
        phongShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        phongShader.setMat4("model", model);

        surface.draw(phongShader, projection, view, model);

        // Bruk ourShader til å rendre normalene
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);

        surface.drawNormals(ourShader, projection, view, model);

        surface.drawPoints(ourShader, projection, view, model);
  
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
    double xpos = static_cast<float>(xposIn);
    double ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

vector<glm::vec3> calculateControlPoints(const vector<glm::vec3>& points, const vector<glm::ivec3>& triangles) {
    vector<glm::vec3> controlPoints;

    // For hver trekant, beregn kontrollpunkter basert på barysentriske koordinater
    for (const auto& tri : triangles) {
        glm::vec3 p0 = points[tri.x];
        glm::vec3 p1 = points[tri.y];
        glm::vec3 p2 = points[tri.z];

        // Barysentriske koordinater for midtpunktene på kantene
        glm::vec3 midpoint01 = (p0 + p1) * 0.5f;
        glm::vec3 midpoint12 = (p1 + p2) * 0.5f;
        glm::vec3 midpoint20 = (p2 + p0) * 0.5f;

        // Senterpunkt av trekanten
        glm::vec3 center = (p0 + p1 + p2) / 3.0f;

        // Legg til punktene som kontrollpunkter
        controlPoints.push_back(p0);
        controlPoints.push_back(p1);
        controlPoints.push_back(p2);
        controlPoints.push_back(midpoint01);
        controlPoints.push_back(midpoint12);
        controlPoints.push_back(midpoint20);
        controlPoints.push_back(center);
    }

    return controlPoints;
}

vector<float> generateKnotVector(int numControlPoints) {
    int m = numControlPoints - 1;
    vector<float> knotVector;

    // Sett første og siste verdi til 0 og m
    for (int i = 0; i < 3; ++i) knotVector.push_back(0.0f);
    for (int i = 1; i < m - 1; ++i) knotVector.push_back(static_cast<float>(i));
    for (int i = 0; i < 3; ++i) knotVector.push_back(static_cast<float>(m));

    return knotVector;
}

float basisFunction(int i, int k, float u, const vector<float>& knots) {
    if (k == 1) {
        return (knots[i] <= u && u < knots[i + 1]) ? 1.0f : 0.0f;
    }
    else {
        float coeff1 = (u - knots[i]) / (knots[i + k - 1] - knots[i]);
        float coeff2 = (knots[i + k] - u) / (knots[i + k] - knots[i + 1]);

        return coeff1 * basisFunction(i, k - 1, u, knots) + coeff2 * basisFunction(i + 1, k - 1, u, knots);
    }
}

glm::vec3 evaluateBSpline(float u, float v, const vector<glm::vec3>& controlPoints, const vector<float>& knotsU, const vector<float>& knotsV) {
    int n = controlPoints.size(); // Antall kontrollpunkter
    glm::vec3 result(0.0f);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float Bu = basisFunction(i, 3, u, knotsU); // Bikvadratisk basis i u
            float Bv = basisFunction(j, 3, v, knotsV); // Bikvadratisk basis i v
            result += Bu * Bv * controlPoints[i * n + j]; // Kombinasjon av basisfunksjoner
        }
    }

    return result;
}

void setupControlPointBuffers(GLuint& VAO, GLuint& VBO, const std::vector<glm::vec3>& controlPoints) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(glm::vec3), controlPoints.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void cleanupBuffers(const std::vector<GLuint>& VAOs, const std::vector<GLuint>& VBOs) {
    for (GLuint VAO : VAOs) {
        glDeleteVertexArrays(1, &VAO);
    }
    for (GLuint VBO : VBOs) {
        glDeleteBuffers(1, &VBO);
    }
}











