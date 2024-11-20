#include "Octree.h"
#include <iostream>
#include <glm/glm.hpp>

// The constuctor initilizes an Octree nose 
Octree::Octree(glm::vec3 minBounds, glm::vec3 maxBounds, int depth, int maxDepth, int maxObjects)
    : minBounds(minBounds), maxBounds(maxBounds), maxDepth(maxDepth), maxObjects(maxObjects)// initilizer list 
{
    for (int i = 0; i < 8; ++i)
    {
        children[i] = nullptr; //Is set to an nullptr because the node doesent have any child nodes yet 
    }

}

// Destructor to clean the memoty of the octree. It goes through the children array and deletes each child node. 
//This prevents memoty leaks when the program ends. 
Octree::~Octree()
{
    for (int i = 0; i < 8; ++i)
    {
        delete children[i];
    }
}

// Check if the node is a leaf. I all the children are nullptr, it returns true. If any child is not a nullptr it returns false. 
//Meaning this node has been subdivided into children. 
bool Octree::isLeaf()
{
    for (int i = 0; i < 8; ++i)
    {
        if (children[i] != nullptr) return false;
    }
    return true;
}

// Insert a ball into the octree
void Octree::insert(int ballIndex, const vector<glm::vec3>& ballPositions, float ballRadius)
{
    glm::vec3 pos = ballPositions[ballIndex];

    // If this node is a leaf and has space or we have rached the maximum depth the ball is added to the node. 
    if (isLeaf() && (balls.size() < maxObjects or maxDepth == 0))
    {
        balls.push_back(ballIndex);
    }
    else
    {
        // If this node is full and it's not at max depth, subdivide if necessary
        if (isLeaf() and maxDepth > 0)
        {
            subdivide(ballPositions, ballRadius);  // Subdivides into 8 smaller nodes
        }

        // Find the correct child for this ball and insert into the child
        int childIndex = getChildIndex(pos);
        children[childIndex]->insert(ballIndex, ballPositions, ballRadius);  // Pass ballPositions and ballRadius as parameters
    }
}

// Subdivide the current node into 8 children
void Octree::subdivide(const vector<glm::vec3>& ballPositions, float ballRadius)
{
    glm::vec3 mid = (minBounds + maxBounds) * 0.5f;

    for (int i = 0; i < 8; ++i)
    {
        glm::vec3 newMin = minBounds;
        glm::vec3 newMax = maxBounds;

        // It calculates the middle of the current node and creates 8 smaller nodes. 
        if (i & 1) newMin.x = mid.x; else newMax.x = mid.x;
        if (i & 2) newMin.y = mid.y; else newMax.y = mid.y;
        if (i & 4) newMin.z = mid.z; else newMax.z = mid.z;

        children[i] = new Octree(newMin, newMax, maxDepth - 1, maxDepth, maxObjects);
    }

    // Goes trough all the balls in currently in this node , figures out which child node they belong to, and inserts them into the 
    //appropriate child node. 
    for (int i = 0; i < balls.size(); ++i)
    {
        int childIndex = getChildIndex(ballPositions[balls[i]]);
        children[childIndex]->insert(balls[i], ballPositions, ballRadius);  // Ensure ballPositions and ballRadius are passed here
    }
    balls.clear();  // When all the balls are redistributed, the current node clears the list of balls since they now belong to the child nodes.
}

// Calculates which of the 8 child nodes a ball should go into, based on its position
int Octree::getChildIndex(glm::vec3 position)
{
    //Calculates the midpoint of the current node. 
    glm::vec3 mid = (minBounds + maxBounds) * 0.5f;
    int index = 0;
    if (position.x > mid.x) index += 1;
    if (position.y > mid.y) index += 2;
    if (position.z > mid.z) index += 4;
    return index; //Tells which node to insert the ball into 
}

// Finds potensial collision pairs of the balls in the octree. For each pair of balls in the current node, it adds indices as a pair to
// collisionPairs (a list if balls that might collide) If the current node is not a leaf, it recursively checks each child node and 
// collects more potensial collisions from them. 
void Octree::getPotentialCollisions(vector<pair<int, int>>& collisionPairs, const vector<glm::vec3>& ballPositions, float ballRadius)
{
    // Check for collisions between balls in the same node
    for (int i = 0; i < balls.size(); ++i)
    {
        for (int j = i + 1; j < balls.size(); ++j)
        {
            collisionPairs.push_back(std::make_pair(balls[i], balls[j]));
        }
    }

    // If this is not a leaf, check the child nodes
    if (!isLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i])
            {
                children[i]->getPotentialCollisions(collisionPairs, ballPositions, ballRadius);  // Pass ballPositions and ballRadius
            }
        }
    }
}

