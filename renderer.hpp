#ifndef HEADERFILE_GAME_RENDERER
#define HEADERFILE_GAME_RENDERER

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <map>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//The window we'll be rendering to
extern SDL_Window* gWindow;
//The window renderer
extern SDL_Renderer* gRenderer;

//Starts up SDL and creates window
bool init(); 

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close_SDL();

SDL_Texture* loadTextureFromFile(char* path);

struct Sprite {
	int frames; 
	SDL_Rect r; //Location in texture. 
	SDL_Texture* texture; 
}; 

struct SpriteSheet {
	std::map<std::string, Sprite> entry_map; 
	SDL_Texture* texture; 
	SpriteSheet(char *path); 
	Sprite getSpriteEntry(std::string entry_name); 
};

void renderSprite(Sprite s, SDL_Rect *r, int frame); 

SDL_Texture* loadTextureFromFile(char *path); 

#endif