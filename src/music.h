#ifndef ARELIA_MUSIC_H
#define ARELIA_MUSIC_H
#include <stdint.h>
typedef struct { uint16_t hz; uint8_t frames; uint8_t voice; } AreliaNote;
typedef struct { uint16_t position; uint8_t countdown; uint8_t last_scene; } AreliaMusic;
void arelia_music_init(AreliaMusic *m);
/* Returns 1 when a new original melodic note should be played. */
int arelia_music_step(AreliaMusic *m, unsigned scene, AreliaNote *out);
/* Sound effect for a game sound_event; returns 0 for none/unknown events. */
int arelia_sfx(unsigned event, AreliaNote *out);
/* Nearest equal-tempered MIDI note (A4=440 Hz=69) without floating point;
 * 0 Hz returns 0. Result is clamped to 0..127. */
uint8_t arelia_midi_from_hz(unsigned hz);
#endif
