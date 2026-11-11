#include "music.h"
/* Original pentatonic-leaning themes for the fictional Sibylline tibia; Hz. */
static const uint16_t title_notes[]={294,349,392,440,392,349,294,262,294,349,440,392,294,262,220,0};
static const uint16_t win_notes[]={523,659,784,1047,988,784,880,1047,0,1047,0,0};
/* In-game ostinato in D dorian: a walking tibia line under the tremors. */
static const uint16_t play_notes[]={294,0,349,392,440,0,392,349,330,349,392,0,294,0,220,0,
 294,0,349,392,440,523,440,392,349,330,294,0,262,0,294,0};
static const uint16_t death_notes[]={392,370,330,294,262,220,196,0};
void arelia_music_init(AreliaMusic *m){m->position=0;m->countdown=0;m->last_scene=255;}
int arelia_music_step(AreliaMusic *m,unsigned scene,AreliaNote *out){
 const uint16_t *melody=title_notes; unsigned len=sizeof(title_notes)/sizeof(title_notes[0]);
 if(scene==2){melody=death_notes;len=sizeof(death_notes)/sizeof(death_notes[0]);}
 else if(scene==3){melody=win_notes;len=sizeof(win_notes)/sizeof(win_notes[0]);}
 else if(scene==1){melody=play_notes;len=sizeof(play_notes)/sizeof(play_notes[0]);}
 else if(scene!=0)return 0;
 if(m->last_scene!=scene){m->last_scene=(uint8_t)scene;m->position=0;m->countdown=0;}
 if(m->countdown){m->countdown--;return 0;}
 out->hz=melody[m->position%len];out->frames=(scene==2)?11:(scene==1)?6:8;out->voice=1;
 m->position=(uint16_t)((m->position+1)%len);m->countdown=(scene==2)?12:(scene==1)?7:9;
 return out->hz!=0;
}
int arelia_sfx(unsigned event,AreliaNote *out){
 /* 1 key, 2 strike, 3 tibia/arch, 4 hurt/miss, 5 rescue, 6 victory. */
 static const uint16_t hz[]={0,440,196,523,659,784,131};
 static const uint8_t frames[]={0,4,8,12,10,14,20};
 if(!out||event==0||event>=sizeof(hz)/sizeof(hz[0]))return 0;
 out->hz=hz[event];out->frames=frames[event];out->voice=0;return 1;
}
uint8_t arelia_midi_from_hz(unsigned hz){
 /* C4..C5 in 1/16 Hz; the 13th entry rounds up into the next octave. */
 static const uint16_t c4[13]={4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902,8372};
 uint32_t h;int octave=0,best=0,i,note;uint32_t distance=0xffffffffu;
 if(!hz)return 0;
 if(hz>20000u)hz=20000u;
 h=(uint32_t)hz*16u;
 while(h<4067u){h*=2u;octave--;}  /* below the B3/C4 midpoint */
 while(h>=8134u){h/=2u;octave++;} /* above the B4/C5 midpoint */
 for(i=0;i<13;i++){uint32_t d=h>c4[i]?h-c4[i]:c4[i]-h;if(d<distance){distance=d;best=i;}}
 note=60+octave*12+best;
 if(note<0)note=0;
 if(note>127)note=127;
 return (uint8_t)note;
}
