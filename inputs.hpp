#ifndef INPUTS_H
#define INPUTS_H

#include <SDL.h>

struct InputState {
	int id; //To match input with corresponding player / entity. 
	int x;
	int y;
	SDL_Point mouse_pos; 
	bool mouse_down; 
	bool right_mouse_down; 
	bool j; 
    bool quit; 
}; 

InputState getSDLInputs(InputState s);

#endif