// #include "../physics.hpp"
#include <stdio.h>
#include <glm/glm.hpp>
#include "../physics.hpp"
#include <SDL.h>

int main( int argc, char* args[] ) {
    //Test point segment collisions. 
    glm::dvec2 p0 = glm::dvec2(0, -1); 
    glm::dvec2 p1 = glm::dvec2(0, 1); 
    glm::dvec2 s0 = glm::dvec2(-1, 0); 
    glm::dvec2 s1 = glm::dvec2(1, 0); 

    Collision c; 
    bool result = point_segment_update(p0, p1, s0, s1, &c); 
    printf("%d, pos: %f, %f, time: %f\n", result, c.pos.x, c.pos.y, c.t); 

    c.t = 1.0; 
    p0 = glm::dvec2(0, 0); 
    p1 = glm::dvec2(2, 2); 
    s0 = glm::dvec2(3.99, 0); 
    s1 = glm::dvec2(0, 3.99); 

    result = point_segment_update(p0, p1, s0, s1, &c); 
    printf("%d, pos: %f, %f, time: %f\n", result, c.pos.x, c.pos.y, c.t); 

    c.t = 2.0;
    s0 = glm::dvec2(4.01, 0); 
    s1 = glm::dvec2(0, 4.01);
    result = point_segment_update(p0, p1, s0, s1, &c); 
    printf("%d, pos: %f, %f, time: %f\n", result, c.pos.x, c.pos.y, c.t); 

    p0 = glm::dvec2(0, -1); 
    p1 = glm::dvec2(0, 1); 
    s0 = glm::dvec2(-1, 0); 
    s1 = glm::dvec2(-0.5, 1); 

    Collision c1;  
    result = point_segment_update(p0, p1, s0, s1, &c1); 
    printf("%d, pos: %f, %f, time: %f\n", result, c1.pos.x, c1.pos.y, c1.t); 
}


