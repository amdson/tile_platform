#include "physics.hpp"
#include "chunk.hpp"

//Intersect moving point with static segment. Return true if collision. 
bool point_segment_update(glm::dvec2 p0, glm::dvec2 p1, glm::dvec2 s0, glm::dvec2 s1, Collision *c) {
    glm::dvec2 delta = p1 - p0; 
    glm::dvec2 side = s1 - s0; 
    glm::dvec2 norm = glm::dvec2(side.y, -side.x); //90 degrees CW. Points out from points ordered CCW. 
    double dn = glm::dot(delta, norm); 
    if(dn == 0) { return false; }
    double t = glm::dot(norm, s0-p0) / dn; 
    if(t < 0 || t > 1) {return false;} 
    glm::dvec2 pos = p0+delta*t; 
    double segment_pos = glm::dot(pos, side); 
    if(t < c->t && segment_pos >= glm::dot(s0, side) && segment_pos <= glm::dot(s1, side)) {
        c->t = t; 
        c->pos = pos; 
        c->norm = norm; 
        return true; 
    }
    return false; 
}

//Detects collisions from a polygon moving into a rectangle. Does not detect active collisions. 
bool polygon_rectangle_update(PhysicsPolygon* poly, glm::dvec2 p0, glm::dvec2 p1, Rect r, Collision *c) {
    bool has_collision = false; 
    glm::dvec2 rv[4]; //Rectangle corners
    rv[0] = r.pos; rv[1] = r.pos + glm::dvec2(r.dim.x, 0);
    rv[2] = r.pos + r.dim; rv[3] = r.pos + glm::dvec2(0, r.dim.y);
    for (int i = 0; i < poly->num_vertices; i++) {
        glm::dvec2 s0 = poly->vertices[i]; glm::dvec2 s1 = poly->vertices[(i+1) % poly->num_vertices];
        for (int j = 0; j < 4; j++) {
             if(point_segment_update(rv[j]-p0, rv[j]-p1, s1, s0, c)) {
                 c->pos = rv[j]; //Special handling to convert point->segment to segment->point. 
             }
             has_collision = has_collision || point_segment_update(s0+p0, s0+p1, rv[j], rv[(j+1)%4], c); 
        }
    }
    return has_collision; 
}

glm::dvec2 getConstrainedSurfaceVel(glm::dvec2 v, glm::dvec2 norm) {
	double norm_vel = std::min(glm::dot(v, norm), 0.0); 
	v -= norm*norm_vel; 
	return v; 
}

//Checks whether position p still meets criteria for contact. 
//Currently rechecks every side. TODO: Optimize by tracking the polygon side to check in TileContact. 
bool checkTileContact(PhysicsPolygon* p, TileContact t) {
	glm::dvec2 tile_pos = glm::dvec2(t.b.col, t.b.row) * TILE_WIDTH;
	if(!t.valid) return false; 
	Rect r = {pos: tile_pos, dim: glm::dvec2(TILE_WIDTH, TILE_WIDTH)}; 
	Collision c;
	glm::dvec2 norm = -t.norm / glm::length(t.norm); 
	glm::dvec2 p2 = p->pos + norm * CONTACT_BUFFER; 
	bool contact = polygon_rectangle_update(p, p->pos, p2, r, &c); 
	return contact && glm::dot(c.norm, norm) >= 0.9; 
}

//Checks whether a block maintains a tile contact, regardless of whether it was originally responsible. 
bool checkTileContactMaintained(PhysicsPolygon* p, TileContact t, BlockIndices b) {
	BlockIndices cb = t.b; 
	int del_row = b.row - cb.row; 
	int del_col = b.col - cb.col; 

	//Check that the new tile is adjacent to the contact tile. 
	if(!((std::abs(del_row) + std::abs(del_col)) <= 1)) {
		return false; 
	}
	glm::dvec2 tile_pos = glm::dvec2(t.b.col, t.b.row) * TILE_WIDTH;
	if(!t.valid) return false; 
	Rect r = {pos: tile_pos, dim: glm::dvec2(TILE_WIDTH, TILE_WIDTH)}; 
	Collision c;
	glm::dvec2 norm = -t.norm / glm::length(t.norm); 
	glm::dvec2 p2 = p->pos + norm * CONTACT_BUFFER; 
	bool contact = polygon_rectangle_update(p, p->pos, p2, r, &c); 
	return contact && glm::dot(c.norm, norm) >= 0.9; 
}

void filterTileContacts(PhysicsPolygon *e, std::vector<BlockIndices> *block_indices, Chunk *main_chunk) {
	//Filter contacts for existence.
	block_indices->clear();
	int valid_count = 0;  
	for (int i = 0; i < e->num_contacts; i++) {
		TileContact t = e->contacts[i]; 
		bool is_valid = checkTileContact(e, t); 
		if(!is_valid) {
			block_indices->clear();
			listTileNeighborSquares(t.b, block_indices); 
			for (int i = 0; i < block_indices->size(); i++) {
				BlockIndices b = block_indices->at(i); 
				if(checkTileContactMaintained(e, t, b) && 
					main_chunk->tiles[b.row*CHUNK_TILES + b.col].tile_id > 0) {
					t.b = b; 
					is_valid = true; 
					break; 
				}
			}
		}
		if(is_valid) {
			e->contacts[valid_count] = t; //Save contact for future loops. 
			valid_count += 1; 
		}
	}
	e->num_contacts = valid_count; 
}

void tilePhysics(PhysicsPolygon* p, std::vector<BlockIndices> *block_indices, Chunk *main_chunk) {
	//Constrain velocity with tile contacts. 
	for (int i = 0; i < p->num_contacts; i++) {
		TileContact t = p->contacts[i]; 
		p->vel = getConstrainedSurfaceVel(p->vel, t.norm); 
	}

	//Tentative new position.
	p->n_pos = p->pos + p->vel; 

	Collision fc; 
	fc.t = INFINITY; 
	BlockIndices contact_block; //Block first contact is with. 

	block_indices->clear(); 
	//List squares that could feasibly collide with entity 
	//TODO Account for intersecting squares for real. Probably with bounding box. 
	listIntersectingSquares(p->pos, p->n_pos, block_indices); 
	listNeighborSquares(p->pos, block_indices); 


	//Iterate through blocks that are near player and check for collisions. 
    Collision c;
	for (int i = 0; i < block_indices->size(); i++) {
		BlockIndices b = block_indices->at(i); 
		if(b.row >= 0 && b.row < CHUNK_TILES && b.col >= 0 && b.col < CHUNK_TILES) {
			//Check that tile isn't already accounted for in contacts. 
			bool not_contact = true; 
			for (int i = 0; i < p->num_contacts; i++) {
				TileContact t = p->contacts[i];
				if(t.b == b) {
					not_contact = false; 
					break; 
				}
			}
			if(not_contact && main_chunk->tiles[b.row*CHUNK_TILES + b.col].tile_id > 0) {
				//Handle collusion. 
                Rect r = {pos:glm::dvec2(b.col*TILE_WIDTH, b.row*TILE_WIDTH), dim:glm::dvec2(TILE_WIDTH, TILE_WIDTH)}; 
                bool contact = polygon_rectangle_update(p, p->pos, p->n_pos, r, &c); 
                if(contact) contact_block = b; 
			}
		}
	}

	if(c.t > 1 || c.t < 0) {
		p->pos = p->n_pos; 
	} else {
		double norm_vel = glm::dot(p->vel, c.norm);
		p->pos = p->pos + (p->n_pos - p->pos) * c.t; 
		if(norm_vel >= -0.3) {
			//Store contact to constrain motion. 
			TileContact col_cont = {c.pos, c.norm, contact_block}; 
			p->contacts[p->num_contacts] = col_cont; 
			p->num_contacts += 1; 
			//Remove normal component of vel
			p->vel -= fc.norm * norm_vel; 
		} else {
			//Bounce vel. 
			TileContact col_cont = {fc.pos, fc.norm, contact_block}; 
			p->contacts[p->num_contacts] = col_cont; 
			p->num_contacts += 1; 
			p->vel -= 1.4*fc.norm * norm_vel; 
		}
	}
}