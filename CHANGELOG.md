# Changelog

## 2.0.0, 2026-11-11: the 3D remake

The game is rebuilt as a third-person 3D action-adventure in the spirit of the late 1990s. The setting, the four districts, the four citizens, the tibia and the historical outcome are unchanged. Every model, map, melody and line of text is original.

- **Fixed-point 3D renderer** (`src/r3d.c`, `src/render.c`):
  - camera with yaw and pitch, five-plane frustum clipping, subpixel convex fill, painter's ordering;
  - floor casting with per-material textures and distance fog;
  - a sky gradient with Vesuvius and its growing plume placed by azimuth;
  - extruded walls, columns, statue bases and tombs;
  - animated low-poly Livia, looters, citizens, rubble, altar, seal, gates, boat and pickups, with blob shadows.
- **Frame and palette:** the scene renders at native 320x200, with the HUD on a 2x2-pixel grid, and uses a 256-colour palette (`material*16 + light*4 + fog`) that darkens district by district.
- **New districts** (`src/world.c`): 16x20-cell 3D maps of the Forum (colonnades, statue, mosaic), the Painted Villa (garden, impluvium atrium, mosaic rooms), the Waterworks (channels, bridges, the Sibyl's island) and the Road to the Sea (tombs, seal, pier).
- **New play:**
  - camera-relative movement with a follow camera that pulls in and rises near walls;
  - **L** focus with letterbox bars and a target marker, plus circle-strafing;
  - **R** dodge roll with invulnerability frames;
  - **B** context action with an on-screen prompt;
  - **C** raises the tibia, which plays five notes;
  - a six-note Sibyl's song learned at the altar and played to sink the seal;
  - looters that chase, wind up, swing, stagger and fall;
  - health in amphora halves with pickups;
  - lapilli with growing shadows on the coast road;
  - a dialogue box with typewriter text and speaker names;
  - district banners, an orbiting title camera and a pier ending that turns to the mountain.
- **Controller contract change:** C, L and R are now used (tibia, focus, roll). AGENTS.md, the README, the colophon and `tools/control_test.c` were updated together.
- **Audio:** new title, exploration, defeat and ending themes. Every game event now has a multi-note phrase. Tibia notes have their own sustained channel (D4 F4 A4 D5 C5), and the music rests while the tibia is raised.
- **Presenter:** it keeps one hash per row instead of a 4-bit shadow of the frame, and resends only changed rows as runs. It is invalidated on palette changes and resyncs every 300 frames.
- **Memory:** RAM use fell from 108,484 to 96,884 bytes, even though the frame is now 256-colour.
- **Bot:** `tools/autoplay.py` rewritten. It parses the maps, plans with BFS and steers relative to the camera, using focus and strike, the tibia song and the pier. The QEMU harness holds directions and sends C/L/R.
- **Tests:** all host tests rewritten for the new mechanics. They add a 20,000-frame random-input smoke run and a renderer bounds check from many camera positions in every district.
- **Cartridge without libc:** `memmove` was added to the shim, and `memcpy` is defined in the cartridge bridge.
- QEMU at 320x200: boot in 10.6 s, 30.3 updates/s, title to ending in 143.6 s (559 bot inputs) with no faults. The 60 s `assets/store/preview.mp4` was re-recorded from QEMU with its audio.
- ESP32-C6 hardware execution, including its 3D frame rate, remains unverified.

## Milestone 11 — District art and eruption

- Added distinct procedural architectural treatments for all four districts.
- Added original named rescue dialogue for Marcus, Flavia, Felix and Tullia.
- Added staged eruption overlay and post-victory animation with a resettable counter.
- Added deterministic eruption regression test and refreshed renderer screenshots.
- Rebuilt portable PRG32 cartridge: 13,528 bytes (10.3% of 128 KiB).
- Firmware/QEMU runtime execution remains unverified.

## 1.0.0 (Milestone 14), 2026-11-11: tested on PRG32 QEMU, Store release

First run on the PRG32 v1.8 QEMU firmware. Fixes for the problems that run and a code review found:

- **Unwinnable game fixed.** The Waterworks rubble was placed on Felix's tile, so the citizen vanished and the ending could never be reached. Each district's rubble now comes from a per-district table and sits beside its citizen.
- **Position-independent cartridge.** The map, room-name and message tables were arrays of pointers, which put 67 absolute `R_RISCV_32` relocations in `.rodata`. Portable images have no relocations and each host loads them at a different address, so the cartridge only worked at 0x40800000. The tables are now fixed-size arrays; `tools/check_relocatable.py` proves that images linked at two addresses are identical.
- District banners were off by one (the Villa announced "THE WATERWORKS").
- The 3x5 font had blank glyphs for 5-9 and '/', so the title read "POMPEII AD".
- The tibia melody announced "THE STONE ARCH OPENS"; it now says "THE SIBYL HEARS. RUN TO THE SEA", and opening an arch with a key shows that message instead.
- Citizens and looters block movement: walking toward one turns Livia to face it, which makes B (rescue) and A (strike) reliable.
- Audio: the v1.8 mixer applies a note's volume twice, so volumes of 80-100 played at about 10-15%. Raised to 170-240; QEMU peak level went from 835 to 3,841. Added an original in-game ostinato (D dorian), so exploration is no longer silent. Note lengths now use the 30 Hz update rate.
- New `tools/autoplay.py`: a bot that plays the whole game with button presses. It runs in `make test` against the host build and drives QEMU through `scripts/qemu_preview.py` (`make qemu`, `make preview`).
- New `tools/regression_test.c` covering every fix above.
- QEMU results: boot in 10.5 s, 30.1 updates/s, title to ending in 29.3 s with no faults. The 60 s `assets/store/preview.mp4` was recorded from QEMU with its audio.
- Store release: `metadata/`, `assets/store/` (icon, QEMU screenshot, preview), `scripts/build.sh` (esp32c6 + qemu variants), `scripts/pack-store-bundle.sh` and `tools/validate_bundle.py`. `dist/pompeii-1.0.0-store.zip` is validated with CartridgeStore's own ingestion code, including the preview video. It has not been submitted.
- Cartridges: 19,238 bytes with the Store trailer; 108,484 bytes of executable RAM (128 KiB profile). The builds are byte-reproducible. `dist/pompeii.prg32` is replaced by the per-architecture files.
- ESP32-C6 hardware execution remains unverified.

## Milestone 13 — 2026-11-10 — PRG32 v1.8 integration

- Added `src/present.c`: a diff-based presenter that sends only changed runs to the persistent PRG32 indexed framebuffer (0.5% of the full-frame rect calls in the host test), with resync every 600 frames.
- Replaced the 16-entry Hz-to-MIDI table with integer equal-tempered conversion; F4 and C3 notes now play at the correct pitch.
- Shared sound-effect tables (`arelia_sfx`) between the PRG32 bridge and the host adapter; SFX durations now match on both.
- Published the district and rescue status to the PRG32 game-info band.
- Added `tools/present_test.c` and `tools/status_test.c`; the build now passes `--cart-ram-kib 128` explicitly.
- Rebuilt with riscv32-esp-elf-gcc 14.2.0 against PRG32 v1.8.0: 11,596 bytes, 107,860 bytes of executable RAM.
- Renderer output is unchanged (screenshots are byte-identical). QEMU/ESP32-C6 runtime remains unverified.

## Milestone 10 — 2026-11-10

- Persist bronze-key collection when moving between districts.
- Persist defeated enemies when returning to a district.
- Add host regression tests for room transitions, key persistence, enemy persistence, and new-game reset.
- Rebuild the experimental PRG32 cartridge (10,284 bytes).
- Runtime execution on QEMU or ESP32-C6 remains unverified.

## Milestone 9

- Map A to attack, B to interact, and START to Select.

## Milestone 12
- Introduced attack-cleared rubble barriers for citizen rescue in three districts.
- Persisted cleared rubble per district across room transitions.
- Added automated hazard rescue and persistence regression test.
- Regenerated screenshots and packaged a 13,708-byte portable cartridge.
