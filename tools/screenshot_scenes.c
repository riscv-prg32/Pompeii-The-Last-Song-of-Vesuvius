/* Capture actual game_render() output from deterministic game states,
 * at the native 320x200 of the PRG32 viewport. */
#include "../src/game.h"
#include <stdio.h>
#include <string.h>
static uint8_t fb[ARELIA_RW * ARELIA_RH];
static uint32_t pal[256];
static AreliaGame game;
static int save(const char *folder, const char *name){
 char path[512]; FILE *out; int x, y;
 if(snprintf(path, sizeof(path), "%s/%s", folder, name) >= (int)sizeof(path)) return 1;
 game_render(&game, fb); game_palette(&game, pal);
 out = fopen(path, "wb"); if(!out) return 2;
 fprintf(out, "P6\n%d %d\n255\n", ARELIA_WIDTH, ARELIA_HEIGHT);
 for(y = 0; y < ARELIA_HEIGHT; y++) for(x = 0; x < ARELIA_WIDTH; x++){
  uint32_t c = pal[fb[y * ARELIA_RW + x]];
  fputc((int)((c >> 16) & 255u), out); fputc((int)((c >> 8) & 255u), out); fputc((int)(c & 255u), out);
 }
 return fclose(out) != 0;
}
static void steps(int n, unsigned b){while(n-- > 0) game_step(&game, b);}
int main(int argc, char **argv){
 int err = 0;
 if(argc != 2) return 2;
 game_init(&game); steps(40, 0); err |= save(argv[1], "title.ppm");
 /* The Forum, just after arriving: walk north toward the looters. */
 game_init(&game); game_step(&game, ARELIA_INPUT_START); game.dialogue = 0;
 steps(30, ARELIA_INPUT_UP); err |= save(argv[1], "forum.ppm");
 /* Focus on a looter and strike. */
 game_debug_enter(&game, 0, 5 * 256 + 128, 10 * 256 + 128, 0); game.ent[4].alive = 0;
 game_step(&game, ARELIA_INPUT_FOCUS); steps(30, 0); steps(3, ARELIA_INPUT_UP);
 game_step(&game, ARELIA_INPUT_A); steps(5, 0); err |= save(argv[1], "focus.ppm");
 /* The Painted Villa atrium. */
 game_init(&game); game_debug_enter(&game, 1, 7 * 256 + 128, 11 * 256 + 128, 0); steps(4, 0); err |= save(argv[1], "villa.ppm");
 /* The Waterworks altar with the tibia raised. */
 game_init(&game); game_debug_enter(&game, 2, 7 * 256 + 128, 7 * 256 + 128, 0); steps(4, 0);
 game_step(&game, ARELIA_INPUT_TIBIA); game_step(&game, 0); game_step(&game, ARELIA_INPUT_UP); game_step(&game, 0);
 game_step(&game, ARELIA_INPUT_LEFT); game_step(&game, 0); err |= save(argv[1], "tibia.ppm");
 /* Road to the Sea with a dialogue box. */
 game_init(&game); game_debug_enter(&game, 3, 7 * 256 + 128, 9 * 256, 0); steps(4, 0);
 game.dialogue = 12; game.dialogue_t = 60; err |= save(argv[1], "road.ppm");
 /* The ending. */
 game.dialogue = 0; game.rescued = 15; game.mode = MODE_WON; game.palette_phase = 4; game.story = 0;
 steps(320, 0); err |= save(argv[1], "ending.ppm");
 return err ? 1 : 0;
}
