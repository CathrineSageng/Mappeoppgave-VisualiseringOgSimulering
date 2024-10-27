#include<iostream>
#include <fstream>
#include <filesystem>
#include<vector>

#include "glm/mat4x3.hpp"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "ShaderFileLoader.h"
#include "Camera.h"

#include <pdal/pdal.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/Options.hpp>

using namespace std;

//This exercise shows data points over the terrain in Nydal. The data points are converted from LAZ(compressed version of LAS files)
// to text files. There are 6 LAZ files that have been converted from LAZ to text files 

// Global variables
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// Camera settings
//This is the starting position of the of the camera 
Camera camera(glm::vec3(2.13f, 11.7f, 0.2f));

//Keeps track of the last position of the mouse cursor 
GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;

//Avoids sudden jumps in the camera orientation when the mouse is first detected. 
bool firstMouse = true;

// Time between current frame and last frame
float deltaTime = 0.0f;
//Stores the timestamp of previous frame. 
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void convertLazFilesToTextFiles(const string& inputFilename, const string& outputFilename);
vector<glm::vec3> loadPointsFromTextFile(const string& filename);
std::vector<glm::vec3> loadPointsFromMultipleTextFiles(const std::vector<std::string>& textFiles);

string vfs = ShaderLoader::LoadShaderFromFile("vs.vs");
string fs = ShaderLoader::LoadShaderFromFile("fs.fs");

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

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    //A List over the 6 .laz files that contains the data points from Nydal. 
    // The data points are ordered on høydedata.no
    vector<string> lazFiles = 
    {
        "32-2-517-155-12.laz"
    };

    // A list of the .txt files that stores the x, y, z coordinates after the conversion
    vector<string> textFiles = 
    {
        "32-2-517-155-12.txt"
    };

    //The loop iterates over the lazFiles and uses the 'convertLazFilesToTextFiles' function to convert the files
    // to text files. 
    for (int i = 0; i < lazFiles.size(); ++i)
    {
        convertLazFilesToTextFiles(lazFiles[i], textFiles[i]);
    }

    // Stores the points from the text file 
    vector<glm::vec3> points = loadPointsFromMultipleTextFiles(textFiles);

    //Checks if the points are available to render
    if (points.empty())
    {
        cerr << "Ingen punkter å rendre." << endl;
        return -1;
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


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

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);

        //Rendering the points 
        glBindVertexArray(VAO);
        glPointSize(3.0f); 
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
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

//Function for conveting .laz file to a text file. The text file will contain x, y, z coordinates from the .laz file 
void convertLazFilesToTextFiles(const string& inputFilename, const string& outputFilename)
{  
    //Removes the text file if the file already exits. The program is not able to open the text file if the old 
    // version is still there. Are not able to open the outputfile 
    if (filesystem::exists(outputFilename))
    {
        filesystem::remove(outputFilename);
    }

    //Sets up PDAL so it can read the .laz file 
    //PDAL(Point Data Abstarction Library)
    //Holds the settings for reading the file 
    pdal::Options options;
    options.add("filename", inputFilename);

    //Creates an objects to read the .laz files
    pdal::LasReader reader;
    reader.setOptions(options);

    //Reads the point data from the .laz file 
    pdal::PointTable table;
    reader.prepare(table);
    pdal::PointViewSet coordinates = reader.execute(table);

    //Opens the text file the data points are going to be converted to. 
    ofstream outFile(outputFilename);
    if (!outFile.is_open())
    {
        cerr << "Not able to open the output file: " << outputFilename << endl;
            return;
    }

    //Counts the total number of data points in the .laz file and writes the total number at the first line of the text file 
    size_t totalPoints = 0;
    for (auto& coordinate : coordinates)
    {
            totalPoints += coordinate->size();
    }
    outFile << totalPoints << endl;

    //Gets every x, y, z coordinate from the .laz file and writes them to the text file. 
    //The points are retrived from the .laz file using 'getFiledAs'
    for (auto& view : coordinates)
    {
        for (pdal::PointId i = 0; i < view->size(); ++i)
        {
            double x = view->getFieldAs<double>(pdal::Dimension::Id::X, i);
            double y = view->getFieldAs<double>(pdal::Dimension::Id::Y, i);
            double z = view->getFieldAs<double>(pdal::Dimension::Id::Z, i);
            outFile << x << " " << y << " " << z << endl;
            }
        }

    //Closes the output file
    outFile.close();
    cout << "Conversion completed for: " << inputFilename << endl;
}

//Function for reading the coordinates from the text file to rendering
vector<glm::vec3> loadPointsFromTextFile(const string& filename)
{
    //ifstream opens the file for reading
    ifstream inFile(filename);
    vector<glm::vec3> points;

    //Checks if the opening of the file was succsessful. 
    if (!inFile.is_open())
    {
        cerr << "Could not open the file " << filename << endl;
        return points;
    }

    //Reads number of points of the text file (the first line of the text file)
    size_t numPoints;
    inFile >> numPoints;
    cout << "Number of points in the file: " << numPoints << endl;

    //The variables are used to scale and translate the data points (coordinates) so we can see them in the camera view
    float x, y, z;

    const float xScale = 0.0001f;   
    const float yScale = 0.0001f;  
    const float zScale = 0.0001f*5;
    //Moves the points near origo
    const glm::vec3 translationOffset(-59.0f, -663.0f, 0.0f); 

    //The loop reads the data points from the text file. The data points are scaled and translated, and then beeing stored in a vector
    size_t pointCounter = 0;
    while (inFile >> x >> y >> z)
    {
        glm::vec3 point(x * xScale, y * yScale, z * zScale); // Forsterker z for bedre synlighet
        point += translationOffset; // Flytter punktene etter skalering
        points.push_back(point);

        // Writes out the 10 coordinates from the text file to see that the translation is ok
        if (pointCounter < 10)
        {
            cout << "Point " << pointCounter + 1 << ": "
                << point.x << ", " << point.y << ", " << point.z <<endl;
        }
        pointCounter++;
    }
    //Closes the file and the function returns the points vector
    inFile.close();
    cout << "Total number of loaded points: " << points.size() <<endl;
    return points;
}

//Function that loads multiple text files and returns one vector with all the coodinates 
vector<glm::vec3> loadPointsFromMultipleTextFiles(const vector<string>& textFiles)
{
    //Stores the data points from all the text files
    vector<glm::vec3> allPoints;

    for (const auto& textFile : textFiles)
    {
        vector<glm::vec3> points = loadPointsFromTextFile(textFile);
        allPoints.insert(allPoints.end(), points.begin(), points.end());
    }

    return allPoints;
}











