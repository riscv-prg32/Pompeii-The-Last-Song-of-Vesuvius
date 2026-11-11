/* Level data and renderer invariants. */
#include "test_util.h"
#include "../src/r3d.h"
static int solid_cell(unsigned r, int x, int z){char t = world_cell(r, x, z); return t == T_WALL || t == T_WATER || t == T_LOW || t == T_COLUMN;}
int main(void){
 static AreliaGame g; static uint8_t fb[ARELIA_RW * ARELIA_RH + 32]; unsigned r, i, k, yaw;
 for(r = 0; r < WORLD_ROOMS; r++){
  int citizens = 0;
  for(i = 0; i < WORLD_H; i++) assert(strlen(world_maps[r][i]) == WORLD_W);
  for(i = 0; i < WORLD_MAX_ENT; i++){
   const WorldSpawn *s = &world_spawns[r][i];
   if(s->type == E_NONE) break;
   if(s->type != E_BOAT) assert(!solid_cell(r, s->x2 / 2, s->z2 / 2));
   if(s->type == E_CITIZEN){
    citizens++;
    if(s->param != 255){
     const WorldSpawn *h = &world_spawns[r][s->param];
     assert(s->param < i && h->type == E_RUBBLE);
     assert(h->x2 / 2 != s->x2 / 2 || h->z2 / 2 != s->z2 / 2);   /* rubble never covers a citizen */
    }
   }
  }
  assert(citizens == 1);
  assert(world_cell(r, 7, 19) == T_SOUTH && (r == 3 || world_cell(r, 7, 0) == T_DOOR));
 }
 /* Every dialogue line fits the box and uses glyphs the font has. */
 for(i = 1; game_dialogue_text(i)[0] || i < 6; i++){
  const char *s = game_dialogue_text(i); unsigned col = 0;
  if(i > 40) break;
  for(k = 0; s[k]; k++){
   if(s[k] == '|'){col = 0; continue;}
   assert(s[k] >= 32 && s[k] <= 90 && ++col <= 37);
  }
 }
 /* The renderer stays inside the frame from any camera in any district. */
 memset(fb + ARELIA_RW * ARELIA_RH, 0x5a, 32);
 start_game(&g);
 for(r = 0; r < WORLD_ROOMS; r++) for(k = 0; k < 40; k++){
  int x = 300 + (int)(k * 911 % 3500), z = 300 + (int)(k * 577 % 4600);
  if(solid_cell(r, x >> 8, z >> 8)) continue;
  for(yaw = 0; yaw < 1024; yaw += 128){game_debug_enter(&g, r, x, z, (int)yaw); g.ticks = k * 13; game_render(&g, fb);}
 }
 for(i = 0; i < 32; i++) assert(fb[ARELIA_RW * ARELIA_RH + i] == 0x5a);
 assert(sizeof(AreliaGame) == 256 && sizeof(AreliaEntity) == 16);
 puts("Level and renderer regression test passed");
 return 0;
}
