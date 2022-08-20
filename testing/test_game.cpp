
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <math.h>  
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#include "../timer.hpp"
#include "../game_world.hpp"
#include "../renderer.hpp"
#include "../particle.hpp"
#include "../combat.hpp"
#include "../inputs.hpp"



SDL_Rect TILE_LOCATION = {0, 0, TILE_PIXELS, TILE_PIXELS}; 
SDL_Rect SPRITE_LOCATION = {0, 0, 32, 32}; 
SpriteSheet *sprite_sheet; 
SDL_Texture *TILE_SHEET;

bool loadMedia() {
	TILE_SHEET = loadTextureFromFile("resources/tile_sheet.png"); 
	sprite_sheet = new SpriteSheet("resources/sprite_entries.json"); 
	return ~(TILE_SHEET == NULL || sprite_sheet == NULL); 
}

int main( int argc, char* args[] ) {
    SDL_Texture *TILE_SHEET = loadTextureFromFile("resources/tile_sheet.png"); 
    SpriteSheet *sprite_sheet = new SpriteSheet("resources/sprite_entries.json"); 
    std::vector<Particle> particles; 
	Gamestate gamestate = Gamestate(sprite_sheet, &particles); 
}


