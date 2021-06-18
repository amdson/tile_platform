#include <stdio.h>
#include <math.h>  
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include "game_world.hpp"

int main( int argc, char* args[] ) {
    glm::dvec2 p1 = glm::dvec2(1.1, 1.1); 
    glm::dvec2 p2 = glm::dvec2(5.1, 1.1); 
    glm::dvec2 dim = glm::dvec2(0.5, 0.5); 

    BlockIndices b = {1, 1};
    Collision c = getTileBoxCollision(b, p1, p2, dim);
    std::cout << "c pos " << c.pos.x << " " << c.pos.y << "\n"; 
	std::cout << "c norm " << c.norm.x << " " << c.norm.y << "\n"; 
}