/* Run the full host frame loop with pseudo-random input and check bounds. */
#include "test_util.h"
#include "../src/platform.h"
static uint8_t fb[ARELIA_RW * ARELIA_RH + 64];
static unsigned seed = 12345, tones;
static unsigned rnd(void){seed = seed * 1103515245u + 12345u; return seed >> 16;}
static unsigned read_buttons(void *c){(void)c; return rnd() & 0x3ffu & ~(rnd() & 3 ? ARELIA_INPUT_START : 0);}
static void tone(void *c, unsigned hz, unsigned f, unsigned v){(void)c; (void)f; (void)v; assert(hz < 5000); tones++;}
int main(void){
 static AreliaGame g; static AreliaHost host; int i;
 memset(fb + ARELIA_RW * ARELIA_RH, 0xa5, 64);
 host.read_buttons = read_buttons; host.play_tone = tone;
 arelia_music_init(&host.music);
 game_init(&g);
 for(i = 0; i < 20000; i++){
  arelia_run_frame(&host, &g, fb);
  assert(g.room < ARELIA_ROOMS && g.hp <= ARELIA_HP_MAX && g.mode <= MODE_WON);
  assert(g.px > 0 && g.pz > 0 && g.px < 16 * 256 && g.pz < 20 * 256);
  assert(g.ent_count <= WORLD_MAX_ENT && g.lapilli_n <= 4);
 }
 for(i = 0; i < 64; i++) assert(fb[ARELIA_RW * ARELIA_RH + i] == 0xa5);
 assert(tones > 100);
 printf("Smoke test passed: 20000 random frames, room %u, %u tones\n", g.room, tones);
 return 0;
}
