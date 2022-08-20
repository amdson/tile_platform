#include "physics.hpp"

//Intersect moving point with static segment. Return true if collision. 
bool point_segment_update(glm::dvec2 p0, glm::dvec2 p1, glm::dvec2 s0, glm::dvec2 s1, Collision *c) {
    glm::dvec2 delta = p1 - p0; 
    glm::dvec2 side = s1 - s0; 
    glm::dvec2 norm = glm::dvec2(side.y, -side.x); //90 degrees CW. Points out from points ordered CCW. 
    double dn = glm::dot(delta, norm); 
    if(dn == 0) { return false; }
    double t = glm::dot(norm, s0-p0) / dn; 
    if(t < 0 || t > 1) {return false;} 
    glm::dvec2 pos = p0+delta*t; 
    double segment_pos = glm::dot(pos, side); 
    if(t < c->t && segment_pos >= glm::dot(s0, side) && segment_pos <= glm::dot(s1, side)) {
        c->t = t; 
        c->pos = pos; 
        c->norm = side; 
        return true; 
    }
    return false; 
}

//Detects collisions from a polygon moving into a rectangle. Does not detect active collisions. 
bool polygon_rectangle_update(PhysicsPolygon poly, glm::dvec2 p0, glm::dvec2 p1, Rect r, Collision *c) {
    bool has_collision = false; 
    glm::dvec2 rv[4]; //Rectangle corners
    rv[0] = r.pos; rv[1] = r.pos + glm::dvec2(r.dim.x, 0);
    rv[2] = r.pos + r.dim; rv[3] = r.pos + glm::dvec2(0, r.dim.y);
    for (int i = 0; i < poly.num_vertices; i++) {
        glm::dvec2 s0 = poly.vertices[i]; glm::dvec2 s1 = poly.vertices[(i+1) % poly.num_vertices];
        for (int j = 0; j < 4; j++) {
             if(point_segment_update(rv[j]-p0, rv[j]-p1, s1, s0, c)) {
                 c->pos = rv[j]; //Special handling to convert point->segment to segment->point. 
             }
             has_collision = has_collision || point_segment_update(s0+p0, s0+p1, rv[j], rv[(j+1)%4], c); 
        }
    }
    return has_collision; 
}

glm::dvec2 getConstrainedSurfaceVel(glm::dvec2 v, glm::dvec2 norm) {
	double norm_vel = std::min(glm::dot(v, norm), 0.0); 
	v -= norm*norm_vel; 
	return v; 
}