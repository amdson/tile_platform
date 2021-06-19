#include "renderer.hpp"
#include <stdio.h>
#include <json.hpp>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

using json = nlohmann::json;

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
	SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0xFF, 0xFF, 0xFF ) );

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

SpriteSheet::SpriteSheet(char *path) {
	std::ifstream ifs(path);
	json j = json::parse(ifs); 
	// std::cout << j; 
	// std::cout << j["texture_path"] << "\n"; 
	std::string str = j["texture_path"].get<std::string>(); 
	char *texture_path = (char *) calloc(str.size(), sizeof(char)); 
	// printf("str size %d\n", str.size()); 
	for (int i = 0; i < str.size(); i++) {
		texture_path[i] = str[i]; 
	}
	
	// printf("loading from %s\n", texture_path); 
	texture = loadTextureFromFile(texture_path); 
	free(texture_path); 
	json sprite_j = j["sprite_entries"]; 
	// iterate the array
	for (auto& entry : sprite_j.items()) {
		std::string s = entry.key(); 
		json info = entry.value(); 
		int frames = info["frames"].get<int>(); 
		json location = info["location"]; 
		int ls[4];
		int i = 0; 
		for (auto& l : location) {
			ls[i] = l.get<int>(); 
			// printf("%d ", ls[i]); 
			i += 1; 
			// printf("\n"); 
		}
		SDL_Rect *r = (SDL_Rect*) &ls; 
		Sprite spr = {frames, *r, texture}; 
		spr.r.w /= spr.frames; 
		// std::cout << s << " " << frames << "\n"; 
		// printf("%d, %d, %d, %d\n", spr.r.x, spr.r.y, spr.r.w, spr.r.h); 
		entry_map[s] = spr; 
	}
} 

Sprite SpriteSheet::getSpriteEntry(std::string sprite_name) {
	return entry_map.at(sprite_name);
}

void renderSprite(Sprite s, SDL_Rect *r, int frame) {
	SDL_Rect src = s.r; 
	src.x += s.r.w * frame;
	SDL_RenderCopy(gRenderer, s.texture, &src, r); 
}

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

void close_SDL()
{
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