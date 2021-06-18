#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <math.h>  
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <json.hpp>



#include "timer.hpp"
#include "game_world.hpp"

using namespace std; 
// for convenience
using json = nlohmann::json;

std::ostream& operator<< (std::ostream &out, glm::dvec2 data) {
    out << "(" << data.x << ', ' << data.y << ")";  
    return out;
}

std::ostream& operator<< (std::ostream &out, Collision c) {
    out << "Collusion: " << c.pos << " " << c.norm << " " << c.t; 
    return out;
}

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int SCREEN_FPS = 60;
const int SCREEN_TICK_PER_FRAME = 1000 / SCREEN_FPS;

const int UPDATES_PER_SECOND = 50;
const int TICKS_PER_UPDATE = 1000 / UPDATES_PER_SECOND; 
const int TICKS_PER_SECOND = 1000; 

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

SDL_Texture* loadTextureFromFile(char* path);

struct Sprite {
	int frames; 
	SDL_Rect r; //Location in texture. 
	SDL_Texture* texture; 
}; 

struct Particle {
	glm::dvec2 pos;
	glm::dvec2 vel; 
	glm::dvec2 dim; 
	Sprite s; 
	int timestep; //Timestep starting from creation. 
	int change_interval; 
	int lifetime; 
	bool gravity; 
};

void renderSprite(Sprite s, SDL_Rect r, int frame); 
void renderParticle(Particle p); 

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

SDL_Texture* loadTextureFromFile(char *path) {
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( path);
	if( loadedSurface == NULL ) {
		printf( "Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError() );
		return NULL; 
	}
	//Color key image
	SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0, 0xFF, 0xFF ) );

	//Create texture from surface pixels
	newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
	if( newTexture == NULL ) {
		printf( "Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError() );
		return NULL; 
	}
	//Get rid of old loaded surface
	SDL_FreeSurface( loadedSurface );
	return newTexture; 
}

SDL_Texture *TILE_SHEET; 
SDL_Texture *SPRITE_SHEET; 
SDL_Texture *DOT_SHEET; 

SDL_Rect TILE_LOCATION = {0, 0, TILE_PIXELS, TILE_PIXELS}; 
SDL_Rect SPRITE_LOCATION = {0, 0, 32, 32}; 

Chunk main_chunk; //Initialized to all zeros. 
Entity player; 
PlayerController player_controller {entity_id: player.id}; 

Camera camera; 


bool init() {
	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		printf( "SDL could not initialize! %s\n", SDL_GetError() );
		return false;
	}

	//Set texture filtering to linear
	if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) ) {
		printf( "Warning: Linear texture filtering not enabled!" );
	}
	
	//Create window
	gWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	if( gWindow == NULL ) {
		printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
		return false;
	}

	//Create renderer for window
	gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	if( gRenderer == NULL ) {
		printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	//Initialize renderer color
	SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

	//Initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	if( !( IMG_Init( imgFlags ) & imgFlags ) ) {
		printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
		return false;
	}

	//Initialize SDL_ttf
	if( TTF_Init() == -1 ) {
		printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
		return false;
	}
	return true;
}

bool loadMedia() {
	TILE_SHEET = loadTextureFromFile("resources/TileTextures.png"); 
	SPRITE_SHEET = loadTextureFromFile("resources/sprite_sheet.png"); 
	DOT_SHEET = loadTextureFromFile("resources/dot.bmp"); 
	return ~(TILE_SHEET == NULL || SPRITE_SHEET == NULL); 
}

void close()
{
	//Free loaded images
	SDL_DestroyTexture(TILE_SHEET);
	SDL_DestroyTexture(SPRITE_SHEET);

	//Destroy window	
	SDL_DestroyRenderer( gRenderer );
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
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
	vector<TileContact> tile_contacts; 
	vector<Particle> particles; 

	//While application is running
	while( !quit ) {
		Uint32 newTime = SDL_GetTicks();
		Uint32 frameTime = newTime - currentTime; 
		currentTime = newTime; 

		accumulator += frameTime; 

		// Game updates //////////////////////////////////////////////////////////

		while (accumulator*UPDATES_PER_SECOND > TICKS_PER_SECOND) {
			InputState curr_input = player_controller.inp; 
			//Handle events on queue
			while( SDL_PollEvent( &e ) != 0 ) {
				//User requests quit
				if( e.type == SDL_QUIT ) {
					quit = true;
				}
				//If a key was pressed
				else if( e.type == SDL_KEYDOWN && e.key.repeat == 0) {
					//Adjust the velocity
					switch( e.key.keysym.sym ) {
						case SDLK_UP: curr_input.y += 1; break;
						case SDLK_DOWN: curr_input.y += -1; break;
						case SDLK_LEFT: curr_input.x += -1; break;
						case SDLK_RIGHT: curr_input.x += 1; break;
						case SDLK_w: curr_input.y += 1; break;
						case SDLK_s: curr_input.y += -1; break;
						case SDLK_a: curr_input.x += -1; break;
						case SDLK_d: curr_input.x += 1; break;
					}
				}
				//If a key was released
				else if( e.type == SDL_KEYUP && e.key.repeat == 0) {
					//Adjust the velocity
					switch( e.key.keysym.sym ) {
						case SDLK_UP: curr_input.y -= 1; break;
						case SDLK_DOWN: curr_input.y -= -1; break;
						case SDLK_LEFT: curr_input.x -= -1; break;
						case SDLK_RIGHT: curr_input.x -= 1; break;
						case SDLK_w: curr_input.y -= 1; break;
						case SDLK_s: curr_input.y -= -1; break;
						case SDLK_a: curr_input.x -= -1; break;
						case SDLK_d: curr_input.x -= 1; break;
					}
				}
				else if(e.type == SDL_MOUSEMOTION) {
					curr_input.mouse_pos.x = e.motion.x;
					curr_input.mouse_pos.y = e.motion.y; 
				} 
				else if(e.type == SDL_MOUSEBUTTONDOWN) {
					switch(e.button.button) {
						case SDL_BUTTON_LEFT: curr_input.mouse_down = true; break; 
						case SDL_BUTTON_RIGHT: curr_input.right_mouse_down = true; break; 
					}
				} else if(e.type == SDL_MOUSEBUTTONUP) {
					switch(e.button.button) {
						case SDL_BUTTON_LEFT: curr_input.mouse_down = false; break; 
						case SDL_BUTTON_RIGHT: curr_input.right_mouse_down = false; break; 
					}
				}
			}
			player_controller.updateInputs(curr_input);

			//Step
			// Place blocks
			// Filter tile contacts
			// Apply input accelerations. 
			// Apply contact contraints. 
			// Calculate candidate positions. 
			// Detect and resolve collisions. 

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
			// printf("initial size %d\n", tile_contacts.size()); 
			//Filter contacts for existence.
			int valid_count = 0;  
			for (int i = 0; i < tile_contacts.size(); i++) {
				TileContact t = tile_contacts[i]; 
				bool is_valid = checkTileContact(player.pos, player.dim, t); 
				if(!is_valid) {
					block_indices.clear();
					listTileNeighborSquares(t.b, &block_indices); 
					for (int i = 0; i < block_indices.size(); i++) {
						BlockIndices b = block_indices[i]; 
						if(checkTileContactMaintained(player.pos, player.dim, t, b) && 
							main_chunk.tiles[b.row*CHUNK_TILES + b.col].tile_id > 0) {
							t.b = b; 
							is_valid = true; 
							break; 
						}
					}
				}
				if(is_valid) {
					tile_contacts[valid_count] = t; //Save contact for future loops. 
					valid_count += 1; 
				}
			}
			// printf("contact num %d\n", valid_count); 
			tile_contacts.resize(valid_count); 

			//Player Step
			//Controls modify velocity. 
			player.vel = player_controller.applyControls(player.vel, &tile_contacts); 

			//Constrain velocity with tile contacts. 
			for (int i = 0; i < tile_contacts.size(); i++) {
				TileContact t = tile_contacts[i]; 
				player.vel = getConstrainedSurfaceVel(player.vel, t.norm); 
			}
			
			//Tentative new position. 	
			player.newPos = player.pos + player.vel; 

			Collision fc; 
			fc.t = INFINITY; 
			BlockIndices contact_block; //Block first contact is with. 

			block_indices.clear(); 
			//List squares that could feasibly collide with player. 
			//TODO Make sure to handle edge case of player path missing blocks, but player block hitting them. 
			listIntersectingSquares(player.pos, player.newPos, &block_indices); 
			listIntersectingSquares(player.pos + player.dim, player.newPos + player.dim, &block_indices); 
			listNeighborSquares(player.pos, &block_indices); 

			//Iterate through blocks that are near player and check for collisions. 
			for (int i = 0; i < block_indices.size(); i++) {
				BlockIndices b = block_indices[i]; 
				if(b.row >= 0 && b.row < CHUNK_TILES && b.col >= 0 && b.col < CHUNK_TILES) {
					//Check that tile isn't already accounted for in contacts. 
					bool not_contact = true; 
					for (int i = 0; i < tile_contacts.size(); i++) {
						TileContact t = tile_contacts[i];
						if(t.b == b) {
							not_contact = false; 
							break; 
						}
					}
					if(not_contact && main_chunk.tiles[b.row*CHUNK_TILES + b.col].tile_id > 0) {
						//Handle collusion. 
						Collision c = getTileBoxCollision(b, player.pos, player.newPos, player.dim); 
						if(c.t < fc.t && c.t >= 0) {
							fc = c; 
							contact_block = b; 
							// cout << "Found collusion at time: " << fc.t << "\n"; 
							// printf("Player pos %f, %f\n", fc.pos.x, fc.pos.y); 						
						}
					}
				}
			}
			if(fc.t > 1 || fc.t < 0) {
				player.pos = player.newPos; 
			} else {
				double norm_vel = glm::dot(player.vel, fc.norm);
				player.pos = fc.pos; 
				if(norm_vel >= -0.3) {
					//Store contact to constrain motion. 
					TileContact col_cont = {fc.pos, fc.norm, contact_block, fc.s, true}; 
					tile_contacts.push_back(col_cont); //TODO give player control over contacts to maintain state better. 
					//Remove normal component of vel
					player.vel -= fc.norm * norm_vel; 
				} else {
					//Bounce vel. 
					player.vel -= 1.4*fc.norm * norm_vel; 
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

			// Spawn particles
			MovementState s = player_controller.prev_state;
			MovementState e = player_controller.state; 
			if(s != e) {
				if(e == MovementState::AIR_JUMP || e == MovementState::GROUND_JUMP) {
					Particle jump_cloud = {
						
					};
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
			//cout << tile_dest.x << " " << tile_dest.y <<  " " << tile_dest.w << " " << tile_dest.h << "\n"; 

			SDL_RenderCopy(gRenderer, TILE_SHEET, &tile_source, &tile_dest);
		}
		//Render player. 
		SDL_Rect sprite_dest = toRect(player.pos, player.dim, camera); 
		SDL_RenderCopy(gRenderer, DOT_SHEET, NULL, &sprite_dest); 



		//Render particles
		for (int i = 0; i < particles.size(); i++) {
			Particle p = particles[i]; 
			SDL_Rect particle_dest = toRect(p.pos, p.dim, camera); 
			int frame = (p.timestep / p.change_interval) % p.s.frames; 
			SDL_Rect particle_src = p.s.r;
			particle_src.x += frame * particle_src.w;  
			SDL_RenderCopy(gRenderer, SPRITE_SHEET, &particle_src, &particle_dest); 
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
	//Free resources and close SDL
	close();

	return 0;
}