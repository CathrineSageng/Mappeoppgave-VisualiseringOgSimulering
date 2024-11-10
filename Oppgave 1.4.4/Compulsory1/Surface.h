#ifndef SURFACE_H
#define SURFACE_H

#include <glm/glm.hpp>
#include <vector>

class Surface {
public:
    Surface(const std::vector<glm::vec3>& controlPoints, int widthU, int widthV,
        const std::vector<float>& knotU, const std::vector<float>& knotV);

    glm::vec3 calculateSurfacePoint(float u, float v) const;
    glm::vec3 calculatePartialDerivative(float u, float v, bool withRespectToU) const;
    std::vector<glm::vec3> generateSurfacePoints(int pointsOnSurface);
    std::vector<glm::vec3> calculateSurfaceNormals(int pointsOnTheSurface) const;

private:
    float BSplineBasis(int i, int k, float t, const std::vector<float>& knots) const;

    std::vector<glm::vec3> controlPoints;
    int widthU, widthV;
    std::vector<float> knotU, knotV;
};

#endif
