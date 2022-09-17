#ifndef HEADERFILE_CHUNK
#define HEADERFILE_CHUNK

#include <glm/glm.hpp>
#include <vector>

const double TILE_WIDTH = 1; 
const int CHUNK_TILES = 32; 
const double CHUNK_WIDTH = TILE_WIDTH * CHUNK_TILES; 

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

//Adds a list of all squares intersecting a line segment to a vector. 
void listIntersectingSquares(glm::dvec2 s, glm::dvec2 e, std::vector<BlockIndices> *l); 
//Adds a list of the squares neighboring s. (Nine total with square containing s included.). 
void listNeighborSquares(glm::dvec2 s, std::vector<BlockIndices> *l); 
void listTileNeighborSquares(BlockIndices t, std::vector<BlockIndices> *l); 
#endif
