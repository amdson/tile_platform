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

Chunk main_chunk; //Initialized to all zeros. 
Entity player; 
PlayerController player_controller {entity_id: player.id}; 
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
	main_chunk.tiles[0].tile_id = 1; 
	main_chunk.tiles[1].tile_id = 2; 
	main_chunk.tiles[37].tile_id = 1; 
	player.pos = glm::dvec2(5, 5); 
	player.vel = glm::dvec2(0, 0); 
	player.dim = glm::dvec2(0.5, 0.5); 
	player.num_contacts = 0; 
	// memset(&player.tile_contacts, 0, 16); 
	player_sprite = sprite_sheet->getSpriteEntry("player_dot"); 

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
	vector<Particle> particles; 
	vector<Entity> entities; 
	vector<Hitbox> hitboxes;
	vector<Hurtbox> hurtboxes; 


	//While application is running
	while(!quit ) {
		Uint32 newTime = SDL_GetTicks();
		Uint32 frameTime = newTime - currentTime; 
		currentTime = newTime; 

		accumulator += frameTime; 

		// Game updates //////////////////////////////////////////////////////////
		while (accumulator*UPDATES_PER_SECOND > TICKS_PER_SECOND) {
			InputState curr_input = getSDLInputs(player_controller.inp); 
			quit = curr_input.quit; 
			player_controller.updateInputs(curr_input);

			//Step
			// Place blocks
			// Filter tile contacts
			// Apply input accelerations. 
			// Apply contact contraints. 
			// Calculate candidate positions. 
			// Detect and resolve collisions. 
			// Generate particles

			//Place blocks
			if(player_controller.inp.mouse_down) {
				glm::dvec2 mouse_s = toPoint(player_controller.prev_inp.mouse_pos, camera); 
				glm::dvec2 mouse_e = toPoint(player_controller.inp.mouse_pos, camera); 
				if(!player_controller.prev_inp.mouse_down) {
					mouse_e = mouse_s; 
				}
				block_indices.clear();
				listIntersectingSquares(mouse_s, mouse_e, &block_indices); 
				for (int i = 0; i < block_indices.size(); i++) {
					BlockIndices t = block_indices[i]; 
					if(t.row >= 0 && t.row < CHUNK_TILES && t.col >= 0 && t.col < CHUNK_TILES) {
						//printf("Placing tile %d, %d\n", t.row, t.col);
						main_chunk.tiles[t.row*CHUNK_TILES + t.col].tile_id = 1; 
					}
				}
			}

			filterTileContacts(&player, &block_indices, &main_chunk); 

			//Player Step
			//Controls modify velocity. 
			player.vel = player_controller.applyControls(player.vel, &player.tile_contacts[0], player.num_contacts); 

			tilePhysics(&player, &block_indices, &main_chunk); 

			//Spawn hitboxes
			//Spawn hurtboxes
			//Find collisions
			if(player_controller.inp.j) {
				Hurtbox h = {hurtboxes.size(), player.id, player.pos, glm::dvec2(0.2, 0.4)};
				hurtboxes.push_back(h); 
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

			// Spawn particles
			MovementState s = player_controller.prev_state;
			MovementState e = player_controller.state; 
			if(s != e) {
				if(e == MovementState::AIR_JUMP) {
					glm::dvec2 cv = glm::dvec2(-0.05*player_controller.inp.x, 0.2*std::min(-player.vel.y, 0.0)); 
					Particle jump_cloud = {pos: player.pos - glm::dvec2(player.dim.x*0.5, player.dim.y+0.05), vel: cv, 
											dim: glm::dvec2(1, 1), s: sprite_sheet->getSpriteEntry("jump_cloud"),
											timestep: 0,
											change_interval: 8,
											lifetime: 23,
											gravity: false
					};
					particles.push_back(jump_cloud); 
				} else if(e == MovementState::GROUND_JUMP) {
					Particle jump_flash = {pos: player.pos - glm::dvec2(player.dim.x*0.5, player.dim.y+0.05), vel: glm::dvec2(0, 0), 
											dim: glm::dvec2(1, 1), s: sprite_sheet->getSpriteEntry("jump_flash"),
											timestep: 0,
											change_interval: 1,
											lifetime: 16,
											gravity: false
					};
					particles.push_back(jump_flash); 
				}
			}
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
			Tile t = main_chunk.tiles[i];
			if(t.tile_id == 0) { //0 for empty tile. 
				continue; 
			} 
			SDL_Rect tile_source = TILE_LOCATION; 
			tile_source.x = TILE_PIXELS*(t.tile_id-1); //Assuming tiles are stored in a horizontal row, starting with first. 
			int row = i / 32;
			int col = i % 32; 
			glm::dvec2 tile_pos = glm::dvec2(main_chunk.col*CHUNK_WIDTH + col*TILE_WIDTH, main_chunk.row*CHUNK_WIDTH + row*TILE_WIDTH);
			glm::dvec2 tile_dim = glm::dvec2(TILE_WIDTH, TILE_WIDTH); 
			SDL_Rect tile_dest = toRect(tile_pos, tile_dim, camera); 

			SDL_RenderCopy(gRenderer, TILE_SHEET, &tile_source, &tile_dest);
		}
		//Render player. 
		SDL_Rect sprite_dest = toRect(player.pos, player.dim, camera); 
		renderSprite(player_sprite, &sprite_dest, 0); 


		//Render particles
		for (int i = 0; i < particles.size(); i++) {
			Particle p = particles[i]; 
			SDL_Rect particle_dest = toRect(p.pos, p.dim, camera); 
			
			int frame = (p.timestep / p.change_interval) % p.s.frames; 
			// printf("particle timestep %d, frame %d, interval %d\n", p.timestep, frame, p.change_interval); 
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