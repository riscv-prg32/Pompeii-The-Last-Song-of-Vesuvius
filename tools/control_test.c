#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 AreliaGame g;
 game_init(&g);
 assert(g.mode==0);
 game_step(&g,ARELIA_INPUT_SWORD);
 assert(g.mode==0); /* A does not select the title */
 game_step(&g,0);
 game_step(&g,ARELIA_INPUT_SELECT);
 assert(g.mode==1); /* START is Select */
 game_step(&g,0);
 g.face=0;
 game_step(&g,ARELIA_INPUT_SWORD);
 assert(g.attack>0); /* A attacks */
 game_step(&g,0);
 g.x=9;g.y=3;g.face=0;
 game_step(&g,ARELIA_INPUT_ACTION);
 assert(g.rescued_count==1); /* B interacts */
 game_step(&g,0);
 game_step(&g,ARELIA_INPUT_MUSIC);
 assert(g.rescued_count==1); /* C remains reserved */
 puts("Controller mapping test passed");
 return 0;
}
