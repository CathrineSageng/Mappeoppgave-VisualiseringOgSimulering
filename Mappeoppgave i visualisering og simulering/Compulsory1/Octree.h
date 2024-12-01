#ifndef OCTREE_H
#define OCTREE_H

#include <vector>
#include <glm/glm.hpp>
#include <utility>

using namespace std;

class Octree
{
public:
    // Constructor for the octree
    //Depth = 0 is the current depth of the octree. The root node is at depth 0
    //The node will no longer subdivide when the depth is resched 4
    //The maximum number of objects is 4 in a node, before it subdivides into 8 smaller child nodes. 
    Octree(glm::vec3 minBounds, glm::vec3 maxBounds, int depth = 0, int maxDepth = 4, int maxObjects = 4);

    // The destructor is called when the Octree object is destroyed. It cleans up dynamically allocated memory. 
    ~Octree();

    // This function inserts a ball into the octree. The ballIndex is the index of the ball in the ballPosition vector. 
    //The octree uses the balls position and the nodes boundaries to determine where to insert the ball in the octree.
    // 
    // const vector<glm::vec3>& ballPositions is passing a reference to the ballPositions vector, which contains a list of 3D vectors 
    //respresenting the ball position. The const makes sure that the function can not modify the actual vector, but can read and access it. 
    // The recerence are the actual vector, not a copy, this improves performance 
    void insert(int ballIndex, const vector<glm::vec3>& ballPositions, float ballRadius);

    // This function finds and returns potensial collision pairs by checking every ball that are in the same node or in the neighboring nodes. 
    //collisionPairs is a vector that contains the potensial balls that can collide
    //This function helps for unnecessary checks. 
    void getPotentialCollisions(vector<pair<int, int>>& collisionPairs, const vector<glm::vec3>& ballPositions, float ballRadius);

private:
    glm::vec3 minBounds;  //The current 3d space the current octree node covers. This is the lowest corner of the space
    glm::vec3 maxBounds;  // The highest corner of the space. 
    int maxDepth;         // Once the node reaches the maximum depth, the octree will not subdivide anymore, even if more objects are inserted
    int maxObjects;       // The maximum number of balls before it subdivides into 8 smaller child nodes. 
    vector<int> balls;  // Stores the indices of the balls that are within the current node. 
    Octree* children[8];  // Represents the 8 child nodes that the current node can be subdivided into. Each shild represents a smaller
    //region of space within the parent node 

    bool isLeaf();        // Checks if this is a leaf node. If a node is a leaf, it's not subdiveded yet. 
    void subdivide(const vector<glm::vec3>& ballPositions, float ballRadius); //Subdivides the node into 8 smaller child nodes
    // when there are too many balls in the node. After the subdivision, the balls are redistributed among the child nodes. 
    int getChildIndex(glm::vec3 position); //This function determines which of the 8 child nodes a ball belongs to based on its position. 
    //It returns the index of the child node that the ball should be inserted to. 
};

#endif

// An octree divides 3D space into smaller regions, here octants. 
//Without an octree, you need to check every pair of balls for collision, leading to O(n^2). (16*15)/2 = 120 pairs. 
//The octree divides the box into smaller regions. When a ball is moving, it is inserted into a octant corresponding to its position. 
//With this approach you check for collision in the same octant or neighboring octants. 
//The root in this project is the box, and the octree would recursively subdivide the box into smaller regions if there are to many 
//balls in a region. 
//The octree redices the number if checks to O(nlogn) or better, depending of the distribution of the balls on the box. 


