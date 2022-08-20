#ifndef HEADERFILE_PHYSICS
#define HEADERFILE_PHYSICS

#include <glm/glm.hpp>
#include <vector>

const int CHUNK_TILES = 32; 

const double COLLISION_BUFFER = 1e-2; //Collisions happen 1e-2 from surface. 
const double CONTACT_BUFFER = 2e-2; //Contacts are maintained when within 2e-2 of surface. 

//Special case for rapid collision testing. 
struct PhysicsRect {
    glm::dvec2 pos;
    glm::dvec2 dim; 
};

struct Tile {
   char tile_id;
   char damage; 
};

struct Chunk {
	int row;
	int col; 
	Tile tiles[CHUNK_TILES*CHUNK_TILES];
}; 

struct BlockIndices {
	int row, col; 
	int chunk_row, chunk_col; 
}; 

struct Collision {
    glm::dvec2 pos, norm;
    double t = 1.012345; 
}; 

//Tracks tile an entity is in contact with. 
struct TileContact {
	glm::dvec2 pos, norm;
	BlockIndices b; 
	bool valid; 
}; 

/*
Polygon assumed convex. Maximum of 6 vertices, larger polygons need to be broken down. 
Physics assumes contacts are always with blocks. 
*/
const uint32_t COLLISION_DESTROY_BLOCKS = 1; 
const uint32_t COLLISION_BOUNCE = 1 << 1; 
const uint32_t COLLISION_IS_PLAYER = 1 << 2; 
const uint32_t COLLISION_FRICTION = 1 << 3; 

struct PhysicsPolygon {
    glm::dvec2 pos; 
    glm::dvec2 vertices[6]; 
    int num_vertices;
    TileContact contacts[16]; 
    int num_contacts = 0; 
    double mass = 1.0; 
    double elasticity = 1.0; 
    double friction_coef = 0.1; 
    uint32_t physics_flags; 
};

struct Rect {
    glm::dvec2 pos;
    glm::dvec2 dim; 
}; 

bool point_segment_update(glm::dvec2 p0, glm::dvec2 p1, glm::dvec2 s0, glm::dvec2 s1, Collision *c); 
bool polygon_rectangle_update(PhysicsPolygon poly, glm::dvec2 p0, glm::dvec2 p1, Rect r, Collision *c); 

glm::dvec2 getConstrainedSurfaceVel(glm::dvec2 v, glm::dvec2 norm); 

bool checkTileContact(glm::dvec2 p, glm::dvec2 d, TileContact t); 
bool checkTileContactMaintained(glm::dvec2 p, glm::dvec2 d, TileContact t, BlockIndices b); 
void invalidateContacts(std::vector<TileContact> *t, glm::dvec2 contact_norm); 
void filterTileContacts(PhysicsPolygon *poly, std::vector<BlockIndices> *block_indices, Chunk *main_chunk);
void tilePhysics(PhysicsPolygon *poly, std::vector<BlockIndices> *block_indices, Chunk *main_chunk);

/*
Physics Loop
1. Apply forces to vel. 
2. Remove invalid contacts by checking whether poly within CONTACT_BUFFER for all active TileContacts
FOR EACH CONTACT REMOVED
    3. Check for intersection within CONTACT_BUFFER distance with surrounding blocks. If 
    present, calculate normal. If normal within maybe 25 degrees of invalidated contact, replace 
    invalidated contact with new tile, normal. 
4. Subtract contact normal components from vel, apply friction... 
5. Calculate nextPos. 
6. Calculate earliest collision on path from pos to nextPos. 
IF COLLISION
    7. Set pos to collision location. 
    8. Add collision_norm * COLLISION_BUFFER to pos
    9. Update velocities based on collision. 
*/

#endif