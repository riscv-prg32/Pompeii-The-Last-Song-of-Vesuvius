# Agent instructions — Pompeii: The Last Song of Vesuvius

These instructions apply to Codex, Claude Code, and other coding agents working in this repository.

## Project intent and boundaries
- Build an **original**, historically inspired 3D action-adventure for the PRG32 educational console, set in Pompeii around AD 79. The feel is inspired by late-1990s third-person 3D adventures (focus, roll, context button, songs on an instrument); never use names, music, symbols, UI or assets of any existing game, and never describe this game with another game's trademark.
- The target cartridge is **128 KiB maximum (131,072 bytes)**, with a 320x200 8-bit indexed framebuffer (the scene renders at native 320x200; the HUD uses a 2x2-pixel grid) and portable RV32 cartridge ABI.
- Preserve the historical outcome: Vesuvius erupts; the protagonist can save people, not prevent the eruption.
- All game code, dialogue, art, and music must be original or explicitly licensed for redistribution. Do not import Nintendo/Zelda assets, trademarks as game branding, copyrighted arrangements, or artwork from modern adaptations. The 1834 novel *The Last Days of Pompeii* is public domain, but avoid copying modern adaptations or translations.
- MIT licensing applies to this project's original contents; the PRG32 SDK is **not vendored** and retains its own licenses.

## Architecture
- `src/game.c`, `src/game.h`: deterministic world state, input, combat, camera, dialogue.
- `src/world.c`, `src/world.h`: the four 16x20 district maps and entity spawns (data only).
- `src/r3d.c`, `src/r3d.h`, `src/trig_table.h` (from `tools/gen_trig.py`): fixed-point 3D rasterizer: camera, frustum clipping, convex fill, 3x5 font. No floats, no 64-bit math.
- `src/render.c`: scene renderer (sky and Vesuvius, floor casting, architecture, low-poly models, HUD) and the 256-colour palette (`material*16 + light*4 + fog`, UI colours 240-255).
- `src/music.c`, `src/music.h`: original music, sound-effect phrases, tibia pitches.
- `src/platform.c`, `src/platform.h`: portable host adapter.
- `src/present.c`, `src/present.h`: incremental presenter (one 32-bit hash per row; changed rows are resent as runs) for the persistent PRG32 indexed framebuffer.
- `src/prg32_cartridge.c`: actual PRG32 SDK bridge (buttons, palette, audio, indexed graphics).
- `src/cartridge_all.c`: single translation unit for the PRG32 cartridge builder.
- `sdk/string.h`: minimal freestanding shim; don't expand without testing.
- `tools/`: host-side smoke tests, narrative tests, rendering, size enforcement.
- `docs/screenshots/`: captured from the actual `game_render` code, not concept art.
- `dist/pompeii-esp32c6.prg32`, `dist/pompeii-qemu.prg32`, `dist/pompeii-<version>-store.zip`: release artifacts (`make cartridge`, `make store`); QEMU-verified, not hardware-validated.
- `tools/autoplay.py`: button-only bot that finishes the game; host test and QEMU driver. It parses the maps from `src/world.c`, plans paths and steers relative to the camera. Its `OFF` table mirrors `AreliaGame` and is checked against the compiler.
- `scripts/qemu_preview.py`: QEMU playthrough, captures and the 60 s preview video. `scripts/build.sh`, `scripts/pack-store-bundle.sh`: cartridges and Store bundle.
- `metadata/`, `assets/store/`: Store metadata, colophon, icon, screenshot, preview (single source of truth).

## Development workflow
1. Read `README.md`, `STORY.md`, and relevant PRG32 SDK headers before editing.
2. Keep the portable engine free of ESP-IDF dependencies, dynamic allocation, and floating point where practical.
3. Run `make test` after changes. Add regression tests for changed behavior.
4. Run `make screenshots` after visual changes and regenerate PNGs from the resulting PPM files. Never represent concept art as gameplay screenshots.
5. For cartridge builds, obtain an external PRG32 checkout, then run `make cartridge PRG32_ROOT=/path/to/PRG32`. Use the SDK's official packer; verify the ABI against the checkout in use. Then run `make qemu` (the bot must reach the ending) and, for releases, `make preview` and `make store`.
6. Run `python3 tools/check_size.py dist/pompeii-esp32c6.prg32` and inspect executable RAM and cartridge output. Do not treat successful packaging as evidence of QEMU/ESP32-C6 runtime success.
7. Update documentation, controls, and any screenshots when behavior changes.

## Controller contract
- D-pad: movement relative to the camera (relative to the target while focused); in tibia mode, notes UP, LEFT, RIGHT, DOWN (and A).
- **A**: strike; advances dialogue.
- **B**: context action (rescue, read, check, talk); lowers the tibia.
- **C**: raise the tibia.
- **L**: focus on the nearest target; with none, recenter the camera.
- **R**: dodge roll.
- **START**: title, rise again after defeat, new game after the ending.
- `tools/control_test.c` pins this contract; update it, the README and the colophon together.

## Code quality and review
- C99, `-Wall -Wextra -Werror -pedantic`; no new compiler warnings.
- Prefer small, explicit, testable state transitions. Bounds-check tile and framebuffer writes.
- Avoid blocking loops and expensive per-pixel PRG32 SDK calls; profile rendering before optimization claims.
- Review memory usage, stack use, and ROM size for each substantial feature.
- Never commit credentials, private SDKs, generated object files, or proprietary assets.
- If the PRG32 checkout or cross compiler is unavailable, run host tests and report native build as **not verified**, rather than inventing results.

## Suggested next tasks
- Measure the 3D frame time on ESP32-C6 hardware (QEMU is done: `docs/qemu/`); the presenter sends about 3,300 runs per moving frame.
- A quest journal and more songs; original double-reed timbres if the PRG32 cartridge ABI gains instrument registration.
- Maintain `tools/persistence_test.c` when changing collection, defeat or room transitions.

## Rendering and finale invariants

- Keep all drawing deterministic, bounded to the 320x200 frame (`tools/regression_test.c` renders every district from many cameras with guard bytes), and allocation-free. Polygons go through `r3d_poly`, which clips to the frustum.
- Keep four district palettes that darken with `palette_phase`, and the original named citizen rescue dialogue.
- Keep the ending advancing while `mode == MODE_WON` (`story`), and reset everything with `game_init`.
- Run `make test`, `make screenshots`, and `make cartridge PRG32_ROOT=/path/to/PRG32` after rendering changes.
- Keep the committed screenshots generated from `game_render()` and never claim hardware validation based on renderer captures.

### Rescue hazard invariant
Districts 1-3 have one breakable rubble obstacle near their citizen (the citizen's spawn `param` names it). A breaks it, B rescues; the `gone` bitmask must persist across room transitions and reset on `game_init`. Run `make test`, especially `tools/hazard_test.c`, after changing room loading or interactions.

### Presentation and RAM invariant
The cartridge uses the 128 KiB executable RAM profile: about 97 KB in use, including the 64,000-byte frame. A full-frame shadow does not fit: the presenter keeps per-row hashes (800 bytes). Palette changes (`palette_phase`) must invalidate the presenter. Re-check `mem=` in the build output after adding static buffers. Keep `tools/present_test.c` passing: after every present, the target must equal the rendered frame. Anything that draws into the game area outside `arelia_present` must call `arelia_present_invalidate`.

### Portability and playability invariants (Milestone 14)
- Portable cartridges carry no relocations. Never put pointers in static data, such as `const char *table[]` or arrays of function pointers. `tools/check_relocatable.py` (run by `scripts/build.sh`) must report "position independent".
- The game must stay winnable with button presses only: `python3 tools/autoplay.py` is part of `make test`. If you change `AreliaGame`, update `OFF`/`SIZE` in `tools/autoplay.py`; the test checks the layout. If you move keys, rubble, citizens, the altar or doors, update the bot's tables.
- Rubble spawns must never cover a citizen; `tools/regression_test.c` checks one citizen per district and spawns on open cells.
- The cartridge links without libc: `sdk/string.h` gives `memset`/`memmove`, and `src/prg32_cartridge.c` defines `memcpy` for struct copies.
- PRG32 v1.8 audio applies note volume twice, so keep volumes high (160-255).
- QEMU key map (v1.8): w/a/s/d, z = A, x = B, k = C, q = L, e = R, Enter = START; the QEMU firmware may boot into SETUP (B leaves it).
