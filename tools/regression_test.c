/* Regressions found while playing the cartridge on PRG32 QEMU (Milestone 14). */
#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void enter_next_room(AreliaGame *g){int k;g->x=9;g->y=1;for(k=0;k<7;k++)game_step(g,0);game_step(g,ARELIA_INPUT_UP);game_step(g,0);}
static int count_tiles(const AreliaGame *g,uint8_t t){int x,y,n=0;for(y=0;y<12;y++)for(x=0;x<20;x++)n+=g->tiles[y][x]==t;return n;}
int main(void){
 static AreliaGame g;static uint8_t fb[ARELIA_WIDTH*ARELIA_HEIGHT];int r,x,y,ink;
 /* "POMPEII AD 79": the digits 7 and 9 must be drawn (they were blank glyphs). */
 game_init(&g);game_render(&g,fb);
 for(ink=0,y=85;y<95;y++)for(x=176;x<192;x++)ink+=fb[y*ARELIA_WIDTH+x]==1;
 assert(ink>10);
 game_step(&g,ARELIA_INPUT_SELECT);game_step(&g,0);
 /* Every district has exactly one citizen and (districts 1-3) one rubble pile;
  * the Waterworks rubble once replaced its citizen, making the game unwinnable.
  * Each district announces its own name on entry. */
 for(r=0;r<ARELIA_ROOMS;r++){
  if(r)enter_next_room(&g);
  assert(g.room==r);
  assert(count_tiles(&g,8)==1);
  assert(count_tiles(&g,7)==(r==0?2:1)); /* plus the Forum's two scenery piles */
  if(r){assert(g.dialogue==8+r);}
 }
 /* Citizens block movement, so pressing toward one turns Livia to face them. */
 game_init(&g);game_step(&g,ARELIA_INPUT_SELECT);game_step(&g,0);
 g.x=9;g.y=3;g.face=1;g.move_cooldown=0;game_step(&g,ARELIA_INPUT_UP);
 assert(g.x==9&&g.y==3&&g.face==0);
 game_step(&g,0);game_step(&g,ARELIA_INPUT_ACTION);assert(g.crystals&1);
 /* Active enemies block movement too: pressing toward one only turns. */
 g.enemies[0].x=5;g.enemies[0].y=9;g.enemies[0].active=1;g.x=4;g.y=9;g.face=2;g.move_cooldown=0;g.enemy_cooldown=0;
 game_step(&g,ARELIA_INPUT_RIGHT);assert(g.x==4&&g.face==3);
 /* Tibia completion and arch opening have their own, truthful messages. */
 g.melody_active=1;g.melody_progress=2;game_step(&g,0);game_step(&g,ARELIA_INPUT_RIGHT);
 assert(g.notes==3&&g.dialogue==12);
 game_init(&g);game_step(&g,ARELIA_INPUT_SELECT);game_step(&g,0);
 g.tiles[9][10]=4;g.keys=1;g.x=9;g.y=9;g.move_cooldown=0;game_step(&g,ARELIA_INPUT_RIGHT);
 assert(g.x==10&&g.keys==0&&g.dialogue==7);
 puts("QEMU regression test passed");
 return 0;
}
