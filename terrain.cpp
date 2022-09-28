#include "terrain.hpp"
#include <stdlib.h>


typedef glm::dvec2 vec; 

void random_grid_vec(vec* grid, int height, int width) {
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            vec v = vec(rand() % 1024, rand() % 1024); 
            grid[r*width+c] = glm::normalize(v);  
        }
    } 
}

void perlin_noise(double* grid, int height, int width, int cell_height, int cell_width) {
    int ph = (height / cell_height) + 1; int pw = (width / cell_width) + 1; 
    vec* ggrid = (vec*) calloc(ph*pw, sizeof(vec));
    random_grid_vec(ggrid, ph, pw);
    for (int r = 0; r < ph-1; r++) {
        for (int c = 0; c < pw-1; c++) {
            for (int cr = 0; cr < cell_height; cr++) {
                for (int cc = 0; cc < cell_width; cc++) {
                    vec p = vec(cr, cc) + 0.5; //Grid cells are always (1x1)
                    double fx = p.x/ ((double) cell_width);
                    double fy = p.y / ((double) cell_height); 
                    double i0 = (1-fx)*(1-fy); double i1 = (1-fx)*fy;
                    double i2 = fx*fy; double i3 = fx*(1-fy);

                    double a0 = glm::dot(ggrid[r*pw+c], p); 
                    double a1 = glm::dot(ggrid[r*pw+c+1], p); 
                    double a2 = glm::dot(ggrid[(r+1)*pw+c+1], p); 
                    double a3 = glm::dot(ggrid[(r+1)*pw+c], p); 

                    grid[(c*cell_width + cc) + (r*cell_height + cr)*width] += a0*i0+a1*i1+a2*i2+a3*i3; 
                }
            }
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
// void expand_grid(double *grid, int height, int width) {

// }


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

