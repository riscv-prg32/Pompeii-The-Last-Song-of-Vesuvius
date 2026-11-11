/* Rescue dialogue, the Sibyl's altar, the song and the seal. */
#include "test_util.h"
int main(void){
 static AreliaGame g; int i, seal = -1;
 static const char *names[4] = {"MARCUS", "FLAVIA", "FELIX", "TULLIA"};
 for(i = 0; i < 4; i++) assert(strstr(game_dialogue_text(2 + i), names[i]) == game_dialogue_text(2 + i));
 /* Marcus is free: B in front of him rescues him. */
 start_game(&g); game_debug_enter(&g, 0, CELLQ(3.5), CELLQ(13.4), 0);
 g.ent[3].alive = g.ent[4].alive = 0;
 steps(&g, 2, 0); assert(g.prompt == 1);
 tap(&g, ARELIA_INPUT_B); assert(g.rescued == 1 && g.dialogue && strstr(game_dialogue_text(g.dialogue), "MARCUS"));
 close_dialogue(&g); tap(&g, ARELIA_INPUT_B); assert(g.rescued == 1);
 /* The altar: reading it raises the tibia; the song teaches the call. */
 game_debug_enter(&g, 2, CELLQ(7.5), CELLQ(6.5), 0);
 g.ent[3].alive = g.ent[4].alive = 0;
 tap(&g, ARELIA_INPUT_B); assert(g.dialogue && strstr(game_dialogue_text(g.dialogue), "ALTAR"));
 close_dialogue(&g); assert(g.action == ACT_TIBIA);
 tap(&g, ARELIA_INPUT_UP); tap(&g, ARELIA_INPUT_DOWN);                 /* a wrong note */
 tap(&g, ARELIA_INPUT_UP); tap(&g, ARELIA_INPUT_LEFT); tap(&g, ARELIA_INPUT_RIGHT);
 tap(&g, ARELIA_INPUT_UP); tap(&g, ARELIA_INPUT_LEFT); assert(!g.song_known);
 tap(&g, ARELIA_INPUT_RIGHT); assert(g.song_known && g.action == ACT_NONE && g.dialogue);
 close_dialogue(&g);
 /* The seal on the Road to the Sea sinks for the song, and only for it. */
 game_debug_enter(&g, 3, CELLQ(7.5), CELLQ(7.2), 0);
 for(i = 0; i < g.ent_count; i++){if(g.ent[i].type == E_SEAL) seal = i; if(g.ent[i].type == E_LOOTER) g.ent[i].alive = 0;}
 assert(seal >= 0);
 steps(&g, 30, ARELIA_INPUT_UP); assert(g.pz > CELLQ(6));          /* blocked */
 tap(&g, ARELIA_INPUT_TIBIA);
 tap(&g, ARELIA_INPUT_UP); tap(&g, ARELIA_INPUT_LEFT); tap(&g, ARELIA_INPUT_RIGHT);
 tap(&g, ARELIA_INPUT_UP); tap(&g, ARELIA_INPUT_LEFT); tap(&g, ARELIA_INPUT_RIGHT);
 assert(g.ent[seal].state == 1); close_dialogue(&g); steps(&g, 70, 0);
 assert(!g.ent[seal].alive);
 steps(&g, 40, ARELIA_INPUT_UP); assert(g.pz < CELLQ(5));
 puts("Narrative, altar and seal test passed");
 return 0;
}
