#include "combat.hpp"


bool checkIntersecting(Hurtbox a, Hitbox h) {
    glm::dvec2 d = a.dim + h.dim;
    glm::dvec2 p = a.pos - h.dim; 
    return h.pos.x >= p.x && h.pos.x <= p.x + d.x && h.pos.y >= p.y && h.pos.y <= p.y + d.y; 
}

//Identify all intersecting hurt and hit boxes and add to hits. Build a  
void addHits(std::vector<Hurtbox> *hurtboxes, std::vector<Hitbox> *hitboxes, std::vector<Hit> *hits) {
    for (auto a = hurtboxes->begin(); a != hurtboxes->end(); a++) {
        for (auto h = hitboxes->begin(); h != hitboxes->end(); h++) {
            if(checkIntersecting(*a, *h)) {
                hits->push_back(Hit {*a, *h}); 
            }
        }
    }
}

