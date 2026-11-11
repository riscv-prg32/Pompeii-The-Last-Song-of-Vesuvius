#!/usr/bin/env python3
"""Play Pompeii: The Last Song of Vesuvius to the end with button presses only.

The policy reads the game state (the C `AreliaGame` struct) and answers with
the buttons to hold for the next tap. It drives two hosts:

* the host build of the game logic through ctypes (`python3 tools/autoplay.py`):
  a regression test that the whole game can be completed from the title;
* the PRG32 QEMU firmware (scripts/qemu_preview.py), where the state is read
  from cartridge RAM through QMP.

Movement is camera relative, as for a player: the bot plans a path over the
district grid (maps parsed from src/world.c), picks the waypoint heading and
presses the D-pad direction whose on-screen meaning is closest to it. It
re-plans after every tap and never writes game memory.
"""
from __future__ import annotations

import ctypes
import math
import re
import subprocess
import sys
import tempfile
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
W, H, ROOMS, MAX_ENT = 16, 20, 4, 10
# Offsets of AreliaGame fields (src/game.h); identical on RV32 ilp32 and on
# LP64 hosts because the struct holds no pointers or longs. check_layout()
# verifies them against the host compiler.
OFF = dict(mode=0, room=1, hp=2, keys=3, song_known=4, rescued=5, action=6, action_t=7, invuln=8, focus=9,
           sound_event=10, dialogue=11, dialogue_t=12, banner_t=13, note_count=14, tibia_purpose=15, notes=16,
           px=24, pz=26, pyaw=28, walk=30, cam_yaw=32, cam_pitch=34, cam_x=36, cam_y=40, cam_z=44, ticks=48,
           room_ticks=52, story=54, gone=56, buttons=64, prev_buttons=66, ent_count=68, prompt=69,
           lapilli_n=70, palette_phase=71, lap_x=72, lap_z=80, lap_t=88, ent=96)
SIZE = 256
ENT_SIZE = 16
BUTTONS = {"up": 1, "down": 2, "left": 4, "right": 8, "a": 16, "c": 32, "b": 64, "start": 128, "l": 256, "r": 512}
MODE_TITLE, MODE_PLAY, MODE_DEAD, MODE_WON = range(4)
ACT_TIBIA = 3
E_NONE, E_LOOTER, E_CITIZEN, E_RUBBLE, E_KEY, E_DOOR, E_ALTAR, E_SEAL, E_BOAT, E_LIFE = range(10)
AI_FALL, AI_LEAVE = 6, 7
SONG = ("up", "left", "right", "up", "left", "right")
SOLID = set("#~bo")


def load_maps() -> list[list[str]]:
    text = (ROOT / "src" / "world.c").read_text()
    body = text[text.index("world_maps"):text.index("world_spawns")]
    rows = re.findall(r'"([^"]{16})"', body)
    assert len(rows) == ROOMS * H, len(rows)
    return [rows[r * H:(r + 1) * H] for r in range(ROOMS)]


MAPS = load_maps()


def s16(raw: bytes, o: int) -> int:
    return int.from_bytes(raw[o:o + 2], "little", signed=True)


class Ent:
    def __init__(self, raw: bytes, i: int):
        o = OFF["ent"] + ENT_SIZE * i
        self.i = i
        self.type, self.alive, self.state, self.hp = raw[o], raw[o + 1], raw[o + 2], raw[o + 3]
        self.x, self.z, self.yaw, self.timer = (s16(raw, o + 4 + 2 * k) for k in range(4))
        self.param = raw[o + 12]


class State:
    def __init__(self, raw: bytes):
        assert len(raw) >= SIZE
        for k in ("mode", "room", "hp", "keys", "song_known", "rescued", "action", "focus", "dialogue",
                  "dialogue_t", "note_count", "ent_count"):
            setattr(self, k, raw[OFF[k]])
        self.room = min(self.room, ROOMS - 1)
        self.px, self.pz, self.pyaw, self.cam_yaw = (s16(raw, OFF[k]) for k in ("px", "pz", "pyaw", "cam_yaw"))
        self.story = int.from_bytes(raw[OFF["story"]:OFF["story"] + 2], "little")
        self.ticks = int.from_bytes(raw[OFF["ticks"]:OFF["ticks"] + 4], "little")
        self.gone = [int.from_bytes(raw[OFF["gone"] + 2 * r:OFF["gone"] + 2 * r + 2], "little") for r in range(ROOMS)]
        self.ents = [Ent(raw, i) for i in range(min(self.ent_count, MAX_ENT))]

    def summary(self) -> str:
        return (f"mode={self.mode} room={self.room} pos=({self.px / 256:.2f},{self.pz / 256:.2f}) hp={self.hp} "
                f"keys={self.keys} saved={bin(self.rescued).count('1')}/4 song={self.song_known} story={self.story}")

    def live(self, kind: int) -> list[Ent]:
        return [e for e in self.ents if e.type == kind and e.alive and e.state not in (AI_FALL, AI_LEAVE)]

    def finished(self) -> bool:
        return self.mode == MODE_WON and self.story >= 360


# ---------------------------------------------------------------- geometry
def angle(dx: float, dz: float) -> int:
    """Yaw of (dx, dz): 0 north (-z), 256 east; 1024 per turn."""
    return int(round(math.atan2(dx, -dz) * 512 / math.pi)) & 1023


def adiff(a: int, b: int) -> int:
    return ((b - a + 512) & 1023) - 512


def dist(s: State, x: float, z: float) -> float:
    return math.hypot(x - s.px, z - s.pz)


DIRS = {"up": (0, 1), "down": (0, -1), "left": (-1, 0), "right": (1, 0),
        "up+left": (-1, 1), "up+right": (1, 1), "down+left": (-1, -1), "down+right": (1, -1)}


def steer(s: State, heading: int) -> str:
    """D-pad combination whose camera-relative direction is closest to heading."""
    if s.focus:     # while focused, movement is relative to the line to the target
        e = s.ents[s.focus - 1]
        base = angle(e.x - s.px, e.z - s.pz)
    else:
        base = s.cam_yaw
    best, err = "up", 9999
    for name, (ix, iz) in DIRS.items():
        e = abs(adiff((base + angle(ix, -iz)) & 1023, heading))
        if e < err:
            best, err = name, e
    return best


def blocked_cells(s: State) -> set:
    cells = set()
    for e in s.ents:
        if not e.alive or e.state == AI_LEAVE:
            continue
        if e.type in (E_DOOR, E_SEAL) and e.state == 0 or e.type in (E_RUBBLE, E_ALTAR, E_CITIZEN):
            cells.add((e.x >> 8, e.z >> 8))
    return cells


def path_heading(s: State, goal_cells: set, tx: float, tz: float) -> int | None:
    """Heading toward the next cell of a shortest path to any goal cell."""
    grid = MAPS[s.room]
    start = (s.px >> 8, s.pz >> 8)
    if start in goal_cells:
        return angle(tx - s.px, tz - s.pz)
    blocked = blocked_cells(s) - goal_cells
    prev = {start: None}
    q = deque([start])
    while q:
        c = q.popleft()
        if c in goal_cells:
            path = [c]
            while prev[path[-1]] is not None:
                path.append(prev[path[-1]])
            path.reverse()
            nxt = path[1]
            if len(path) == 2:
                return angle(tx - s.px, tz - s.pz)
            # Aim for the next cell centre; skip it once Livia is close.
            cx, cz = nxt[0] * 256 + 128, nxt[1] * 256 + 128
            if math.hypot(cx - s.px, cz - s.pz) < 70 and len(path) > 2:
                cx, cz = path[2][0] * 256 + 128, path[2][1] * 256 + 128
            return angle(cx - s.px, cz - s.pz)
        for dx, dz in ((0, -1), (0, 1), (-1, 0), (1, 0)):
            n = (c[0] + dx, c[1] + dz)
            if n in prev or not (0 <= n[0] < W and 0 <= n[1] < H):
                continue
            if (grid[n[1]][n[0]] in SOLID or n in blocked) and n not in goal_cells:
                continue
            prev[n] = c
            q.append(n)
    return None


def go(s: State, tx: float, tz: float) -> str:
    h = path_heading(s, {(int(tx) >> 8, int(tz) >> 8)}, tx, tz)
    return "wait" if h is None else steer(s, h)


def use(s: State, e: Ent, button: str, reach: float) -> str:
    """Walk up to an entity, face it and press button."""
    if dist(s, e.x, e.z) > reach:
        return go(s, e.x, e.z)
    want = angle(e.x - s.px, e.z - s.pz)
    if abs(adiff(s.pyaw, want)) > 90:
        return steer(s, want)   # pushing toward a blocker only turns Livia
    return button


# ---------------------------------------------------------------- policy
def decide(s: State) -> str:
    if s.mode == MODE_WON:
        return "wait"
    if s.mode in (MODE_TITLE, MODE_DEAD):
        return "start"
    if s.dialogue:
        return "a" if s.dialogue_t > 40 else "wait"
    r = s.room
    if s.action == ACT_TIBIA:
        near_altar = any(dist(s, e.x, e.z) < 500 for e in s.live(E_ALTAR)) and not s.song_known
        near_seal = s.song_known and any(e.state == 0 and dist(s, e.x, e.z) < 800 for e in s.live(E_SEAL))
        if (near_altar or near_seal) and s.note_count < 6:
            return SONG[s.note_count]
        return "b"
    # Fight: focus the nearest looter, close in and strike.
    looters = sorted(s.live(E_LOOTER), key=lambda e: dist(s, e.x, e.z))
    if looters and dist(s, looters[0].x, looters[0].z) < 2.6 * 256:
        e = looters[0]
        if not s.focus:
            return "l"
        f = s.ents[s.focus - 1]
        return "a" if dist(s, f.x, f.z) < 300 else "up"
    if s.focus:
        return "l"
    gone = s.gone[r]
    def by_type(kind: int) -> list[Ent]:
        return [e for e in s.ents if e.type == kind and e.alive]
    for e in by_type(E_KEY):
        return go(s, e.x, e.z)
    if r == 2 and not s.song_known:
        return use(s, by_type(E_ALTAR)[0], "b", 340)
    for e in by_type(E_RUBBLE):
        return use(s, e, "a", 300)
    if not s.rescued & (1 << r):
        for e in by_type(E_CITIZEN):
            if e.state != AI_LEAVE:
                return use(s, e, "b", 300)
    for e in by_type(E_SEAL):
        if e.state == 0:
            return "c" if dist(s, e.x, e.z) < 600 else go(s, e.x, e.z + 300)
    for e in by_type(E_DOOR):
        if e.state == 0:
            return go(s, e.x, e.z) if dist(s, e.x, e.z) > 200 else "up"
    if r == 3:
        return go(s, 7.5 * 256, 1.4 * 256)
    door = next((x, z) for z, row in enumerate(MAPS[r]) for x, t in enumerate(row) if t == "D")
    return go(s, door[0] * 256 + 128, 20)


def buttons_of(key: str) -> int:
    return sum(BUTTONS.get(k, 0) for k in key.split("+"))


# ---------------------------------------------------------------- host build
def check_layout(tmp: Path) -> None:
    src = tmp / "layout.c"
    fields = [k for k in OFF]
    src.write_text('#include "game.h"\n#include <stddef.h>\n#include <stdio.h>\nint main(void){'
                   + "".join(f'printf("%u ",(unsigned)offsetof(AreliaGame,{k}));' for k in fields)
                   + 'printf("%u %u\\n",(unsigned)sizeof(AreliaGame),(unsigned)sizeof(AreliaEntity));return 0;}\n')
    exe = tmp / "layout"
    subprocess.run(["cc", "-I", str(ROOT / "src"), str(src), "-o", str(exe)], check=True)
    got = [int(v) for v in subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout.split()]
    want = [OFF[k] for k in fields] + [SIZE, ENT_SIZE]
    assert got == want, f"AreliaGame layout changed: {got} != {want}; update OFF in tools/autoplay.py"


def host_playthrough(hold: int = 4, release: int = 2, max_taps: int = 6000, verbose: bool = False) -> tuple[int, int]:
    with tempfile.TemporaryDirectory(prefix="pompeii-autoplay-") as tmp_s:
        tmp = Path(tmp_s)
        check_layout(tmp)
        lib = tmp / "libgame.so"
        subprocess.run(["cc", "-std=c99", "-O1", "-shared", "-fPIC", "-o", str(lib),
                        *(str(ROOT / "src" / f) for f in ("r3d.c", "world.c", "game.c"))], check=True)
        game = ctypes.CDLL(str(lib))
        buf = ctypes.create_string_buffer(SIZE)
        game.game_init(buf)
        frames = 0
        for taps in range(1, max_taps + 1):
            s = State(buf.raw)
            if s.finished():
                return taps, frames
            key = decide(s)
            if verbose:
                print(f"{s.summary()} -> {key}")
            b = buttons_of(key)
            for f in range(hold + release):
                game.game_step(buf, b if f < hold else 0)
            frames += hold + release
        raise AssertionError(f"not finished after {max_taps} taps: {State(buf.raw).summary()}")


if __name__ == "__main__":
    taps, frames = host_playthrough(verbose="-v" in sys.argv)
    print(f"Full playthrough test passed: {taps} taps, {frames} frames ({frames / 30:.0f} s at 30 fps)")
