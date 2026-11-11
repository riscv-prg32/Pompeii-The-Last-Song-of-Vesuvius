/* PRG32 portable cartridge entrypoints, checked against the PRG32 v1.8 SDK
 * header (components/prg32/include/prg32.h) and its portable ABI table. */
#include "prg32.h"
#include "game.h"
#include "music.h"
#include "present.h"

/* The v1.8 mixer applies a note's volume twice (voice and channel), so the
 * level is roughly (v/256)^2: 100 was barely audible (~15%). */
#define SFX_VOLUME 240
#define THEME_VOLUME 230
#define PLAY_VOLUME 170

static AreliaGame game;
static AreliaMusic music;
static AreliaPresenter presenter;
static uint8_t framebuffer[ARELIA_WIDTH * ARELIA_HEIGHT];
static char band_text[ARELIA_STATUS_MAX];

/* Analog sticks are folded into the D-pad bits by the firmware, so digital
 * mapping alone covers digital pads, analog sticks, QEMU keys and remote keys. */
static uint32_t buttons(void) {
    uint32_t p = prg32_input_read();
    uint32_t a = 0;
    if (p & PRG32_BTN_UP) a |= ARELIA_INPUT_UP;
    if (p & PRG32_BTN_DOWN) a |= ARELIA_INPUT_DOWN;
    if (p & PRG32_BTN_LEFT) a |= ARELIA_INPUT_LEFT;
    if (p & PRG32_BTN_RIGHT) a |= ARELIA_INPUT_RIGHT;
    if (p & PRG32_BTN_A) a |= ARELIA_INPUT_SWORD;
    if (p & PRG32_BTN_B) a |= ARELIA_INPUT_ACTION;
    if (p & PRG32_BTN_C) a |= ARELIA_INPUT_MUSIC;
    if (p & PRG32_BTN_START) a |= ARELIA_INPUT_SELECT;
    return a;
}

static void update_band(void) {
    char text[ARELIA_STATUS_MAX];
    unsigned i;
    game_status_text(&game, text, sizeof(text));
    for (i = 0; text[i] == band_text[i]; ++i)
        if (!text[i]) return; /* unchanged: avoid a firmware band redraw */
    for (i = 0; i < sizeof(text); ++i) band_text[i] = text[i];
    prg32_band_set_game_info(band_text);
}

static void run(void *ctx, int x, int y, int w, uint8_t color) {
    (void)ctx;
    prg32_gfx_rect_indexed(x, y, w, 1, color);
}

void arelia_init(void) {
    unsigned i;
    game_init(&game);
    arelia_music_init(&music);
    arelia_present_invalidate(&presenter);
    band_text[0] = 0;
    for (i=0; i<16; ++i) {
        uint32_t c = arelia_palette[i];
        uint16_t rgb565 = (uint16_t)(((c >> 19) & 31u) << 11 |
                                      ((c >> 10) & 63u) << 5 |
                                      ((c >> 3) & 31u));
        prg32_palette_set((uint8_t)i, rgb565);
    }
    update_band();
}

void arelia_update(void) {
    AreliaNote note;
    unsigned scene;
    game_step(&game, buttons());
    if (arelia_sfx(game.sound_event, &note))
        prg32_audio_note(0, 0, arelia_midi_from_hz(note.hz), SFX_VOLUME, note.frames * 16u);
    game.sound_event = 0;
    scene = game.won ? 3u : (game.dead ? 2u : (game.mode ? 1u : 0u));
    if (arelia_music_step(&music, scene, &note))
        prg32_audio_note(1, 0, arelia_midi_from_hz(note.hz), scene == 1 ? PLAY_VOLUME : THEME_VOLUME, note.frames * 33u);
    update_band();
}

void arelia_draw(void) {
    game_render(&game, framebuffer);
    /* The firmware keeps the indexed game framebuffer between frames, so only
     * changed runs are sent; this also shrinks the dirty rectangle to flush. */
    arelia_present(&presenter, framebuffer, run, 0);
}
