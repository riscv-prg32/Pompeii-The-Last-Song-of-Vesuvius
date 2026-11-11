#ifndef ARELIA_GAME_H
#define ARELIA_GAME_H
/* Pompeii: The Last Song of Vesuvius: deterministic 3D action-adventure state.
 * The struct holds no pointers or longs, so its layout is identical on RV32
 * (cartridge) and LP64 hosts (tests, tools/autoplay.py). */
#include <stdint.h>
#include "world.h"

#define ARELIA_WIDTH 320          /* PRG32 game viewport */
#define ARELIA_HEIGHT 200
#define ARELIA_RW 320             /* render target: the full viewport */
#define ARELIA_RH 200
#define ARELIA_ROOMS WORLD_ROOMS

#define ARELIA_INPUT_UP 1u
#define ARELIA_INPUT_DOWN 2u
#define ARELIA_INPUT_LEFT 4u
#define ARELIA_INPUT_RIGHT 8u
#define ARELIA_INPUT_A 16u        /* strike; closes dialogue */
#define ARELIA_INPUT_TIBIA 32u    /* C: raise the tibia */
#define ARELIA_INPUT_B 64u        /* context action: talk, rescue, check; lowers the tibia */
#define ARELIA_INPUT_START 128u
#define ARELIA_INPUT_FOCUS 256u   /* L: focus on the nearest target, or recenter the camera */
#define ARELIA_INPUT_ROLL 512u    /* R: dodge roll */
/* Names kept for older host code. */
#define ARELIA_INPUT_SWORD ARELIA_INPUT_A
#define ARELIA_INPUT_ACTION ARELIA_INPUT_B
#define ARELIA_INPUT_MUSIC ARELIA_INPUT_TIBIA
#define ARELIA_INPUT_SELECT ARELIA_INPUT_START

enum { MODE_TITLE, MODE_PLAY, MODE_DEAD, MODE_WON };
enum { ACT_NONE, ACT_STRIKE, ACT_ROLL, ACT_TIBIA, ACT_HURT };
enum { AI_IDLE, AI_CHASE, AI_WINDUP, AI_SWING, AI_RECOVER, AI_STAGGER, AI_FALL, AI_LEAVE };

#define ARELIA_SONG_LEN 6
#define ARELIA_HP_MAX 6           /* three amphorae of two halves */

typedef struct {
 uint8_t type, alive, state, hp;
 int16_t x, z, yaw, timer;
 uint8_t param, flash, spare0, spare1;
} AreliaEntity;

typedef struct {
 uint8_t mode, room, hp, keys;
 uint8_t song_known, rescued, action, action_t;
 uint8_t invuln, focus, sound_event, dialogue;
 uint8_t dialogue_t, banner_t, note_count, tibia_purpose;
 uint8_t notes[8];
 int16_t px, pz, pyaw, walk;
 int16_t cam_yaw, cam_pitch;
 int32_t cam_x, cam_y, cam_z;
 uint32_t ticks;
 uint16_t room_ticks, story;
 uint16_t gone[WORLD_ROOMS];   /* per district: entities defeated, broken, rescued, taken, opened */
 uint16_t buttons, prev_buttons;
 uint8_t ent_count, prompt, lapilli_n, palette_phase;
 int16_t lap_x[4], lap_z[4], lap_t[4];
 AreliaEntity ent[WORLD_MAX_ENT];
} AreliaGame;

void game_init(AreliaGame *g);
void game_step(AreliaGame *g, unsigned buttons);
/* Render to a ARELIA_RW x ARELIA_RH indexed frame (palette: game_palette). */
void game_render(const AreliaGame *g, uint8_t *fb);
/* 256 RGB888 colours; they darken as the eruption advances (palette_phase). */
void game_palette(const AreliaGame *g, uint32_t rgb[256]);
/* One-line status for the PRG32 game-info band, e.g. "THE FORUM SAVED 1/4". */
#define ARELIA_STATUS_MAX 40
void game_status_text(const AreliaGame *g, char *out, unsigned cap);
/* Text of a dialogue id (for tests and tools); "" when unknown. */
const char *game_dialogue_text(unsigned id);
/* Test and tool hook: play from district room at (x, z) Q8 facing yaw. */
void game_debug_enter(AreliaGame *g, unsigned room, int x, int z, int yaw);

/* Sound events (game.sound_event). */
enum { SND_NONE, SND_KEY, SND_SWING, SND_DOOR, SND_HURT, SND_RESCUE, SND_VICTORY, SND_HIT,
       SND_BREAK, SND_SONG, SND_ROLL, SND_FOCUS, SND_IMPACT, SND_LOCKED, SND_TALK,
       SND_NOTE0 = 20 /* + note 0..4 */ };
#endif
