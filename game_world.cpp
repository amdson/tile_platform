#include "game_world.hpp"

bool operator==(const BlockIndices b1, const BlockIndices b2){
	return b1.col == b2.col && b1.row == b2.row && b1.chunk_col == b2.chunk_col && b1.chunk_row == b2.chunk_row; 
}

bool box_intersect(glm::dvec2 p1, glm::dvec2 d1, glm::dvec2 p2, glm::dvec2 d2) {
	p2 -= p1;
	return p2.x >= d2.x && p2.y >= d2.y && p2.x <= d1.x && p2.y <= d1.y; 
}

SDL_Point toPixelPoint(glm::dvec2 p, Camera c) {
	glm::dvec2 frame_pos = p - c.pos;
	int x = (int) ((frame_pos.x / c.dim.x) * ((double) c.SCREEN_HEIGHT)); 
	int y = c.SCREEN_HEIGHT - (int) ((frame_pos.y / c.dim.x) * ((double) c.SCREEN_HEIGHT));
	SDL_Point r = {x, y};
	return r; 
}

glm::dvec2 toPoint(SDL_Point p, Camera c) {
	glm::dvec2 frame_pos;
	frame_pos.x = c.dim.x * ((double) p.x) / c.SCREEN_HEIGHT;
	frame_pos.y = c.dim.x * ((double) (c.SCREEN_HEIGHT - p.y)) / c.SCREEN_HEIGHT;
	return frame_pos + c.pos; 
}

SDL_Rect toRect(glm::dvec2 p, glm::dvec2 d, Camera c) {
	glm::dvec2 frame_pos = p - c.pos;
	int x = (int) ((frame_pos.x / c.dim.x) * ((double) c.SCREEN_HEIGHT)); 
	int y = c.SCREEN_HEIGHT - (int) ((frame_pos.y / c.dim.x) * ((double) c.SCREEN_HEIGHT));
	int w = (int) ((d.x / c.dim.x) * ((double) c.SCREEN_HEIGHT));
	int h = (int) ((d.y / c.dim.y) * ((double) c.SCREEN_HEIGHT));
	SDL_Rect r = {x, y - h, w, h}; 
	return r; 
}

//Squares are represented by row and column. 
void listIntersectingSquares(glm::dvec2 s, glm::dvec2 e, std::vector<BlockIndices> *l) {
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

//p1, p2 initially represent lower left corners, d represents entity's box dimensions. 
Collision getTileBoxCollision(BlockIndices b, glm::dvec2 p1, glm::dvec2 p2, glm::dvec2 d) {
    glm::dvec2 tile_pos = glm::dvec2(b.col, b.row);
	tile_pos *= TILE_WIDTH;  
	tile_pos -= d; //Include entity box in tile pos/dim. 
    glm::dvec2 tile_dim = glm::dvec2(TILE_WIDTH, TILE_WIDTH) + d; 
    glm::dvec2 vel = p2 - p1; 

	double tx1 = -1; double tx2 = -1; 
    double ty1 = -1; double ty2 = -1; //Time of first collusion moving along x / y axes. 
    if(std::abs(vel.x) >= 1e-9) { //Arbitrary number that's far enough from zero that I won't get weird behavior. 
        tx1 = (tile_pos.x - p1.x) / vel.x; 
        tx2 = (tile_dim.x + tile_pos.x - p1.x) / vel.x; 
    } else if(p1.x > tile_pos.x && p1.x < tile_pos.x + tile_dim.x) {
			tx1 = -INFINITY; 
			tx2 = INFINITY; 
	}

    if(std::abs(vel.y) >= 1e-9) {
        ty1 = (tile_pos.y - p1.y) / vel.y; 
        ty2 = (tile_dim.y + tile_pos.y - p1.y) / vel.y; 
    } else if(p1.y > tile_pos.y && p1.y < tile_pos.y + tile_dim.y) {
		ty1 = -INFINITY; 
		ty2 = INFINITY; 
	}

	double temp = tx1; tx1 = std::min(tx1, tx2); tx2 = std::max(temp, tx2); 
	temp = ty1; ty1 = std::min(ty1, ty2); ty2 = std::max(temp, ty2); 

	//Path intersects during overlap of x and y intervals. If x interval comes after, then collision
	// is on x-axis. Otherwise y-axis. 
	bool x_side = (tx1 > ty1); 
	double ts = std::max(tx1, ty1); double te = std::min(tx2, ty2); 

	Collision c;
	c.t = INFINITY; 
	if(te <= ts || te < 0 || ts > 1) return c; //No collision. 

	// if(ts < 1e-5) {
	// 	if(ts == 0) {
	// 		printf("wtf\n"); 
	// 	}
	// 	if(te == 0) {
	// 		printf("wwtf\n"); 
	// 	}
	// 	printf("x times %f, %f y times %f, %f\n", tx1, tx2, ty1, ty2);
	// 	printf("intersection times %f %f\n", ts, te); 
	// 	printf("start point %f, %f\n", p1.x, p1.y);
	// 	printf("end point %f %f\n", p2.x, p2.y); 
	// 	printf("vel %f %f\n", vel.x, vel.y); 
	// 	printf("xd1 %f, xd2 %f\n", (tile_pos.x - p1.x), (tile_dim.x + tile_pos.x - p1.x));
	// 	printf("tile pos %f, %f\n", tile_pos.x, tile_pos.y); 
	// }

	// Entity is either inside or about to collide with tile. 
	c.t = ts; 
	if(x_side) {
		c.norm = glm::dvec2(2*((int) std::signbit(vel.x)) - 1, 0); 
		c.s = c.norm.x > 0 ? ContactSide::RIGHT : ContactSide::LEFT;
	} else {
		c.norm = glm::dvec2(0, 2*((int) std::signbit(vel.y)) - 1);
		c.s = c.norm.y > 0 ? ContactSide::TOP : ContactSide::BOTTOM;
	}
    c.pos = p1 + vel * c.t + c.norm * COLLISION_BUFFER; 
	return c; 
}

glm::dvec2 getConstrainedSurfaceVel(glm::dvec2 v, glm::dvec2 norm) {
	double norm_vel = std::min(glm::dot(v, norm), 0.0); 
	v -= norm*norm_vel; 
	// switch (s)
	// {
	// case ContactSide::TOP:
	// 	v.y = std::max(v.y, 0.0); 
	// 	break;
	// case ContactSide::BOTTOM:
	// 	v.y = std::min(v.y, 0.0); 
	// 	break;
	// case ContactSide::RIGHT:
	// 	v.y = std::max(v.x, 0.0); 
	// 	break;
	// case ContactSide::LEFT:
	// 	v.y = std::min(v.x, 0.0); 
	// 	break;
	// default:
	// 	break;
	// }
	return v; 
}

void invalidateContacts(std::vector<TileContact> *t, ContactSide c) {
	for (int i = 0; i < t->size(); i++) {
		if(t->at(i).s == c) {
			t->at(i).valid = false; 
			if(t->at(i).valid) {
				printf("invalidation failed, you can't code\n"); 
			}
		}
	}
}

void PlayerController::updateInputs(InputState new_inp) {
	prev_inp = inp; 
	inp = new_inp; 
}

//Identify which sides the player is in contact with, and update movement state. 
void PlayerController::applyContacts(glm::dvec2 v, std::vector<TileContact> *t) {
	//Check which sides the player is in contact with. 
	std::fill_n(contact_sides, 4, false);
	for (int i = 0; i < t->size(); i++) {
		TileContact tc = t->at(i); 
		// printf("side val %d\n", tc.s); 
		contact_sides[tc.s] = true; 
	}
	MovementState output = state; 
	switch (state) {
	case MovementState::GROUND:
		if(!contact_sides[ContactSide::TOP]) {
			if(contact_sides[ContactSide::LEFT] || contact_sides[ContactSide::RIGHT]) {
				output = MovementState::WALL; 
			} else {
				output = MovementState::JUMP1; 
			}
		}
		break;
	case MovementState::WALL:
		if (contact_sides[ContactSide::TOP]) {
			output = MovementState::GROUND; 
		} else if (!(contact_sides[ContactSide::LEFT] || contact_sides[ContactSide::RIGHT])) {
			output = MovementState::GROUND_JUMP; 
		}
		break;
	case MovementState::FALLING:
	case MovementState::AIR_JUMP:
	case MovementState::GROUND_JUMP:
	case MovementState::JUMP1:
	case MovementState::WALL_JUMP: 
		if (contact_sides[ContactSide::TOP]) {
			output = MovementState::GROUND; 
		} else if ((contact_sides[ContactSide::LEFT] || contact_sides[ContactSide::RIGHT]) && std::abs(v.y) <= 0.04) {
			output = MovementState::WALL; 
		}
		break;
	default:
		break;
	}
	if(state != output) {
		state = output;
		// printf("A-switching to state %d at timestep %d\n", state, timestep); 
		timestep = 0;  
	}
}

glm::dvec2 PlayerController::applyControls(glm::dvec2 v, std::vector<TileContact> *t) {
	prev_state = state; 
	applyContacts(v, t); 
	//Controller physics
	MovementState next_state = state; 
	bool right_face = contact_sides[ContactSide::RIGHT]; 
	switch (state) {
	case MovementState::GROUND:
		if(inp.x > 0) {
			v.x += GROUND_ACC; 
		} else if(inp.x < 0) {
			v.x -= GROUND_ACC; 
		}
		if(inp.y < 0) {
			v.x *= 0.6; 
		} else if(inp.y > 0) {
			//Remove contact with tiles under player so jump isn't repeated. 
			// invalidateContacts(t, ContactSide::TOP);
			v.y += GROUND_JUMP_VEL; 
			next_state = MovementState::GROUND_JUMP; 
		}
		break;
	case MovementState::WALL:
		if((right_face && inp.x < 0) || (!right_face && inp.x > 0)) {
			v.y *= 0.5; 
		}
		if(inp.x > 0) {
			v.x += GROUND_ACC; 
		} else if(inp.x < 0) {
			v.x -= GROUND_ACC; 
		}
		if(inp.y > 0 && prev_inp.y <= 0) {
			v.y += 0.2; 
			if(right_face) {
				v.x += 0.25;
			} else {
				v.x -= 0.25; 
			}
			next_state = MovementState::GROUND_JUMP; 
		}
		break; 
	case MovementState::GROUND_JUMP:
		if(inp.x > 0) {
			v.x += 0.02; 
		} else if(inp.x < 0) {
			v.x -= 0.02; 
		}
		if(timestep < 10 && inp.y > 0) {
			v.y += 0.03; 
		} else {
			next_state = JUMP1; 
		}
		break;
	case MovementState::JUMP1:
		if(inp.x > 0) {
			v.x += AIR_ACC; 
		} else if(inp.x < 0) {
			v.x -= AIR_ACC; 
		}
		if(inp.y < 0) {
			v.y -= 0.02; 
		}
		if(inp.y > 0 and prev_inp.y <= 0) { //Input must be re-entered. 
			v.y += AIR_JUMP_VEL;
			next_state = MovementState::AIR_JUMP; 
		}
		break;
	case MovementState::AIR_JUMP:
		if(inp.x > 0) {
			v.x += 0.02; 
		} else if(inp.x < 0) {
			v.x -= 0.02; 
		}
		if(timestep < 10 && inp.y > 0) {
			v.y += 0.04; 
		} else {
			next_state = FALLING; 
		}
		break; 
	case MovementState::FALLING:
		if(inp.x > 0) {
			v.x += AIR_ACC; 
		} else if(inp.x < 0) {
			v.x -= AIR_ACC; 
		}
		if(inp.y < 0) {
			v.y -= 0.02; 
		}
		break;
	default:
		break;
	}

	//Basic physics 
	v.y -= 0.02; //Gravity
	if(contact_sides[ContactSide::TOP]) {
		v.x *= 0.8; //Ground friction
	} else {
		// v *= 0.9; //Air resistance
		double l = glm::length(v); 
		v = v / (1 + 0.14 * l); 
	}
	if(contact_sides[ContactSide::LEFT] || contact_sides[ContactSide::RIGHT]) {
		v.y *= 0.9; //Wall friction
	}
	timestep += 1; 
	if(next_state != state) {
		state = next_state; 
		// printf("C-switching to state %d at timestep %d\n", state, timestep); 
		timestep = 0; 
	}
	return v; 
}

//Checks whether position p still meets criteria for contact. 
bool checkTileContact(glm::dvec2 p, glm::dvec2 d, TileContact t) {
	glm::dvec2 tile_pos = glm::dvec2(t.b.col, t.b.row) * TILE_WIDTH - d;

	// glm::dvec2 offset = glm::max(t.norm, glm::dvec2(0, 0)) * TILE_WIDTH; 
	// glm::dvec2 surface_start = tile_pos + offset; 
	// glm::dvec2 surface_dir = glm::abs(glm::dvec2(t.norm.y, t.norm.x)); 
	// double norm_dist = glm::dot(p - surface_start, t.norm); //Distance out from surface. 
	// double surface_dist = glm::dot(p - surface_start, surface_dir); //Distance along surface. 
	// return norm_dist >= 0 && norm_dist <= CONTACT_BUFFER && surface_dist >= 0 && surface_dist <= TILE_WIDTH; 

	return t.valid && p.x >= tile_pos.x - CONTACT_BUFFER && p.x <= tile_pos.x + d.x + TILE_WIDTH + CONTACT_BUFFER &&
			p.y >= tile_pos.y - CONTACT_BUFFER && p.y <= tile_pos.y + d.y + TILE_WIDTH + CONTACT_BUFFER; 
}

//Checks whether a block maintains a tile contact, regardless of whether it was originally responsible. 
bool checkTileContactMaintained(glm::dvec2 p, glm::dvec2 d, TileContact t, BlockIndices b) {
	BlockIndices cb = t.b; 
	int del_row = b.row - cb.row; 
	int del_col = b.col - cb.col; 

	//Check that the new tile is adjacent to the contact tile. 
	if(!(std::abs(del_row) + std::abs(del_col) <= 1)) {
		return false; 
	} 
	//Check that if the new tile maintains contact, its surface is flush with the contact surface. 
	if (! ( (std::abs(del_row) == 1 && (t.s == ContactSide::LEFT || t.s == ContactSide::RIGHT) ) || 
		(std::abs(del_col) == 1 && (t.s == ContactSide::TOP || t.s == ContactSide::BOTTOM) ) ) ) {
			return false; 
	}

	//Check that body is within contact buffer of new tile. 
	glm::dvec2 tile_pos = glm::dvec2(b.col, b.row) * TILE_WIDTH - d;
	return t.valid && p.x >= tile_pos.x - CONTACT_BUFFER && p.x <= tile_pos.x + d.x + TILE_WIDTH + CONTACT_BUFFER &&
			p.y >= tile_pos.y - CONTACT_BUFFER && p.y <= tile_pos.y + d.y + TILE_WIDTH + CONTACT_BUFFER; 
}
