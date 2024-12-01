#include "BilinearSurface.h"
#include <algorithm>
#include <iostream>

BilinearSurface::BilinearSurface() : VAO(0), VBO(0), EBO(0), VAONormals(0), VBONormals(0), VAOPoints(0), VBOPoints(0) {}

BilinearSurface::~BilinearSurface() {
    cleanup();
}

void BilinearSurface::loadAndInitialize(const std::string& filename, float reductionCellSize) {
    points = loadPointsFromTextFile(filename);
    points = reducePoints(points, reductionCellSize);

    triangles = delaunayTriangulation(points);
    vertices = calculateVertices(points, triangles);

    // Prepare normal line data
    for (const auto& vertex : vertices) {
        glm::vec3 start = vertex.position;
        glm::vec3 end = start + vertex.normal * 0.001f; // Adjust normal length
        normalLines.push_back(start);
        normalLines.push_back(end);
    }

    setupPointBuffers();
    setupBuffers();
    setupNormalBuffers();
}

std::vector<glm::vec3> BilinearSurface::loadPointsFromTextFile(const std::string& filename) {
    std::ifstream inFile(filename);
    std::vector<glm::vec3> points;

    if (!inFile.is_open()) {
        std::cerr << "Could not open the file " << filename << std::endl;
        return points;
    }

    size_t numPoints;
    inFile >> numPoints;
    std::cout << "Number of points in the file: " << numPoints << std::endl;

    float x, y, z;
    const float xScale = 0.0001f;
    const float yScale = 0.0001f;
    const float zScale = 0.0002f;
    const glm::vec3 translationOffset(-59.0f, -663.0f, 0.0f);

    while (inFile >> x >> y >> z) {
        glm::vec3 point(x * xScale, y * yScale, z * zScale);
        point += translationOffset;
        points.push_back(point);
    }

    inFile.close();
    std::cout << "Total number of loaded points: " << points.size() << std::endl;
    return points;
}

std::vector<glm::vec3> BilinearSurface::reducePoints(const std::vector<glm::vec3>& points, float cellSize) {
    std::unordered_map<std::pair<int, int>, glm::vec3, pair_hash> grid;

    for (const auto& point : points) {
        int xIdx = static_cast<int>(point.x / cellSize);
        int yIdx = static_cast<int>(point.y / cellSize);
        std::pair<int, int> gridCell = { xIdx, yIdx };

        if (grid.find(gridCell) == grid.end()) {
            grid[gridCell] = point;
        }
    }

    std::vector<glm::vec3> reducedPoints;
    for (const auto& cell : grid) {
        reducedPoints.push_back(cell.second);
    }
    return reducedPoints;
}

void BilinearSurface::drawPoints(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) {
   
    glBindVertexArray(VAOPoints);
    glPointSize(2.0f); // Make points larger for visibility
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
    glBindVertexArray(0);
}

void BilinearSurface::draw(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) {
 

    glBindVertexArray(VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangles.size() * 3), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void BilinearSurface::drawNormals(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) {

    glBindVertexArray(VAONormals);
    glLineWidth(2.0f); // Adjust line width for visibility
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(normalLines.size()));
    glBindVertexArray(0);
}

std::vector<glm::ivec3> BilinearSurface::delaunayTriangulation(std::vector<glm::vec3>& points) {
    std::vector<glm::ivec3> triangles;
    addSuperTriangle(points);
    int superTriangleStartIndex = points.size() - 3;

    triangles.push_back(glm::ivec3(superTriangleStartIndex, superTriangleStartIndex + 1, superTriangleStartIndex + 2));

    for (size_t i = 0; i < superTriangleStartIndex; ++i) {
        std::vector<glm::ivec3> badTriangles;
        std::vector<std::pair<int, int>> polygon;

        for (const auto& tri : triangles) {
            if (inCircumcircle(points[tri.x], points[tri.y], points[tri.z], points[i])) {
                badTriangles.push_back(tri);
                polygon.emplace_back(tri.x, tri.y);
                polygon.emplace_back(tri.y, tri.z);
                polygon.emplace_back(tri.z, tri.x);
            }
        }

        triangles.erase(std::remove_if(triangles.begin(), triangles.end(),
            [&](const glm::ivec3& tri) {
                return std::find(badTriangles.begin(), badTriangles.end(), tri) != badTriangles.end();
            }),
            triangles.end());

        for (auto it = polygon.begin(); it != polygon.end();) {
            auto reverseEdge = std::make_pair(it->second, it->first);
            if (std::count(polygon.begin(), polygon.end(), reverseEdge)) {
                polygon.erase(std::remove(polygon.begin(), polygon.end(), reverseEdge), polygon.end());
                it = polygon.erase(it);
            }
            else {
                ++it;
            }
        }

        for (const auto& edge : polygon) {
            triangles.push_back(glm::ivec3(edge.first, edge.second, static_cast<int>(i)));
        }
    }

    triangles.erase(std::remove_if(triangles.begin(), triangles.end(),
        [superTriangleStartIndex](const glm::ivec3& tri) {
            return tri.x >= superTriangleStartIndex || tri.y >= superTriangleStartIndex || tri.z >= superTriangleStartIndex;
        }),
        triangles.end());
    return triangles;
}

std::vector<BilinearSurface::VertexData> BilinearSurface::calculateVertices(const std::vector<glm::vec3>& points, const std::vector<glm::ivec3>& triangles) {
    std::vector<glm::vec3> normals(points.size(), glm::vec3(0.0f));

    for (const auto& tri : triangles) {
        glm::vec3 p0 = points[tri.x];
        glm::vec3 p1 = points[tri.y];
        glm::vec3 p2 = points[tri.z];

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        normals[tri.x] += normal;
        normals[tri.y] += normal;
        normals[tri.z] += normal;
    }

    for (auto& normal : normals) {
        normal = glm::normalize(normal);
    }

    std::vector<VertexData> vertexData;
    for (size_t i = 0; i < points.size(); ++i) {
        vertexData.push_back({ points[i], normals[i] });
    }

    return vertexData;
}

bool BilinearSurface::inCircumcircle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p) {
    float ax = a.x - p.x, ay = a.y - p.y;
    float bx = b.x - p.x, by = b.y - p.y;
    float cx = c.x - p.x, cy = c.y - p.y;
    float det = (ax * ax + ay * ay) * (bx * cy - by * cx) -
        (bx * bx + by * by) * (ax * cy - ay * cx) +
        (cx * cx + cy * cy) * (ax * by - ay * bx);
    return det > 0;
}

void BilinearSurface::addSuperTriangle(std::vector<glm::vec3>& points) {
    float maxCoordinate = 50.0f;
    points.push_back(glm::vec3(-maxCoordinate, -maxCoordinate, 0.0f));
    points.push_back(glm::vec3(maxCoordinate, -maxCoordinate, 0.0f));
    points.push_back(glm::vec3(0.0f, maxCoordinate, 0.0f));
}

void BilinearSurface::setupBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexData), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(glm::ivec3), triangles.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void BilinearSurface::setupPointBuffers() {
    glGenVertexArrays(1, &VAOPoints);
    glGenBuffers(1, &VBOPoints);

    glBindVertexArray(VAOPoints);

    glBindBuffer(GL_ARRAY_BUFFER, VBOPoints);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void BilinearSurface::setupNormalBuffers() {
    glGenVertexArrays(1, &VAONormals);
    glGenBuffers(1, &VBONormals);

    glBindVertexArray(VAONormals);

    glBindBuffer(GL_ARRAY_BUFFER, VBONormals);
    glBufferData(GL_ARRAY_BUFFER, normalLines.size() * sizeof(glm::vec3), normalLines.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void BilinearSurface::cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAONormals);
    glDeleteBuffers(1, &VBONormals);
}
