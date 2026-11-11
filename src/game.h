#ifndef ARELIA_GAME_H
#define ARELIA_GAME_H
#include <stdint.h>
#define ARELIA_WIDTH 320
#define ARELIA_HEIGHT 200
#define ARELIA_INPUT_UP 1u
#define ARELIA_INPUT_DOWN 2u
#define ARELIA_INPUT_LEFT 4u
#define ARELIA_INPUT_RIGHT 8u
#define ARELIA_INPUT_SWORD 16u
#define ARELIA_INPUT_MUSIC 32u
#define ARELIA_INPUT_ACTION 64u
#define ARELIA_INPUT_SELECT 128u
/* Legacy alias for existing host-side tests. */
#define ARELIA_INPUT_START ARELIA_INPUT_SELECT
#define ARELIA_ROOMS 4
#define ARELIA_ENEMIES 6
typedef struct {int8_t x,y,hp,active;} AreliaEnemy;
typedef struct {
 uint8_t tiles[12][20]; uint8_t room, x,y,face,hp,keys,crystals,notes,won,dead;
 uint8_t attack, invuln,move_cooldown, enemy_cooldown, mode, sound_event;
 uint32_t ticks; uint16_t eruption_frames; uint32_t visited; uint8_t collected_keys[ARELIA_ROOMS]; uint8_t defeated_enemies[ARELIA_ROOMS]; uint8_t gate_open[ARELIA_ROOMS]; uint8_t cleared_hazards[ARELIA_ROOMS]; uint8_t dialogue, dialogue_timer, previous_buttons, melody_progress, melody_active, rescued_count;
 AreliaEnemy enemies[ARELIA_ENEMIES];
} AreliaGame;
extern const uint32_t arelia_palette[16];
void game_init(AreliaGame *g);
void game_step(AreliaGame *g, unsigned buttons);
void game_render(const AreliaGame *g,uint8_t *fb);
/* One-line status for the PRG32 game-info band, e.g. "FORUM SAVED 1/4".
 * Always NUL-terminates when cap>0; never writes more than cap bytes. */
#define ARELIA_STATUS_MAX 40
void game_status_text(const AreliaGame *g,char *out,unsigned cap);
#endif
