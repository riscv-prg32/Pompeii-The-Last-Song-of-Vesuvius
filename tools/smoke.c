#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
static AreliaGame g;
static uint8_t fb[ARELIA_WIDTH*ARELIA_HEIGHT];
int main(void){int i;game_init(&g);assert(g.hp==6&&g.room==0);game_step(&g,ARELIA_INPUT_START);for(i=0;i<300;i++){game_step(&g,(i%40<20)?ARELIA_INPUT_RIGHT:ARELIA_INPUT_LEFT);game_render(&g,fb);}assert(g.hp<=6);assert(g.room<ARELIA_ROOMS);puts("Arelia smoke test passed");return 0;}
