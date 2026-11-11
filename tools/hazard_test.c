#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
/* Regression test for rescuing trapped residents and room persistence. */
int main(void){
 AreliaGame g; game_init(&g);game_step(&g,ARELIA_INPUT_SELECT);
 /* Enter the villa using the normal north-door transition. */
 g.x=9;g.y=1;for(int k=0;k<7;k++)game_step(&g,0);game_step(&g,ARELIA_INPUT_UP);
 assert(g.room==1 && g.tiles[9][10]==7);
 /* A blocked citizen cannot be rescued before the rubble is cleared. */
 g.x=11;g.y=10;g.face=0;game_step(&g,0);game_step(&g,ARELIA_INPUT_ACTION);
 assert(g.rescued_count==0 && g.dialogue==13);
 /* Strike the fallen masonry from below; the obstacle is now cleared. */
 g.x=9;g.y=9;g.face=3;game_step(&g,0);game_step(&g,ARELIA_INPUT_SWORD);
 assert(g.cleared_hazards[1] && g.tiles[9][10]==0);
 /* Return to Forum and revisit Villa to ensure the hazard stays cleared. */
 g.x=9;g.y=10;for(int k=0;k<7;k++)game_step(&g,0);game_step(&g,ARELIA_INPUT_DOWN);
 assert(g.room==0);
 g.x=9;g.y=1;for(int k=0;k<7;k++)game_step(&g,0);game_step(&g,ARELIA_INPUT_UP);
 assert(g.room==1 && g.tiles[9][10]==0);
 g.x=11;g.y=10;g.face=0;game_step(&g,0);game_step(&g,ARELIA_INPUT_ACTION);
 assert(g.rescued_count==1 && (g.crystals&2));
 puts("Hazard rescue and persistence test passed");return 0;
}
