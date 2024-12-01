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

using namespace std; 

class BilinearSurface
{

public:
    BilinearSurface();
    ~BilinearSurface();

    void loadAndInitialize(const string& filename, float reductionCellSize);
    void draw(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    void drawNormals(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    void drawPoints(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    void drawControlPoints(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);

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


    vector<glm::vec3> loadPointsFromTextFile(const string& filename);
    vector<glm::vec3> reducePoints(const vector<glm::vec3>& points, float cellSize);

    vector<glm::ivec3> delaunayTriangulation(vector<glm::vec3>& points);
    vector<VertexData> calculateVertices(const vector<glm::vec3>& points, const vector<glm::ivec3>& triangles);
    bool inCircumcircle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p);
    void addSuperTriangle(vector<glm::vec3>& points);
    vector<glm::vec3> calculateControlPoints(const std::vector<glm::vec3>& points, const vector<glm::ivec3>& triangles);
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
