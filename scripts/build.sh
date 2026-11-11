#!/usr/bin/env bash
# Build the Pompeii cartridges for every PRG32 target.
#
#   scripts/build.sh            # esp32c6 + qemu
#   scripts/build.sh qemu       # one architecture
#
# Environment:
#   PRG32_REPO   path to the PRG32 repository (default ../PRG32)
#
# Outputs dist/pompeii-<arch>.prg32 with the Store trailer (metadata, icon,
# screenshot, colophon) attached, dist/SHA256SUMS, and prints the budgets.
# Packaging success is not evidence of runtime success: run
# scripts/qemu_preview.py for QEMU; ESP32-C6 hardware needs a manual check.
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prg32_repo="$(cd "${PRG32_REPO:-"$repo_dir/../PRG32"}" && pwd)"
name="pompeii"
# The 64,000-byte indexed frame plus the 32,000-byte presenter shadow need the
# SDK's large 128 KiB executable RAM profile (the default firmware profile).
cart_ram_kib=128
package_limit=65536      # one Store cartridge slot: image + Store trailer

if ! command -v riscv32-esp-elf-gcc >/dev/null 2>&1; then
  toolchain="$(ls -d "$HOME"/.espressif/tools/riscv32-esp-elf/*/riscv32-esp-elf/bin 2>/dev/null | tail -1 || true)"
  [[ -n "$toolchain" ]] && export PATH="$toolchain:$PATH"
fi
command -v riscv32-esp-elf-gcc >/dev/null 2>&1 || {
  echo "error: riscv32-esp-elf-gcc not found; install ESP-IDF or its RISC-V toolchain" >&2
  exit 2
}

archs=("$@")
[[ ${#archs[@]} -eq 0 ]] && archs=(esp32c6 qemu)
build_dir="$repo_dir/build"
dist_dir="$repo_dir/dist"
mkdir -p "$build_dir" "$dist_dir"
prg32() { (cd "$prg32_repo" && python3 -m prg32 "$@"); }
# Minimal freestanding string.h shim for the cartridge translation unit.
export CPATH="$repo_dir/sdk${CPATH:+:$CPATH}"

# Portable cartridges carry no relocations: prove the image is load-address independent.
PRG32_REPO="$prg32_repo" python3 "$repo_dir/tools/check_relocatable.py" \
  --symbols "$build_dir/cart_symbols.json" | tail -1

for arch in "${archs[@]}"; do
  raw="$build_dir/$name-$arch.raw.prg32"
  out="$dist_dir/$name-$arch.prg32"
  log="$build_dir/$name-$arch.build.log"
  prg32 cartridge build "$repo_dir/src/cartridge_all.c" \
    --portable \
    --optional-feature audio \
    --entry-prefix arelia \
    --name Pompeii \
    --architecture "$arch" \
    --cart-ram-kib "$cart_ram_kib" \
    --out "$raw" > "$log"
  mem="$(grep -o 'mem=[0-9]*' "$log" | tail -1 | cut -d= -f2)"

  if [[ -f "$repo_dir/assets/store/icon.png" && -f "$repo_dir/assets/store/screenshot.png" ]]; then
    prg32 store attach-metadata "$raw" --out "$out" \
      --metadata "$repo_dir/metadata/metadata.json" \
      --icon "$repo_dir/assets/store/icon.png" \
      --screenshot "$repo_dir/assets/store/screenshot.png" \
      --colophon "$repo_dir/metadata/colophon.json" \
      --architecture "$arch" >/dev/null
  else
    echo "note: assets/store art missing (tools/make_store_art.py); $out has no Store trailer" >&2
    cp "$raw" "$out"
  fi
  size=$(wc -c < "$out" | tr -d ' ')
  echo "$out: package $size/$package_limit, RAM $mem/$(( cart_ram_kib * 1024 ))"
  (( size <= package_limit )) || { echo "error: package too large" >&2; exit 1; }
  (( mem <= cart_ram_kib * 1024 )) || { echo "error: executable RAM exceeded" >&2; exit 1; }
  prg32 cartridge summary "$out" > "$build_dir/$name-$arch.summary.json"
done
(cd "$dist_dir" && shasum -a 256 $name-*.prg32 > SHA256SUMS)
echo "summaries in build/*.summary.json; checksums in dist/SHA256SUMS"
