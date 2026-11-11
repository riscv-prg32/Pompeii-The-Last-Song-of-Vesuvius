/* The eruption advances by district; the ending plays and returns to the title. */
#include "test_util.h"
static uint32_t pal[256];
static unsigned sky_luma(const AreliaGame *g){unsigned i, s = 0; game_palette(g, pal); for(i = 0; i < 16; i++) s += (pal[i] >> 16 & 255) + (pal[i] >> 8 & 255) + (pal[i] & 255); return s;}
int main(void){
 static AreliaGame g; static uint8_t fb[ARELIA_RW * ARELIA_RH]; unsigned r, prev = 1u << 30, i;
 start_game(&g);
 for(r = 0; r < ARELIA_ROOMS; r++){
  game_debug_enter(&g, r, CELLQ(7.5), CELLQ(15), 0);
  assert(g.palette_phase == r);
  assert(sky_luma(&g) < prev); prev = sky_luma(&g);
 }
 /* Reaching the pier with everyone saved starts the ending. */
 game_debug_enter(&g, 3, CELLQ(7.5), CELLQ(2.5), 0); g.rescued = 15;
 for(i = 0; i < g.ent_count; i++) if(g.ent[i].type == E_LOOTER) g.ent[i].alive = 0;
 steps(&g, 30, ARELIA_INPUT_UP); assert(g.mode == MODE_WON && g.palette_phase == 4);
 for(i = 0; i < 200; i++){game_step(&g, i & 1 ? ARELIA_INPUT_START : 0); game_render(&g, fb);}
 assert(g.mode == MODE_WON && g.story >= 200);       /* START waits for the ending */
 steps(&g, 200, 0); tap(&g, ARELIA_INPUT_START);
 assert(g.mode == MODE_TITLE && !g.rescued && !g.story && !g.gone[0] && g.palette_phase == 0);
 /* Without everyone, the pier sends Livia back. */
 start_game(&g); game_debug_enter(&g, 3, CELLQ(7.5), CELLQ(2.5), 0); g.rescued = 7;
 for(i = 0; i < g.ent_count; i++) if(g.ent[i].type == E_LOOTER) g.ent[i].alive = 0;
 steps(&g, 30, ARELIA_INPUT_UP); assert(g.mode == MODE_PLAY && g.dialogue);
 puts("Eruption progression test passed");
 return 0;
}
