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
#include "Ball.h"
#include "Octree.h"
#include "PhysicsCalculations.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace std;
//---------------------GLOBALE VARIABLER----------------------------------------------------------------------------------------------------------------//
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

//Grene på B-Sline flaten. Ballene skal ikke rulle utenfor disse verdiene. 
float xMin = 0.0f; 
float xMax = 3.0f; 
float yMin = 0.0f; 
float yMax = 2.0f; 

//Ballenes fart 
float ballSpeed = 2.5f;

//Ballene beveger seg ikke når programmet starter
bool ballsMoving = false;
//Størrelsen på ballene 
float ballRadius = 0.1f;

bool firstMouse = true;

double deltaTime = 0.0f;

double lastFrame = 0.0f;

GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;

const double fixedTimeStep = 0.01f;
double accumulator = 0.0f;

bool pointSelected = false;
glm::vec3 selectedPoint;

// Friksjonsområde (definerer grenser)
float frictionAreaXMin = 0.0f;
float frictionAreaXMax = 1.0f;
float frictionAreaYMin = 0.0f;
float frictionAreaYMax = 1.0f;

float normalFriction = 0.01f; // Lav friksjon utenfor området
float highFriction = 0.5f;   // Høy friksjon i området (stor reduksjon)

//-----------------------------------------------------------------------------------------------------------------------------------------------------//

//Posisjonen på lyset til teksturen og phong-shaderen 
glm::vec3 sunPos(2.0f, 2.0f, 2.0f);

//Startposisjonen på ballene 
glm::vec3 ball1(0.0f, 0.0f, 0.1f);
glm::vec3 ball2(3.0f, 2.0f, 0.1f);

std::vector<glm::vec3> ballPositions = 
{
    ball1, ball2
};

//Startretningen på ballene 
glm::vec3 ballVelocity01(2.0f, -1.0f, 0.0f);
glm::vec3 ballVelocity02(-2.0f, -1.0f, 0.0f);

vector<glm::vec3> ballVelocities = 
{
    ballVelocity01, ballVelocity02
};

vector<glm::vec3> ballColors =
{
    //Hvit bakgrunnsfare på teksturen
    glm::vec3(1.0f, 1.0f, 1.0f),
};

std::vector<std::vector<glm::vec3>> ballTrajectories(ballPositions.size());

//Startposisjonen for kameraet
Camera camera(glm::vec3(1.5f, 1.0f, 3.0f));

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

//----------------------------------------------------------------------------------------------------------------------------------------------//

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadTexture(const char* path);
void selectStartPointForBall(Surface& surface, glm::vec3& ballPosition, float xMin, float xMax, float yMin, float yMax, float ballRadius);

//----------------Shadere til tekstur, phongShader---------------------------------------------------------------------------------------------//
//PhongShader
string vfsp = ShaderLoader::LoadShaderFromFile("phong.vert");
string fsp = ShaderLoader::LoadShaderFromFile("phong.frag");

//Tekstur shader til tekstur på ballene 
string vfsT = ShaderLoader::LoadShaderFromFile("Texture.vs");
string fsT = ShaderLoader::LoadShaderFromFile("Texture.fs");

//---------------------------------------------------------------------------------------------------------------------------------------------//

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Spline-kurver", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" <<endl;
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
    Shader phongShader("phong.vert", "phong.frag");
    Shader textureShader("Texture.vs", "Texture.fs");

    glEnable(GL_DEPTH_TEST);

    Surface surface(controlPoints, 4, 3, knotVectorU, knotVectorV);
    // The Octree is created over the box with the mininum and maximum boundaries. 
    Octree octree(glm::vec3(xMin, yMin, xMin), glm::vec3(xMax, yMax, xMax), 0, 4, 4);

    vector<Ball> balls;

    PhysicsCalculations physics(xMin, xMax, yMin, yMax, ballRadius);

    unsigned int surfaceVAO, surfaceVBO, colorVBO, normalVBO, EBO, normalVAO, normalLineVBO;

    //Hvor glatt overflaten skal være 
    int pointsOnTheSurface = 20;

    // Oppdater setupBuffers-kallet med friksjonsgrenser
    surface.setupBuffers(surfaceVAO, surfaceVBO, colorVBO, normalVBO, EBO, normalVAO, normalLineVBO,
        pointsOnTheSurface, frictionAreaXMin, frictionAreaXMax,
        frictionAreaYMin, frictionAreaYMax);

    physics.updatePhysics(ballPositions, ballVelocities, ballTrajectories, octree, ballsMoving,
        fixedTimeStep, ballRadius, xMin, xMax, yMin, yMax, surface,
        normalFriction, highFriction, frictionAreaXMin, frictionAreaXMax,
        frictionAreaYMin, frictionAreaYMax);

    for (int i = 0; i < ballPositions.size(); ++i)
    {
        glm::vec3 ballColor = ballColors[i % ballColors.size()]; // Assign a color from the color list
        balls.push_back(Ball(ballRadius, 30, 30, ballColor));    // Create the ball with the color
    }

    //Teksturen på ballene
    unsigned int diffuseMap1 = loadTexture("Textures/ball.jpg");
    unsigned int specularMap = loadTexture("Textures/ball2.jpg");

    textureShader.use();
    textureShader.setInt("material.diffuse", 0);
    textureShader.setInt("material.specular", 1);

    

    // Vis koordinatgrenser
    std::cout << "Koordinatgrenser:\n";
    std::cout << "x: [" << xMin << ", " << xMax << "]\n";
    std::cout << "y: [" << yMin << ", " << yMax << "]\n";

    // Obligatorisk interaktiv plassering av ballene
    for (int i = 0; i < ballPositions.size(); ++i) {
        std::cout << "Velg posisjon for ball " << i + 1 << ":\n";
        selectStartPointForBall(surface, ballPositions[i], xMin, xMax, yMin, yMax, ballRadius);

    }

    while (!glfwWindowShouldClose(window))
    {
        double currentFrame = static_cast<float>(glfwGetTime());
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

        //Rendrer b-spline kurven som er sporing av banen til ballene. 
        for (int i = 0; i < ballTrajectories.size(); ++i) 
        {
            if (ballTrajectories[i].size() > 1) 
            {
                auto curvePoints = surface.calculateBSplineCurve(ballTrajectories[i], 3, 50);
                surface.renderBSplineCurve(curvePoints, phongShader, projection, view);
            }
        }

        textureShader.use();
        textureShader.setVec3("light.position", sunPos);
        textureShader.setVec3("viewPos", camera.Position);

        // Light properties
        textureShader.setVec3("light.ambient", 0.5f, 0.6f, 0.6f);
        textureShader.setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
        textureShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // material properties
        textureShader.setFloat("material.shininess", 64.0f);

        textureShader.setMat4("projection", projection);
        textureShader.setMat4("view", view);

        accumulator += deltaTime;

        while (accumulator >= fixedTimeStep)
        {
            physics.updatePhysics(
                ballPositions,         // Ballenes posisjoner
                ballVelocities,        // Ballenes hastigheter
                ballTrajectories,      // Ballenes spor (trajectories)
                octree,                // Octree for kollisjonsberegning
                ballsMoving,           // Om ballene beveger seg
                fixedTimeStep,         // Fast tidssteg
                ballRadius,            // Radiusen til ballene
                xMin, xMax, yMin, yMax,// Grenser for flaten
                surface,               // Overflate (B-spline flate)
                normalFriction,        // Lav friksjon
                highFriction,          // Høy friksjon
                frictionAreaXMin, frictionAreaXMax, // Friksjonsområde i x-retning
                frictionAreaYMin, frictionAreaYMax  // Friksjonsområde i y-retning
            );
            accumulator -= fixedTimeStep;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap1);

        // Rendre ballene 
        for (int i = 0; i < ballPositions.size(); ++i)
        {
            // Oppdaterer rotasjonen med tanke på hastigheten 
            balls[i].UpdateRotation(ballVelocities[i], deltaTime, ballsMoving);

            model = glm::mat4(1.0f);
            model = glm::translate(model, ballPositions[i]);
            model = model * balls[i].rotationMatrix;
            textureShader.setMat4("model", model);
            balls[i].DrawBall();
        }

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

    // Ballene begynner å bevege seg når 'space' er trykket på 
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        ballsMoving = true; 
    }
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

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void selectStartPointForBall(Surface& surface, glm::vec3& ballPosition, float xMin, float xMax, float yMin, float yMax, float ballRadius)
{
    float x, y;
    std::cout << "Velg startpunkt for ballen innenfor oppgitte koordinatgrenser:\n";
    std::cout << "x (mellom " << xMin << " og " << xMax << "): ";
    std::cin >> x;
    std::cout << "y (mellom " << yMin << " og " << yMax << "): ";
    std::cin >> y;

    // Begrens til gyldige koordinater
    x = glm::clamp(x, xMin, xMax);
    y = glm::clamp(y, yMin, yMax);

    // Beregn normaliserte verdier for B-spline-flaten
    float u = (x - xMin) / (xMax - xMin);
    float v = (y - yMin) / (yMax - yMin);

    // Finn høyden på B-spline-flaten (z-verdi)
    glm::vec3 surfacePoint = surface.calculateSurfacePoint(u, v);

    // Sett ballens posisjon
    ballPosition = glm::vec3(x, y, surfacePoint.z + ballRadius);

    std::cout << "Ballen er plassert på punkt ("
        << ballPosition.x << ", "
        << ballPosition.y << ", "
        << ballPosition.z << ").\n";
}











