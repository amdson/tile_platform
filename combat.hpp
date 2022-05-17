#ifndef HEADERFILE_COMBAT
#define HEADERFILE_COMBAT

#include <glm/glm.hpp>
#include <vector>

struct Hurtbox {
    int id;
    int parent_id;
    glm::dvec2 pos, dim, vel;
    double weight, power;
};

struct Hitbox {
    int id;
    int parent_id;
    glm::dvec2 pos, dim;
    int hitflags; 
};

struct Hit {
    Hurtbox hurtbox;
    Hitbox hitbox; 
}; 

void addHits(std::vector<Hurtbox> *, std::vector<Hitbox> *, std::vector<Hit> *); 
#endif