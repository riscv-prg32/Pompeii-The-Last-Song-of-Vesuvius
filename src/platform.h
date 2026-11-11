#ifndef ARELIA_PLATFORM_H
#define ARELIA_PLATFORM_H
#include <stdint.h>
#include "music.h"
/* Portable host contract (desktop ports, tests). The frame is ARELIA_RW x
 * ARELIA_RH indexed pixels with a 256-entry RGB888 palette. */
typedef struct {
 unsigned (*read_buttons)(void *ctx);
 void (*present_indexed)(void *ctx, const uint8_t *pixels, const uint32_t *rgb_palette, unsigned width, unsigned height);
 void (*play_tone)(void *ctx, unsigned hz, unsigned frames, unsigned voice);
 void (*wait_frame)(void *ctx);
 void *ctx;
 AreliaMusic music;
 AreliaSfx sfx;
 uint32_t palette[256];
} AreliaHost;
void arelia_run_frame(AreliaHost *host, void *game, uint8_t *framebuffer);
#endif
