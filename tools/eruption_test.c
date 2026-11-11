#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
static unsigned char frame[ARELIA_WIDTH*ARELIA_HEIGHT];
int main(void){AreliaGame g;unsigned i;game_init(&g);game_step(&g,ARELIA_INPUT_SELECT);g.won=1;g.eruption_frames=0;game_render(&g,frame);assert(frame[0]==1);for(i=0;i<190;i++)game_step(&g,0);assert(g.eruption_frames==190);game_render(&g,frame);assert(frame[20*ARELIA_WIDTH+20]==1);game_step(&g,ARELIA_INPUT_SELECT);assert(g.won==0&&g.eruption_frames==0);puts("Eruption progression test passed");return 0;}
