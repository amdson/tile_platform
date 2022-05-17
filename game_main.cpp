#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <math.h>  
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#include "timer.hpp"
#include "game_world.hpp"
#include "renderer.hpp"
#include "particle.hpp"
#include "combat.hpp"
#include "inputs.hpp"

using namespace std; 

const int SCREEN_FPS = 60;
const int SCREEN_TICK_PER_FRAME = 1000 / SCREEN_FPS;

const int UPDATES_PER_SECOND = 50;
const int TICKS_PER_UPDATE = 1000 / UPDATES_PER_SECOND; 
const int TICKS_PER_SECOND = 1000; 

void renderParticle(Particle p); 

SDL_Texture *TILE_SHEET; 
SpriteSheet *sprite_sheet; 

SDL_Rect TILE_LOCATION = {0, 0, TILE_PIXELS, TILE_PIXELS}; 
SDL_Rect SPRITE_LOCATION = {0, 0, 32, 32}; 

 //Initialized to all zeros. 
Sprite player_sprite; 

Camera camera; 

bool loadMedia() {
	TILE_SHEET = loadTextureFromFile("resources/tile_sheet.png"); 
	sprite_sheet = new SpriteSheet("resources/sprite_entries.json"); 
	return ~(TILE_SHEET == NULL || sprite_sheet == NULL); 
}

int main( int argc, char* args[] ) {
	//Start up SDL and create window
	if( !init() ) {
		printf( "Failed to initialize!\n" );
		exit(EXIT_FAILURE); 
	}

	if( !loadMedia() ) {
		printf( "Failed to load media!\n" );
		exit(EXIT_FAILURE); 
	}

	camera.pos = glm::dvec2(0, 0);
	camera.dim = glm::dvec2(16, 16); 
	camera.SCREEN_HEIGHT = SCREEN_HEIGHT; 
	camera.SCREEN_WIDTH = SCREEN_WIDTH; 

	vector<Particle> particles; 
	Gamestate gamestate = Gamestate(sprite_sheet, &particles); 

	gamestate.main_chunk.tiles[0].tile_id = 1; 
	gamestate.main_chunk.tiles[1].tile_id = 2; 
	gamestate.main_chunk.tiles[37].tile_id = 1; 

	//Main loop flag
	bool quit = false;

	//Event handler
	SDL_Event e;

	//Set text color as black
	SDL_Color textColor = { 0, 0, 0, 255 };

	//The frames per second timer
	LTimer fpsTimer;

	//Start counting frames per second
	int countedFrames = 0;
	int countedUpdates = 0; 

	fpsTimer.start();

	Uint32 t = 0; 
	const Uint32 dt = TICKS_PER_UPDATE;

	Uint32 mStartTicks = SDL_GetTicks();
	Uint32 accumulator = 0;

	Uint32 currentTime = 0; 

	vector<BlockIndices> block_indices; 

	//Main player is first entry in gamestate.player_data
	Entity player = {entity_id:0, pos: glm::dvec2(5, 5), vel: glm::dvec2(0, 0), dim: glm::dvec2(0.5, 0.5), num_contacts:0}; 
	player.entity_id = gamestate.push_player();
	gamestate.entities[gamestate.entity_map[player.entity_id]] = player; 

	player_sprite = sprite_sheet->getSpriteEntry("player_dot"); 

	glm::dvec2 firefly_pos = glm::dvec2(8, 8); 
	int firefly_id = gamestate.push_firefly(firefly_pos); 

	//While application is running
	while(!quit ) {
		Uint32 newTime = SDL_GetTicks();
		Uint32 frameTime = newTime - currentTime; 
		currentTime = newTime; 

		accumulator += frameTime; 

		// Game updates //////////////////////////////////////////////////////////
		while (accumulator*UPDATES_PER_SECOND > TICKS_PER_SECOND) {
			int pid = gamestate.player_data[0].entity_id; 
			Entity *p = &gamestate.entities[pid]; 

			// printf("getting inputs\n"); 
			InputState curr_input = getSDLInputs(gamestate.player_data[gamestate.player_map[0]].inp); 
			quit = curr_input.quit; 
			updateInputs(curr_input, &gamestate.player_data[gamestate.player_map[0]]);

			gamestate.hurtboxes.clear(); 
			gamestate.hitboxes.clear(); 
			gamestate.hits.clear(); 

			//Step
			// Place blocks
			// Filter tile contacts
			// Apply input accelerations. 
			// Apply contact contraints. 
			// Calculate candidate positions. 
			// Detect and resolve collisions. 
			// Generate particles

			// printf("getting blocks\n"); 
			//Place blocks
			PlayerData *pd = &gamestate.player_data[gamestate.player_map[0]];
			pd->fire_cooldown -= 1; 

			if(pd->inp.right_mouse_down) {
				glm::dvec2 mouse_s = toPoint(pd->prev_inp.mouse_pos, camera); 
				glm::dvec2 mouse_e = toPoint(pd->inp.mouse_pos, camera); 
				if(!pd->prev_inp.mouse_down) {
					mouse_e = mouse_s; 
				}
				block_indices.clear();
				listIntersectingSquares(mouse_s, mouse_e, &block_indices); 
				for (int i = 0; i < block_indices.size(); i++) {
					BlockIndices t = block_indices[i]; 
					if(t.row >= 0 && t.row < CHUNK_TILES && t.col >= 0 && t.col < CHUNK_TILES) {
						//printf("Placing tile %d, %d\n", t.row, t.col);
						gamestate.main_chunk.tiles[t.row*CHUNK_TILES + t.col].tile_id = 1; 
					}
				}
			}

			if(pd->inp.mouse_down && pd->fire_cooldown <= 0) {
				pd->fire_cooldown = 10; 
				glm::dvec2 mouse_e = toPoint(pd->inp.mouse_pos, camera); 
				glm::dvec2 dir = mouse_e - p->pos; 
				dir = 0.3 * dir / glm::length(dir); 
				glm::dvec2 fp = 4.0*dir + p->pos; 
				int fid = gamestate.push_fireball(fp, dir); //TODO Replace with queue to make sure pushes happen between frames. 
				AIData fb = gamestate.ai_data[gamestate.ai_map[fid]]; 
				printf("New fireball data\n id %d, eid %d, step %d, lifespan %d\n", fid, fb.entity_id, 
								fb.data.fa.step, fb.data.fa.lifespan);
			}
			// printf("len ai %d\n", gamestate.ai_data.size()); 
			for (int i = 0; i < gamestate.ai_data.size(); i++) {
				AIData *ad = &gamestate.ai_data[i]; 
				int eid = ad->entity_id; 
				if(gamestate.ai_map[eid] != i) {
					continue; 
				}
				int ai_type = ad->type; 
				// printf("AIData id %d, AI type %d\n", eid, ai_type); 
				if(ai_type == FIREBALL) {
					FireballAI *fb_a = &ad->data.fa; 
					Entity *e = &gamestate.entities[gamestate.entity_map[eid]]; 
					fb_a->step += 1; 
					Hurtbox h = {id: gamestate.hurtboxes.size(), parent_id: e->entity_id, pos: e->pos, dim: e->dim, weight: 1, power: fb_a->power};
					gamestate.hurtboxes.push_back(h);
					if(fb_a->step >= fb_a->lifespan) {
						// for (int i = 0; i < gamestate.ai_data.size(); i++) {
						// 	AIData *ad = &gamestate.ai_data[i]; 
						// 	int eid = ad->entity_id; 
						// 	printf("%d ", eid); 
						// }
						// printf("\n"); 
						// for (int i = 0; i < 10; i++) {
						// 	int ind = gamestate.ai_map[i];
						// 	printf("(%d, %d) ", i, ind);
						// }
						// printf("\n");
						gamestate.delete_entity(ad->entity_id, gamestate.entity_gen[ad->entity_id]); 
					}
				} else if (ai_type = FIREFLY) {
					Entity *e = &gamestate.entities[gamestate.entity_map[eid]]; 
					Hitbox h = {id: gamestate.hitboxes.size(), parent_id: e->entity_id, pos: e->pos, dim: e->dim}; 
					gamestate.hitboxes.push_back(h); 
				}
			}

			// printf("entity physics\n"); 
			//Physics loop
			for (int i = 0; i < gamestate.entities.size(); i++) {
				Entity *e = &gamestate.entities[i]; 
				int eid = e->entity_id; 
				if(gamestate.entity_map[eid] != i) {
					continue; 
				}
				filterTileContacts(e, &block_indices, &gamestate.main_chunk); 
			}

			for (int i = 0; i < gamestate.player_data.size(); i++) {
				PlayerData *p = &gamestate.player_data[i]; 
				int entity_id = p->entity_id; 
				Entity *e = &gamestate.entities[gamestate.entity_map[entity_id]]; 
				player_physics_update(e, p, &gamestate); 
			}
			
			//Tile physics solver for all entities. 
			for (int i = 0; i < gamestate.entities.size(); i++) {
				Entity *e = &gamestate.entities[i]; 
				int eid = e->entity_id; 
				if(gamestate.entity_map[eid] != i) {
					continue; 
				}
				tilePhysics(e, &block_indices, &gamestate.main_chunk); 
			}

			//Broadcast player hitbox
			Hitbox h = {id: 0, parent_id: pid, pos: p->pos, dim: p->dim}; 
			gamestate.hitboxes.push_back(h); 

			//Broadcast player hurtbox
			if(curr_input.j) {
				glm::dvec2 hurt_dim = glm::dvec2(2, 2); 
				glm::dvec2 hurt_pos = p->pos + glm::dvec2(1, 1); 
				Hurtbox h = {id: 0, parent_id: p->entity_id, pos: hurt_pos, 
				dim: hurt_dim, weight: 1, power: 3}; 
				gamestate.hurtboxes.push_back(h); 
			}

			// printf("running hitboxes\n"); 
			addHits(&gamestate.hurtboxes, &gamestate.hitboxes, &gamestate.hits); 
			// printf("Num hits %d\n", gamestate.hits.size()); 
			for (int i = 0; i < gamestate.hits.size(); i++) {
				Hit h = gamestate.hits[i]; 
				Hitbox hitbox = h.hitbox; 
				Hurtbox hurtbox = h.hurtbox; 

				int target_id = hitbox.parent_id; 
				int attacker_id = hurtbox.parent_id; 

				Entity *t = &gamestate.entities[gamestate.entity_map[target_id]]; 
				Entity *a = &gamestate.entities[gamestate.entity_map[attacker_id]];

				glm::dvec2 hit_vel = a->vel + hurtbox.vel - t->vel; 
				glm::dvec2 delta_v = hit_vel * hurtbox.weight / t->mass; 
				a->vel -= delta_v; 
				t->vel += delta_v; 

				if(gamestate.health_map[target_id] > 0) {
					HealthData *h = &gamestate.health_data[gamestate.health_map[target_id]]; 
					h->health = std::max(0, (int) (h->health - hurtbox.power)); 
				}
			}

			for (int i = 0; i < gamestate.health_data.size(); i++) {
				HealthData *h = &gamestate.health_data[i]; 
				if(h->health >= h->max_health) {
					h->health = h->max_health; 
				} else if (h->health > h->max_health - h->buffer_health) {
					h->health += h->buffer_regen; 
				} else {
					h->health += h->health_regen; 
				}
			}

			//Filter and update particles. 
			int valid_particles = 0; 
			for (int i = 0; i < particles.size(); i++) {
				Particle p = particles[i]; 
				if(p.timestep < p.lifetime) {
					p.pos += p.vel; 
					if (p.gravity) {
						p.vel -= 0.02; 
					}
					p.timestep += 1; 
					particles[valid_particles] = p; 
					valid_particles += 1; 
				}
			}
			particles.resize(valid_particles); 

			countedUpdates ++; 
			accumulator -= TICKS_PER_UPDATE; 
		}

		// Render ////////////////////////////////////////////////////////////

		//Calculate and correct fps
		float avgFPS = (countedFrames + 1) / ( (fpsTimer.getTicks() + 1) / 1000.f );
		float avgUPS = (countedUpdates + 1) / ( (fpsTimer.getTicks() + 1) / 1000.f );
		

		//Clear screen
		SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
		SDL_RenderClear( gRenderer );

		//Render tiles in main_chunk. 
		for (int i = 0; i < 32*32; i++) {
			Tile t = gamestate.main_chunk.tiles[i];
			if(t.tile_id == 0) { //0 for empty tile. 
				continue; 
			} 
			SDL_Rect tile_source = TILE_LOCATION; 
			tile_source.x = TILE_PIXELS*(t.tile_id-1); //Assuming tiles are stored in a horizontal row, starting with first. 
			int row = i / 32;
			int col = i % 32; 
			glm::dvec2 tile_pos = glm::dvec2(gamestate.main_chunk.col*CHUNK_WIDTH + col*TILE_WIDTH, gamestate.main_chunk.row*CHUNK_WIDTH + row*TILE_WIDTH);
			glm::dvec2 tile_dim = glm::dvec2(TILE_WIDTH, TILE_WIDTH); 
			SDL_Rect tile_dest = toRect(tile_pos, tile_dim, camera); 

			SDL_RenderCopy(gRenderer, TILE_SHEET, &tile_source, &tile_dest);
		}

		// printf("rendering %d entities\n", gamestate.entities.size()); 
		//Render entities
		for (int i = 0; i < gamestate.entities.size(); i++) {
			//Entity body
			Entity *e = &gamestate.entities[i]; 
			int eid = e->entity_id; 
			if(gamestate.entity_map[eid] != i) {
				continue; 
			}
			// printf("entity id: %d\n", e->entity_id); 
			SDL_Rect sprite_dest = toRect(e->pos, e->dim, camera); 
			renderSprite(player_sprite, &sprite_dest, 0); 

			//Entity healthbar
			if(gamestate.health_map[e->entity_id] >= 0 && e->entity_id != gamestate.player_data[0].entity_id) {
				HealthData *h = &gamestate.health_data[gamestate.health_map[e->entity_id]]; 
				glm::dvec2 bar_dim = glm::dvec2(e->dim.x, e->dim.x / 8); 
				glm::dvec2 bar_pos = glm::dvec2(0, 0.1+e->dim.y); 
				SDL_Rect bar_dest = toRect(e->pos + bar_pos, bar_dim, camera); 
				renderHealthbar(h->health, h->max_health, bar_dest); 
			}
		}

		//Render player healthbar
		SDL_Rect bar_dest = {0, 0, 100, 12};
		HealthData *h = &gamestate.health_data[gamestate.health_map[gamestate.player_data[0].entity_id]]; 
		renderHealthbar(h->health, h->max_health, bar_dest); 


		//Render hitboxes
		SDL_SetRenderDrawColor(gRenderer, 0, 128, 0, 255);
		for (int i = 0; i < gamestate.hitboxes.size(); i++) {
			Hitbox *h = &gamestate.hitboxes[i]; 
			SDL_Rect hitbox_dest = toRect(h->pos, h->dim, camera); 
			SDL_RenderDrawRect(gRenderer, &hitbox_dest);
		}

		//Render hurtboxes
		SDL_SetRenderDrawColor(gRenderer, 128, 0, 0, 255);
		for (int i = 0; i < gamestate.hurtboxes.size(); i++) {
			Hurtbox *h = &gamestate.hurtboxes[i]; 
			SDL_Rect hurtbox_dest = toRect(h->pos, h->dim, camera); 
			SDL_RenderDrawRect(gRenderer, &hurtbox_dest);
		}

		//Render particles
		for (int i = 0; i < particles.size(); i++) {
			Particle p = particles[i]; 
			SDL_Rect particle_dest = toRect(p.pos, p.dim, camera); 
			
			int frame = (p.timestep / p.change_interval) % p.s.frames; 
			renderSprite(p.s, &particle_dest, frame); 
		}

		//Update screen
		SDL_RenderPresent( gRenderer );
		++countedFrames;


		// Delay //////////////////////////////////////////////////////////////////////////////////

		//If frame finished early
		int frameTicks = SDL_GetTicks() - currentTime; 
		if( frameTicks < SCREEN_TICK_PER_FRAME ) {
			//Wait remaining time
			SDL_Delay( SCREEN_TICK_PER_FRAME - frameTicks );
		}
	}

	SDL_DestroyTexture(TILE_SHEET);

	//Free resources and close SDL
	close_SDL();

	return 0;
}