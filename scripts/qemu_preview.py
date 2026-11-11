#!/usr/bin/env python3
"""Run Pompeii: The Last Song of Vesuvius in PRG32 QEMU and play it to the end.

The tools/autoplay.py policy reads the game state from cartridge RAM (QMP
`pmemsave`) and answers with one key on the QEMU UART keyboard; nothing in
the guest is written except key presses. Outputs, in --out:

  console.log, qemu.log   firmware console and QEMU output
  title.png ... ending.png framebuffer captures at story milestones
  run.json                 timings, frame rate, final state, verdict
  audio.wav               firmware audio stream (22,050 Hz mono)
  preview.mp4             with --video: 60 s of real QEMU video and audio

  scripts/qemu_preview.py                          # smoke: play to the end
  scripts/qemu_preview.py --video --out build/qemu-preview

Requirements: the PRG32 QEMU firmware in $PRG32_REPO/build-qemu
(`python3 -m prg32 qemu build`), Espressif qemu-riscv32, riscv32-esp-elf
binutils, Pillow, numpy, and ffmpeg for --video. The flash image is copied to
a temporary directory, so the PRG32 checkout is never modified.

Adapted from scripts/qemu_preview.py of Drumfight Napoli 97 and Galleria2007
(PRG32 project family, MIT).
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time
import wave
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import autoplay  # noqa: E402

PRG32 = Path(os.environ.get("PRG32_REPO", ROOT.parent / "PRG32")).resolve()
CONSOLE_PORT, AUDIO_PORT, QMP_PORT = 4542, 4541, 4543
RATE = 22050
# PRG32 v1.8 QEMU UART keyboard: w/a/s/d, z (or h) = A, x (or j) = B, k = C,
# q = L, e = R, Enter = START. Each byte holds its button for 120 ms and a
# repeat extends the hold, so combinations and held directions work.
KEYS = {"up": b"w", "down": b"s", "left": b"a", "right": b"d", "a": b"z", "b": b"x", "c": b"k",
        "l": b"q", "r": b"e", "start": b"\r"}
MOVES = {"up", "down", "left", "right"}
VIDEO_SECONDS = 60.0


def tool(name: str) -> str:
    found = shutil.which("riscv32-esp-elf-" + name)
    if found:
        return found
    hits = sorted(Path.home().glob(f".espressif/tools/riscv32-esp-elf/*/riscv32-esp-elf/bin/riscv32-esp-elf-{name}"))
    return str(hits[-1]) if hits else "riscv32-esp-elf-" + name


def find_qemu() -> str:
    env = os.environ.get("QEMU_BIN")
    if env:
        return env
    found = sorted(Path.home().glob(".espressif/tools/qemu-riscv32/*/qemu/bin/qemu-system-riscv32"))
    return str(found[-1]) if found else "qemu-system-riscv32"


def firmware_symbols(*names: str) -> dict:
    out = subprocess.run([tool("nm"), str(PRG32 / "build-qemu/PRG32.elf")], capture_output=True, text=True, check=True).stdout
    table = {p[2]: int(p[0], 16) for p in (l.split() for l in out.splitlines()) if len(p) == 3 and p[2] in names}
    missing = set(names) - set(table)
    if missing:
        raise SystemExit(f"firmware ELF lacks {sorted(missing)}")
    return table


def cart_symbols(tmp: Path) -> dict:
    out = tmp / "symbols.json"
    env = dict(os.environ, PRG32_REPO=str(PRG32), PATH=str(Path(tool("gcc")).parent) + os.pathsep + os.environ["PATH"])
    subprocess.run([sys.executable, str(ROOT / "tools/check_relocatable.py"), "--symbols", str(out)],
                   check=True, env=env, stdout=subprocess.DEVNULL)
    return json.loads(out.read_text())


def connect(port: int, proc: subprocess.Popen, timeout: float = 30) -> socket.socket:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError(f"QEMU exited early ({proc.returncode})")
        s = socket.socket()
        try:
            s.connect(("127.0.0.1", port))
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            return s
        except ConnectionRefusedError:
            s.close()
            time.sleep(0.1)
    raise TimeoutError(f"port {port}")


class Qmp:
    def __init__(self, sock: socket.socket):
        self.f = sock.makefile("rw")
        self.f.readline()
        self.lock = threading.Lock()
        self.execute("qmp_capabilities")

    def execute(self, name: str, **arguments):
        msg = {"execute": name}
        if arguments:
            msg["arguments"] = arguments
        self.f.write(json.dumps(msg) + "\n")
        self.f.flush()
        while True:
            reply = json.loads(self.f.readline())
            if "return" in reply:
                return reply["return"]
            if "error" in reply:
                raise RuntimeError(reply["error"])

    def read(self, addr: int, size: int, path: Path, pause: bool = False) -> bytes:
        with self.lock:
            if pause:
                self.execute("stop")
            self.execute("pmemsave", val=addr, size=size, filename=str(path))
            if pause:
                self.execute("cont")
        return path.read_bytes()


def rgb565_image(raw: bytes) -> Image.Image:
    v = np.frombuffer(raw[:320 * 240 * 2], dtype="<u2").reshape(240, 320).astype(np.uint32)
    rgb = np.stack([((v >> 11) & 31) * 527 + 23 >> 6, ((v >> 5) & 63) * 259 + 33 >> 6, (v & 31) * 527 + 23 >> 6], axis=-1)
    return Image.fromarray(rgb.astype(np.uint8), "RGB")


def pump(sock: socket.socket, stop: threading.Event, sink: bytearray, credits: bool):
    """Collect a socket stream; for audio, grant one 20 ms credit ('K') every 20 ms."""
    sock.settimeout(0.005 if credits else 0.3)
    nxt = time.monotonic()
    while not stop.is_set():
        if credits and time.monotonic() >= nxt:
            try:
                sock.sendall(b"K")
            except OSError:
                return
            nxt += 0.02
            if nxt < time.monotonic() - 0.2:
                nxt = time.monotonic()
        try:
            data = sock.recv(8192)
        except (TimeoutError, socket.timeout):
            continue
        except OSError:
            return
        if not data:
            return
        sink.extend(data)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cart", type=Path, default=ROOT / "dist/pompeii-qemu.prg32")
    ap.add_argument("--out", type=Path, default=ROOT / "build/qemu-smoke")
    ap.add_argument("--video", action="store_true", help="record a 60 s MP4 with audio")
    ap.add_argument("--fps", type=int, default=15)
    ap.add_argument("--timeout", type=float, default=600.0)
    ap.add_argument("--tap-gap", type=float, default=None,
                    help="seconds between taps (the firmware holds a key 120 ms); default 0.22, 0.30 with --video")
    ap.add_argument("--title-seconds", type=float, default=5.0, help="time on the title screen in the video")
    args = ap.parse_args()
    tap_gap = args.tap_gap if args.tap_gap is not None else (0.30 if args.video else 0.22)
    args.out.mkdir(parents=True, exist_ok=True)
    flash_src, efuse_src = PRG32 / "build-qemu/qemu_flash.bin", PRG32 / "build-qemu/qemu_efuse.bin"
    for f in (flash_src, efuse_src, args.cart):
        if not f.exists():
            raise SystemExit(f"missing {f}")
    fw = firmware_symbols("g_fb", "prg32_cart_exec")

    with tempfile.TemporaryDirectory(prefix="pompeii-qemu-") as tmp_s:
        tmp = Path(tmp_s)
        game_addr = fw["prg32_cart_exec"] + cart_symbols(tmp)["game"]
        flash, efuse = tmp / "flash.bin", tmp / "efuse.bin"
        shutil.copy2(flash_src, flash)
        shutil.copy2(efuse_src, efuse)
        subprocess.run([sys.executable, "-m", "prg32", "qemu", "upload", str(args.cart.resolve()), "--flash", str(flash)],
                       cwd=PRG32, check=True, stdout=subprocess.DEVNULL)
        cmd = [find_qemu(), "-M", "esp32c3", "-m", "4M",
               "-drive", f"file={flash},if=mtd,format=raw",
               "-drive", f"file={efuse},if=none,format=raw,id=efuse",
               "-global", "driver=nvram.esp32c3.efuse,property=drive,value=efuse",
               "-global", "driver=timer.esp32c3.timg,property=wdt_disable,value=true",
               "-nic", "user,model=open_eth", "-display", "sdl",
               "-qmp", f"tcp:127.0.0.1:{QMP_PORT},server=on,wait=off",
               "-serial", f"tcp::{CONSOLE_PORT},server=on,wait=off,nodelay=on",
               "-serial", f"tcp::{AUDIO_PORT},server=on,wait=on,nodelay=on"]
        log = open(tmp / "qemu.log", "wb")
        env = dict(os.environ, SDL_VIDEODRIVER=os.environ.get("SDL_VIDEODRIVER", "offscreen"))
        proc = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT, env=env)
        stop, video_stop = threading.Event(), threading.Event()
        pcm, transcript = bytearray(), bytearray()
        frames: list[tuple[float, Path]] = []
        report: dict = {"cart": str(args.cart.name), "game_addr": hex(game_addr), "events": []}
        try:
            audio = connect(AUDIO_PORT, proc)
            threading.Thread(target=pump, args=(audio, stop, pcm, True), daemon=True).start()
            console = connect(CONSOLE_PORT, proc)
            threading.Thread(target=pump, args=(console, stop, transcript, False), daemon=True).start()
            qmp = Qmp(connect(QMP_PORT, proc))

            # PRG32 v1.8: g_fb is a pointer to the QEMU RGB panel's own frame
            # memory (older firmware had the array itself); resolve it once the
            # display is up. Fall back to the symbol when it is not a pointer.
            fb_addr = [fw["g_fb"]]

            def resolve_fb() -> None:
                ptr = int.from_bytes(qmp.read(fw["g_fb"], 4, tmp / "ptr.raw"), "little")
                if ptr and ptr != fw["g_fb"] and not (0x3FC80000 <= fw["g_fb"] < ptr < fw["g_fb"] + 4):
                    fb_addr[0] = ptr

            def state() -> autoplay.State:
                return autoplay.State(qmp.read(game_addr, autoplay.SIZE, tmp / "state.raw"))

            def grab(path: Path) -> None:
                rgb565_image(qmp.read(fb_addr[0], 320 * 240 * 2, tmp / "fb.raw", pause=True)).save(path)

            # Wait for the cartridge: the title screen (mode 0) of a fresh game.
            t0 = time.monotonic()
            while True:
                if time.monotonic() - t0 > 60:
                    raise SystemExit("cartridge did not start within 60 s; see console.log")
                s = state()
                if s.mode == autoplay.MODE_TITLE and s.hp == 6 and s.ent_count == 5 and s.ticks > 10 and s.room == 0:
                    break
                # This firmware may boot into PRG32 SETUP; B ("BACK") leaves it
                # and autoloads the only stored cartridge.
                if b"setup_menu" in transcript and time.monotonic() - t0 > 8:
                    console.sendall(KEYS["b"])
                time.sleep(1.0)
            time.sleep(1.5)
            resolve_fb()
            report["framebuffer"] = hex(fb_addr[0])
            report["boot_seconds"] = round(time.monotonic() - t0, 1)

            rec_start = [0.0]
            audio_mark = [0]

            def recorder():
                n, period = 0, 1.0 / args.fps
                nxt = time.monotonic()
                while not video_stop.is_set():
                    raw = tmp / f"f{n:05d}.raw"
                    qmp.read(fb_addr[0], 320 * 240 * 2, raw, pause=True)
                    frames.append((time.monotonic() - rec_start[0], raw))
                    n += 1
                    nxt += period
                    time.sleep(max(0.0, nxt - time.monotonic()))

            if args.video:
                rec_start[0] = time.monotonic()
                audio_mark[0] = len(pcm)
                threading.Thread(target=recorder, daemon=True).start()
                time.sleep(args.title_seconds)        # linger on the title screen
            grab(args.out / "title.png")

            seen, taps = set(), 0
            ticks0, play_t0 = None, time.monotonic()
            won_at = None
            while time.monotonic() - play_t0 < args.timeout:
                s = state()
                if s.mode and ticks0 is None:
                    ticks0, tick_t0 = s.ticks, time.monotonic()
                saved = bin(s.rescued).count("1")
                won, dead = s.mode == autoplay.MODE_WON, s.mode == autoplay.MODE_DEAD
                milestone = (s.room, saved, s.song_known, won, dead)
                if milestone not in seen:
                    seen.add(milestone)
                    report["events"].append({"t": round(time.monotonic() - play_t0, 2), "state": s.summary()})
                    print(f"[{time.monotonic() - play_t0:6.1f}s] {s.summary()}", flush=True)
                    if s.mode and not won:
                        grab(args.out / f"room{s.room}_saved{saved}.png")
                if won:
                    if won_at is None:
                        won_at = time.monotonic()
                        report["ticks_per_second"] = round((s.ticks - ticks0) / (won_at - tick_t0), 1) if ticks0 is not None else None
                    if s.finished():
                        break
                key = autoplay.decide(s)
                parts = [k for k in key.split("+") if k in KEYS]
                if parts and all(k in MOVES for k in parts):
                    # Hold the direction: repeat it inside the 120 ms hold window.
                    for _ in range(3):
                        console.sendall(b"".join(KEYS[k] for k in parts))
                        time.sleep(0.07)
                    taps += 1
                elif parts:
                    console.sendall(b"".join(KEYS[k] for k in parts))
                    taps += 1
                    time.sleep(tap_gap)
                else:
                    time.sleep(0.1)
            final = state()
            final_won = final.mode == autoplay.MODE_WON
            report.update(taps=taps, final=final.summary(), won=final_won,
                          play_seconds=round((won_at or time.monotonic()) - play_t0, 1))
            if final_won:
                time.sleep(1.0)
                grab(args.out / "ending.png")
            if args.video:
                while time.monotonic() - rec_start[0] < VIDEO_SECONDS + 0.5:
                    time.sleep(0.2)
                video_stop.set()
                time.sleep(0.5)
        finally:
            stop.set()
            video_stop.set()
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
            log.close()
            (args.out / "console.log").write_bytes(transcript)
            shutil.copy2(tmp / "qemu.log", args.out / "qemu.log")

        start = audio_mark[0] - audio_mark[0] % 2
        data = bytes(pcm[start:])
        data = data[:len(data) - len(data) % 2]
        with wave.open(str(args.out / "audio.wav"), "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(RATE)
            w.writeframes(data)
        report["audio_seconds"] = round(len(data) / 2 / RATE, 1)
        report["audio_peak"] = int(np.abs(np.frombuffer(data, dtype="<i2")).max()) if data else 0
        if args.video and frames:
            # Constant-rate 30 fps video of the 320x200 game viewport (rows 20-219
            # of the panel; the firmware bands are cropped), scaled 2x: each
            # output frame repeats the latest capture taken before its time.
            vdir = tmp / "v"
            vdir.mkdir()
            out_fps, total = 30, int(VIDEO_SECONDS * 30)
            j = 0
            for k in range(total):
                t = k / out_fps
                while j + 1 < len(frames) and frames[j + 1][0] <= t:
                    j += 1
                rgb565_image(frames[j][1].read_bytes()).crop((0, 20, 320, 220)).resize((640, 400), Image.NEAREST).save(vdir / f"{k:05d}.png")
            mp4 = args.out / "preview.mp4"
            subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", str(out_fps), "-i", str(vdir / "%05d.png"),
                            "-i", str(args.out / "audio.wav"), "-t", f"{VIDEO_SECONDS:.2f}",
                            "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "20",
                            "-c:a", "aac", "-b:a", "96k", "-af", "apad", str(mp4)], check=True)
            report["video"] = {"file": str(mp4.name), "seconds": VIDEO_SECONDS, "captures": len(frames),
                               "capture_fps": round(len(frames) / frames[-1][0], 1) if frames[-1][0] else None}
        report["cartridge_loaded"] = b"cartridge" in transcript.lower()
        (args.out / "run.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps({k: v for k, v in report.items() if k != "events"}, indent=2))
        return 0 if report.get("won") else 1


if __name__ == "__main__":
    raise SystemExit(main())
