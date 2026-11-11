#ifndef ARELIA_PRESENT_H
#define ARELIA_PRESENT_H
#include <stdint.h>
#include "game.h"
/* Incremental presenter for a persistent indexed target framebuffer.
 * A full 8-bit copy of the 320x200 frame would not fit the 128 KiB RAM
 * profile next to the frame itself, so the presenter keeps one 32-bit hash
 * per row of the last frame sent and resends only the rows whose hash
 * changed, as runs of equal colour. A periodic full resync bounds the effect
 * of a hash collision or of anything else drawing over the game area. */
#define ARELIA_PRESENT_RESYNC_FRAMES 300u
typedef void (*AreliaRunFn)(void *ctx, int x, int y, int w, uint8_t color);
typedef struct {
 uint32_t row_hash[ARELIA_RH];
 uint16_t frames_since_full;
 uint8_t valid;
} AreliaPresenter;
/* Forget the target contents: the next present sends every pixel. Call it
 * after anything else drew over the game area or the palette changed. */
void arelia_present_invalidate(AreliaPresenter *p);
/* FNV-1a hash of one frame row (exposed for tests). */
uint32_t arelia_present_row_hash(const uint8_t *row);
/* Emit the runs needed to turn the target into fb; returns the run count. */
unsigned arelia_present(AreliaPresenter *p, const uint8_t *fb, AreliaRunFn emit, void *ctx);
#endif
