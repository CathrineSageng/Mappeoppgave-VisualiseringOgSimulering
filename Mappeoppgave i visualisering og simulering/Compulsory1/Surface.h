#ifndef SURFACE_H
#define SURFACE_H

#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include "Shader.h"

using namespace std;

class Surface
{
public:
    //Constructoren tar parameter for kontrollpunktene p� overflaten, antall kontrollpunkter det er horisontal og vertikal retning, 
    //og skj�tevektorer for u (horisontal) og v (vertikal) retning. 
    Surface(const vector<glm::vec3>& controlPoints, int widthU, int widthV,
        const vector<float>& knotU, const vector<float>& knotV);

    // Regner ut et punkt p� overflaten. 
    glm::vec3 calculateSurfacePoint(float u, float v) const;

    //Regner ut alle punktene p� overflaten 
    vector<glm::vec3> calculateSurfacePoints(int pointsOnTheSurface) const;

    //Regner ut normalene til hvert punkt p� overflaten. Dette brukes til belysning (phong shaderen). 
    vector<glm::vec3> calculateSurfaceNormals(int pointsOnTheSurface) const;

    //Lager en trekantmesh
    vector<unsigned int> generateIndices(int pointsOnTheSurface) const;

    //Oppretter buffere for overflatepunkter og normalene
    void setupBuffers(unsigned int& surfaceVAO, unsigned int& surfaceVBO, unsigned int& colorVBO,
        unsigned int& normalVBO, unsigned int& EBO, unsigned int& normalVAO, unsigned int& normalLineVBO,
        int pointsOnTheSurface, float frictionAreaXMin, float frictionAreaXMax,
        float frictionAreaYMin, float frictionAreaYMax);

    glm::vec3 calculatePartialDerivative(float u, float v, bool evaluateInUDirection) const;

    //For sporingen av ballene 
    std::vector<glm::vec3> calculateBSplineCurve(const vector<glm::vec3>& controlPoints, int degree, int resolution) const;
    void renderBSplineCurve(const vector<glm::vec3>& curvePoints, Shader& shader, glm::mat4& projection, glm::mat4& view) const;

private:
    //Regner ut B-spline basisfunksjonene
    float BSplineBasisFunctions(int i, int d, float t, const vector<float>& knots) const;

    //Kontrollpunktene p� overflaten 
    vector<glm::vec3> controlPoints;
    //Bredde og h�yde p� kontrollpunktene
    int widthU, widthV;
    //Skj�tevektorene u og v 
    vector<float> knotU, knotV;
};

#endif

