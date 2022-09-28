#include <glm/glm.hpp>
#include <vector>
#include "chunk.hpp"


void add_grid(double* g1, double* g2, double alpha, double beta, double* gout, int height, int width); 
void mul_grid(double alpha, double *grid, int height); 
void perlin_noise(double* grid, int height, int width);

struct World {
	std::unordered_map<ChunkIndices, Chunk*> chunks;
	std::vector<int> world_operations;  
}; 

void gen_chunk(World *w, int chunk_row, int chunk_col); 
void load_chunk(World *w, ChunkIndices c); 
void unload_chunk(World *w, ChunkIndices c); 
Tile query_tile(World *w, BlockIndices b); //May fail and return empty tile
Chunk* query_chunk(World *w, int chunk_row, int chunk_col); //May fail and return null
bool set_tile(World *w, BlockIndices b, Tile t); 


/*

*/



