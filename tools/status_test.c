/* Band status text and Hz-to-MIDI conversion used by the PRG32 bridge. */
#include "test_util.h"
#include "../src/music.h"
int main(void){
 static AreliaGame g; char s[ARELIA_STATUS_MAX]; char tiny[4];
 game_init(&g); game_status_text(&g, s, sizeof(s)); assert(!strcmp(s, "POMPEII AD 79"));
 g.mode = MODE_PLAY; g.rescued = 1; game_status_text(&g, s, sizeof(s)); assert(!strcmp(s, "THE FORUM SAVED 1/4"));
 g.room = 2; g.rescued = 7; g.song_known = 1; game_status_text(&g, s, sizeof(s)); assert(!strcmp(s, "WATERWORKS SAVED 3/4 SONG"));
 g.mode = MODE_DEAD; game_status_text(&g, s, sizeof(s)); assert(!strcmp(s, "ASH AND SILENCE"));
 g.mode = MODE_WON; g.rescued = 15; game_status_text(&g, s, sizeof(s)); assert(!strcmp(s, "SURVIVORS SAVED 4/4"));
 memset(tiny, 'x', sizeof(tiny)); game_status_text(&g, tiny, sizeof(tiny)); assert(!strcmp(tiny, "SUR"));
 assert(arelia_midi_from_hz(0) == 0);
 assert(arelia_midi_from_hz(440) == 69 && arelia_midi_from_hz(349) == 65 && arelia_midi_from_hz(262) == 60);
 assert(arelia_midi_from_hz(294) == 62 && arelia_midi_from_hz(587) == 74 && arelia_midi_from_hz(523) == 72);
 assert(arelia_midi_from_hz(220) == 57 && arelia_midi_from_hz(131) == 48 && arelia_midi_from_hz(1047) == 84);
 assert(arelia_midi_from_hz(370) == 66 && arelia_midi_from_hz(20000) <= 127);
 puts("Status band and MIDI pitch test passed");
 return 0;
}
