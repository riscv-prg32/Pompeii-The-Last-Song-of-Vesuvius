/* Band status text and Hz-to-MIDI conversion used by the PRG32 bridge. */
#include "../src/game.h"
#include "../src/music.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 static AreliaGame g;char s[ARELIA_STATUS_MAX];char tiny[4];AreliaNote n;
 game_init(&g);game_status_text(&g,s,sizeof(s));assert(!strcmp(s,"POMPEII AD 79"));
 g.mode=1;g.crystals=1;game_status_text(&g,s,sizeof(s));assert(!strcmp(s,"FORUM SAVED 1/4"));
 g.room=2;g.crystals=7;g.notes=3;game_status_text(&g,s,sizeof(s));assert(!strcmp(s,"WATERWORKS SAVED 3/4 TIBIA"));
 g.dead=1;game_status_text(&g,s,sizeof(s));assert(!strcmp(s,"ASH AND SILENCE"));
 g.dead=0;g.won=1;g.crystals=15;game_status_text(&g,s,sizeof(s));assert(!strcmp(s,"SURVIVORS SAVED 4/4"));
 memset(tiny,'x',sizeof(tiny));game_status_text(&g,tiny,sizeof(tiny));assert(!strcmp(tiny,"SUR"));
 /* Every note of the original themes must keep its pitch on the PRG32 mixer. */
 assert(arelia_midi_from_hz(0)==0);
 assert(arelia_midi_from_hz(440)==69);assert(arelia_midi_from_hz(349)==65);
 assert(arelia_midi_from_hz(262)==60);assert(arelia_midi_from_hz(294)==62);
 assert(arelia_midi_from_hz(220)==57);assert(arelia_midi_from_hz(131)==48);
 assert(arelia_midi_from_hz(196)==55);assert(arelia_midi_from_hz(1047)==84);
 assert(arelia_midi_from_hz(988)==83);assert(arelia_midi_from_hz(784)==79);
 assert(arelia_midi_from_hz(370)==66);assert(arelia_midi_from_hz(20000)<=127);
 assert(!arelia_sfx(0,&n)&&!arelia_sfx(99,&n));
 assert(arelia_sfx(6,&n)&&n.hz==131&&n.frames==20);
 puts("Status band and MIDI pitch test passed");
 return 0;
}
