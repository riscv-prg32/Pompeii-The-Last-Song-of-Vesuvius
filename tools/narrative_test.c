#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
int main(void){AreliaGame g;game_init(&g);game_step(&g,ARELIA_INPUT_START);g.x=9;g.y=3;g.face=0;game_step(&g,ARELIA_INPUT_ACTION);assert(g.rescued_count==1);assert(g.crystals&1);game_step(&g,0);game_step(&g,ARELIA_INPUT_ACTION);assert(g.rescued_count==1);g.room=2;g.x=8;g.y=6;g.tiles[6][8]=5;game_step(&g,0);game_step(&g,ARELIA_INPUT_ACTION);assert(g.melody_active);game_step(&g,0);game_step(&g,ARELIA_INPUT_UP);game_step(&g,0);game_step(&g,ARELIA_INPUT_LEFT);game_step(&g,0);game_step(&g,ARELIA_INPUT_RIGHT);assert(g.notes==3&&!g.melody_active);puts("Narrative and tibia test passed");return 0;}
