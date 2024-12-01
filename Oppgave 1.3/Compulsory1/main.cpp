#include<iostream>
#include <fstream>
#include <filesystem>
#include<vector>
#include <limits>
#include <algorithm>

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

// Strukturerer en trekant for enkel lagring av indekser
struct Triangle {
    int a, b, c;
};

struct VertexData {
    glm::vec3 position;
    glm::vec3 color;
};

struct VertexData1 {
    glm::vec3 position;
    glm::vec3 normal;
};
vector<glm::vec3> superTrianglePoints;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void convertLazFilesToTextFiles(const string& inputFilename, const string& outputFilename);
vector<glm::vec3> loadPointsFromTextFile(const string& filename, int maxPoints);
std::vector<glm::vec3> loadPointsFromMultipleTextFiles(const std::vector<std::string>& textFiles);
bool inCircumcircle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p);
void addSuperTriangle(vector<glm::vec3>& points);
vector<glm::ivec3> delaunayTriangulation(vector<glm::vec3>& points);
vector<glm::vec3> calculateNormals(const vector<glm::vec3>& points, const vector<glm::ivec3>& triangles);


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

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    vector<string> lazFiles = { "32-2-517-155-12.laz" };
    vector<string> textFiles = { "32-2-517-155-12.txt" };

    for (int i = 0; i < lazFiles.size(); ++i) {
        convertLazFilesToTextFiles(lazFiles[i], textFiles[i]);
    }

    vector<glm::vec3> points = loadPointsFromMultipleTextFiles(textFiles);
    if (points.empty()) {
        cerr << "Ingen punkter å rendre." << endl;
        return -1;
    }

    // Setter opp punktskyens VAO og VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    vector<glm::ivec3> triangles = delaunayTriangulation(points);

    // Beregn normalene og sett opp vertexdata med posisjoner og normaler
    vector<VertexData1> vertexData1;
    vector<glm::vec3> normals1 = calculateNormals(points, triangles);

    for (size_t i = 0; i < points.size(); ++i) {
        VertexData1 data;
        data.position = points[i];
        data.normal = normals1[i];
        vertexData1.push_back(data);
    }

    GLuint VAOTri, VBOTri, EBOTri;
    glGenVertexArrays(1, &VAOTri);
    glGenBuffers(1, &VBOTri);
    glGenBuffers(1, &EBOTri);

    glBindVertexArray(VAOTri);
    glBindBuffer(GL_ARRAY_BUFFER, VBOTri);
    glBufferData(GL_ARRAY_BUFFER, vertexData1.size() * sizeof(VertexData), vertexData1.data(), GL_DYNAMIC_DRAW);

    // Oppsett av posisjonsattributtet
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, position));
    glEnableVertexAttribArray(0);

    // Oppsett av normalattributtet
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData1), (void*)offsetof(VertexData1, normal));
    glEnableVertexAttribArray(1);

    // Last opp indeksdataene for trekantene
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOTri);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(glm::ivec3), triangles.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Variabel for å lagre alle punktene for normal-linjesegmentene
    vector<VertexData> normalVertices;

    float normalLength = 0.0001f; // Lengden på normal-linjesegmentene
    glm::vec3 pinkColor(1.0f, 0.0f, 0.5f); // Rosa farge for normalene

    // Beregn normaler
    vector<glm::vec3> normals = calculateNormals(points, triangles);

    // Legg til linjesegmentene for hver normal med farge
    for (size_t i = 0; i < points.size(); ++i) {
        glm::vec3 start = points[i];                     // Startpunkt for normalen
        glm::vec3 end = start + normals[i] * normalLength; // Sluttpunkt for normalen

        // Legg til start- og sluttpunkt til normalVertices med rosa farge
        normalVertices.push_back({ start, pinkColor });
        normalVertices.push_back({ end, pinkColor });
    }

    // Opprett buffer for normalvektorene
    GLuint VAONormals, VBONormals;
    glGenVertexArrays(1, &VAONormals);
    glGenBuffers(1, &VBONormals);

    glBindVertexArray(VAONormals);
    glBindBuffer(GL_ARRAY_BUFFER, VBONormals);
    glBufferData(GL_ARRAY_BUFFER, normalVertices.size() * sizeof(VertexData), normalVertices.data(), GL_DYNAMIC_DRAW);

    // Sett opp posisjonsattributtet
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);
    glEnableVertexAttribArray(0);

    // Sett opp fargeattributtet
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, color));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Lys- og materialegenskaper
    glm::vec3 lightPos(10.0f, 10.0f, 10.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 objectColor(0.1f, 1.0f, 0.1f); // Sett fargen til grønn

    // Bruk Phong-shader
    Shader phongShader("phong.vert", "phong.frag");
    phongShader.use();
    phongShader.setVec3("lightPos", lightPos);
    phongShader.setVec3("viewPos", camera.Position);  // Passerer kameraets posisjon
    phongShader.setVec3("lightColor", lightColor);
    phongShader.setVec3("objectColor", objectColor);

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
       /* glBindVertexArray(VAO);
        glPointSize(3.0f); 
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
        glBindVertexArray(0);*/

        // Rendering av normalvektorer som linjesegmenter
        glBindVertexArray(VAONormals);
        glLineWidth(2.0f); // Tykkelsen på normal-linjene
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(normalVertices.size()));
        glBindVertexArray(0);

        phongShader.use();
        phongShader.setMat4("projection", projection);
        phongShader.setMat4("view", view);
        phongShader.setMat4("model", model);
        phongShader.setVec3("viewPos", camera.Position);

        // Rendering av trianguleringen
        glBindVertexArray(VAOTri);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangles.size() * 3), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

     

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAOTri);
    glDeleteBuffers(1, &VBOTri);
    glDeleteBuffers(1, &EBOTri);
    glDeleteVertexArrays(1, &VAONormals);
    glDeleteBuffers(1, &VBONormals);
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


// Endre `loadPointsFromTextFile` til å begrense antall punkter
vector<glm::vec3> loadPointsFromTextFile(const string& filename, int maxPoints) {
    ifstream inFile(filename);
    vector<glm::vec3> points;

    if (!inFile.is_open()) {
        cerr << "Could not open the file " << filename << endl;
        return points;
    }

    size_t numPoints;
    inFile >> numPoints;
    cout << "Number of points in the file: " << numPoints << endl;

    // Skalerings- og offset-variabler
    float x, y, z;
    const double xScale = 0.0001f;
    const double yScale = 0.0001f;
    const double zScale = 0.0001f;
    const glm::vec3 translationOffset(-59.0f, -663.0f, 0.0f);

    size_t pointCounter = 0;  
    while (inFile >> x >> y >> z && pointCounter < maxPoints) {
        glm::vec3 point(x * xScale, y * yScale, z * zScale);
        point += translationOffset;
        points.push_back(point);
       

        // Writes out the 10 coordinates from the text file to see that the translation is ok
            if (pointCounter < 10)
            {
                cout << "Point " << pointCounter + 1 << ": "
                    << point.x << ", " << point.y << ", " << point.z << endl;
            }
            pointCounter++;
    }

     

    inFile.close();
    cout << "Total number of loaded points (limited): " << points.size() << endl;
    return points;
}


//Function that loads multiple text files and returns one vector with all the coodinates 
vector<glm::vec3> loadPointsFromMultipleTextFiles(const vector<string>& textFiles)
{
    //Stores the data points from all the text files
    vector<glm::vec3> allPoints;

    for (const auto& textFile : textFiles)
    {
        vector<glm::vec3> points = loadPointsFromTextFile(textFile, 150126);
        allPoints.insert(allPoints.end(), points.begin(), points.end());
    }

    return allPoints;
}

bool inCircumcircle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p) {
    double ax = a.x - p.x;
    double ay = a.y - p.y;
    double bx = b.x - p.x;
    double by = b.y - p.y;
    double cx = c.x - p.x;
    double cy = c.y - p.y;

    double det = (ax * ax + ay * ay) * (bx * cy - by * cx) -
        (bx * bx + by * by) * (ax * cy - ay * cx) +
        (cx * cx + cy * cy) * (ax * by - ay * bx);

    return det > 0;
}

// Opprett en supertrekant som dekker alle punktene
void addSuperTriangle(vector<glm::vec3>& points) {
    float maxCoordinate = 10000.0f;
    points.push_back(glm::vec3(-maxCoordinate, -maxCoordinate, 0.0f));
    points.push_back(glm::vec3(maxCoordinate, -maxCoordinate, 0.0f));
    points.push_back(glm::vec3(0.0f, maxCoordinate, 0.0f));
}

// Delaunay-triangulering med inkrementell tilnærming
vector<glm::ivec3> delaunayTriangulation(vector<glm::vec3>& points) {
    vector<glm::ivec3> triangles;

    // Legg til en supertrekant som dekker alle punktene
    int superTriangleStartIndex = points.size();
    addSuperTriangle(points);
    triangles.push_back(glm::ivec3(superTriangleStartIndex, superTriangleStartIndex + 1, superTriangleStartIndex + 2));

    // Sett inn hvert punkt
    for (size_t i = 0; i < superTriangleStartIndex; ++i) {
        vector<glm::ivec3> badTriangles;
        vector<pair<int, int>> polygon;

        // Finn alle trekantene hvis omkrets inneholder punktet
        for (const auto& tri : triangles) {
            if (inCircumcircle(points[tri.x], points[tri.y], points[tri.z], points[i])) {
                badTriangles.push_back(tri);
                polygon.emplace_back(tri.x, tri.y);
                polygon.emplace_back(tri.y, tri.z);
                polygon.emplace_back(tri.z, tri.x);
            }
        }

        // Fjern alle "dårlige" trekanter
        triangles.erase(remove_if(triangles.begin(), triangles.end(),
            [&](const glm::ivec3& tri) {
                return find(badTriangles.begin(), badTriangles.end(), tri) != badTriangles.end();
            }),
            triangles.end());

        // Fjern doble kanter
        for (size_t j = 0; j < polygon.size(); ++j) {
            for (size_t k = j + 1; k < polygon.size(); ++k) {
                if ((polygon[j].first == polygon[k].second) && (polygon[j].second == polygon[k].first)) {
                    polygon.erase(polygon.begin() + k);
                    polygon.erase(polygon.begin() + j);
                    --j;
                    break;
                }
            }
        }

        // Lag nye trekanter fra kanten til det nye punktet
        for (const auto& edge : polygon) {
            triangles.push_back(glm::ivec3(edge.first, edge.second, static_cast<int>(i)));
        }
    }

    // Fjern trekantene som inneholder supertrekantens punkter
    triangles.erase(remove_if(triangles.begin(), triangles.end(),
        [superTriangleStartIndex](const glm::ivec3& tri) {
            return tri.x >= superTriangleStartIndex ||
                tri.y >= superTriangleStartIndex ||
                tri.z >= superTriangleStartIndex;
        }),
        triangles.end());

    return triangles;
}

vector<glm::vec3> calculateNormals(const vector<glm::vec3>& points, const vector<glm::ivec3>& triangles) {
    vector<glm::vec3> normals(points.size(), glm::vec3(0.0f)); // Initierer normalene til (0, 0, 0) for hvert punkt

    for (const auto& tri : triangles) {
        // Få punktkoordinatene for trekantens hjørner
        glm::vec3 p0 = points[tri.x];
        glm::vec3 p1 = points[tri.y];
        glm::vec3 p2 = points[tri.z];

        // Beregn trekantens normal ved kryssprodukt
        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 triangleNormal = glm::normalize(glm::cross(edge1, edge2));

        // Legg til trekantens normal til hvert av trekantens hjørner
        normals[tri.x] += triangleNormal;
        normals[tri.y] += triangleNormal;
        normals[tri.z] += triangleNormal;
    }

    // Normaliser hver normalvektor
    for (auto& normal : normals) {
        normal = glm::normalize(normal);
    }

    return normals;
}












