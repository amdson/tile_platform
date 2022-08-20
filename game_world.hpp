#ifndef HEADERFILE_GAME_WORLD
#define HEADERFILE_GAME_WORLD

#include <SDL.h>
#include <glm/glm.hpp>
#include <iostream>
#include <math.h>  
#include <vector>
#include "inputs.hpp"
#include "particle.hpp"
#include "renderer.hpp"
#include "combat.hpp"


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

struct Entity {
	uint32_t entity_id; //For reference in hashmaps, etc. 
	glm::dvec2 pos; 
	glm::dvec2 vel; 
	glm::dvec2 newPos; 
	glm::dvec2 dim; 
	double mass; 
	int num_contacts; 
	TileContact tile_contacts[16]; //Track tiles entity is in contact with. 
	int operator_index; 
	uint32_t flags; 
	bool deleted; 
}; 

static const uint32_t COLLIDE_TILES = 1; 
static const uint32_t BOUNCE_ON_HIT = 1 << 1; 
static const uint32_t ZERO_GRAVITY = 1 << 2; 
static const uint32_t IN_COLLISION = 1 << 3; 
static const uint32_t ATTACKER_HIT = 1 << 4; 
static const uint32_t TARGET_HIT = 1 << 5; 


enum MovementState 
{   GROUND, GROUND_JUMP, JUMP1, AIR_JUMP, FALLING, WALL, WALL_JUMP
};

struct Camera { //Used to convert units to pixels. 
	glm::dvec2 pos; 
	glm::dvec2 dim; 
	int SCREEN_WIDTH; 
	int SCREEN_HEIGHT; 
	int player_id; 
}; 

struct HealthData {
	int entity_id; //Index of entity in gamestate.entities
	int health; 
	int max_health;
	int health_regen;
	int buffer_health; 
	int buffer_regen; 
	int max_stamina; 
	int stamina; 
	int stamina_regen; 
	uint32_t flags;  
}; 

static const uint32_t FIREBALL = 1; 
static const uint32_t FIREFLY = 2; 
static const uint32_t ROLLYPOLLY = 3; 
static const uint32_t LEACH = 4; 
static const uint32_t HUNTER = 5; 
static const uint32_t CANNON = 6; 
static const uint32_t NEEDLEWORKER = 7; 
static const uint32_t SANDBOXER = 8; 


struct FireballAI {
	bool tracking; 
	int power; 
	int step; 
	int lifespan; 
}; 

struct FireflyAI {
	bool tracking; 
	int power; 
}; 

struct RollypollyAI {
	bool tracking; 
	int power; 
}; 

union InternalAI {
	FireballAI fa; 
	FireflyAI fb; 
	RollypollyAI fc; 
}; 

struct AIData {
	int entity_id; //Index of entity in gamestate.entities
	uint32_t type; 
	uint32_t flags; 
	InternalAI data; 
};

struct PlayerData {
	int entity_id; //Index of entity in gamestate.entities
	MovementState prev_state; 
	MovementState state; 
	InputState inp; 
	InputState prev_inp; 
	bool contact_sides[4]; 
	int timestep; //Timestep starting from last state change. 
	int fire_cooldown; 
};
PlayerData init_player_data(); 


/*
ECS Write-Up
ECS system is optimized for saving state every iteration. 

Startup: 
1. Initialize components
2. Add component names to hashmap
Runtime: 
1. Iterate through vector of main component normally, reference entity id for other components. 
2. Push component naively to add to entity. 
3. Delete components 
4. Store remaining components
5. Copy components for next iteration. 
Rules for ECS
*/

const int MAX_ENTITIES = 2048; 

struct RollbackStorage {
	std::vector<int> ids; 
	std::vector<Entity> entities; 
	std::vector<PlayerData> player_data; 
	std::vector<HealthData> health_data; 
	std::vector<AIData> ai_data;  
}; 

struct RollbackECS {
	//Sparse vectors mapping entity ids to location in dense vector.
	std::vector<int> id_map;  
	std::vector<int> entity_map; 
	std::vector<int> player_map; 
	std::vector<int> health_map; 
	std::vector<int> ai_map; 

	std::vector<int> ids; 
	std::vector<Entity> entities; 
	std::vector<PlayerData> player_data; 
	std::vector<HealthData> health_data; 
	std::vector<AIData> ai_data; 

	std::vector<int> free_ids; //Ids freed by deletion. 

	std::vector<RollbackStorage> r; 
	int r_pos; 

	int new_entity(); 
	bool delete_entity(int e); 
	void save_update(RollbackStorage *s); 
	void roll_save(); 

	int push_player(); 
	int push_fireball(glm::dvec2 p, glm::dvec2 v); 
	int push_firefly(glm::dvec2 p); 
	int push_npc(); 
	RollbackECS(int rollback_window); 
}; 

struct Gamestate {
	RollbackECS *ecs; 
	std::vector<Hitbox> hitboxes;
	std::vector<Hurtbox> hurtboxes; 
	std::vector<Hit> hits; 

	Chunk main_chunk;

	std::vector<Particle> *particles; 
	SpriteSheet *sprite_sheet; 
	Gamestate(SpriteSheet *sheet, std::vector<Particle> *p);
}; 

void updateInputs(InputState new_inp, PlayerData *p); 
void player_physics_update(Entity *e, PlayerData *p, Gamestate *g); 

bool box_intersect(glm::dvec2 p1, glm::dvec2 d1, glm::dvec2 p2, glm::dvec2 d2); 

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
void filterTileContacts(Entity *e, std::vector<BlockIndices> *block_indices, Chunk *main_chunk);
void tilePhysics(Entity *e, std::vector<BlockIndices> *block_indices, Chunk *main_chunk);

glm::dvec2 getConstrainedSurfaceVel(glm::dvec2 v, glm::dvec2 norm); 

#endif