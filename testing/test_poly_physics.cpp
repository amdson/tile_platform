#include <stdio.h>
#include <glm/glm.hpp>
#include "../physics.hpp"
#include <SDL.h>
#include <iostream>
#include <vector>

typedef glm::dvec2 vec; 

struct Camera { //Used to convert units to pixels. 
	glm::dvec2 pos; 
	glm::dvec2 dim; 
	int SCREEN_WIDTH; 
	int SCREEN_HEIGHT; 
	int player_id; 
}; 

SDL_Point toPixelPoint(glm::dvec2 p, Camera c) {
	glm::dvec2 frame_pos = p - c.pos;
	int x = (int) ((frame_pos.x / c.dim.x) * ((double) c.SCREEN_HEIGHT)); 
	int y = c.SCREEN_HEIGHT - (int) ((frame_pos.y / c.dim.x) * ((double) c.SCREEN_HEIGHT));
	SDL_Point r = {x, y};
	return r; 
}

void draw_poly(PhysicsPolygon poly, Camera cam, SDL_Renderer* renderer) {
    for (int i = 0; i < poly.num_vertices; i++) {
        vec s0 = poly.vertices[i]; vec s1 = poly.vertices[(i+1) % poly.num_vertices];
        SDL_Point p0 = toPixelPoint(s0 + poly.pos, cam);
        SDL_Point p1 = toPixelPoint(s1 + poly.pos, cam);
        printf("Drawing p0: (%d, %d) -> p1: (%d, %d)\n", p0.x, p0.y, p1.x, p1.y); 
        SDL_RenderDrawLine(renderer, p0.x, p0.y, p1.x, p1.y);
    }
}

void draw_rect(Rect rect, Camera cam, SDL_Renderer* renderer) {
    vec rv[4]; //Rectangle corners
    rv[0] = rect.pos; rv[1] = rect.pos + vec(rect.dim.x, 0);
    rv[2] = rect.pos + rect.dim; rv[3] = rect.pos + vec(0, rect.dim.y);
    for (int i = 0; i < 4; i++) {
        vec s0 = rv[i]; vec s1 = rv[(i+1) % 4];
        SDL_Point p0 = toPixelPoint(s0, cam);
        SDL_Point p1 = toPixelPoint(s1, cam);
        SDL_RenderDrawLine(renderer, p0.x, p0.y, p1.x, p1.y);
    }
}

void draw_arrow(vec a0, vec a1, Camera cam, SDL_Renderer* renderer) {
    vec delta = a1 - a0; 
    vec norm = vec(delta.y, -delta.x); 
    vec t0 = delta*0.9 + norm*0.1 + a0;
    vec t1 = delta*0.9 - norm*0.1 + a0; 
    SDL_Point p0 = toPixelPoint(a0, cam);
    SDL_Point p1 = toPixelPoint(a1, cam);
    SDL_Point pt0 = toPixelPoint(t0, cam);
    SDL_Point pt1 = toPixelPoint(t1, cam); 
    SDL_RenderDrawLine(renderer, p0.x, p0.y, p1.x, p1.y);
    SDL_RenderDrawLine(renderer, p1.x, p1.y, pt1.x, pt1.y);
    SDL_RenderDrawLine(renderer, p1.x, p1.y, pt0.x, pt0.y);
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("Prueba", 0, 0, 960, 540, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL) {
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    Camera cam = {pos: glm::dvec2(0, 0), dim: glm::dvec2(10, 6), SCREEN_WIDTH: 1000, SCREEN_HEIGHT: 600}; 

    SDL_RenderClear(ren);

    SDL_Color color = {.r = 255, .g = 255, .b = 255, .a = 255 };
    PhysicsPolygon poly = {pos: glm::dvec2(5.0, 5.0)}; 
    glm::dvec2 v[] = {glm::dvec2(0, 0), glm::dvec2(1.0, 1.0), glm::dvec2(1, 2), glm::dvec2(0, 3), glm::dvec2(-1, 2), glm::dvec2(-1, 1)};
    memcpy(&poly.vertices, v, 6*sizeof(glm::dvec2)); 
    poly.num_vertices = 6; 

    Rect rect = {pos: vec(6.5, 5.1), dim: vec(1, 1)}; 

    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);  
    draw_poly(poly, cam, ren); 
    draw_rect(rect, cam, ren); 

    vec p2 = poly.pos + vec(1, 0); 
    {
        Collision c; 
        bool result = polygon_rectangle_update(&poly, poly.pos, p2, rect, &c); 
        printf("%d, pos: %f, %f, time: %f\n", result, c.pos.x, c.pos.y, c.t); 
        poly.pos += (p2-poly.pos)*c.t;
        draw_poly(poly, cam, ren); 
        draw_arrow(c.pos, c.pos+c.norm, cam, ren); 
    }

    rect.pos = vec(5, 0);
    poly.pos = vec(5, 1.5);
    p2 = poly.pos + vec(0, -1); 
    draw_poly(poly, cam, ren); 
    draw_rect(rect, cam, ren); 

    Collision c; 
    bool result = polygon_rectangle_update(&poly, poly.pos, p2, rect, &c); 
    printf("%d, pos: %f, %f, time: %f\n", result, c.pos.x, c.pos.y, c.t); 
    poly.pos += (p2-poly.pos)*c.t;
    draw_poly(poly, cam, ren); 
    draw_arrow(c.pos, c.pos+c.norm, cam, ren); 



    SDL_RenderPresent(ren);

    getchar();

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}