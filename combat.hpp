#include <glm/glm.hpp>
#include <vector>

struct Hurtbox {
    int id;
    int parent_id;
    glm::dvec2 pos, dim;
    double weight, power;
};

struct Hitbox {
    int id;
    int parent_id;
    glm::dvec2 pos, dim;
};

struct Hit {
    Hurtbox hurtbox;
    Hitbox hitbox; 
}; 

void addHits(std::vector<Hurtbox> *, std::vector<Hitbox> *, std::vector<Hit> *); 