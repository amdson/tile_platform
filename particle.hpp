#include <glm/glm.hpp>
#include "renderer.hpp"

struct Particle {
	glm::dvec2 pos;
	glm::dvec2 vel; 
	glm::dvec2 dim; 
	Sprite s; 
	int timestep; //Timestep starting from creation. 
	int change_interval; 
	int lifetime; 
	bool gravity; 
};
