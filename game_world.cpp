#include "game_world.hpp"
#include "particle.hpp"
#include "renderer.hpp"

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
		}
	}
}

void updateInputs(InputState new_inp, PlayerData *p) {
	p->prev_inp = p->inp; 
	p->inp = new_inp; 
}

//Identify which sides the player is in contact with, and update movement state. 
void applyContacts(glm::dvec2 v, TileContact *t, int num_contacts, PlayerData *p) {
	//Check which sides the player is in contact with. 
	std::fill_n(p->contact_sides, 4, false);
	for (int i = 0; i < num_contacts; i++) {
		TileContact tc = t[i]; 
		// printf("side val %d\n", tc.s); 
		p->contact_sides[tc.s] = true; 
	}
	MovementState output = p->state; 
	switch (p->state) {
	case MovementState::GROUND:
		if(!p->contact_sides[ContactSide::TOP]) {
			if(p->contact_sides[ContactSide::LEFT] || p->contact_sides[ContactSide::RIGHT]) {
				output = MovementState::WALL; 
			} else {
				output = MovementState::JUMP1; 
			}
		}
		break;
	case MovementState::WALL:
		if (p->contact_sides[ContactSide::TOP]) {
			output = MovementState::GROUND; 
		} else if (!(p->contact_sides[ContactSide::LEFT] || p->contact_sides[ContactSide::RIGHT])) {
			output = MovementState::GROUND_JUMP; 
		}
		break;
	case MovementState::FALLING:
	case MovementState::AIR_JUMP:
	case MovementState::GROUND_JUMP:
	case MovementState::JUMP1:
	case MovementState::WALL_JUMP: 
		if (p->contact_sides[ContactSide::TOP]) {
			output = MovementState::GROUND; 
		} else if ((p->contact_sides[ContactSide::LEFT] || p->contact_sides[ContactSide::RIGHT]) && std::abs(v.y) <= 0.04) {
			output = MovementState::WALL; 
		}
		break;
	default:
		break;
	}
	if(p->state != output) {
		p->state = output;
		// printf("A-switching to state %d at timestep %d\n", state, timestep); 
		p->timestep = 0;  
	}
}

Gamestate::Gamestate(SpriteSheet *s, std::vector<Particle> *p) {
	sprite_sheet = s; 
	particles = p; 
	memset(&main_chunk, 0, sizeof(Chunk));

	player_map.assign(MAX_ENTITIES, -1); 
	health_map.assign(MAX_ENTITIES, -1); 
	entity_map.assign(MAX_ENTITIES, -1); 
	ai_map.assign(MAX_ENTITIES, -1); 
	entity_gen.assign(MAX_ENTITIES, 0);
	
}

int Gamestate::push_entity() {
	int e; 
	if(free_ids.size() <= 0) {
		e = entities.size(); 
		entity_gen.push_back(0); 
	} else {
		e = free_ids.back(); 
		printf("reusing id %d\n", e);
		free_ids.pop_back(); 
		entity_gen[e] += 1; 
	}
	return e; 
}

PlayerData init_player_data() {
	PlayerData p; 
	p.prev_state = MovementState::FALLING;
	p.state = MovementState::FALLING; 
	p.inp = {0}; 
	p.prev_inp = {0}; 
	p.timestep = 0;
	return p; 
}

int Gamestate::push_player() {
	int p_id = push_entity(); 
	PlayerData p = init_player_data(); 
	p.entity_id = p_id; 
	p.fire_cooldown = 0; 
	player_map[p_id] = player_data.size(); 
	player_data.push_back(p); 

	Entity e; 
	e.entity_id = p_id; 
	entity_map[p_id] = entities.size(); 
	entities.push_back(e);

	HealthData d; 
	d.entity_id = p_id; 
	d.max_health = 100; 
	d.health = 100; 
	d.health_regen = 1; 
	d.buffer_regen = 10;
	d.buffer_health = 20;  
	health_map[p_id] = health_data.size(); 
	health_data.push_back(d);
	return p_id; 
}

int Gamestate::push_npc() {
	int p_id = push_entity(); 
	AIData a; 
	a.entity_id = p_id; 
	ai_map[p_id] = ai_data.size(); 
	ai_data.push_back(a); 

	Entity e = {entity_id:0, pos: glm::dvec2(0, 0), vel: glm::dvec2(0, 0), dim: glm::dvec2(1, 1), num_contacts:0};
	e.entity_id = p_id;
	entity_map[p_id] = entities.size(); 
	entities.push_back(e);

	HealthData d; 
	d.entity_id = p_id;
	health_map[p_id] = health_data.size(); 
	health_data.push_back(d);
	return p_id; 
}

int Gamestate::push_firefly(glm::dvec2 p) {
	int fb_id = push_npc(); 
	Entity *e = &entities[entity_map[fb_id]]; 
	e->dim = glm::dvec2(1, 1); 
	e->pos = p; 
	e->vel = glm::dvec2(0, 0); 
	e->flags = ZERO_GRAVITY; 

	HealthData *d = &health_data[health_map[fb_id]]; 
	d->max_health = 100; d->health = 100; d->health_regen = 0; d->buffer_health = 0; d->buffer_regen = 0; 
	AIData *ad = &ai_data[ai_map[fb_id]]; 
	ad->type = FIREFLY;
	return fb_id; 
}

int Gamestate::push_fireball(glm::dvec2 p, glm::dvec2 v) {
	int fb_id = push_npc(); 
	Entity *e = &entities[entity_map[fb_id]]; 
	e->dim = glm::dvec2(0.5, 1); 
	e->pos = p; 
	e->vel = v;
	e->flags = ZERO_GRAVITY; 
	HealthData *d = &health_data[health_map[fb_id]]; 
	d->max_health = 5; d->health = 5; d->health_regen = 0; d->buffer_health = 0; d->buffer_regen = 0; 
	AIData *ad = &ai_data[ai_map[fb_id]]; 
	ad->type = FIREBALL; 
	FireballAI *fba = &ai_data[ai_map[fb_id]].data.fa; 
	fba->lifespan = 25; fba->power = 10; fba->tracking = false; fba->step = 0; 
	return fb_id; 
}

bool Gamestate::delete_entity(int entity_id, int gen) {
	assert(entity_id < MAX_ENTITIES); 
	entity_gen[entity_id] += 1; 
	free_ids.push_back(entity_id); 
	player_map[entity_id] = -1; 
	health_map[entity_id] = -1;  
	entity_map[entity_id] = -1; 
	ai_map[entity_id] = -1; 
	entity_gen[entity_id] = -1; 
}

glm::dvec2 applyControls(glm::dvec2 v, TileContact *t, int num_contacts, PlayerData *p) {
	p->prev_state = p->state; 
	applyContacts(v, t, num_contacts, p); 
	//Controller physics
	MovementState next_state = p->state; 
	bool right_face = p->contact_sides[ContactSide::RIGHT]; 
	switch (p->state) {
	case MovementState::GROUND:
		if(p->inp.x > 0) {
			v.x += GROUND_ACC; 
		} else if(p->inp.x < 0) {
			v.x -= GROUND_ACC; 
		}
		if(p->inp.y < 0) {
			v.x *= 0.6; 
		} else if(p->inp.y > 0) {
			//Remove contact with tiles under player so jump isn't repeated. 
			// invalidateContacts(t, ContactSide::TOP);
			v.y += GROUND_JUMP_VEL; 
			next_state = MovementState::GROUND_JUMP; 
		}
		break;
	case MovementState::WALL:
		if((right_face && p->inp.x < 0) || (!right_face && p->inp.x > 0)) {
			v.y *= 0.5; 
		}
		if(p->inp.x > 0) {
			v.x += GROUND_ACC; 
		} else if(p->inp.x < 0) {
			v.x -= GROUND_ACC; 
		}
		if(p->inp.y > 0 && p->prev_inp.y <= 0) {
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
		if(p->inp.x > 0) {
			v.x += 0.02; 
		} else if(p->inp.x < 0) {
			v.x -= 0.02; 
		}
		if(p->timestep < 10 && p->inp.y > 0) {
			v.y += 0.03; 
		} else {
			next_state = JUMP1; 
		}
		break;
	case MovementState::JUMP1:
		if(p->inp.x > 0) {
			v.x += AIR_ACC; 
		} else if(p->inp.x < 0) {
			v.x -= AIR_ACC; 
		}
		if(p->inp.y < 0) {
			v.y -= 0.02; 
		}
		if(p->inp.y > 0 and p->prev_inp.y <= 0) { //Input must be re-entered. 
			v.y += AIR_JUMP_VEL;
			next_state = MovementState::AIR_JUMP; 
		}
		break;
	case MovementState::AIR_JUMP:
		if(p->inp.x > 0) {
			v.x += 0.02; 
		} else if(p->inp.x < 0) {
			v.x -= 0.02; 
		}
		if(p->timestep < 10 && p->inp.y > 0) {
			v.y += 0.04; 
		} else {
			next_state = FALLING; 
		}
		break; 
	case MovementState::FALLING:
		if(p->inp.x > 0) {
			v.x += AIR_ACC; 
		} else if(p->inp.x < 0) {
			v.x -= AIR_ACC; 
		}
		if(p->inp.y < 0) {
			v.y -= 0.02; 
		}
		break;
	default:
		break;
	}

	//Basic physics 
	v.y -= 0.02; //Gravity
	if(p->contact_sides[ContactSide::TOP]) {
		v.x *= 0.8; //Ground friction
	} else {
		// v *= 0.9; //Air resistance
		double l = glm::length(v); 
		v = v / (1 + 0.14 * l); 
	}
	if(p->contact_sides[ContactSide::LEFT] || p->contact_sides[ContactSide::RIGHT]) {
		v.y *= 0.9; //Wall friction
	}
	p->timestep += 1; 
	if(next_state != p->state) {
		p->state = next_state; 
		// printf("C-switching to state %d at timestep %d\n", state, timestep); 
		p->timestep = 0; 
	}
	return v; 
}

void player_physics_update(Entity *entity, PlayerData *p, Gamestate *g) {
	entity->vel = applyControls(entity->vel, &entity->tile_contacts[0], entity->num_contacts, p);
	
	// Spawn particles
	MovementState s = p->prev_state;
	MovementState e = p->state; 
	if(s != e) {
		if(e == MovementState::AIR_JUMP) {
			glm::dvec2 cv = glm::dvec2(-0.05*p->inp.x, 0.2*std::min(-entity->vel.y, 0.0)); 
			Particle jump_cloud = {pos: entity->pos - glm::dvec2(entity->dim.x*0.5, entity->dim.y+0.05), vel: cv, 
									dim: glm::dvec2(1, 1), s: g->sprite_sheet->getSpriteEntry("jump_cloud"),
									timestep: 0,
									change_interval: 8,
									lifetime: 23,
									gravity: false
			};
			g->particles->push_back(jump_cloud); 
		}  else if (e == MovementState::GROUND_JUMP) {
			Particle jump_flash = {pos: entity->pos - glm::dvec2(entity->dim.x*0.5, entity->dim.y+0.05), vel: glm::dvec2(0, 0), 
									dim: glm::dvec2(1, 1), s: g->sprite_sheet->getSpriteEntry("jump_flash"),
									timestep: 0,
									change_interval: 1,
									lifetime: 16, gravity: false };
			g->particles->push_back(jump_flash); 
		}
	}

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

void filterTileContacts(Entity *e, std::vector<BlockIndices> *block_indices, Chunk *main_chunk) {
	//Filter contacts for existence.
	block_indices->clear();
	int valid_count = 0;  
	for (int i = 0; i < e->num_contacts; i++) {
		TileContact t = e->tile_contacts[i]; 
		bool is_valid = checkTileContact(e->pos, e->dim, t); 
		if(!is_valid) {
			block_indices->clear();
			listTileNeighborSquares(t.b, block_indices); 
			for (int i = 0; i < block_indices->size(); i++) {
				BlockIndices b = block_indices->at(i); 
				if(checkTileContactMaintained(e->pos, e->dim, t, b) && 
					main_chunk->tiles[b.row*CHUNK_TILES + b.col].tile_id > 0) {
					t.b = b; 
					is_valid = true; 
					break; 
				}
			}
		}
		if(is_valid) {
			e->tile_contacts[valid_count] = t; //Save contact for future loops. 
			valid_count += 1; 
		}
	}
	e->num_contacts = valid_count; 
}

void tilePhysics(Entity *e, std::vector<BlockIndices> *block_indices, Chunk *main_chunk) {
	//Constrain velocity with tile contacts. 
	for (int i = 0; i < e->num_contacts; i++) {
		TileContact t = e->tile_contacts[i]; 
		e->vel = getConstrainedSurfaceVel(e->vel, t.norm); 
	}
	//Tentative new position.
	e->newPos = e->pos + e->vel; 

	Collision fc; 
	fc.t = INFINITY; 
	BlockIndices contact_block; //Block first contact is with. 

	block_indices->clear(); 
	//List squares that could feasibly collide with entity 
	//TODO Make sure to handle edge case of player path missing blocks, but player block hitting them. 
	listIntersectingSquares(e->pos, e->newPos, block_indices); 
	listIntersectingSquares(e->pos + e->dim, e->newPos + e->dim, block_indices); 
	listNeighborSquares(e->pos, block_indices); 

	//Iterate through blocks that are near player and check for collisions. 
	for (int i = 0; i < block_indices->size(); i++) {
		BlockIndices b = block_indices->at(i); 
		if(b.row >= 0 && b.row < CHUNK_TILES && b.col >= 0 && b.col < CHUNK_TILES) {
			//Check that tile isn't already accounted for in contacts. 
			bool not_contact = true; 
			for (int i = 0; i < e->num_contacts; i++) {
				TileContact t = e->tile_contacts[i];
				if(t.b == b) {
					not_contact = false; 
					break; 
				}
			}
			if(not_contact && main_chunk->tiles[b.row*CHUNK_TILES + b.col].tile_id > 0) {
				//Handle collusion. 
				Collision c = getTileBoxCollision(b, e->pos, e->newPos, e->dim); 
				if(c.t < fc.t && c.t >= 0) {
					fc = c; 
					contact_block = b; 
				}
			}
		}
	}

	if(fc.t > 1 || fc.t < 0) {
		e->pos = e->newPos; 
	} else {
		double norm_vel = glm::dot(e->vel, fc.norm);
		e->pos = fc.pos; 
		if(norm_vel >= -0.3) {
			//Store contact to constrain motion. 
			TileContact col_cont = {fc.pos, fc.norm, contact_block, fc.s, true}; 
			e->tile_contacts[e->num_contacts] = col_cont; 
			e->num_contacts += 1; 
			//Remove normal component of vel
			e->vel -= fc.norm * norm_vel; 
		} else {
			//Bounce vel. 
			TileContact col_cont = {fc.pos, fc.norm, contact_block, fc.s, true}; 
			e->tile_contacts[e->num_contacts] = col_cont; 
			e->num_contacts += 1; 
			e->vel -= 1.4*fc.norm * norm_vel; 
		}
	}
}