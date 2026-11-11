/* District maps and entity spawns of Pompeii (original level design). */
#include "world.h"

const char world_maps[WORLD_ROOMS][WORLD_H][WORLD_W + 1] = {
 { /* 0: the Forum: colonnades, a statue base, a mosaic and market stalls */
  "#######D########","#..............#","#.o.o.o..o.o.o.#","#..............#","##............##",
  "#o.....bb.....o#","#......bb......#","#o............o#","#..............#","#o....mmmm....o#",
  "#.....mmmm.....#","#o....mmmm....o#","#..............#","#o............o#","##............##",
  "#..............#","#.bb........bb.#","#.bb........bb.#","#..............#","#######S########"},
 { /* 1: the Painted Villa: garden, atrium with impluvium, mosaic rooms */
  "#######D########","#ggggggg.gggggg#","#go.o.o...o.o.g#","#ggggggg.gggggg#","#ggggggg.gggggg#",
  "####.###.###.###","#..............#","#.o..o....o..o.#","#.....~~~~.....#","#.....~~~~.....#",
  "#.o..o....o..o.#","#..............#","###.############","#mmm#mmmmmm#mmm#","#mmm#mmmmmm#mmm#",
  "#mmmmmmmmmmmmmm#","#mmm#mmmmmm#mmm#","######mmmm######","#.....mmmm.....#","#######S########"},
 { /* 2: the Waterworks: channels, bridges and the Sibyl's island */
  "#######D########","#..............#","#.~~~~~~=~~~~~.#","#.~..........~.#","#.~.oo....oo.~.#",
  "#.=....mm....=.#","#.~.oo....oo.~.#","#.~..........~.#","#.~~~~~~=~~~~~.#","#..............#",
  "####.######.####","#..............#","#.~~~~.~~.~~~~.#","#.~..~.~~.~..~.#","#.~..~.....~.~.#",
  "#..............#","##.##########.##","#..............#","#..............#","#######S########"},
 { /* 3: the Road to the Sea: tombs, the Sibyl's seal and the pier */
  "~~~~~~~~~~~~~~~~","~~~~~~~=~~~~~~~~","~~~~~~~=~~~~~~~~","#......=.......#","#..............#",
  "#######.########","#...#..........#","#.bb.bb..bb.bb.#","#.bb.bb..bb.bb.#","#..............#",
  "#..o..o..o..o..#","#..............#","#.bb.bb..bb.bb.#","#.bb.bb..bb.bb.#","#..............#",
  "#..............#","#.bbb......bbb.#","#..............#","#..............#","#######S########"}
};

/* Spawns in half cells. CITIZEN param: index of the rubble that traps them
 * (255: free). DOOR param 1: needs a bronze key. */
const WorldSpawn world_spawns[WORLD_ROOMS][WORLD_MAX_ENT] = {
 {{E_DOOR,15,1,1},{E_KEY,27,37,0},{E_CITIZEN,7,25,255},{E_LOOTER,11,15,3},{E_LOOTER,21,9,3}},
 {{E_DOOR,15,1,1},{E_RUBBLE,23,31,0},{E_CITIZEN,27,27,1},{E_KEY,5,3,0},{E_LOOTER,7,19,3},{E_LOOTER,25,17,3}},
 {{E_ALTAR,15,11,0},{E_RUBBLE,25,29,0},{E_CITIZEN,23,27,1},{E_LOOTER,11,35,3},{E_LOOTER,25,23,4}},
 {{E_SEAL,15,11,0},{E_RUBBLE,9,17,0},{E_CITIZEN,9,15,1},{E_BOAT,15,1,0},{E_LOOTER,23,21,4},{E_LOOTER,7,29,4}}
};

const char world_room_names[WORLD_ROOMS][16] = {"THE FORUM", "PAINTED VILLA", "WATERWORKS", "ROAD TO THE SEA"};
const char world_citizen_names[WORLD_ROOMS][8] = {"MARCUS", "FLAVIA", "FELIX", "TULLIA"};

char world_cell(unsigned room, int cx, int cz){
 if(room >= WORLD_ROOMS || cx < 0 || cz < 0 || cx >= WORLD_W || cz >= WORLD_H) return T_WALL;
 return world_maps[room][cz][cx];
}

int world_solid(unsigned room, int32_t x, int32_t z){
 int cx, cz; char t;
 if(x < 0 || z < 0) return 1;
 cx = (int)(x >> 8); cz = (int)(z >> 8);
 t = world_cell(room, cx, cz);
 switch(t){
  case T_WALL: case T_WATER: case T_LOW: return 1;
  case T_COLUMN: {int fx = (int)(x & 255) - 128, fz = (int)(z & 255) - 128;
   return fx > -72 && fx < 72 && fz > -72 && fz < 72;}
  case T_SOUTH: return room == 0;
  default: return 0;
 }
}
