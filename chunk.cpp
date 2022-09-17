#include "chunk.hpp"
#include <math.h>  
#include <stdio.h>

bool operator==(const BlockIndices b1, const BlockIndices b2){
	return b1.col == b2.col && b1.row == b2.row && b1.chunk_col == b2.chunk_col && b1.chunk_row == b2.chunk_row; 
}

//Squares are represented by row and column. 
void listIntersectingSquares(glm::dvec2 s, glm::dvec2 e, std::vector<BlockIndices> *l) {
	float can = s.x + s.y + e.x + e.y; 
	if(!isfinite(can) || isnan(can)) {
		printf("WARNING: Non-finite input to listIntersectingSquares\n"); 
		return; 
	}
	s /= TILE_WIDTH; //Set tile size to unit length. 
	e /= TILE_WIDTH; 
	BlockIndices t; 
	glm::dvec2 del = e - s; 
	glm::dvec2 dela = glm::abs(del); 
	bool steep = dela.y > dela.x; 
	if(steep) {
		s = glm::dvec2(s.y, s.x);
		e = glm::dvec2(e.y, e.x); 
	}
	if(s.x > e.x) {
		glm::dvec2 t = s;
		s = e;
		e = t; 
	}
	del = e - s; 
	double gradient = 1; 
	if(del.x != 0) {
		gradient = del.y / del.x; 
	}
	double xend = ceil(s.x); 
	double yend = s.y + gradient * (xend - s.x); 
	int xpxl = (int) s.x; 
	int ypxl = (int) s.y;
    if(steep) {
        t.row = (int) xpxl;
        t.col = (int) ypxl;
    } else {
        t.col = (int) xpxl;
        t.row = (int) ypxl;
    }
    l->push_back(t); 

	double intery = yend; 
	for (int x = xpxl + 1; x <= (int) e.x; x++) {
		ypxl = (int) intery; 
		if(steep) {
			t.row = x;
			t.col = (int) ypxl;
		} else {
			t.col = x;
			t.row = (int) ypxl;
		}
		l->push_back(t); 
		intery += gradient; 
        if((int) intery != ypxl && x <= (int) e.x) {
			ypxl = (int) intery; 
			if(steep) {
				t.row = x;
				t.col = (int) ypxl;
			} else {
				t.col = x;
				t.row = (int) ypxl;
			}
			l->push_back(t); 
		}
	}
}

//Squares are represented by row and column. 
void listNeighborSquares(glm::dvec2 s, std::vector<BlockIndices> *l) {
	s /= TILE_WIDTH; //Set tile size to unit length. 
	BlockIndices t; 
	int col = (int) s.x;
	int row = (int) s.y;
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			t.col = col + i;
			t.row = row + j; 
			l->push_back(t); 
		}
	}
}

//Squares are represented by row and column. 
void listTileNeighborSquares(BlockIndices t, std::vector<BlockIndices> *l) {
	int col = t.col;
	int row = t.row;
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			t.col = col + i;
			t.row = row + j; 
			l->push_back(t); 
		}
	}
}
