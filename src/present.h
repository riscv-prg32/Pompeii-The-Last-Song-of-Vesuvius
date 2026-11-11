#ifndef ARELIA_PRESENT_H
#define ARELIA_PRESENT_H
#include <stdint.h>
#include "game.h"
/* Incremental presenter for a persistent indexed target framebuffer.
 * A 4-bpp shadow copy of the last frame sent to the target lets each frame
 * emit only the horizontal runs whose pixels changed, instead of one SDK call
 * per run of the whole 320x200 frame. Palette indices must be 0..15. */
#define ARELIA_PRESENT_RESYNC_FRAMES 600u
typedef void (*AreliaRunFn)(void *ctx, int x, int y, int w, uint8_t color);
typedef struct {
 uint8_t shadow[ARELIA_WIDTH * ARELIA_HEIGHT / 2];
 uint16_t frames_since_full;
 uint8_t valid;
} AreliaPresenter;
/* Forget the target contents: the next present sends every pixel. */
void arelia_present_invalidate(AreliaPresenter *p);
/* Emit the runs needed to turn the target into fb; returns the run count. */
unsigned arelia_present(AreliaPresenter *p, const uint8_t *fb, AreliaRunFn emit, void *ctx);
#endif
