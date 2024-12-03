#include "Octree.h"
#include <iostream>
#include <glm/glm.hpp>

//Referanse https://github.com/CathrineSageng/Spillmotorarkitektur-Compulsory_1/tree/main

Octree::Octree(glm::vec3 minBounds, glm::vec3 maxBounds, int depth, int maxDepth, int maxObjects)
    : minBounds(minBounds), maxBounds(maxBounds), maxDepth(maxDepth), maxObjects(maxObjects)
{
    for (int i = 0; i < 8; ++i)
    {
        children[i] = nullptr; 
    }

}


Octree::~Octree()
{
    for (int i = 0; i < 8; ++i)
    {
        delete children[i];
    }
}


bool Octree::isLeaf()
{
    for (int i = 0; i < 8; ++i)
    {
        if (children[i] != nullptr) return false;
    }
    return true;
}

void Octree::insert(int ballIndex, const vector<glm::vec3>& ballPositions, float ballRadius)
{
    glm::vec3 pos = ballPositions[ballIndex];

    if (isLeaf() && (balls.size() < maxObjects or maxDepth == 0))
    {
        balls.push_back(ballIndex);
    }
    else
    {
        if (isLeaf() and maxDepth > 0)
        {
            subdivide(ballPositions, ballRadius); 
        }

        
        int childIndex = getChildIndex(pos);
        children[childIndex]->insert(ballIndex, ballPositions, ballRadius);  
    }
}

void Octree::subdivide(const vector<glm::vec3>& ballPositions, float ballRadius)
{
    glm::vec3 mid = (minBounds + maxBounds) * 0.5f;

    for (int i = 0; i < 8; ++i)
    {
        glm::vec3 newMin = minBounds;
        glm::vec3 newMax = maxBounds;

        if (i & 1) newMin.x = mid.x; else newMax.x = mid.x;
        if (i & 2) newMin.y = mid.y; else newMax.y = mid.y;
        if (i & 4) newMin.z = mid.z; else newMax.z = mid.z;

        children[i] = new Octree(newMin, newMax, maxDepth - 1, maxDepth, maxObjects);
    }

    for (int i = 0; i < balls.size(); ++i)
    {
        int childIndex = getChildIndex(ballPositions[balls[i]]);
        children[childIndex]->insert(balls[i], ballPositions, ballRadius);  
    }
    balls.clear();  
}


int Octree::getChildIndex(glm::vec3 position)
{
   
    glm::vec3 mid = (minBounds + maxBounds) * 0.5f;
    int index = 0;
    if (position.x > mid.x) index += 1;
    if (position.y > mid.y) index += 2;
    if (position.z > mid.z) index += 4;
    return index; 
}


void Octree::getPotentialCollisions(vector<pair<int, int>>& collisionPairs, const vector<glm::vec3>& ballPositions, float ballRadius)
{
   
    for (int i = 0; i < balls.size(); ++i)
    {
        for (int j = i + 1; j < balls.size(); ++j)
        {
            collisionPairs.push_back(std::make_pair(balls[i], balls[j]));
        }
    }

    if (!isLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i])
            {
                children[i]->getPotentialCollisions(collisionPairs, ballPositions, ballRadius);  
            }
        }
    }
}

