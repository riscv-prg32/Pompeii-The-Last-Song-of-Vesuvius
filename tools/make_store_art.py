#!/usr/bin/env python3
"""Make the Cartridge Store icon and screenshot.

  assets/store/icon.png        64x64: Vesuvius smoking over a Pompeian column
  assets/store/screenshot.png  320x200 gameplay frame from PRG32 QEMU

The screenshot is a real frame of the cartridge running on the PRG32 QEMU
firmware, written by `scripts/qemu_preview.py --video --out build/qemu-preview`
(320x240 with the firmware bands, cropped here to the 320x200 viewport).
Both images use the game's 16-colour palette (src/game.c) and are palettised
because the Store trailer counts towards the 64 KiB cartridge slot.
"""
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
PAL = [0x181824, 0x3e3334, 0x607445, 0x9b8c62, 0x476878, 0x5c6542, 0xe3c58e, 0x665a52,
       0xaba08a, 0xb04437, 0x986d4a, 0x619b91, 0xe8b650, 0x9c687b, 0xe7d7b2, 0xffffff]
C = [((c >> 16) & 255, (c >> 8) & 255, c & 255) for c in PAL]


def palettised(img: Image.Image) -> Image.Image:
    pal = Image.new("P", (1, 1))
    pal.putpalette([v for rgb in C for v in rgb] + [0] * (768 - 48))
    return img.convert("RGB").quantize(palette=pal, dither=Image.Dither.NONE)


def icon() -> Image.Image:
    img = Image.new("RGB", (64, 64), C[0])
    d = ImageDraw.Draw(img)
    for y in range(40):                                   # dusk sky fading into ash
        d.line((0, y, 63, y), fill=C[13] if y < 14 else C[9] if y < 26 else C[12])
    d.polygon([(2, 46), (22, 22), (30, 24), (36, 21), (60, 46)], fill=C[1])   # Vesuvius
    d.polygon([(22, 22), (30, 24), (36, 21), (34, 26), (25, 27)], fill=C[9])   # glowing crater
    for x, y, r in ((29, 15, 6), (24, 9, 5), (34, 7, 5), (29, 3, 4), (40, 12, 4)):  # plume
        d.ellipse((x - r, y - r, x + r, y + r), fill=C[7])
    for x, y in ((14, 18), (45, 24), (50, 16), (10, 30), (55, 33)):                 # embers
        d.rectangle((x, y, x + 1, y + 1), fill=C[12])
    d.rectangle((0, 46, 63, 63), fill=C[3])                                        # street
    d.rectangle((8, 30, 15, 33), fill=C[14])                                       # column capital
    d.rectangle((9, 34, 14, 58), fill=C[14])
    d.line((11, 34, 11, 58), fill=C[8])
    d.rectangle((7, 58, 16, 61), fill=C[8])
    hero = ["..aaaa..", ".aaaaaa.", ".266662.", ".66ff66.", ".2eeee2.", ".2eeee2.", "..7777..", ".77..77."]
    for y, row in enumerate(hero):                                                  # Livia (game sprite)
        for x, ch in enumerate(row):
            if ch != ".":
                d.rectangle((40 + x * 2, 46 + y * 2, 41 + x * 2, 47 + y * 2), fill=C[int(ch, 16)])
    d.rectangle((0, 0, 63, 63), outline=C[12])
    return img


def screenshot(source: Path) -> Image.Image:
    img = Image.open(source).convert("RGB")
    if img.size == (320, 240):
        img = img.crop((0, 20, 320, 220))
    if img.size != (320, 200):
        raise SystemExit(f"{source}: expected a 320x240 QEMU frame or 320x200 viewport, got {img.size}")
    return img


def main() -> int:
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build/qemu-preview/room2_saved3.png"
    if not source.exists():
        raise SystemExit(f"missing {source}: run scripts/qemu_preview.py --video --out build/qemu-preview")
    out = ROOT / "assets/store"
    out.mkdir(parents=True, exist_ok=True)
    palettised(icon()).save(out / "icon.png", optimize=True)
    palettised(screenshot(source)).save(out / "screenshot.png", optimize=True)
    for f in ("icon.png", "screenshot.png"):
        print(f"{out / f}: {(out / f).stat().st_size} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
