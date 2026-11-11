#ifndef ARELIA_MUSIC_H
#define ARELIA_MUSIC_H
#include <stdint.h>
typedef struct { uint16_t hz; uint8_t frames; uint8_t voice; } AreliaNote;
typedef struct { uint16_t position; uint8_t countdown; uint8_t last_scene; } AreliaMusic;
/* A short sound-effect phrase: one note per step. */
typedef struct { uint8_t event, step, countdown; } AreliaSfx;
/* Scenes: 0 title, 1 play, 2 dead, 3 won, 4 silence (tibia raised). */
void arelia_music_init(AreliaMusic *m);
/* Returns 1 when a new original melodic note should be played. */
int arelia_music_step(AreliaMusic *m, unsigned scene, AreliaNote *out);
/* Start the phrase for a game sound_event (replaces the current one). */
void arelia_sfx_start(AreliaSfx *s, unsigned event);
/* Returns 1 when the phrase has a note to play this frame. */
int arelia_sfx_step(AreliaSfx *s, AreliaNote *out);
/* First note of an event's phrase; returns 0 for none/unknown events. */
int arelia_sfx(unsigned event, AreliaNote *out);
/* Pitch of tibia note 0..4 (UP, LEFT, RIGHT, DOWN, A). */
unsigned arelia_tibia_hz(unsigned note);
/* Nearest equal-tempered MIDI note (A4=440 Hz=69) without floating point;
 * 0 Hz returns 0. Result is clamped to 0..127. */
uint8_t arelia_midi_from_hz(unsigned hz);
#endif
