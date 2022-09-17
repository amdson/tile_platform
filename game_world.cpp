#include "game_world.hpp"
#include "particle.hpp"
#include "renderer.hpp"

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


RollbackECS::RollbackECS(int rollback_window) {
	id_map.assign(MAX_ENTITIES, -1); 
	player_map.assign(MAX_ENTITIES, -1); 
	health_map.assign(MAX_ENTITIES, -1); 
	entity_map.assign(MAX_ENTITIES, -1); 
	ai_map.assign(MAX_ENTITIES, -1); 

	for (int i = 0; i < rollback_window; i++) {
		r.push_back(RollbackStorage()); 
	}
	r_pos = 0; 
}

int RollbackECS::new_entity() {
	if(ids.size() == MAX_ENTITIES) {
		printf("MAX ENTITIES REACHED\n");
		return -1; 
	}
	int e; 
	if(free_ids.size() <= 0) {
		e = ids.size(); 
	} else {
		e = free_ids.back(); 
		// printf("reusing id %d\n", e);
		free_ids.pop_back(); 
	}
	id_map[e] = ids.size(); 
	ids.push_back(e); 
	return e; 
}

bool RollbackECS::delete_entity(int entity_id) {
	free_ids.push_back(entity_id); 
	id_map[entity_id] = -1; 
	player_map[entity_id] = -1; 
	health_map[entity_id] = -1;  
	entity_map[entity_id] = -1; 
	ai_map[entity_id] = -1; 
}

//Take vectors in RollbackStorage, and swap their contents with active vectors. 
//Now active vectors contain out-of-date info, and storage vectors contain previous run. 
//Iterate through the storage vectors, and replace content of active vectors while removing deleted entries. 
//Deleted entries are left in storage vectors for efficiency. 
void RollbackECS::save_update(RollbackStorage *s) {
	s->ids.resize(ids.size()); s->entities.resize(entities.size()); s->player_data.resize(player_data.size()); 
	s->ai_data.resize(ai_data.size()); s->health_data.resize(health_data.size()); 
	
	s->ids.swap(ids); s->entities.swap(entities); s->ai_data.swap(ai_data); 
	s->health_data.swap(health_data); s->player_data.swap(player_data); 
	
	int count = 0; 
	for (int i = 0; i < s->ids.size(); i++) {
		int id = s->ids[i]; 
		if(id_map[id] == i) {
			ids[count] = id;
			id_map[id] = count; 
			count += 1; 
		}
	}
	ids.resize(count); 
	count = 0; 
	for (int i = 0; i < s->entities.size(); i++) {
		Entity e = s->entities[i]; 
		if(entity_map[e.entity_id] == i) {
			entities[count] = e;
			entity_map[e.entity_id] = count; 
			count += 1; 
		}
	}
	entities.resize(count); 
	count = 0; 
	for (int i = 0; i < s->player_data.size(); i++) {
		PlayerData e = s->player_data[i]; 
		if(player_map[e.entity_id] == i) {
			player_data[count] = e;
			player_map[e.entity_id] = count; 
			count += 1; 
		}
	}
	player_data.resize(count); 
	count = 0; 
	for (int i = 0; i < s->ai_data.size(); i++) {
		AIData e = s->ai_data[i]; 
		if(ai_map[e.entity_id] == i) {
			ai_data[count] = e;
			ai_map[e.entity_id] = count; 
			count += 1; 
		}
	}
	ai_data.resize(count); 
	count = 0; 
	for (int i = 0; i < s->health_data.size(); i++) {
		HealthData e = s->health_data[i]; 
		if(health_map[e.entity_id] == i) {
			health_data[count] = e;
			health_map[e.entity_id] = count; 
			count += 1; 
		}
	}
	health_data.resize(count); 
}

void RollbackECS::roll_save() {
	RollbackStorage *rs = &r[r_pos % r.size()];
	save_update(rs); 
	r_pos = (r_pos+1)%r.size(); 
}

int RollbackECS::push_npc() {
	int p_id = new_entity(); 

	AIData a; 
	a.entity_id = p_id; 
	ai_map[p_id] = ai_data.size(); 
	ai_data.push_back(a); 

	Entity e = {entity_id:p_id};
	entity_map[p_id] = entities.size(); 
	entities.push_back(e);

	HealthData d; 
	d.entity_id = p_id;
	health_map[p_id] = health_data.size(); 
	health_data.push_back(d);
	return p_id; 
}

int RollbackECS::push_firefly(glm::dvec2 p) {
	int fb_id = push_npc(); 
	Entity *e = &entities[entity_map[fb_id]]; 
	PhysicsPolygon poly = {pos: glm::dvec2(0, 0), mass:1.0}; 
    glm::dvec2 v[] = {glm::dvec2(0, 0), glm::dvec2(1.0, 0), glm::dvec2(1.0, 1.0), glm::dvec2(0, 1.0)};
    memcpy(&poly.vertices, v, 4*sizeof(glm::dvec2)); 
    poly.num_vertices = 4; 
	e->poly = poly;  
	e->flags = ZERO_GRAVITY; 

	HealthData *d = &health_data[health_map[fb_id]]; 
	d->max_health = 100; d->health = 100; d->health_regen = 0; d->buffer_health = 0; d->buffer_regen = 0; 
	AIData *ad = &ai_data[ai_map[fb_id]]; 
	ad->type = FIREFLY;
	return fb_id; 
}

int RollbackECS::push_fireball(glm::dvec2 p, glm::dvec2 v) {
	int fb_id = push_npc(); 
	Entity *e = &entities[entity_map[fb_id]];

	PhysicsPolygon poly = {pos: glm::dvec2(0, 0), mass:1.0}; 
    glm::dvec2 vert[] = {glm::dvec2(0, 0), glm::dvec2(1.0, 0), glm::dvec2(1.0, 1.0), glm::dvec2(0, 1.0)};
    memcpy(&poly.vertices, vert, 4*sizeof(glm::dvec2)); 
    poly.num_vertices = 4;  

	e->poly = poly;  
	e->flags = ZERO_GRAVITY; 

	HealthData *d = &health_data[health_map[fb_id]]; 
	d->max_health = 5; d->health = 5; d->health_regen = 0; d->buffer_health = 0; d->buffer_regen = 0; 
	AIData *ad = &ai_data[ai_map[fb_id]]; 
	ad->type = FIREBALL; 
	FireballAI *fba = &ai_data[ai_map[fb_id]].data.fa; 
	fba->lifespan = 25; fba->power = 1; fba->tracking = false; fba->step = 0; 
	return fb_id; 
}


int RollbackECS::push_player() {
	int p_id = new_entity(); 
	PlayerData p = init_player_data(); 
	p.entity_id = p_id; 
	p.fire_cooldown = 0; 
	player_map[p_id] = player_data.size(); 
	player_data.push_back(p); 

	Entity e; 
	e.entity_id = p_id; 
	PhysicsPolygon poly = {pos: glm::dvec2(0, 0), mass:1.0}; 
    glm::dvec2 v[] = {glm::dvec2(0, 0), glm::dvec2(1.0, 1.0), glm::dvec2(1, 2), glm::dvec2(0, 3), glm::dvec2(-1, 2), glm::dvec2(-1, 1)};
    memcpy(&poly.vertices, v, 6*sizeof(glm::dvec2)); 
    poly.num_vertices = 6; 
	e.poly = poly;  

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

Gamestate::Gamestate(SpriteSheet *s, std::vector<Particle> *p) {
	ecs = new RollbackECS(16); 
	sprite_sheet = s; 
	particles = p; 
	memset(&main_chunk, 0, sizeof(Chunk));
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

PhysicsPolygon init_player_poly() {
	PhysicsPolygon poly;
	glm::dvec2 v[] = {glm::dvec2(0, 0), glm::dvec2(1.0, 1.0), glm::dvec2(1, 2), glm::dvec2(0, 3), glm::dvec2(-1, 2), glm::dvec2(-1, 1)};
    memcpy(&poly.vertices, v, 6*sizeof(glm::dvec2)); 
    poly.num_vertices = 6; 
	return poly; 
}

void player_physics_update(PhysicsPolygon *poly, PlayerData *p, Gamestate *g) {
	poly->vel = applyControls(poly->vel, &poly->contacts[0], poly->num_contacts, p);
	
	// Spawn particles
	MovementState s = p->prev_state;
	MovementState e = p->state; 
	if(s != e) {
		if(e == MovementState::AIR_JUMP) {
			glm::dvec2 cv = glm::dvec2(-0.05*p->inp.x, 0.2*std::min(-poly->vel.y, 0.0)); 
			Particle jump_cloud = {pos: poly->pos - glm::dvec2(1, 1), vel: cv, 
									dim: glm::dvec2(1, 1), s: g->sprite_sheet->getSpriteEntry("jump_cloud"),
									timestep: 0,
									change_interval: 8,
									lifetime: 23,
									gravity: false
			};
			g->particles->push_back(jump_cloud); 
		}  else if (e == MovementState::GROUND_JUMP) {
			Particle jump_flash = {pos: poly->pos - glm::dvec2(1, 1), vel: glm::dvec2(0, 0), 
									dim: glm::dvec2(1, 1), s: g->sprite_sheet->getSpriteEntry("jump_flash"),
									timestep: 0,
									change_interval: 1,
									lifetime: 16, gravity: false };
			g->particles->push_back(jump_flash); 
		}
	}
}