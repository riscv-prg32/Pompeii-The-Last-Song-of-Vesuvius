/* Controller contract: D-pad camera-relative move, A strike, B context,
 * C tibia, L focus, R roll, START. */
#include "test_util.h"
int main(void){
 static AreliaGame g; int z0, x0;
 game_init(&g); assert(g.mode == MODE_TITLE);
 tap(&g, ARELIA_INPUT_A); assert(g.mode == MODE_TITLE);
 tap(&g, ARELIA_INPUT_START); assert(g.mode == MODE_PLAY && g.dialogue);
 tap(&g, ARELIA_INPUT_A); assert(g.dialogue);            /* text still appearing */
 close_dialogue(&g); assert(!g.dialogue);
 /* UP walks away from the camera (north at the start). */
 z0 = g.pz; steps(&g, 20, ARELIA_INPUT_UP); assert(g.pz < z0 - 200);
 /* RIGHT turns and walks east relative to the camera. */
 x0 = g.px; steps(&g, 20, ARELIA_INPUT_RIGHT); assert(g.px > x0 + 150);
 /* A: strike, which locks movement for its duration. */
 tap(&g, ARELIA_INPUT_A); assert(g.action == ACT_STRIKE);
 steps(&g, 16, 0); assert(g.action == ACT_NONE);
 /* R: dodge roll, faster than walking. */
 game_debug_enter(&g, 0, CELLQ(7.5), CELLQ(17.5), 0);
 z0 = g.pz; tap(&g, ARELIA_INPUT_ROLL); assert(g.action == ACT_ROLL); steps(&g, 13, 0);
 assert(z0 - g.pz > 400);
 /* C raises the tibia: the D-pad plays notes instead of walking; B lowers it. */
 steps(&g, 4, 0); tap(&g, ARELIA_INPUT_TIBIA); assert(g.action == ACT_TIBIA);
 z0 = g.pz; tap(&g, ARELIA_INPUT_UP); assert(g.pz == z0 && g.note_count == 1 && g.notes[7] == 0);
 tap(&g, ARELIA_INPUT_LEFT); assert(g.notes[7] == 1 && g.note_count == 2);
 tap(&g, ARELIA_INPUT_B); assert(g.action == ACT_NONE);
 /* L focuses a looter in range; movement then circles it; L again releases. */
 game_debug_enter(&g, 0, CELLQ(5.5), CELLQ(9.5), 0);
 tap(&g, ARELIA_INPUT_FOCUS); assert(g.focus && g.ent[g.focus - 1].type == E_LOOTER);
 tap(&g, ARELIA_INPUT_FOCUS); assert(!g.focus);
 /* L with nothing near recenters the camera behind Livia. */
 game_debug_enter(&g, 0, CELLQ(10.5), CELLQ(18.5), 0); g.cam_yaw = 300; g.ent[3].alive = g.ent[4].alive = 0;
 tap(&g, ARELIA_INPUT_FOCUS); assert(!g.focus && g.cam_yaw == g.pyaw);
 puts("Controller contract test passed");
 return 0;
}
