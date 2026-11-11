/* Shared helpers for the host tests. */
#ifndef ARELIA_TEST_UTIL_H
#define ARELIA_TEST_UTIL_H
#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define CELLQ(c) ((int)((c) * 256))
static inline void steps(AreliaGame *g, int n, unsigned b){while(n-- > 0) game_step(g, b);}
/* Press a button once (fresh edge) and release it. */
static inline void tap(AreliaGame *g, unsigned b){game_step(g, b); game_step(g, 0);}
/* Close any open dialogue the way a player does. */
static inline void close_dialogue(AreliaGame *g){int n = 0; while(g->dialogue && n++ < 400){game_step(g, n & 1 ? ARELIA_INPUT_A : 0);} if(g->prev_buttons) game_step(g, 0);}
static inline void start_game(AreliaGame *g){game_init(g); tap(g, ARELIA_INPUT_START); close_dialogue(g);}
#endif
