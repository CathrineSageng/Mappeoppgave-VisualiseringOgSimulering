#include "Surface.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

Surface::Surface(const std::vector<glm::vec3>& controlPoints, int widthU, int widthV,
    const std::vector<float>& knotU, const std::vector<float>& knotV)
    : controlPoints(controlPoints), widthU(widthU), widthV(widthV), knotU(knotU), knotV(knotV) {}

float Surface::BSplineBasis(int i, int k, float t, const std::vector<float>& knots) const {
    if (i < 0 || i >= knots.size() - 1) return 0.0f;
    if (k == 0) return (knots[i] <= t && t < knots[i + 1]) ? 1.0f : 0.0f;
    float denom1 = knots[i + k] - knots[i];
    float term1 = (denom1 != 0.0f) ? (t - knots[i]) / denom1 * BSplineBasis(i, k - 1, t, knots) : 0.0f;
    float denom2 = knots[i + k + 1] - knots[i + 1];
    float term2 = (denom2 != 0.0f) ? (knots[i + k + 1] - t) / denom2 * BSplineBasis(i + 1, k - 1, t, knots) : 0.0f;
    return term1 + term2;
}

glm::vec3 Surface::calculateSurfacePoint(float u, float v) const {
    glm::vec3 point(0.0f);
    int degreeU = 2, degreeV = 2;
    float scaledU = std::min(u * (knotU[knotU.size() - degreeU - 1] - knotU.front()) + knotU.front(), knotU[knotU.size() - degreeU - 1] - 0.001f);
    float scaledV = std::min(v * (knotV[knotV.size() - degreeV - 1] - knotV.front()) + knotV.front(), knotV[knotV.size() - degreeV - 1] - 0.001f);

    for (int i = 0; i < widthU; ++i) {
        for (int j = 0; j < widthV; ++j) {
            int index = j * widthU + i;
            if (index < controlPoints.size()) {
                float basisU = BSplineBasis(i, degreeU, scaledU, knotU);
                float basisV = BSplineBasis(j, degreeV, scaledV, knotV);
                point += basisU * basisV * controlPoints[index];
            }
        }
    }
    return point;
}

glm::vec3 Surface::calculatePartialDerivative(float u, float v, bool withRespectToU) const {
    glm::vec3 derivative(0.0f);
    int degreeU = 2, degreeV = 2;
    float scaledU = u * (knotU[knotU.size() - degreeU - 1] - knotU.front()) + knotU.front();
    float scaledV = v * (knotV[knotV.size() - degreeV - 1] - knotV.front()) + knotV.front();

    for (int i = 0; i < widthU; ++i) {
        for (int j = 0; j < widthV; ++j) {
            int index = j * widthU + i;
            if (index < controlPoints.size()) {
                float basisU = withRespectToU ? BSplineBasis(i, degreeU - 1, scaledU, knotU) : BSplineBasis(i, degreeU, scaledU, knotU);
                float basisV = withRespectToU ? BSplineBasis(j, degreeV, scaledV, knotV) : BSplineBasis(j, degreeV - 1, scaledV, knotV);
                derivative += basisU * basisV * controlPoints[index];
            }
        }
    }
    return derivative;
}

std::vector<glm::vec3> Surface::generateSurfacePoints(int pointsOnSurface)
{
    std::vector<glm::vec3> surfacePoints;
    for (int i = 0; i < pointsOnSurface; ++i) {
        for (int j = 0; j < pointsOnSurface; ++j) {
            float u = i / static_cast<float>(pointsOnSurface - 1);
            float v = j / static_cast<float>(pointsOnSurface - 1);
            surfacePoints.push_back(calculateSurfacePoint(u, v));
        }
    }
    return surfacePoints;
}

std::vector<glm::vec3> Surface::calculateSurfaceNormals(int pointsOnTheSurface) const {
    std::vector<glm::vec3> normals;
    float epsilon = 0.001f;

    for (int i = 0; i < pointsOnTheSurface; ++i) {
        for (int j = 0; j < pointsOnTheSurface; ++j) {
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
