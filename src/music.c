#include "music.h"
/* Original themes for the fictional Sibylline tibia, in Hz (0 = rest). */
static const uint16_t title_notes[] = {294,0,440,392,349,392,440,0,523,440,392,349,330,294,0,0,
 294,349,392,440,523,587,523,440,392,0,349,330,294,0,0,0};
/* Exploration: a D dorian walking line under the tremors. */
static const uint16_t play_notes[] = {294,0,349,392,440,0,392,349,330,349,392,0,294,0,220,0,
 294,0,349,392,440,523,440,392,349,330,294,0,262,0,294,0,
 392,0,440,494,523,0,494,440,392,349,330,0,294,0,0,0};
static const uint16_t death_notes[] = {392,370,330,294,262,220,196,0};
static const uint16_t win_notes[] = {294,349,440,587,0,523,440,587,0,698,659,587,523,587,0,0,
 440,523,587,659,587,523,440,392,440,0,294,0,0,0,0,0};
static const uint16_t tibia_hz[5] = {294, 349, 440, 587, 523};   /* D4 F4 A4 D5 C5 */

void arelia_music_init(AreliaMusic *m){m->position = 0; m->countdown = 0; m->last_scene = 255;}
int arelia_music_step(AreliaMusic *m, unsigned scene, AreliaNote *out){
 const uint16_t *melody; unsigned len, step;
 switch(scene){
  case 0: melody = title_notes; len = sizeof(title_notes) / 2; step = 9; break;
  case 1: melody = play_notes; len = sizeof(play_notes) / 2; step = 7; break;
  case 2: melody = death_notes; len = sizeof(death_notes) / 2; step = 12; break;
  case 3: melody = win_notes; len = sizeof(win_notes) / 2; step = 9; break;
  default: m->last_scene = (uint8_t)scene; return 0;
 }
 if(m->last_scene != scene){m->last_scene = (uint8_t)scene; m->position = 0; m->countdown = 0;}
 if(m->countdown){m->countdown--; return 0;}
 out->hz = melody[m->position % len]; out->frames = (uint8_t)(step - 1); out->voice = 1;
 m->position = (uint16_t)((m->position + 1) % len); m->countdown = (uint8_t)step;
 return out->hz != 0;
}

/* Phrases: up to 6 notes (Hz, frames); 0 Hz ends the phrase. Indexed by event. */
#define PHRASES 25
static const uint16_t phrase_hz[PHRASES][6] = {
 {0}, {523,659,784,1047,0}, {196,0}, {262,392,0}, {165,131,0}, {392,494,587,784,0}, {587,740,880,1175,0},
 {330,0}, {110,98,0}, {294,349,440,587,698,880}, {147,0}, {880,0}, {82,0}, {131,123,0}, {440,0},
 {0},{0},{0},{0},{0}, {294,0}, {349,0}, {440,0}, {587,0}, {523,0}
};
static const uint8_t phrase_frames[PHRASES] = {0,3,5,4,5,4,6, 3,6,4,4,2,8,5,2, 0,0,0,0,0, 9,9,9,9,9};

void arelia_sfx_start(AreliaSfx *s, unsigned event){
 if(!s) return;
 s->event = (uint8_t)(event < PHRASES && phrase_hz[event][0] ? event : 0); s->step = 0; s->countdown = 0;
}
int arelia_sfx_step(AreliaSfx *s, AreliaNote *out){
 unsigned e;
 if(!s || !s->event) return 0;
 if(s->countdown){s->countdown--; return 0;}
 e = s->event;
 if(s->step >= 6 || !phrase_hz[e][s->step]){s->event = 0; return 0;}
 out->hz = phrase_hz[e][s->step]; out->frames = phrase_frames[e]; out->voice = 0;
 s->step++; s->countdown = (uint8_t)(phrase_frames[e] - 1);
 return 1;
}
int arelia_sfx(unsigned event, AreliaNote *out){
 AreliaSfx s; arelia_sfx_start(&s, event);
 return out ? arelia_sfx_step(&s, out) : 0;
}
unsigned arelia_tibia_hz(unsigned note){return note < 5 ? tibia_hz[note] : 0;}

uint8_t arelia_midi_from_hz(unsigned hz){
 /* C4..C5 in 1/16 Hz; the 13th entry rounds up into the next octave. */
 static const uint16_t c4[13] = {4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902,8372};
 uint32_t h; int octave = 0, best = 0, i, note; uint32_t distance = 0xffffffffu;
 if(!hz) return 0;
 if(hz > 20000u) hz = 20000u;
 h = (uint32_t)hz * 16u;
 while(h < 4067u){h *= 2u; octave--;}
 while(h >= 8134u){h /= 2u; octave++;}
 for(i = 0; i < 13; i++){uint32_t d = h > c4[i] ? h - c4[i] : c4[i] - h; if(d < distance){distance = d; best = i;}}
 note = 60 + octave * 12 + best;
 if(note < 0) note = 0;
 if(note > 127) note = 127;
 return (uint8_t)note;
}
