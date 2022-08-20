#include "../game_world.hpp"
#include <SDL.h>

void populate_ecs(RollbackECS *ecs) {
	for (int i = 0; i < 3; i++) {
		int pid = ecs->push_player(); 
		HealthData *h = &ecs->health_data[ecs->health_map[pid]]; 
		h->health = 100+i; 
		int eid = ecs->push_fireball(glm::dvec2(0, 0), glm::dvec2(1, 1)); 
	}
}

void print_ecs(RollbackECS *ecs) {
	printf("ECS Players\n"); 
	for (int i = 0; i < ecs->player_data.size(); i++) {
		printf("Index %d: ", i); 
		PlayerData p = ecs->player_data[i]; 
		if(ecs->player_map[p.entity_id] == i) {
			HealthData h = ecs->health_data[ecs->health_map[p.entity_id]]; 
			printf("PID %d, cooldown %d, health %d, ref index %d\n", 
					p.entity_id, p.fire_cooldown, h.health, ecs->player_map[p.entity_id]);
		} else {
			printf("Deleted\n"); 
		}
	}
	printf("ECS AI\n"); 
	for (int i = 0; i < ecs->ai_data.size(); i++) {
		printf("Index %d: ", i); 
		AIData p = ecs->ai_data[i]; 
		if(ecs->ai_map[p.entity_id] == i) {
			Entity e = ecs->entities[ecs->entity_map[p.entity_id]]; 
			printf("AI ID %d, ref index %d, y-pos %f\n", p.entity_id, ecs->ai_map[p.entity_id], e.pos.y);
		} else {
			printf("Deleted\n"); 
		}
	}
	printf("Used IDs\n["); 
	for (int i = 0; i < ecs->ids.size(); i++) {
		printf("%d, ", ecs->ids[i]);
	}
	printf("]\n");
	printf("Freed IDs\n[");
	for (int i = 0; i < ecs->free_ids.size(); i++) {
		printf("%d, ", ecs->free_ids[i]);
	}
	printf("]\n");
}

int main( int argc, char* args[] ) {
	RollbackECS ecs = RollbackECS(16); 
	printf("Generating entities\n\n\n"); 
	populate_ecs(&ecs); 
	print_ecs(&ecs); 


	uint32_t mStartTicks = SDL_GetTicks();
	for (int i = 0; i < 128000; i++) {
		for (int j = 0; j < ecs.ai_data.size(); j++) {
			AIData f = ecs.ai_data[j]; 
			if(ecs.ai_map[f.entity_id] == j) {
				// printf("Deleting %d\n", f.entity_id); 
				ecs.delete_entity(f.entity_id); 
				break; 
			}
		}
		ecs.push_fireball(glm::dvec2(0, 10*i), glm::dvec2(1, 1)); 

		ecs.roll_save(); 

		// printf("\nEntity Data Roll %d\n", i); 
		// print_ecs(&ecs); 
		// for (int j = 0; j < ecs.ai_data.size(); j++) {
		// 	AIData f = ecs.ai_data[0]; 
		// 	printf("Index %d\n", j); 
		// 	if(ecs.ai_map[f.entity_id] < 0) {
		// 		continue; 
		// 	}
		// 	AIData fa = ecs.ai_data[ecs.ai_map[f.entity_id]];
		// 	Entity e = ecs.entities[ecs.entity_map[f.entity_id]]; 
		// 	printf("EID %d, %d, AI type %d, y-pos %f\n", f.entity_id, fa.entity_id, f.type, e.pos.y); 
		// }
	}
	uint32_t endTicks = SDL_GetTicks(); 
	printf("Finished rollout in %d\n",  endTicks - mStartTicks); 
	print_ecs(&ecs); 
}


	// printf("\nEntity Data\n"); 
	// for (int i = 0; i < ecs.player_data.size(); i++) {
	// 	PlayerData p = ecs.player_data[i]; 
	// 	int pid = p.entity_id; 
	// 	HealthData h = ecs.health_data[ecs.health_map[pid]]; 
	// 	printf("Player %d has health %d\n", p.entity_id, h.health); 
	// }
	// printf("\nDeleting Entities\n"); 
	// ecs.delete_entity(0); ecs.delete_entity(2); ecs.delete_entity(4); 
	
	// printf("\nEntity Data\n"); 
	// for (int i = 0; i < ecs.player_data.size(); i++) {
	// 	printf("Index %d\n", i); 
	// 	PlayerData p = ecs.player_data[i]; 
	// 	if(ecs.id_map[p.entity_id] < 0) {
	// 		continue; 
	// 	}
	// 	int pid = p.entity_id; 
	// 	HealthData h = ecs.health_data[ecs.health_map[pid]]; 
	// 	printf("Player %d has health %d\n", p.entity_id, h.health); 
	// }
	
	// printf("\nRoll Save\n"); 
	// ecs.roll_save(); 

	// printf("\nEntity Data\n"); 
	// for (int i = 0; i < ecs.player_data.size(); i++) {
	// 	printf("Index %d\n", i); 
	// 	PlayerData p = ecs.player_data[i]; 
	// 	if(ecs.id_map[p.entity_id] < 0) {
	// 		continue; 
	// 	}
	// 	int pid = p.entity_id; 
	// 	HealthData h = ecs.health_data[ecs.health_map[pid]]; 
	// 	printf("Player %d has health %d\n", p.entity_id, h.health); 
	// }