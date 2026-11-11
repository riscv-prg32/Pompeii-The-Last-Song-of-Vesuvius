#ifndef ARELIA_WORLD_H
#define ARELIA_WORLD_H
/* The four districts of Pompeii: 16x20 cell maps (north is row 0) and the
 * entities that live in them. Data only; no pointers (portable cartridges
 * carry no relocations). */
#include <stdint.h>

#define WORLD_W 16
#define WORLD_H 20
#define WORLD_ROOMS 4
#define WORLD_MAX_ENT 10
#define CELL 256                 /* Q8: one cell is one world unit */

/* Map cells. */
#define T_FLOOR '.'
#define T_WALL '#'
#define T_COLUMN 'o'
#define T_WATER '~'
#define T_BRIDGE '='
#define T_GRASS 'g'
#define T_MOSAIC 'm'
#define T_LOW 'b'                /* low block: statue base, tomb, basin rim */
#define T_DOOR 'D'               /* north doorway: leads to the next district */
#define T_SOUTH 'S'              /* south doorway: back to the previous district */

/* Entity types. */
enum { E_NONE, E_LOOTER, E_CITIZEN, E_RUBBLE, E_KEY, E_DOOR, E_ALTAR, E_SEAL, E_BOAT, E_LIFE };

typedef struct { uint8_t type, x2, z2, param; } WorldSpawn;  /* x2,z2 in half cells */

extern const char world_maps[WORLD_ROOMS][WORLD_H][WORLD_W + 1];
extern const WorldSpawn world_spawns[WORLD_ROOMS][WORLD_MAX_ENT];
extern const char world_room_names[WORLD_ROOMS][16];
extern const char world_citizen_names[WORLD_ROOMS][8];

char world_cell(unsigned room, int cx, int cz);
/* Static collision of the map at a Q8 point (walls, water, columns, blocks). */
int world_solid(unsigned room, int32_t x, int32_t z);
#endif
