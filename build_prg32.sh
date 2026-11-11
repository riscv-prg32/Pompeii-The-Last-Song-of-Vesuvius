#!/bin/sh
# Compatibility wrapper: ./build_prg32.sh /path/to/PRG32
# Builds dist/pompeii-esp32c6.prg32 and dist/pompeii-qemu.prg32 (see scripts/build.sh).
set -eu
if [ "$#" -lt 1 ]; then
  echo 'Usage: ./build_prg32.sh /path/to/PRG32' >&2
  exit 2
fi
PROJECT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PRG32_REPO=$(cd "$1" && pwd) exec "$PROJECT/scripts/build.sh"
