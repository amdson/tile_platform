#include "terrain.hpp"
#include <stdlib.h>


typedef glm::dvec2 vec; 

void random_grid(vec* grid, int height, int width) {
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            vec v = vec(rand() % 1024, rand() % 1024); 
            grid[r*width+c] = glm::normalize(v);  
        }
    } 
}

void add_grid(double* g1, double* g2, double alpha, double beta, double* gout, int height, int width) {
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            gout[r*width + c] = alpha*g1[r*width + c] + beta*g2[r*width + c]; 
        }
    }
}
void mul_grid(double alpha, double *grid, int height, int width) {
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            grid[r*width + c] = alpha*grid[r*width+c]; 
        }
    }
}
void expand_grid(double *grid, int height, int width) {

}

void perlin_noise(double* grid, int height, int width);

// void gen_chunk(World *w, int chunk_row, int chunk_col) {

// }

Tile query_tile(World *w, BlockIndices b) {
    ChunkIndices c = b2c(b); 
    if(w->chunks.count(c) > 0) {
        Chunk* chunk = w->chunks.at(c); 
        return chunk->tiles[b.row*CHUNK_TILES + b.col]; 
    }
    return {-1}; //Null ID
}
Chunk* query_chunk(World *w, ChunkIndices c) {
    if(w->chunks.count(c) > 0) {
        return w->chunks.at(c); 
    }
    return nullptr; 
}

bool set_tile(World *w, BlockIndices b, Tile t) {
    ChunkIndices c = b2c(b); 
    if(w->chunks.count(c) > 0) {
        Chunk* chunk = w->chunks.at(c); 
        chunk->tiles[b.row*CHUNK_TILES + b.col] = t; 
        return true;
    }
    return false; 
}

