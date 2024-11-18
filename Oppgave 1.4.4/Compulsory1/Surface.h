#ifndef SURFACE_H
#define SURFACE_H

#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>

using namespace std;

class Surface
{
public:
    //Constructoren tar parameter for kontrollpunktene på overflaten, antall kontrollpunkter det er horisontal og vertikal retning, 
    //og skjøtevektorer for u (horisontal) og v (vertikal) retning. 
    Surface(const vector<glm::vec3>& controlPoints, int widthU, int widthV,
        const vector<float>& knotU, const vector<float>& knotV);

    // Regner ut et punkt på overflaten. 
    glm::vec3 calculateSurfacePoint(float u, float v) const;

    //Regner ut alle punktene på overflaten 
    vector<glm::vec3> calculateSurfacePoints(int pointsOnTheSurface) const;

    //Regner ut normalene til hvert punkt på overflaten. Dette brukes til belysning (phong shaderen). 
    vector<glm::vec3> calculateSurfaceNormals(int pointsOnTheSurface) const;

    //Lager en trekantmesh
    vector<unsigned int> generateIndices(int pointsOnTheSurface) const;

    //Oppretter buffere for overflatepunkter og normalene
    void setupBuffers(unsigned int& surfaceVAO, unsigned int& surfaceVBO, unsigned int& normalVBO, unsigned int& EBO,
        unsigned int& normalVAO, unsigned int& normalLineVBO, int pointsOnTheSurface);

    glm::vec3 calculatePartialDerivative(float u, float v, bool evaluateInUDirection) const;


private:
    //Regner ut B-spline basisfunksjonene
    float BSplineBasisFunctions(int i, int d, float t, const vector<float>& knots) const;

    //Kontrollpunktene på overflaten 
    vector<glm::vec3> controlPoints;
    //Bredde og høyde på kontrollpunktene
    int widthU, widthV;
    //Skjøtevektorene u og v 
    vector<float> knotU, knotV;
};

#endif

