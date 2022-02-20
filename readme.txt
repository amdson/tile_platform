Outline
----------------------------------------
Player Actions
Players move and jump with the arrow keys, place blocks by right clicking, and attack by left-clicking. 

Block Access
Each block is contained in a 32x32 chunk, and has a unique associated BlockIndices.
Each chunk is identified by a row and column, and can be dynamically loaded by a block engine.
At all times, the nine chunks directly surrounding the player are loaded. 

Physics Engine
Input: Entities + Contacts + Tiles
Modifies: Entity positions, velocities. 
Output: Collisions, Contacts

Step
Place blocks
Filter tile contacts
Apply input accelerations. 
Apply contact contraints. 
Calculate candidate positions. 
Detect and resolve collisions. 
Spawn hitboxes
Spawn hurtboxes
Detect hit events
Handle hit events
Spawn particles

This project is linked against:
----------------------------------------
Windows:
SDL2
SDL2main
SDL2_image
SDL2_ttf
