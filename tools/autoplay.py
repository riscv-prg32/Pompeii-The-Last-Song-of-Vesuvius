#!/usr/bin/env python3
"""Play Pompeii: The Last Song of Vesuvius to the end with button presses only.

The policy reads the game state (the C `AreliaGame` struct) and answers with
one button tap. It drives two hosts:

* the host build of src/game.c through ctypes (`python3 tools/autoplay.py`):
  a regression test that the whole game can be completed from the title;
* the PRG32 QEMU firmware (scripts/qemu_preview.py), where the state is read
  from cartridge RAM through QMP.

The bot re-plans after every tap, so it tolerates frame-timing differences
between hosts. It only presses buttons; it never writes game memory.
"""
from __future__ import annotations

import ctypes
import subprocess
import sys
import tempfile
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
W, H, ROOMS, ENEMIES = 20, 12, 4, 6
# Offsets of AreliaGame fields (src/game.h); identical on RV32 ilp32 and on
# LP64 hosts because the struct holds no pointers or longs. check_layout()
# verifies them against the host compiler.
OFF = dict(tiles=0, room=240, x=241, y=242, face=243, hp=244, keys=245, crystals=246, notes=247,
           won=248, dead=249, attack=250, invuln=251, move_cooldown=252, enemy_cooldown=253, mode=254,
           sound_event=255, ticks=256, eruption_frames=260, visited=264, collected_keys=268,
           defeated_enemies=272, gate_open=276, cleared_hazards=280, dialogue=284, dialogue_timer=285,
           previous_buttons=286, melody_progress=287, melody_active=288, rescued_count=289, enemies=290)
SIZE = 316
BUTTONS = {"up": 1, "down": 2, "left": 4, "right": 8, "a": 16, "b": 64, "start": 128}
STEP = {"up": (0, -1), "down": (0, 1), "left": (-1, 0), "right": (1, 0)}
FACE = {"up": 0, "down": 1, "left": 2, "right": 3}
MELODY = ("up", "left", "right")
# Per district: key, rubble, citizen, and (district 2) the Sibyl's altar.
KEY = {0: (16, 5), 1: (4, 6)}
RUBBLE = {1: (10, 9), 2: (13, 6), 3: (14, 6)}
CITIZEN = {0: (9, 2), 1: (11, 9), 2: (14, 6), 3: (15, 6)}
ALTAR, DOOR, EXIT = (8, 6), (9, 1), (9, 6)


class State:
    def __init__(self, raw: bytes):
        assert len(raw) >= SIZE
        self.raw = raw
        u8 = lambda k: raw[OFF[k]]
        for k in ("room", "x", "y", "face", "hp", "keys", "crystals", "notes", "won", "dead",
                  "attack", "mode", "dialogue", "melody_progress", "melody_active", "rescued_count"):
            setattr(self, k, u8(k))
        self.ticks = int.from_bytes(raw[256:260], "little")
        self.eruption_frames = int.from_bytes(raw[260:262], "little")
        self.tiles = [list(raw[y * W:(y + 1) * W]) for y in range(H)]
        self.collected_keys = list(raw[268:272])
        self.cleared = list(raw[280:284])
        self.enemies = []
        for i in range(ENEMIES):
            ex, ey, hp, act = (int.from_bytes(raw[290 + 4 * i + j:291 + 4 * i + j], "little", signed=True) for j in range(4))
            if act:
                self.enemies.append((ex, ey))

    def summary(self) -> str:
        return (f"room={self.room} pos=({self.x},{self.y}) hp={self.hp} keys={self.keys} "
                f"saved={bin(self.crystals).count('1')}/4 notes={self.notes} won={self.won} dead={self.dead}")


def passable(s: State, x: int, y: int) -> bool:
    if not (0 <= x < W and 0 <= y < H) or (x, y) in s.enemies:
        return False
    t = s.tiles[y][x]
    # Spend the bronze key only on the Sea Road arch that guards the exit.
    return t not in (1, 2, 7, 8) and (t != 4 or (s.keys > 0 and s.room == 3))


def first_step(s: State, goals: set) -> str | None:
    """First direction of a shortest path to any goal cell (BFS)."""
    start = (s.x, s.y)
    if start in goals:
        return None
    prev = {start: None}
    q = deque([start])
    while q:
        c = q.popleft()
        for name, (dx, dy) in STEP.items():
            n = (c[0] + dx, c[1] + dy)
            if n in prev or not passable(s, *n):
                continue
            prev[n] = (c, name)
            if n in goals:
                while prev[n][0] != start:
                    n = prev[n][0]
                return prev[n][1]
            q.append(n)
    return "wait"


def toward(s: State, tx: int, ty: int) -> str:
    return {(0, -1): "up", (0, 1): "down", (-1, 0): "left", (1, 0): "right"}[(tx - s.x, ty - s.y)]


def act_on(s: State, target: tuple, button: str) -> str:
    """Stand next to a blocking target, face it, press the button."""
    tx, ty = target
    sides = {(tx + dx, ty + dy) for dx, dy in STEP.values() if passable(s, tx + dx, ty + dy) or (tx + dx, ty + dy) == (s.x, s.y)}
    step = first_step(s, sides)
    if step:
        return step
    d = toward(s, tx, ty)
    return button if s.face == FACE[d] else d   # pressing toward a blocker only turns


def walk_to(s: State, target: tuple, then: str | None = None) -> str:
    step = first_step(s, {target})
    return step if step else (then or "wait")


def decide(s: State) -> str:
    if s.won:
        return "wait"
    if s.dead or not s.mode:
        return "start"
    if s.melody_active:
        return MELODY[s.melody_progress % 3]
    for ex, ey in s.enemies:                      # fight anything adjacent
        if abs(ex - s.x) + abs(ey - s.y) == 1:
            d = toward(s, ex, ey)
            return "a" if s.face == FACE[d] else d
    r = s.room
    if r in KEY and not s.collected_keys[r]:
        return walk_to(s, KEY[r])
    if r in RUBBLE and not s.cleared[r]:
        return act_on(s, RUBBLE[r], "a")
    if not s.crystals & (1 << r):
        return act_on(s, CITIZEN[r], "b")
    if r == 2 and s.notes < 3:
        return walk_to(s, ALTAR, "b")
    if r == 3:
        return walk_to(s, EXIT)
    return walk_to(s, DOOR, "up")


# ---------------------------------------------------------------- host build
def check_layout(tmp: Path) -> None:
    src = tmp / "layout.c"
    fields = [k for k in OFF]
    src.write_text('#include "game.h"\n#include <stddef.h>\n#include <stdio.h>\nint main(void){'
                   + "".join(f'printf("%u ",(unsigned)offsetof(AreliaGame,{k}));' for k in fields)
                   + 'printf("%u\\n",(unsigned)sizeof(AreliaGame));return 0;}\n')
    exe = tmp / "layout"
    subprocess.run(["cc", "-I", str(ROOT / "src"), str(src), "-o", str(exe)], check=True)
    got = [int(v) for v in subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout.split()]
    want = [OFF[k] for k in fields] + [SIZE]
    assert got == want, f"AreliaGame layout changed: {got} != {want}; update OFF in tools/autoplay.py"


def host_playthrough(hold: int = 4, release: int = 3, max_taps: int = 2000, verbose: bool = False) -> tuple[int, int]:
    with tempfile.TemporaryDirectory(prefix="pompeii-autoplay-") as tmp_s:
        tmp = Path(tmp_s)
        check_layout(tmp)
        lib = tmp / "libgame.so"
        subprocess.run(["cc", "-std=c99", "-O1", "-shared", "-fPIC", "-o", str(lib),
                        *(str(ROOT / "src" / f) for f in ("game.c", "sprites.c", "music.c"))], check=True)
        game = ctypes.CDLL(str(lib))
        buf = ctypes.create_string_buffer(SIZE)
        game.game_init(buf)
        frames = 0
        for taps in range(1, max_taps + 1):
            s = State(buf.raw)
            if s.won and s.eruption_frames >= 240:
                return taps, frames
            key = decide(s)
            if verbose:
                print(f"{s.summary()} -> {key}")
            for f in range(hold + release):
                game.game_step(buf, BUTTONS.get(key, 0) if f < hold else 0)
            frames += hold + release
        raise AssertionError(f"not finished after {max_taps} taps: {State(buf.raw).summary()}")


if __name__ == "__main__":
    taps, frames = host_playthrough(verbose="-v" in sys.argv)
    print(f"Full playthrough test passed: {taps} taps, {frames} frames")
