#include "Surface.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

Surface::Surface(const vector<glm::vec3>& controlPoints, int widthU, int widthV,
    const vector<float>& knotU, const vector<float>& knotV)
    : controlPoints(controlPoints), widthU(widthU), widthV(widthV), knotU(knotU), knotV(knotV) {}

//Denne funsjonen regner ut B-spline basesfunksjonene. 
//i= hvilke del av sjøtevektoren som påvirker basesfunksjonen d= er graden på basisfunksjonen, 
// t = parameteren, knots er delen av skjøtevektoren som er aktiv. 
float Surface::BSplineBasisFunctions(int i, int d, float t, const vector<float>& knots) const
{
    if (i < 0 || i >= knots.size() - 1) return 0.0f;
    //Begynner med grad 0 og undersøker som parameterverdien ligger innenfor intervallet. 
    //Hvis t ligger innenfor intervallet, bidrar basisfunsjonen i dette intervallet. 
    if (d == 0) return (knots[i] <= t && t < knots[i + 1]) ? 1.0f : 0.0f;
    //Lager basisfunksjoner av høyere grader, bygget på lavere grader basisfunksjoner. 
    float intervalLength1 = knots[i + d] - knots[i];
    float basisContribution1 = (intervalLength1 != 0.0f) ? (t - knots[i]) / intervalLength1 * BSplineBasisFunctions(i, d - 1, t, knots) : 0.0f;
    float intervalLength2 = knots[i + d + 1] - knots[i + 1];
    float basisContribution2 = (intervalLength2 != 0.0f) ? (knots[i + d + 1] - t) / intervalLength2 * BSplineBasisFunctions(i + 1, d - 1, t, knots) : 0.0f;
    return basisContribution1 + basisContribution2;
}

//Denne funksjonen er viktig for utregning av normaler. 
//Her ser man hvordan overflaten endrer seg lokalt ved å finne tangentvektoren til overflaten i u og v retning ved å kombinere basisfunksjoner og 
//kontrollpunkter 
//Bestemmer retningen av derivaten (true beregner derivaten med hensyn på u
glm::vec3 Surface::calculatePartialDerivative(float u, float v, bool evaluateInUDirection) const
{
    glm::vec3 derivative(0.0f);
    int degreeU = 2, degreeV = 2;
    float scaledU = u * (knotU[knotU.size() - degreeU - 1] - knotU.front()) + knotU.front();
    float scaledV = v * (knotV[knotV.size() - degreeV - 1] - knotV.front()) + knotV.front();

    //Går gjennom alle kontrollpunktene. 
    for (int i = 0; i < widthU; ++i) {
        for (int j = 0; j < widthV; ++j) {
            int index = j * widthU + i;
            if (index < controlPoints.size()) {
                float basisU = evaluateInUDirection ? BSplineBasisFunctions(i, degreeU - 1, scaledU, knotU) : BSplineBasisFunctions(i, degreeU, scaledU, knotU);
                float basisV = evaluateInUDirection ? BSplineBasisFunctions(j, degreeV, scaledV, knotV) : BSplineBasisFunctions(j, degreeV - 1, scaledV, knotV);
                derivative += basisU * basisV * controlPoints[index];
            }
        }
    }
    return derivative;
}

//Finner et punkt på B-spline overflaten ved å kombinere kontrollpunktene og basisfunksjonene 
//Skalerer parametrene i u og v retning og iterer gjennom alle kontrollpunktene. Basisfunksjonene i u og v retning blir beregnet. 
//Legger sammen basisfunksjoene i u og v retning og kontrollpunktene. 
glm::vec3 Surface::calculateSurfacePoint(float u, float v) const
{
    glm::vec3 point(0.0f);
    int degreeU = 2, degreeV = 2;
    float scaledU = std::min(u * (knotU[knotU.size() - degreeU - 1] - knotU.front()) + knotU.front(), knotU[knotU.size() - degreeU - 1] - 0.001f);
    float scaledV = std::min(v * (knotV[knotV.size() - degreeV - 1] - knotV.front()) + knotV.front(), knotV[knotV.size() - degreeV - 1] - 0.001f);

    for (int i = 0; i < widthU; ++i)
    {
        for (int j = 0; j < widthV; ++j)
        {
            int index = j * widthU + i;
            if (index < controlPoints.size())
            {
                float basisU = BSplineBasisFunctions(i, degreeU, scaledU, knotU);
                float basisV = BSplineBasisFunctions(j, degreeV, scaledV, knotV);
                point += basisU * basisV * controlPoints[index];
            }
        }
    }
    return point;
}

//Regner ut alle punktene på en B- spline overflate og lager en vektor med liste over eller 3D punktene. 
vector<glm::vec3> Surface::calculateSurfacePoints(int pointsOnTheSurface) const
{
    vector<glm::vec3> surfacePoints;
    for (int i = 0; i < pointsOnTheSurface; ++i)
    {
        for (int j = 0; j < pointsOnTheSurface; ++j)
        {
            float u = i / static_cast<float>(pointsOnTheSurface - 1);
            float v = j / static_cast<float>(pointsOnTheSurface - 1);
            surfacePoints.push_back(calculateSurfacePoint(u, v));
        }
    }
    return surfacePoints;
}

//Renger ut normalvektorene på en overflate på et rutenett av punkter. Normalene regnet ut ved hjelp av funsjonen
//calculatePartialDerivative. Normalene står vinkelrett på overflaten ved hvert punkt. 
vector<glm::vec3> Surface::calculateSurfaceNormals(int pointsOnTheSurface) const
{
    vector<glm::vec3> normals;
    //Epsilon brukes for å forskyve kantene av intervallet [0,1]. Verdiene 0 og 1 kan bli ustabil eller regnes feil fordi de kan evaluere utenfor sjøtvektorens 
    //domene. 
    float epsilon = 0.001f;

    for (int i = 0; i < pointsOnTheSurface; ++i)
    {
        for (int j = 0; j < pointsOnTheSurface; ++j)
        {

            float u = i / static_cast<float>(pointsOnTheSurface - 1);
            float v = j / static_cast<float>(pointsOnTheSurface - 1);

            if (u <= 0.0f) u += epsilon;
            else if (u >= 1.0f) u -= epsilon;

            if (v <= 0.0f) v += epsilon;
            else if (v >= 1.0f) v -= epsilon;

            glm::vec3 partialU = calculatePartialDerivative(u, v, true);
            glm::vec3 partialV = calculatePartialDerivative(u, v, false);

            glm::vec3 normal = glm::normalize(glm::cross(partialU, partialV));

            normals.push_back(normal);
        }
    }
    return normals;
}

//Bregner en lise av indekser som gjør at man kan tegne et rutenett av trekanter. 
vector<unsigned int> Surface::generateIndices(int pointsOnTheSurface) const
{
    vector<unsigned int> indices;
    for (int i = 0; i < pointsOnTheSurface - 1; ++i)
    {
        for (int j = 0; j < pointsOnTheSurface - 1; ++j)
        {
            int start = i * pointsOnTheSurface + j;
            indices.push_back(start);
            indices.push_back(start + 1);
            indices.push_back(start + pointsOnTheSurface);
            indices.push_back(start + 1);
            indices.push_back(start + pointsOnTheSurface);
            indices.push_back(start + pointsOnTheSurface + 1);
        }
    }
    return indices;
}

//Denne funksjonene beregner overflatepunkter, normaler og oppterrer VAO, VBO for overflaten og normalene. 
//den fyller og binder bufferne med data for punkter normaler og trekantindekser. 
void Surface::setupBuffers(unsigned int& surfaceVAO, unsigned int& surfaceVBO, unsigned int& normalVBO,
    unsigned int& EBO, unsigned int& normalVAO, unsigned int& normalLineVBO,
    int pointsOnTheSurface)
{
    vector<glm::vec3> surfacePoints = calculateSurfacePoints(pointsOnTheSurface);
    vector<glm::vec3> normals = calculateSurfaceNormals(pointsOnTheSurface);
    vector<unsigned int> indices = generateIndices(pointsOnTheSurface);

    glGenVertexArrays(1, &surfaceVAO);
    glGenBuffers(1, &surfaceVBO);
    glGenBuffers(1, &normalVBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(surfaceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, surfaceVBO);
    glBufferData(GL_ARRAY_BUFFER, surfacePoints.size() * sizeof(glm::vec3), &surfacePoints[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    vector<glm::vec3> normalLines;
    float normalLength = 0.05f;
    for (int i = 0; i < surfacePoints.size(); ++i) {
        glm::vec3 startPoint = surfacePoints[i];
        glm::vec3 endPoint = startPoint + normals[i] * normalLength;
        normalLines.push_back(startPoint);
        normalLines.push_back(endPoint);
    }

    glGenVertexArrays(1, &normalVAO);
    glGenBuffers(1, &normalLineVBO);

    glBindVertexArray(normalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, normalLineVBO);
    glBufferData(GL_ARRAY_BUFFER, normalLines.size() * sizeof(glm::vec3), &normalLines[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

