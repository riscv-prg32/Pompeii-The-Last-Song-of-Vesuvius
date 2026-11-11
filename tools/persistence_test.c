/* Room persistence through the real exits: keys, doors and defeated looters
 * stay as left; walking through the doorways changes district. */
#include "test_util.h"
int main(void){
 static AreliaGame g; int i, n;
 start_game(&g);
 /* Take the Forum key by walking onto it. */
 game_debug_enter(&g, 0, CELLQ(11.5), CELLQ(18.5), 256);
 g.ent[3].alive = g.ent[4].alive = 0;
 steps(&g, 30, ARELIA_INPUT_UP); assert(g.keys == 1 && (g.gone[0] & 2)); close_dialogue(&g);
 /* Defeat a looter. */
 game_debug_enter(&g, 0, CELLQ(5.5), CELLQ(7.0), 512);
 for(n = 0; n < 40 && g.ent[3].alive && g.ent[3].state != AI_FALL; n++){tap(&g, ARELIA_INPUT_A); steps(&g, 14, 0);}
 assert(g.gone[0] & 8);
 /* Bumping the locked door with the key opens it; then north to the Villa. */
 game_debug_enter(&g, 0, CELLQ(7.5), CELLQ(1.6), 0); g.ent[4].alive = 0;
 steps(&g, 6, ARELIA_INPUT_UP); assert(!g.keys && g.ent[0].state == 1 && (g.gone[0] & 1));
 close_dialogue(&g);
 for(n = 0; n < 80 && g.room == 0; n++) game_step(&g, ARELIA_INPUT_UP);
 assert(g.room == 1 && g.pz > CELLQ(18));
 /* Back south through the doorway into the Forum's north end. */
 for(n = 0; n < 80 && g.room == 1; n++) game_step(&g, ARELIA_INPUT_DOWN);
 assert(g.room == 0 && g.pz < CELLQ(2));
 assert(!g.ent[0].alive && !g.ent[1].alive && !g.ent[3].alive && g.ent[2].alive);
 for(i = 0; i < 40; i++) game_step(&g, 0);
 /* Dying keeps the progress and restarts at the district entrance. */
 g.hp = 1; g.invuln = 0;
 g.mode = MODE_DEAD; g.story = 0; steps(&g, 40, 0); tap(&g, ARELIA_INPUT_START);
 assert(g.mode == MODE_PLAY && g.hp == ARELIA_HP_MAX && (g.gone[0] & 8) && !g.ent[3].alive);
 puts("Room persistence test passed");
 return 0;
}
