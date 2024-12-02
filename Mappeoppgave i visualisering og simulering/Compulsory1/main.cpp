#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
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
#include "Surface.h"
#include "Ball.h"
#include "Octree.h"
#include "PhysicsCalculations.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace std;

//----------------------------------------------------------------------------------------------------------------------------------------------------//

const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;


Camera camera(glm::vec3(2.13f, 11.7f, 0.4f));
GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//Posisjonen på lyset til teksturen og phong-shaderen 
glm::vec3 sunPos(2.0f, 11.0f, 2.0f);


//Grensene på B-Sline flaten. Ballene skal ikke rulle utenfor disse verdiene. 
float xMin = 2.04f;
float xMax = 2.199f;
float yMin = 11.64f;
float yMax = 11.76f;

//Ballene beveger seg ikke når programmet starter og ballenes størrelse 
bool ballsMoving = false;
float ballRadius = 0.005f;

//Styring av tiden 
float deltaTime = 0.0f;
float lastFrame = 0.0f;
const float fixedTimeStep = 0.01f;
float accumulator = 0.0f;

// Området på B-spline overflaten hvor det er høyere friksjon
float frictionAreaXMin = 2.04f;
float frictionAreaXMax = 2.1f;
float frictionAreaYMin = 11.64f;
float frictionAreaYMax = 11.7f;

//Den lave friksjonen er på mesteparten av B-spline flaten og høye friksjonen er i området avgrenset med høyere friksjon. 
float normalFriction = 0.01f;
float highFriction = 0.5f;

vector<glm::vec3> controlPoints =
{
    glm::vec3(2.04f, 11.64f, 0.041f), glm::vec3(2.093f, 11.64f, 0.040f), glm::vec3(2.148f,11.64f, 0.037f), glm::vec3(2.199f,11.64f, 0.035f),
    glm::vec3(2.04f, 11.7f, 0.043f), glm::vec3(2.093f, 11.7f, 0.037f), glm::vec3(2.148f, 11.7f, 0.0390f), glm::vec3(2.199f, 11.7f, 0.035f),
    glm::vec3(2.04f, 11.76f, 0.044f), glm::vec3(2.093f, 11.76f, 0.039f), glm::vec3(2.148f, 11.76f, 0.044f), glm::vec3(2.199f,11.76f, 0.077f),
};

// Uniform skjøtvektor horisontal retning
vector<float> knotVectorU = { 0, 0, 0, 1, 2, 2, 2 };
// Uniform skjøtvektor vertikal retning 
vector<float> knotVectorV = { 0, 0, 0, 1, 1, 1 };

//----------------------------------------------------------------------------------------------------------------------------------------------------//

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadTexture(char const* path);
void selectStartPointForBall(Surface& surface, glm::vec3& ballPosition, float xMin, float xMax, float yMin, float yMax, float ballRadius);

//-----------------------------------------------------------------------------------------------------------------------------------------------------//

//Til punktskyen og normalene 
string vfs = ShaderLoader::LoadShaderFromFile("vs.vs");
string fs = ShaderLoader::LoadShaderFromFile("fs.fs");

//PhongShader
string vfsp = ShaderLoader::LoadShaderFromFile("phong.vert");
string fsp = ShaderLoader::LoadShaderFromFile("phong.frag");

//Tekstur shader til tekstur på ballene 
string vfsT = ShaderLoader::LoadShaderFromFile("Texture.vs");
string fsT = ShaderLoader::LoadShaderFromFile("Texture.fs");

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Terreng", NULL, NULL);
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
    Shader textureShader("Texture.vs", "Texture.fs");

    glEnable(GL_DEPTH_TEST);

    Surface surface(controlPoints, 4, 3, knotVectorU, knotVectorV); 
    Octree octree(glm::vec3(xMin, yMin, xMin), glm::vec3(xMax, yMax, xMax), 0, 4, 4);
    PhysicsCalculations physics(xMin, xMax, yMin, yMax, ballRadius);

    //Oppretter objekter for ballene med posisjon og hastighetsretning og bakgrunnsfage
    vector<glm::vec3> ballPositions = { {2.04f, 11.76f, 0.05f}, {2.199f, 11.76f, 0.05f} };
    vector<glm::vec3> ballVelocities = { {0.3f, -0.1f, 0.0f}, {-0.3f, -0.1f, 0.0f} };
    vector<vector<glm::vec3>> ballTrack(ballPositions.size());
    vector<Ball> balls;
    for (int i = 0; i < ballPositions.size(); ++i) 
    {
        balls.push_back(Ball(ballRadius, 30, 30, glm::vec3(1.0f, 1.0f, 1.0f)));
    }

    //Antall punkter på B-spline flaten for å bestemme hvor "glatt" den skal være 
    int pointsOnTheSurface = 20;

    //Oppretter buffere og VAO for flaten 
    unsigned int surfaceVAO, surfaceVBO, colorVBO, normalVBO, EBO, normalVAO, normalLineVBO;
    surface.setupBuffers(surfaceVAO, surfaceVBO, colorVBO, normalVBO, EBO, normalVAO, normalLineVBO,
        pointsOnTheSurface, frictionAreaXMin, frictionAreaXMax,
        frictionAreaYMin, frictionAreaYMax);

    //Oppdaterer fysikken i prosjektet 
    physics.updatePhysics(ballPositions, ballVelocities, ballTrack, octree, ballsMoving,
        fixedTimeStep, ballRadius, xMin, xMax, yMin, yMax, surface,
        normalFriction, highFriction, frictionAreaXMin, frictionAreaXMax,
        frictionAreaYMin, frictionAreaYMax);

    //Plassering av bellene på B-spline flaten. Ballene kan ikke plasseres utenfor 
    cout << "Koordinatgrenser for flaten:"<< endl;
    cout << "x: [" << xMin << ", " << xMax << "]"<<endl;
    cout << "y: [" << yMin << ", " << yMax << "]"<<endl;

    for (int i = 0; i < ballPositions.size(); ++i) 
    {
        cout << "Velg posisjon for ball " << i + 1 << ":"<<endl;
        selectStartPointForBall(surface, ballPositions[i], xMin, xMax, yMin, yMax, ballRadius);
    }

    //Teksturen på ballene
    unsigned int diffuseMap1 = loadTexture("Textures/ball.jpg");
    unsigned int specularMap = loadTexture("Textures/ball2.jpg");

    textureShader.use();
    textureShader.setInt("material.diffuse", 0);
    textureShader.setInt("material.specular", 1);

    BilinearSurface bilinear;
    bilinear.loadFunctions("32-2-517-155-12.txt", 0.0008f);


  while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        phongShader.use();

        phongShader.setVec3("light.position", sunPos);
        phongShader.setVec3("viewPos", camera.Position);

        // Egenskaper for lys
        phongShader.setVec3("light.ambient", 0.2f, 0.3f, 0.2f);  
        phongShader.setVec3("light.diffuse", 0.3f, 0.5f, 0.3f);   
        phongShader.setVec3("light.specular", 0.4f, 0.4f, 0.4f);  

        // Egenskaper for materialer
        phongShader.setVec3("material.ambient", 0.1f, 0.3f, 0.1f); 
        phongShader.setVec3("material.diffuse", 0.2f, 0.6f, 0.2f); 
        phongShader.setVec3("material.specular", 0.1f, 0.2f, 0.1f);
        phongShader.setFloat("material.shininess", 16.0f);         

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        phongShader.setMat4("projection", projection);
        phongShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        phongShader.setMat4("model", model);

         //Rendrer bikvadratisk b- spline tensorprodukt flate med en del som har høyere friksjon
       glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
       glBindVertexArray(surfaceVAO);
       glDrawElements(GL_TRIANGLES, pointsOnTheSurface * pointsOnTheSurface * 6, GL_UNSIGNED_INT, 0);
       glBindVertexArray(0);

       //Rendrer b-spline kurven som er sporing av banen til ballene. 
       for (int i = 0; i < ballTrack.size(); ++i) 
       {
           if (ballTrack[i].size() > 1) 
           {
               auto curvePoints = surface.calculateBSplineCurve(ballTrack[i], 3, 50);
               surface.renderBSplineCurve(curvePoints, phongShader, projection, view);
           }
       }

       //bilinear.draw(phongShader, projection, view, model);

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

       ////Oppdaterer all fysikken 
       accumulator += deltaTime;
       while (accumulator >= fixedTimeStep)
       {
           physics.updatePhysics(
               ballPositions,          // Oppdaterer posisjonen til ballene 
               ballVelocities,         // Oppdaterer bellenes hastighet
               ballTrack,              // Oppdaterer sporenene til ballene 
               octree,                 // Octree for kollisjonsberegning
               ballsMoving,            // Undersøker om ballene beveger seg 
               fixedTimeStep,          // For å få jevn hastighet på ballene 
               ballRadius,             // Størrelsen/ radisuen på ballene 
               xMin, xMax, yMin, yMax, // Størrelsen på B-spline flaten 
               surface,                // Overflaten- B-spline flaten 
               normalFriction,         // Friksjonen på størstedelen av flaten 
               highFriction,           // Høyere friksjon på delen det lagt til friksjon 
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

        // Bruk ourShader til å rendre normalene
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);

        //bilinear.drawNormals(ourShader, projection, view, model);

        //bilinear.drawPoints(ourShader, projection, view, model);

        //bilinear.drawControlPoints(ourShader, projection, view, model);
  
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

//Velger startposisjonen for ballene. Startposisjonen for ballene kan kun velges innenfor gitte koordinater. 
void selectStartPointForBall(Surface& surface, glm::vec3& ballPosition, float xMin, float xMax, float yMin, float yMax, float ballRadius)
{
    //Informerer om tilatte verdier ballene kan settes og ber brukeren skrive inn x og y koordinater. 
    float x, y;
    cout << "Velg startpunkt for ballen innenfor oppgitte koordinatgrenser:" << endl;
    cout << "x (mellom " << xMin << " og " << xMax << "): ";
    cin >> x;
    cout << "y (mellom " << yMin << " og " << yMax << "): ";
    cin >> y;

    //clamp gjør at x og y verdiene til ballen er innenfor de gitte grensene. 
    //Hvis brukeren setter vedier utenfor angitte koorfinater blir verdiene justert til nærmeste gyldig verdi. 
    x = glm::clamp(x, xMin, xMax);
    y = glm::clamp(y, yMin, yMax);

    //Normaliserer koordinatene, brukes for høyderegning av z koordinaten 
    float u = (x - xMin) / (xMax - xMin);
    float v = (y - yMin) / (yMax - yMin);
    //Beregner høyden på B-spline flaten. 
    glm::vec3 surfacePoint = surface.calculateSurfacePoint(u, v);
    //Den nye oppdaterte posisjonen til ballen 
    ballPosition = glm::vec3(x, y, surfacePoint.z + ballRadius);

    cout << "Ballen er plassert på punkt (" << ballPosition.x << ", " << ballPosition.y << ", " << ballPosition.z << ")." << endl;
}












