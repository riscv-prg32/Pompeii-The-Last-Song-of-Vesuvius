/* PRG32 portable cartridge entrypoints, checked against the PRG32 v1.8 SDK
 * header (components/prg32/include/prg32.h) and its portable ABI table. */
#include "prg32.h"
#include "game.h"
#include "music.h"
#include "present.h"

/* The v1.8 mixer applies a note's volume twice (voice and channel), so the
 * level is roughly (v/256)^2; keep volumes high. */
#define SFX_VOLUME 240
#define THEME_VOLUME 225
#define PLAY_VOLUME 165

/* GCC lowers large struct copies to memcpy calls; the cartridge has no libc. */
void *memcpy(void *d, const void *s, size_t n);
void *memcpy(void *d, const void *s, size_t n) {
    unsigned char *a = (unsigned char *)d;
    const unsigned char *b = (const unsigned char *)s;
    while (n--) *a++ = *b++;
    return d;
}

static AreliaGame game;
static AreliaMusic music;
static AreliaSfx sfx;
static AreliaPresenter presenter;
static uint8_t framebuffer[ARELIA_RW * ARELIA_RH];
static uint32_t palette_rgb[256];
static char band_text[ARELIA_STATUS_MAX];
static uint8_t palette_phase = 255;

/* Analog sticks are folded into the D-pad bits by the firmware. */
static uint32_t buttons(void) {
    uint32_t p = prg32_input_read();
    uint32_t a = 0;
    if (p & PRG32_BTN_UP) a |= ARELIA_INPUT_UP;
    if (p & PRG32_BTN_DOWN) a |= ARELIA_INPUT_DOWN;
    if (p & PRG32_BTN_LEFT) a |= ARELIA_INPUT_LEFT;
    if (p & PRG32_BTN_RIGHT) a |= ARELIA_INPUT_RIGHT;
    if (p & PRG32_BTN_A) a |= ARELIA_INPUT_A;
    if (p & PRG32_BTN_B) a |= ARELIA_INPUT_B;
    if (p & PRG32_BTN_C) a |= ARELIA_INPUT_TIBIA;
    if (p & PRG32_BTN_START) a |= ARELIA_INPUT_START;
    if (p & PRG32_BTN_L) a |= ARELIA_INPUT_FOCUS;
    if (p & PRG32_BTN_R) a |= ARELIA_INPUT_ROLL;
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

/* The palette darkens as the eruption advances; it changes only on room
 * transitions, so the whole-screen redraw it causes is rare. */
static void update_palette(void) {
    unsigned i;
    if (game.palette_phase == palette_phase) return;
    palette_phase = game.palette_phase;
    game_palette(&game, palette_rgb);
    for (i = 0; i < 256; ++i) {
        uint32_t c = palette_rgb[i];
        prg32_palette_set((uint8_t)i, (uint16_t)(((c >> 19) & 31u) << 11 | ((c >> 10) & 63u) << 5 | ((c >> 3) & 31u)));
    }
    /* Displays that store RGB pixels keep the old colours: resend everything. */
    arelia_present_invalidate(&presenter);
}

static void run(void *ctx, int x, int y, int w, uint8_t color) {
    (void)ctx;
    prg32_gfx_rect_indexed(x, y, w, 1, color);
}

void arelia_init(void) {
    game_init(&game);
    arelia_music_init(&music);
    arelia_sfx_start(&sfx, 0);
    arelia_present_invalidate(&presenter);
    band_text[0] = 0;
    palette_phase = 255;
    update_palette();
    update_band();
}

void arelia_update(void) {
    AreliaNote note;
    unsigned scene;
    game_step(&game, buttons());
    if (game.sound_event >= SND_NOTE0) {
        /* Tibia notes sustain on their own channel, like a reed. */
        unsigned hz = arelia_tibia_hz(game.sound_event - SND_NOTE0);
        if (hz) prg32_audio_note(2, 0, arelia_midi_from_hz(hz), 255, 420u);
    } else if (game.sound_event) {
        arelia_sfx_start(&sfx, game.sound_event);
    }
    game.sound_event = 0;
    if (arelia_sfx_step(&sfx, &note))
        prg32_audio_note(0, 0, arelia_midi_from_hz(note.hz), SFX_VOLUME, note.frames * 33u);
    scene = game.mode;
    if (game.mode == MODE_PLAY && game.action == ACT_TIBIA) scene = 4;
    if (arelia_music_step(&music, scene, &note))
        prg32_audio_note(1, 0, arelia_midi_from_hz(note.hz), scene == 1 ? PLAY_VOLUME : THEME_VOLUME, note.frames * 33u);
    update_palette();
    update_band();
}

void arelia_draw(void) {
    game_render(&game, framebuffer);
    /* The firmware keeps the indexed game framebuffer between frames, so only
     * rows that changed are sent, as runs of equal colour. */
    arelia_present(&presenter, framebuffer, run, 0);
}
