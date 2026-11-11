CC ?= cc
CFLAGS ?= -std=c99 -Os -Wall -Wextra -Werror -pedantic
COMMON = src/r3d.c src/world.c src/game.c src/render.c src/music.c src/present.c
.PHONY: all check test screenshots clean cartridge qemu preview store
all: test
check:
	$(CC) $(CFLAGS) -fsyntax-only $(COMMON) src/platform.c tools/screenshot_scenes.c

test: check
	@mkdir -p .build
	$(CC) $(CFLAGS) $(COMMON) src/platform.c tools/smoke.c -o .build/smoke
	./.build/smoke
	$(CC) $(CFLAGS) $(COMMON) tools/narrative_test.c -o .build/narrative_test
	./.build/narrative_test
	$(CC) $(CFLAGS) $(COMMON) tools/av_test.c -o .build/av_test
	./.build/av_test
	$(CC) $(CFLAGS) $(COMMON) tools/control_test.c -o .build/control_test
	./.build/control_test
	$(CC) $(CFLAGS) $(COMMON) tools/persistence_test.c -o .build/persistence_test
	./.build/persistence_test
	$(CC) $(CFLAGS) $(COMMON) tools/eruption_test.c -o .build/eruption_test
	./.build/eruption_test
	$(CC) $(CFLAGS) $(COMMON) tools/hazard_test.c -o .build/hazard_test
	./.build/hazard_test
	$(CC) $(CFLAGS) $(COMMON) tools/present_test.c -o .build/present_test
	./.build/present_test
	$(CC) $(CFLAGS) $(COMMON) tools/status_test.c -o .build/status_test
	./.build/status_test
	$(CC) $(CFLAGS) $(COMMON) tools/regression_test.c -o .build/regression_test
	./.build/regression_test
	python3 tools/autoplay.py

screenshots:
	@mkdir -p .build docs/screenshots
	$(CC) $(CFLAGS) $(COMMON) tools/screenshot_scenes.c -o .build/screenshot_scenes
	./.build/screenshot_scenes docs/screenshots

PRG32_ROOT ?= ../PRG32
cartridge:
	@test -n "$(PRG32_ROOT)" || (echo 'Set PRG32_ROOT=/path/to/PRG32' >&2; exit 1)
	PRG32_REPO="$(PRG32_ROOT)" scripts/build.sh
	python3 tools/check_size.py dist/pompeii-esp32c6.prg32
	python3 tools/check_size.py dist/pompeii-qemu.prg32

# Play the QEMU cartridge to the end on the PRG32 QEMU firmware.
qemu:
	PRG32_REPO="$(PRG32_ROOT)" python3 scripts/qemu_preview.py --cart dist/pompeii-qemu.prg32 --out build/qemu-smoke

# Record the 60 s preview video (real QEMU frames and audio).
preview:
	PRG32_REPO="$(PRG32_ROOT)" python3 scripts/qemu_preview.py --cart dist/pompeii-qemu.prg32 --video --out build/qemu-preview

# Pack and validate the Cartridge Store bundle.
store:
	PRG32_REPO="$(PRG32_ROOT)" scripts/pack-store-bundle.sh

clean:
	rm -rf .build build
