/* Rescue hazards: a trapped citizen needs the rubble cleared with A first;
 * cleared rubble stays cleared across districts until a new game. */
#include "test_util.h"
int main(void){
 static AreliaGame g; int i;
 start_game(&g);
 game_debug_enter(&g, 1, CELLQ(13.5), CELLQ(14.4), 0);
 for(i = 0; i < g.ent_count; i++) if(g.ent[i].type == E_LOOTER) g.ent[i].alive = 0;
 assert(g.ent[1].type == E_RUBBLE && g.ent[2].type == E_CITIZEN && g.ent[2].param == 1);
 tap(&g, ARELIA_INPUT_B); assert(!(g.rescued & 2) && g.dialogue); close_dialogue(&g);
 /* Break the rubble: two strikes from beside it. */
 game_debug_enter(&g, 1, CELLQ(10.4), CELLQ(15.5), 256);
 for(i = 0; i < g.ent_count; i++) if(g.ent[i].type == E_LOOTER) g.ent[i].alive = 0;
 tap(&g, ARELIA_INPUT_A); steps(&g, 16, 0); assert(g.ent[1].alive && g.ent[1].hp == 1);
 tap(&g, ARELIA_INPUT_A); steps(&g, 16, 0); assert(!g.ent[1].alive && (g.gone[1] & 2));
 close_dialogue(&g);
 /* Leave and come back: the rubble is still gone. */
 game_debug_enter(&g, 0, CELLQ(7.5), CELLQ(10), 0);
 game_debug_enter(&g, 1, CELLQ(13.5), CELLQ(14.4), 0);
 assert(!g.ent[1].alive);
 for(i = 0; i < g.ent_count; i++) if(g.ent[i].type == E_LOOTER) g.ent[i].alive = 0;
 tap(&g, ARELIA_INPUT_B); assert(g.rescued & 2);
 game_init(&g); assert(!g.gone[1] && !g.rescued);
 puts("Rescue hazard test passed");
 return 0;
}
