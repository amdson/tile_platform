const double AIR_ACC = 0.01; 
const double GROUND_ACC = 0.04; 
const double GROUND_JUMP_VEL = 0.15; 
const double AIR_JUMP_VEL = 0.2; 

enum MovementState 
{   GROUND, GROUND_JUMP, JUMP1, AIR_JUMP, FALLING, WALL, WALL_JUMP, CROUCH
};

enum ActionState {
    BASE, BLOCK_PLACE, SLASH, DOWN_SLASH, STAB, SHOOT, SHIELD, SHIELD_BASH
};

struct Action {
    ActionState type; 
    int frames; 
    MovementState compatible[16]; 
    int num_compatible; 
}; 

struct PlayerState {
    MovementState movement_state; 
    ActionState action_state;
    int action_frame; 
}; 

/*
MovementStates (In order of priority), Pre-conditions, Conditions, Post-transition
GROUND (NULL | Ground contact) -> NULL
GROUND_JUMP (Near ground, JUMP_COMMAND | No contact) -> JUMP1
CROUCH (NULL | Ground contact) -> NULL
WALL (Active wall contact, Wall contact) -> WALL_JUMP
WALL_JUMP (Near wall, JUMP_COMMAND | No contact) -> JUMP1
FALLING (Failed | No ground contact) -> NULL
AIR_JUMP (JUMP1, JUMP_COMMAND | No ground contact) -> FALLING
JUMP1 (NULL | No ground contact) -> NULL

Movement Algorithm
- If current movement conditions violated.
    - Terminate current movement. 
- If current movement finishes.
    - Switch to post-transition movement. 
- Iterate through movements in order of priority. 
    - If pre-conditions met, or movement is active, set as current movement. 

ActionStates (In order of priority), Pre-conditions, Conditions
SHOOT (SHIFT & LEFT_CLICK | NULL)
SHIELD (SHIFT & RIGHT_CLICK & Shield free | Shield free)
SHIELD_BASH (SHIFT & RIGHT_CLICK & FORWARDSWEEP, SHIELD | NULL)
BLOCK_PLACE (Valid block, RIGHT_CLICK | Valid block)
DOWN_SLASH (DOWNSWEEP & LEFT_CLICK, No contact | No contact)
DOWN_STAB (DOWNSWEEP & LEFT_CLICKDOWN, No contact | No contact)
SLASH (FAST_CLICK | NULL)
STAP (FORWARDSWEEP, STAB_READY | NULL)
STAB_READY (STAB_PREP mature | LEFT_DOWN)
STAB_PREP (BACKSWEEP & LEFT_CLICK_DOWN & LEFT_DOWNCLICK | LEFT_DOWN)
BASE (NULL, NULL)

Modifiers
- Movement and action states modify player properties. 


Action Algorithm
- If current action conditions violated.
    - Terminate current action. 
- Iterate through actions in order of priority. 
    - If current input matches action conditions and action is not active, preempt active action. 



Ground contact: In contact with normal within 60 deg of vertical. 
Wall contact: In contact with normal between 60 and 100 deg from vertical. 
Active wall contact: Wall contact, and attempted movement into wall. 
No contact: No contact with anything. 
Near ground: Within buffer distance of potential ground contact. 
Near wall: Within buffer distance of potential wall contact. 
Valid block: Mouse over empty tile. Adjacent filled tile. Blocks in inventory. 
Shield free: Shield wasn't recently hit. 
*/