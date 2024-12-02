#ifndef BILINEARSURFACE_H
#define BILINEARSURFACE_H

#include <vector>
#include "glm/mat4x3.hpp"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include "Shader.h"
#include <unordered_map> 
#include <utility>  
#include <algorithm>

using namespace std; 

class BilinearSurface
{

public:
    BilinearSurface();
    ~BilinearSurface();

    //Laser opp punktene fra tesktfilen, reduserer antall punkter som skal bli rendret, kaller Delaunay trianguleringen, normalene for overflaten
    //og kontrollpunktene for B-spline overflaten 
    void loadFunctions(const string& filename, float reductionCellSize);
    //Rendrer trianguleringen 
    void draw(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    //Rendrer normalene 
    void drawNormals(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    //Rendrer punktskyen 
    void drawPoints(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    //Rendrer controllpunktene for B-spline overflaten 
    void drawControlPoints(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);

//Referanse https://www.geeksforgeeks.org/how-to-create-an-unordered_map-of-pairs-in-c/
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& pair) const {
            auto hash1 = hash<T1>{}(pair.first);
            auto hash2 = hash<T2>{}(pair.second);
            return hash1 ^ (hash2 << 1);
        }
    };
    unordered_map<pair<int, int>, glm::vec3, pair_hash> grid;


private:

    struct VertexData
    {
        glm::vec3 position;
        glm::vec3 normal;
    };

    GLuint VAO, VBO, EBO;
    GLuint VAONormals, VBONormals;
    GLuint VAOPoints, VBOPoints;
    GLuint VAOControlPoints, VBOControlPoints;

    vector<VertexData> vertices;
    vector<glm::ivec3> triangles;
    vector<glm::vec3> normalLines;
    vector<glm::vec3> points;
    vector<glm::vec3> controlPoints;

    //Laster punktene fra tesktstfil 
    vector<glm::vec3> loadsPointsFromTextfile(const string& filename);
    //Reduserer antall punkter som skal bli rendret 
    vector<glm::vec3> reducePoints(const vector<glm::vec3>& points, float cellSize);
    //Regulær Delaunay triangulering 
    vector<glm::ivec3> delaunayTriangulation(vector<glm::vec3>& points);
    //
    vector<VertexData> Normals(const vector<glm::vec3>& points, const vector<glm::ivec3>& triangles);
    //Ser etter om et punkt ligger innenfor den omskrevne sirkelen til en trekant. 
    bool inCircumcircle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p);
    //Lager en stor trekant som danner en trekant rundt alle punktene 
    void addSuperTriangle(vector<glm::vec3>& points);
    //Kalkulerer kontrollpunktene for den bikvadratiske tensorprodukt B-spline flaten. 
    vector<glm::vec3> calculateControlPoints(const std::vector<glm::vec3>& points, const vector<glm::ivec3>& triangles);
    //Lager skjøtvektoren som skal brukes for B-spline beregningene 
    vector<float> generateKnotVector(int numControlPoints);
    glm::vec3 evaluateBSpline(float u, float v, const vector<glm::vec3>& controlPoints, const vector<float>& knotsU, const vector<float>& knotsV);
    float basisFunction(int i, int k, float u, const vector<float>& knots);

    void setupBuffers();
    void setupNormalBuffers();
    void setupPointBuffers();
    void setupControlPointBuffers();
    void cleanup();
};

#endif // BILINEARSURFACE_H
