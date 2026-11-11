#include "platform.h"
#include "game.h"
void arelia_run_frame(AreliaHost *host, void *state, uint8_t *framebuffer) {
 AreliaGame *game=(AreliaGame *)state;
 if (!host || !game || !framebuffer) return;
 game_step(game,host->read_buttons ? host->read_buttons(host->ctx) : 0u);
 game_render(game,framebuffer);
 if (host->present_indexed) host->present_indexed(host->ctx,framebuffer,arelia_palette,ARELIA_WIDTH,ARELIA_HEIGHT);
 if (host->play_tone && game->sound_event) {
  AreliaNote sfx;
  if(arelia_sfx(game->sound_event,&sfx)) host->play_tone(host->ctx,sfx.hz,sfx.frames,sfx.voice);
  game->sound_event=0;
 }
 {
  AreliaNote note;
  unsigned scene=game->won?3u:(game->dead?2u:(game->mode?1u:0u));
  if(host->play_tone && arelia_music_step(&host->music,scene,&note))
   host->play_tone(host->ctx,note.hz,note.frames,note.voice);
 }
 if(host->wait_frame) host->wait_frame(host->ctx);
}
