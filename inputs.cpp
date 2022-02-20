#include "inputs.hpp"

//Handle events on queue
InputState getSDLInputs(InputState curr_input) {
    SDL_Event e;
    while( SDL_PollEvent( &e ) != 0 ) {
        //User requests quit
        if( e.type == SDL_QUIT ) {
            curr_input.quit = true;
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
                case SDLK_j: curr_input.j = 1; break; 
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
                case SDLK_j: curr_input.j = 0; break; 
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
    return curr_input; 
}