#ifndef OCTREE_H
#define OCTREE_H

#include <vector>
#include <glm/glm.hpp>
#include <utility>

using namespace std;

class Octree
{
public:
  
    Octree(glm::vec3 minBounds, glm::vec3 maxBounds, int depth = 0, int maxDepth = 4, int maxObjects = 4);

    
    ~Octree();

    void insert(int ballIndex, const vector<glm::vec3>& ballPositions, float ballRadius);
    void getPotentialCollisions(vector<pair<int, int>>& collisionPairs, const vector<glm::vec3>& ballPositions, float ballRadius);

private:
    glm::vec3 minBounds; 
    glm::vec3 maxBounds;  
    int maxDepth;        
    int maxObjects;      
    vector<int> balls;  
    Octree* children[8];  
    
    bool isLeaf();        
    void subdivide(const vector<glm::vec3>& ballPositions, float ballRadius); 
    int getChildIndex(glm::vec3 position); 
   
};

#endif


