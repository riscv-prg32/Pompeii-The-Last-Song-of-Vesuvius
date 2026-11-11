#ifndef ARELIA_SPRITES_H
#define ARELIA_SPRITES_H
#include <stdint.h>
/* 8x8, 4-bpp packed indexed sprite, transparent index zero. */
void arelia_draw_sprite8(uint8_t *fb,int x,int y,const uint8_t *packed);
extern const uint8_t arelia_hero_sprite[32];
extern const uint8_t arelia_enemy_sprite[32];
extern const uint8_t arelia_crystal_sprite[32];
#endif
