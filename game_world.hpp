#ifndef HEADERFILE_GAME_WORLD
#define HEADERFILE_GAME_WORLD

#include <SDL.h>
#include <glm/glm.hpp>
#include <iostream>
#include <math.h>  
#include <vector>

const int TILE_PIXELS = 128; 
const double TILE_WIDTH = 1; 
const int CHUNK_TILES = 32; 
const double CHUNK_WIDTH = TILE_WIDTH * CHUNK_TILES; 
const double COLLISION_BUFFER = 1e-2; //Collisions happen 1e-2 from surface. 
const double CONTACT_BUFFER = 2e-2; //Contacts are maintained when within 2e-2 of surface. 

const double AIR_ACC = 0.01; 
const double GROUND_ACC = 0.04; 
const double GROUND_JUMP_VEL = 0.15; 
const double AIR_JUMP_VEL = 0.2; 

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

bool operator==(const BlockIndices b1, const BlockIndices b2); 

struct Entity {
	glm::dvec2 pos; 
	glm::dvec2 vel; 
	glm::dvec2 newPos; 
	glm::dvec2 dim; 
	uint32_t id; //For reference in hashmaps, etc. 
}; 

struct InputState {
	int id; //To match input with corresponding player / entity. 
	int x;
	int y;
	SDL_Point mouse_pos; 
	bool mouse_down; 
	bool right_mouse_down; 
}; 

enum MovementState 
{   GROUND, GROUND_JUMP, JUMP1, AIR_JUMP, FALLING, WALL, WALL_JUMP
};

enum ContactSide {
	LEFT=0, RIGHT, TOP, BOTTOM
}; 

struct Collision {
    glm::dvec2 pos, norm;
	ContactSide s; 
    double t; 
}; 

//Tracks tile an entity is in contact with. 
struct TileContact {
	glm::dvec2 pos, norm;
	BlockIndices b; 
	ContactSide s; 
	bool valid; 
}; 

struct Camera { //Used to convert units to pixels. 
	glm::dvec2 pos; 
	glm::dvec2 dim; 
	int SCREEN_WIDTH; 
	int SCREEN_HEIGHT; 
}; 

struct PlayerController {
	int entity_id; //ID of player physics object. 
	MovementState prev_state; 
	MovementState state; 
	InputState inp; 
	InputState prev_inp; 
	bool contact_sides[4]; 
	int timestep; //Timestep starting from last state change. 
	void updateInputs(InputState new_inp); 
	void applyContacts(glm::dvec2 v, std::vector<TileContact> *t); 
	glm::dvec2 applyControls(glm::dvec2 v, std::vector<TileContact> *t); 
}; 

struct PlayerData {
	int entity_id; 
	int max_health; 
	int health; 
	int max_stamina; 
	int stamina; 
}; 


//Converts a point in units to a point in pixels. 
SDL_Point toPixelPoint(glm::dvec2 p); 
glm::dvec2 toPoint(SDL_Point p, Camera c); 

//Converts a rectangle in units to a pixel rectangle in the camera. 
SDL_Rect toRect(glm::dvec2 p, glm::dvec2 d, Camera c);

//Adds a list of all squares intersecting a line segment to a vector. 
void listIntersectingSquares(glm::dvec2 s, glm::dvec2 e, std::vector<BlockIndices> *l); 
//Adds a list of the squares neighboring s. (Nine total with square containing s included.). 
void listNeighborSquares(glm::dvec2 s, std::vector<BlockIndices> *l); 
void listTileNeighborSquares(BlockIndices t, std::vector<BlockIndices> *l); 

//Returns a collusion object for the collusion between a tile and a moving box. 
Collision getTileBoxCollision(BlockIndices b, glm::dvec2 p1, glm::dvec2 p2, glm::dvec2 d); 
bool checkTileContact(glm::dvec2 p, glm::dvec2 d, TileContact t); 
bool checkTileContactMaintained(glm::dvec2 p, glm::dvec2 d, TileContact t, BlockIndices b); 
void invalidateContacts(std::vector<TileContact> *t, ContactSide c); 


glm::dvec2 getConstrainedSurfaceVel(glm::dvec2 v, glm::dvec2 norm); 

#endif