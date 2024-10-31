#include<iostream>
#include "glm/mat4x3.hpp"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<vector>
#include "Shader.h"
#include "ShaderFileLoader.h"
#include "Camera.h"

using namespace std;

// Global variables
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// Camera settings
//This is the starting position of the of the camera 
Camera camera(glm::vec3(1.0f, 1.0f, 5.0f));
//Keeps track of the last position of the mouse cursor 
GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
//Avoids sudden jumps in the camera orientation when the mouse is first detected. 
bool firstMouse = true;

// Time between current frame and last frame
float deltaTime = 0.0f;
//Stores the timestamp of previous frame. 
float lastFrame = 0.0f;

//The control points from the Matematikk 3 lecture file, chapter 12. 
std::vector<glm::vec3> controlPoints = {
    glm::vec3(0.0f, 0.0f,  0.0f), glm::vec3(1.0f, 0.0f,  0.0f), glm::vec3(2.0f, 0.0f,  0.0f), glm::vec3(3.0f, 0.0f,  0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f,  2.0f), glm::vec3(2.0f, 1.0f,  2.0f), glm::vec3(3.0f, 1.0f,  0.0f),
    glm::vec3(0.0f, 2.0f,  0.0f), glm::vec3(1.0f,  2.0f,  0.0f), glm::vec3(2.0f,  2.0f,  0.0f), glm::vec3(3.0f,  2.0f,  0.0f),
};

// Knutevektorer
std::vector<float> knotVectorU = { 0, 0, 0, 1, 2, 2, 2 };
std::vector<float> knotVectorV = { 0, 0, 0, 1, 1, 1 };

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
float BSplineBasis(int i, int k, float t, const vector<float>& knots);
glm::vec3 BSplineSurface(float u, float v, const vector<glm::vec3>& controlPoints, int widthU, int widthV, const vector<float>& knotU, const vector<float>& knotV);
glm::vec3 BSplinePartialDerivative(float u, float v, const vector<glm::vec3>& controlPoints, int widthU, int widthV, const vector<float>& knotU, const vector<float>& knotV, bool withRespectToU);
vector<glm::vec3> CalculateSurfaceNormals(const vector<glm::vec3>& surfacePoints, int pointsOnTheSurface, const vector<glm::vec3>& controlPoints, int widthU, int widthV, const vector<float>& knotU, const vector<float>& knotV);


std::string vfs = ShaderLoader::LoadShaderFromFile("vs.vs");
std::string fs = ShaderLoader::LoadShaderFromFile("fs.fs");

int main()
{
    std::cout << "vfs " << vfs.c_str() << std::endl;
    std::cout << "fs " << fs.c_str() << std::endl;
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Spline-kurver", NULL, NULL);
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

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    vector<glm::vec3> surfacePoints;
    int pointsOnTheSurface = 10;
    for (int i = 0; i < pointsOnTheSurface; ++i)
    {
        for (int j = 0; j < pointsOnTheSurface; ++j)
        {
            float u = i / static_cast<float>(pointsOnTheSurface - 1);
            float v = j / static_cast<float>(pointsOnTheSurface - 1);
            surfacePoints.push_back(BSplineSurface(u, v, controlPoints, 4, 3, knotVectorU, knotVectorV));
        }
    }

    vector<unsigned int> indices;
    for (int i = 0; i < pointsOnTheSurface - 1; ++i)
    {
        for (int j = 0; j < pointsOnTheSurface - 1; ++j)
        {
            indices.push_back(i * pointsOnTheSurface + j);
            indices.push_back((i + 1) * pointsOnTheSurface + j);
            indices.push_back(i * pointsOnTheSurface + j);
            indices.push_back(i * pointsOnTheSurface + (j + 1));
        }
    }


    //For the wireframe 
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, surfacePoints.size() * sizeof(glm::vec3), &surfacePoints[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    //Points on the surface
    unsigned int surfaceVBO, surfaceVAO;
    glGenVertexArrays(1, &surfaceVAO);
    glGenBuffers(1, &surfaceVBO);

    glBindVertexArray(surfaceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, surfaceVBO);
    glBufferData(GL_ARRAY_BUFFER, surfacePoints.size() * sizeof(glm::vec3), &surfacePoints[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //For the contol points 
    unsigned int controlVBO, controlVAO;
    glGenVertexArrays(1, &controlVAO);
    glGenBuffers(1, &controlVBO);

    glBindVertexArray(controlVAO);
    glBindBuffer(GL_ARRAY_BUFFER, controlVBO);
    glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(glm::vec3), &controlPoints[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Beregn overflatepunkter og normaler
    vector<glm::vec3> normals = CalculateSurfaceNormals(surfacePoints, pointsOnTheSurface, controlPoints, 4, 3, knotVectorU, knotVectorV);

    // Sett opp VBO og VAO for normalene
    vector<glm::vec3> normalLines;
    float normalLength = 0.1f; // Velg en passende lengde for normalene

    for (int i = 0; i < surfacePoints.size(); ++i) {
        glm::vec3 startPoint = surfacePoints[i];
        glm::vec3 endPoint = startPoint + normals[i] * normalLength;
        normalLines.push_back(startPoint); // Legg til startpunkt
        normalLines.push_back(endPoint);   // Legg til sluttpunkt
    }

    unsigned int normalVBO, normalVAO;
    glGenVertexArrays(1, &normalVAO);
    glGenBuffers(1, &normalVBO);

    glBindVertexArray(normalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normalLines.size() * sizeof(glm::vec3), &normalLines[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        // Time calculation for movement
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);

        //Render wireframe
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Render surface points
        glBindVertexArray(surfaceVAO);
        glPointSize(6.0f);
        glDrawArrays(GL_POINTS, 0, surfacePoints.size());
        glBindVertexArray(0);

        // Render control points
        glBindVertexArray(controlVAO);
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, controlPoints.size());
        glBindVertexArray(0);

        // Render normalene
        glBindVertexArray(normalVAO);
        glDrawArrays(GL_LINES, 0, normalLines.size());
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
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

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
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

float BSplineBasis(int i, int k, float t, const vector<float>& knots)
{
    if (i < 0 || i >= knots.size() - 1) {
        return 0.0f;
    }

    if (k == 0)
    {
        return (knots[i] <= t && t < knots[i + 1]) ? 1.0f : 0.0f;
    }
    float denom1 = knots[i + k] - knots[i];
    float term1 = (denom1 != 0.0f) ? (t - knots[i]) / denom1 * BSplineBasis(i, k - 1, t, knots) : 0.0f;

    float denom2 = knots[i + k + 1] - knots[i + 1];
    float term2 = (denom2 != 0.0f) ? (knots[i + k + 1] - t) / denom2 * BSplineBasis(i + 1, k - 1, t, knots) : 0.0f;

    return term1 + term2;
}

glm::vec3 BSplineSurface(float u, float v, const vector<glm::vec3>& controlPoints, int widthU, int widthV, const vector<float>& knotU, const vector<float>& knotV)
{
    glm::vec3 point(0.0f);
    int degreeU = 2, degreeV = 2;

    // Juster skaleringsformel for `u` og `v` slik at de ikke n�r eksakt siste verdien
    float scaledU = std::min(u * (knotU[knotU.size() - degreeU - 1] - knotU.front()) + knotU.front(), knotU[knotU.size() - degreeU - 1] - 0.001f);
    float scaledV = std::min(v * (knotV[knotV.size() - degreeV - 1] - knotV.front()) + knotV.front(), knotV[knotV.size() - degreeV - 1] - 0.001f);

    for (int i = 0; i < widthU; ++i) // Iterer over kolonner i u-retningen
    {
        for (int j = 0; j < widthV; ++j) // Iterer over rader i v-retningen
        {
            int index = j * widthU + i; // Beregn indeksen riktig
            if (index < controlPoints.size()) {
                float basisU = BSplineBasis(i, degreeU, scaledU, knotU);
                float basisV = BSplineBasis(j, degreeV, scaledV, knotV);
                point += basisU * basisV * controlPoints[index];
            }
        }
    }
    return point;
}

// Funksjon for � beregne den partielle deriverte med hensyn til u eller v
glm::vec3 BSplinePartialDerivative(float u, float v, const vector<glm::vec3>& controlPoints, int widthU, int widthV, const vector<float>& knotU, const vector<float>& knotV, bool withRespectToU)
{
    glm::vec3 derivative(0.0f);
    int degreeU = 2, degreeV = 2;

    float scaledU = u * (knotU[knotU.size() - degreeU - 1] - knotU.front()) + knotU.front();
    float scaledV = v * (knotV[knotV.size() - degreeV - 1] - knotV.front()) + knotV.front();

    for (int i = 0; i < widthU; ++i)
    {
        for (int j = 0; j < widthV; ++j)
        {
            int index = j * widthU + i;
            if (index < controlPoints.size()) {
                float basisU = withRespectToU ? BSplineBasis(i, degreeU - 1, scaledU, knotU) : BSplineBasis(i, degreeU, scaledU, knotU);
                float basisV = withRespectToU ? BSplineBasis(j, degreeV, scaledV, knotV) : BSplineBasis(j, degreeV - 1, scaledV, knotV);
                derivative += basisU * basisV * controlPoints[index];
            }
        }
    }
    return derivative;
}

vector<glm::vec3> CalculateSurfaceNormals(const vector<glm::vec3>& surfacePoints, int pointsOnTheSurface, const vector<glm::vec3>& controlPoints, int widthU, int widthV, const vector<float>& knotU, const vector<float>& knotV)
{
    vector<glm::vec3> normals;
    float epsilon = 0.001f; // Liten verdi for � justere u og v n�r yttergrensene

    for (int i = 0; i < pointsOnTheSurface; ++i)
    {
        for (int j = 0; j < pointsOnTheSurface; ++j)
        {
            // Beregn u og v, og unng� eksakt 0 eller 1 p� yttergrensene
            float u = i / static_cast<float>(pointsOnTheSurface - 1);
            float v = j / static_cast<float>(pointsOnTheSurface - 1);

            if (u <= 0.0f) u += epsilon;                  // Juster nedre u-grense
            else if (u >= 1.0f) u -= epsilon;             // Juster �vre u-grense

            if (v <= 0.0f) v += epsilon;                  // Juster nedre v-grense
            else if (v >= 1.0f) v -= epsilon;             // Juster �vre v-grense

            // Beregn partielle deriverte
            glm::vec3 partialU = BSplinePartialDerivative(u, v, controlPoints, widthU, widthV, knotU, knotV, true);
            glm::vec3 partialV = BSplinePartialDerivative(u, v, controlPoints, widthU, widthV, knotU, knotV, false);

            // Beregn normalen som kryssproduktet av de partielle derivatene
            glm::vec3 normal = glm::normalize(glm::cross(partialU, partialV));
            normals.push_back(normal);
        }
    }
    return normals;
}









