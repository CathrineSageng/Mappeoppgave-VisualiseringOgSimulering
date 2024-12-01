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

class BilinearSurface {
public:
    BilinearSurface();
    ~BilinearSurface();

    void loadAndInitialize(const std::string& filename, float reductionCellSize);
    void draw(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    void drawNormals(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    void drawPoints(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);

    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& pair) const {
            auto hash1 = std::hash<T1>{}(pair.first);
            auto hash2 = std::hash<T2>{}(pair.second);
            return hash1 ^ (hash2 << 1);
        }
    };
    std::unordered_map<std::pair<int, int>, glm::vec3, pair_hash> grid;


private:
    struct VertexData {
        glm::vec3 position;
        glm::vec3 normal;
    };

    GLuint VAO, VBO, EBO;
    GLuint VAONormals, VBONormals;
    GLuint VAOPoints, VBOPoints;

    std::vector<VertexData> vertices;
    std::vector<glm::ivec3> triangles;
    std::vector<glm::vec3> normalLines;
    std::vector<glm::vec3> points;

    std::vector<glm::vec3> loadPointsFromTextFile(const std::string& filename);
    std::vector<glm::vec3> reducePoints(const std::vector<glm::vec3>& points, float cellSize);

    std::vector<glm::ivec3> delaunayTriangulation(std::vector<glm::vec3>& points);
    std::vector<VertexData> calculateVertices(const std::vector<glm::vec3>& points, const std::vector<glm::ivec3>& triangles);
    bool inCircumcircle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p);
    void addSuperTriangle(std::vector<glm::vec3>& points);
    void setupBuffers();
    void setupNormalBuffers();
    void setupPointBuffers();
    void cleanup();
};

#endif // BILINEARSURFACE_H
