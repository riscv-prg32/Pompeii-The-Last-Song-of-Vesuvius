#include "platform.h"
#include "game.h"
void arelia_run_frame(AreliaHost *host, void *state, uint8_t *framebuffer){
 AreliaGame *game = (AreliaGame *)state;
 AreliaNote note;
 if(!host || !game || !framebuffer) return;
 game_step(game, host->read_buttons ? host->read_buttons(host->ctx) : 0u);
 game_render(game, framebuffer);
 game_palette(game, host->palette);
 if(host->present_indexed) host->present_indexed(host->ctx, framebuffer, host->palette, ARELIA_RW, ARELIA_RH);
 if(game->sound_event >= SND_NOTE0){
  if(host->play_tone) host->play_tone(host->ctx, arelia_tibia_hz(game->sound_event - SND_NOTE0), 12, 2);
 } else if(game->sound_event) arelia_sfx_start(&host->sfx, game->sound_event);
 game->sound_event = 0;
 if(host->play_tone && arelia_sfx_step(&host->sfx, &note)) host->play_tone(host->ctx, note.hz, note.frames, note.voice);
 if(host->play_tone && arelia_music_step(&host->music, game->mode == MODE_PLAY && game->action == ACT_TIBIA ? 4u : game->mode, &note))
  host->play_tone(host->ctx, note.hz, note.frames, note.voice);
 if(host->wait_frame) host->wait_frame(host->ctx);
}
