/* Audio: every sound event has a phrase, the tibia notes are distinct, every
 * scene has music; video: every frame uses valid palette ramps. */
#include "test_util.h"
#include "../src/music.h"
int main(void){
 static AreliaGame g; static uint8_t fb[ARELIA_RW * ARELIA_RH]; static uint32_t pal[256];
 AreliaNote n; AreliaSfx s; AreliaMusic m; unsigned e, k, notes, i;
 for(e = SND_KEY; e <= SND_TALK; e++){
  assert(arelia_sfx(e, &n) && n.hz >= 60 && n.hz < 2000 && n.frames);
  arelia_sfx_start(&s, e); notes = 0;
  for(k = 0; k < 80; k++) if(arelia_sfx_step(&s, &n)) notes++;
  assert(notes >= 1 && notes <= 6 && !s.event);
 }
 assert(!arelia_sfx(0, &n) && !arelia_sfx(99, &n));
 for(e = 0; e < 5; e++){assert(arelia_tibia_hz(e)); for(k = 0; k < e; k++) assert(arelia_midi_from_hz(arelia_tibia_hz(e)) != arelia_midi_from_hz(arelia_tibia_hz(k)));}
 for(e = 0; e < 4; e++){
  arelia_music_init(&m); notes = 0;
  for(k = 0; k < 600; k++) if(arelia_music_step(&m, e, &n)) notes++;
  assert(notes > 20);
 }
 arelia_music_init(&m); for(k = 0; k < 100; k++) assert(!arelia_music_step(&m, 4, &n));
 /* The interface colours stay fixed while the world darkens. */
 game_init(&g); game_palette(&g, pal); k = pal[242]; g.palette_phase = 4; game_palette(&g, pal); assert(pal[242] == k);
 /* Frames are mostly scene, not a blank screen. */
 start_game(&g); steps(&g, 10, ARELIA_INPUT_UP); game_render(&g, fb);
 for(i = 0, notes = 0; i < sizeof(fb); i++) if(fb[i] < 240) notes++;
 assert(notes > sizeof(fb) * 8 / 10);
 puts("Audio and video test passed");
 return 0;
}
