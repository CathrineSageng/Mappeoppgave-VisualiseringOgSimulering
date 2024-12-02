#include "BilinearSurface.h"
#include <algorithm>
#include <iostream>

BilinearSurface::BilinearSurface() : VAO(0), VBO(0), EBO(0), VAONormals(0), VBONormals(0),
VAOPoints(0), VBOPoints(0), VAOControlPoints(0), VBOControlPoints(0){}

BilinearSurface::~BilinearSurface()
{
    cleanup();
}

void BilinearSurface::loadFunctions(const string& filename, float reductionCellSize) 
{
    points = loadsPointsFromTextfile(filename);
    points = reducePoints(points, reductionCellSize);

    triangles = delaunayTriangulation(points);
    vertices = Normals(points, triangles);

    for (const auto& vertex : vertices) 
    {
        glm::vec3 start = vertex.position;
        glm::vec3 end = start + vertex.normal * 0.001f;
        normalLines.push_back(start);
        normalLines.push_back(end);
    }

    controlPoints = calculateControlPoints(points, triangles);

    setupPointBuffers();
    setupBuffers();
    setupNormalBuffers();
    setupControlPointBuffers(); 
}

//Referanse https://www.geeksforgeeks.org/how-to-read-from-a-file-in-cpp/
vector<glm::vec3> BilinearSurface::loadsPointsFromTextfile(const string& filename) 
{
    ifstream inFile(filename);
    vector<glm::vec3> points;

    if (!inFile.is_open()) 
    {
        return points;
    }

    size_t numPoints;
    inFile >> numPoints;

    float x, y, z;
    const float xScale = 0.0001f;
    const float yScale = 0.0001f;
    const float zScale = 0.0002f;
    const glm::vec3 translationOffset(-59.0f, -663.0f, 0.0f);

    while (inFile >> x >> y >> z) 
    {
        glm::vec3 point(x * xScale, y * yScale, z * zScale);
        point += translationOffset;
        points.push_back(point);
    }

    inFile.close();
    return points;
}

//Denne funksjonen reduserer antall punkter i punktskyen. Den bruker hash funksjoenen til å 
//organisere punktene i et rutenett og reduserer antall punkter slik at hver celle i rutenettet
//inneholder et punkt 
vector<glm::vec3> BilinearSurface::reducePoints(const vector<glm::vec3>& points, float cellSize) {
    unordered_map<pair<int, int>, glm::vec3, pair_hash> grid;

    for (const auto& point : points) 
    {
        int xIdx = static_cast<int>(point.x / cellSize);
        int yIdx = static_cast<int>(point.y / cellSize);
        std::pair<int, int> gridCell = { xIdx, yIdx };

        if (grid.find(gridCell) == grid.end()) 
        {
            grid[gridCell] = point;
        }
    }

    vector<glm::vec3> reducedPoints;
    for (const auto& cell : grid) 
    {
        reducedPoints.push_back(cell.second);
    }
    return reducedPoints;
}

void BilinearSurface::drawPoints(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) 
{
    glBindVertexArray(VAOPoints);
    glPointSize(2.0f);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
    glBindVertexArray(0);
}

void BilinearSurface::draw(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) 
{
    glBindVertexArray(VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangles.size() * 3), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void BilinearSurface::drawNormals(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) 
{
    glBindVertexArray(VAONormals);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(normalLines.size()));
    glBindVertexArray(0);
}

void BilinearSurface::drawControlPoints(const Shader& shader, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) {
    glBindVertexArray(VAOControlPoints);
    glPointSize(6.0f); 
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(controlPoints.size()));
    glBindVertexArray(0);
}

//Utfører Delaunay trianguleringen på punktskyen, legger først til trkanten som omfavner alle punktene. 
// Går gjennom hvert punkt og ser om punktene er innenfor en omskrevne sirkelen til en trelant, fjerner trekanter 
// som ikke følger Delaunay egenskapen, bruker kantene fra trekantene til å lage nye trekanter. 
// Tilslutt fjernes alle trekanter som bruker noen av punktene til den store trekanten. 
//Referanse https://github.com/Maknee/Delaunay-Triangulation/tree/master/source
vector<glm::ivec3> BilinearSurface::delaunayTriangulation(vector<glm::vec3>& points) 
{
    vector<glm::ivec3> triangles;
    addSuperTriangle(points);
    int superTriangleStartIndex = points.size() - 3;

    triangles.push_back(glm::ivec3(superTriangleStartIndex, superTriangleStartIndex + 1, superTriangleStartIndex + 2));

    for (size_t i = 0; i < superTriangleStartIndex; ++i) {
        vector<glm::ivec3> badTriangles;
        vector<pair<int, int>> polygon;

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

//Ser etter om et punkt ligger innenfor den omskrevne sirkelen til en trekant. 
//Bruker determinanter til å avgjøre om punktet ligger innenfor den omskrevne sirkelen. 
bool BilinearSurface::inCircumcircle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p)
{
    float ax = a.x - p.x, ay = a.y - p.y;
    float bx = b.x - p.x, by = b.y - p.y;
    float cx = c.x - p.x, cy = c.y - p.y;
    float determinant =
        (ax * ax + ay * ay) * (bx * cy - by * cx) -
        (bx * bx + by * by) * (ax * cy - ay * cx) +
        (cx * cx + cy * cy) * (ax * by - ay * bx);
    return determinant > 0;
}

//Legger til en stor trekant hvor hele punktskyen er innenfor trekanten. 
//Dette gjøres for at det ikke skal være noen annen kode f.eks andre punkter son kan forstyrre Delaunay trianguleringen 
void BilinearSurface::addSuperTriangle(vector<glm::vec3>& points) 
{
    float maxCoordinate = 50.0f;
    points.push_back(glm::vec3(-maxCoordinate, -maxCoordinate, 0.0f));
    points.push_back(glm::vec3(maxCoordinate, -maxCoordinate, 0.0f));
    points.push_back(glm::vec3(0.0f, maxCoordinate, 0.0f));
}

//Referanse https://stackoverflow.com/questions/30120636/calculating-vertex-normals-in-opengl-with-c
//Regner ut normalvektorer til punktene på den bilineære flaten. Disse brukes til lysetting for phong shaderen. 
vector<BilinearSurface::VertexData> BilinearSurface::Normals(const vector<glm::vec3>& points, const vector<glm::ivec3>& triangles) 
{
    vector<glm::vec3> normals(points.size(), glm::vec3(0.0f));

    for (const auto& triangle : triangles) 
    {
        glm::vec3 p0 = points[triangle.x];
        glm::vec3 p1 = points[triangle.y];
        glm::vec3 p2 = points[triangle.z];

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        normals[triangle.x] += normal;
        normals[triangle.y] += normal;
        normals[triangle.z] += normal;
    }

    for (auto& normal : normals) 
    {
        normal = glm::normalize(normal);
    }

    vector<VertexData> vertexData;
    for (size_t i = 0; i < points.size(); ++i) 
    {
        vertexData.push_back({ points[i], normals[i] });
    }

    return vertexData;
}

//Ser på trekantene som blir generert av Delaunay trianguleringen. Senteret av trekantene blir kontrollpunktene for B-spline flaten 
vector<glm::vec3> BilinearSurface::calculateControlPoints(const vector<glm::vec3>& points, const vector<glm::ivec3>& triangles) 
{
    vector<glm::vec3> controlPoints;

    for (const auto& triangle : triangles) 
    {
        glm::vec3 p0 = points[triangle.x];
        glm::vec3 p1 = points[triangle.y];
        glm::vec3 p2 = points[triangle.z];

        glm::vec3 midpoint01 = (p0 + p1) * 0.5f;
        glm::vec3 midpoint12 = (p1 + p2) * 0.5f;
        glm::vec3 midpoint20 = (p2 + p0) * 0.5f;

        glm::vec3 center = (p0 + p1 + p2) / 3.0f;

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

//Lager skjøtvektoren for B-spine flaten. Den lager jevnt stigende verdier (uniform skjøtvektor)
vector<float> BilinearSurface::generateKnotVector(int numControlPoints) 
{
    int m = numControlPoints - 1;
    std::vector<float> knotVector;

    for (int i = 0; i < 3; ++i) knotVector.push_back(0.0f);
    for (int i = 1; i < m - 1; ++i) knotVector.push_back(static_cast<float>(i));
    for (int i = 0; i < 3; ++i) knotVector.push_back(static_cast<float>(m));

    return knotVector;
}

//Ser på B-spline overflaten i u og v retning ved et kontrollpunkt. 
//Basisfuksjonene brukes til å bestemme hvor mye hvert kontrollpunkt påvirker overflaten. 
glm::vec3 BilinearSurface::evaluateBSpline(float u, float v, const vector<glm::vec3>& controlPoints, const vector<float>& knotsU, const vector<float>& knotsV) 
{
    int n = controlPoints.size();
    glm::vec3 result(0.0f);

    for (int i = 0; i < n; ++i) 
    {
        for (int j = 0; j < n; ++j) 
        {
            float Bu = basisFunction(i, 3, u, knotsU);
            float Bv = basisFunction(j, 3, v, knotsV);
            result += Bu * Bv * controlPoints[i * n + j];
        }
    }

    return result;
}

//Basisfunksjonene for B-spline flaten 
float BilinearSurface::basisFunction(int i, int k, float u, const vector<float>& knots) 
{
    if (k == 1) 
    {
        return (knots[i] <= u && u < knots[i + 1]) ? 1.0f : 0.0f;
    }
    else {
        float coefficient1 = (u - knots[i]) / (knots[i + k - 1] - knots[i]);
        float coefficient2 = (knots[i + k] - u) / (knots[i + k] - knots[i + 1]);

        return coefficient1 * basisFunction(i, k - 1, u, knots) + coefficient2 * basisFunction(i + 1, k - 1, u, knots);
    }
}

void BilinearSurface::setupBuffers() 
{
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

void BilinearSurface::setupControlPointBuffers() 
{
    glGenVertexArrays(1, &VAOControlPoints);
    glGenBuffers(1, &VBOControlPoints);

    glBindVertexArray(VAOControlPoints);

    glBindBuffer(GL_ARRAY_BUFFER, VBOControlPoints);
    glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(glm::vec3), controlPoints.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void BilinearSurface::setupPointBuffers() 
{
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

void BilinearSurface::setupNormalBuffers() 
{
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

void BilinearSurface::cleanup() 
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAONormals);
    glDeleteBuffers(1, &VBONormals);
    glDeleteVertexArrays(1, &VAOPoints);
    glDeleteBuffers(1, &VBOPoints);
    glDeleteVertexArrays(1, &VAOControlPoints);
    glDeleteBuffers(1, &VBOControlPoints);
}
