#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""DARK-1 Wander, in the real app (the host simulator drives the firmware's own UI code).

Drives the panel: SEL + FX, the SELECT dial to the WANDER pages, the four knobs; then lets the music play and reads the app's state
(the same probes the legacy scenarios use). Checks, over minutes of music:

  * Wander off: nothing changes by itself.
  * Wander on (every 4 bars, wild, any genre, all four kinds of change): pieces and genres change; sounds are swapped; knobs offset;
    layers muted -- and each shows in the ENGINE (its own value of the parameter is the override / the piece's value plus the offset).
  * The layers it mutes are never the kick or the snare; the bass only when wild.
  * It never chooses a piece again within four changes.
  * Pieces "GENRE": the genre never changes; pieces "STAY": the piece never changes.
  * Wander turned off clears all of it, and the engine is back on the piece's own values.
Usage: test_wander.py [--quick]
"""
import os, re, subprocess, sys, tempfile

SRC = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
HOST = os.path.join(SRC, "build", "host", "amb_host")
QUICK = "--quick" in sys.argv
fails = []


def check(ok, msg):
    print(("  ok   " if ok else "  FAIL ") + msg)
    if not ok:
        fails.append(msg)


def pidx():
    """the numbers of the parameters Wander moves (amb.h's enum, as compiled)"""
    d = os.path.join(SRC, "build", "dark")
    os.makedirs(d, exist_ok=True)
    c = os.path.join(d, "pidx.c")
    names = ["P_AC_CUT", "P_AC_DENS", "P_BRIGHT", "P_DR_CUT", "P_CM_DENS", "P_DRIVE", "P_REVERB", "P_ECHO", "P_EVOLVE", "P_DETUNE", "P_TEMPO",
             "P_KIT_KICK", "P_KIT_SNARE", "P_KIT_HAT", "P_KIT_PERC", "P_PADTONE", "P_DR_INST", "P_AC_WAVE", "P_CM_SND"]
    open(c, "w").write('#include <stdio.h>\n#include "amb.h"\nint main(void){' + "".join(f'printf("%d ",{n});' for n in names) + "return 0;}\n")
    subprocess.check_call(["cc", "-DEN_HOST", "-I" + os.path.join(SRC, "firmware", "src", "dsp"), "-o", os.path.join(d, "pidx"), c])
    v = list(map(int, subprocess.check_output([os.path.join(d, "pidx")]).split()))
    return v[:11], v[11:]


KNOB_P, SND_P = pidx()
PAGE_SPINS = 23                                  # SEL + FX, then 23 turns of SELECT: the WANDER page (26); one more: WANDER 2 (27)


def go_wander(extra):
    """the commands that open the WANDER page and set: [on, every idx delta, amount delta, pieces delta], and the second page's [kits, sounds, knobs, layers] deltas"""
    L = ["seed 0", "wait 1500", "press SEL", "tap FX", "release SEL", "wait 100"]
    L += ["spin SELECT 1", "wait 30"] * PAGE_SPINS
    L += ["expect fxpage 26"]
    return L + extra


def probe_lines(names):
    return [f"expect {n} 999999" for n in names]


NAMES = (["wander", "genre", "piece", "wmute"] + [f"wovr{i}" for i in range(8)] + [f"woff{i}" for i in range(11)] + [f"wtgt{i}" for i in range(11)]
         + [f"whist{i}" for i in range(4)] + [f"enpar{p}" for p in KNOB_P + SND_P] + [f"par{p}" for p in KNOB_P + SND_P])


def run(lines, samples, step_ms):
    """lines (setup), then `samples` times: wait step_ms and read every probe. Returns the list of dicts."""
    L = list(lines)
    for _ in range(samples):
        L.append(f"wait {step_ms}")
        L += probe_lines(NAMES)
    with tempfile.TemporaryDirectory() as td:
        f = os.path.join(td, "w.amb")
        open(f, "w").write("\n".join(L) + "\n")
        out = subprocess.run([HOST, f, td], capture_output=True, text=True).stdout.splitlines()
    rows, cur = [], {}
    for l in out:
        m = re.match(r"FAIL expect (\w+) == 999999, got (-?\d+)", l)
        if m:
            cur[m[1]] = int(m[2])
            if m[1] == NAMES[-1]:
                rows.append(cur)
                cur = {}
        elif l.startswith("FAIL"):
            print("  (setup) " + l)
            fails.append(l)
    return rows, out


def clamp_range():
    return {p: None for p in KNOB_P}


# ------------------------------------------------------------------------------------------------------------------------------
print("Wander off: nothing changes by itself")
rows, _ = run(["seed 0", "wait 1500"], 4 if QUICK else 8, 15000)
check(len(rows) > 0, f"{len(rows)} samples")
check(all(r["wander"] == 0 and r["wmute"] == 0 and all(r[f"wovr{i}"] == -1 for i in range(8)) and all(r[f"woff{i}"] == 0 for i in range(11)) for r in rows), "no override, offset or mute")
check(all(r["piece"] == rows[0]["piece"] and r["genre"] == rows[0]["genre"] for r in rows), "the piece and genre stay")
check(all(r[f"enpar{p}"] == r[f"par{p}"] for r in rows for p in KNOB_P + SND_P), "the engine plays the piece's own values")

# ------------------------------------------------------------------------------------------------------------------------------
print("Wander on: every 4 bars, wild, any genre, all four kinds of change")
setup = go_wander(["spin K1 1", "wait 50", "expect wander 1", "spin K2 -2", "wait 50", "spin K3 1", "wait 50", "spin K4 1", "wait 50", "tap FX", "wait 100"])
samples = 12 if QUICK else 36
rows, out = run(setup, samples, 7500)
check(len(rows) == samples, f"{len(rows)} samples")
check(all(r["wander"] == 1 for r in rows), "on throughout")
check(len({r["genre"] for r in rows}) >= 3, f"{len({r['genre'] for r in rows})} different genres visited")
check(len({(r["genre"], r["piece"]) for r in rows}) >= 5, f"{len({(r['genre'], r['piece']) for r in rows})} different pieces visited")
seen_ovr = {i for r in rows for i in range(8) if r[f"wovr{i}"] >= 0}
check(len(seen_ovr) >= 2, f"sounds swapped on slots {sorted(seen_ovr)}")
check(any(any(r[f"woff{i}"] != 0 for i in range(11)) for r in rows) or any(any(r[f"wtgt{i}"] != 0 for i in range(11)) for r in rows), "knobs sent drifting")
check(any(r["wmute"] != 0 for r in rows), "layers muted at times")
bad_mute = [r["wmute"] for r in rows if r["wmute"] & 0b101]                 # bit 0 kick, bit 2 snare
check(not bad_mute, "the kick and the snare are never muted")
# the engine shows what Wander did
eng_ok = True
for r in rows:
    for i, p in enumerate(SND_P):
        want = r[f"wovr{i}"] if r[f"wovr{i}"] >= 0 else r[f"par{p}"]
        if r[f"enpar{p}"] != want:
            eng_ok = False
            print(f"    sound {i}: override {r[f'wovr{i}']} piece {r[f'par{p}']} engine {r[f'enpar{p}']}")
check(eng_ok, "the engine's sounds are the overrides, or the piece's own")
knob_ok = True
for r in rows:
    for i, p in enumerate(KNOB_P):
        base, off, eng = r[f"par{p}"], r[f"woff{i}"], r[f"enpar{p}"]
        if off == 0 and eng != base:
            knob_ok = False
            print(f"    knob {i} (param {p}): offset 0, piece {base}, engine {eng}")
        elif off != 0 and not (abs(eng - base) <= abs(off) and (eng - base) * off >= 0 and (eng == base + off or eng - base != 0 or eng == base)):
            knob_ok = False
            print(f"    knob {i} (param {p}): offset {off}, piece {base}, engine {eng}")
check(knob_ok, "the engine's knobs are the piece's value plus Wander's offset")
hist_ok = all(len({r[f'whist{k}'] for k in range(4) if r[f'whist{k}']}) == len([1 for k in range(4) if r[f'whist{k}']]) for r in rows)
check(hist_ok, "no piece is chosen again within four changes")
allowed = 0b11101010 | 0b00010000
check(all((r["wmute"] & ~allowed) == 0 for r in rows), "only hats, perc, acid, pad, pluck and (wild) bass are muted")

# ------------------------------------------------------------------------------------------------------------------------------
print("Wander on, pieces: THIS GENRE")
setup = go_wander(["spin K1 1", "wait 50", "spin K2 -2", "wait 50", "spin K3 1", "wait 50", "tap FX", "wait 100"])     # pieces stay at the default (GENRE)
rows, _ = run(setup, 8 if QUICK else 20, 7500)
check(all(r["genre"] == 0 for r in rows), "the genre never changes")
check(len({r["piece"] for r in rows}) >= 2, f"{len({r['piece'] for r in rows})} different pieces in it")

print("Wander on, pieces: STAY")
setup = go_wander(["spin K1 1", "wait 50", "spin K2 -2", "wait 50", "spin K3 1", "wait 50", "spin K4 -1", "wait 50", "tap FX", "wait 100"])
rows, _ = run(setup, 8 if QUICK else 20, 7500)
check(all(r["piece"] == 0 and r["genre"] == 0 for r in rows), "the piece never changes")
check(any(r["wmute"] != 0 or any(r[f"wovr{i}"] >= 0 for i in range(8)) or any(r[f"woff{i}"] != 0 for i in range(11)) for r in rows), "but the sounds, knobs and layers do")

# ------------------------------------------------------------------------------------------------------------------------------
print("Wander turned off: everything goes back")
on = go_wander(["spin K1 1", "wait 50", "spin K2 -2", "wait 50", "spin K3 1", "wait 50", "spin K4 -1", "wait 50", "tap FX", "wait 60000"])   # a minute of it
off = ["press SEL", "tap FX", "release SEL", "wait 100"] + ["spin SELECT 1", "wait 30"] * PAGE_SPINS + ["spin K1 -1", "wait 60", "tap FX", "wait 1500"]
rows, _ = run(on + off, 1, 1000)
r = rows[0] if rows else {}
check(r.get("wander") == 0, "off")
check(r.get("wmute") == 0 and all(r.get(f"wovr{i}") == -1 for i in range(8)) and all(r.get(f"woff{i}") == 0 for i in range(11)), "no override, offset or mute left")
check(all(r.get(f"enpar{p}") == r.get(f"par{p}") for p in KNOB_P + SND_P), "the engine plays the piece's own values again")

# ------------------------------------------------------------------------------------------------------------------------------
print("Wander's settings are kept across a power-off (1.0.2); ON+KEEP starts it on")
def kept(keep):
    L = go_wander(["spin K1 %d" % (2 if keep else 1), "wait 50", "spin K2 1", "wait 50", "spin K3 1", "wait 50", "spin K4 -1", "wait 50", "tap FX", "wait 100", "wait 9000", "reboot", "wait 3000"])
    # after the reboot: the on state, and the page's knobs (uiv19 every, 20 amount, 21 pieces)
    return [l for l in L] + [f"expect {n} 999999" for n in ("wander", "uiv19", "uiv20", "uiv21", "uiv22")]
for keep in (0, 1):
    rows, out = run(kept(keep), 0, 0)
    got = {}
    for l in out:
        m = re.match(r"FAIL expect (\w+) == 999999, got (-?\d+)", l)
        if m:
            got[m[1]] = int(m[2])
    check(got.get("wander") == (2 if keep else 0), f"after power-off, Wander is {'on again (ON+KEEP)' if keep else 'off'}: {got.get('wander')}")
    check(got.get("uiv19") == 3 and got.get("uiv20") == 2 and got.get("uiv21") == 0 and got.get("uiv22") == 1, f"every/amount/pieces/kits kept: {got}")

# ------------------------------------------------------------------------------------------------------------------------------
print("Dice (SEL + PLAY, 1.0.2): a roll of changes with Wander off; the piece stays; the engine plays them")
rows, _ = run(["seed 0", "wait 1500", "wait 40000", "press SEL", "tap PLAY", "release SEL", "wait 1500"], 4, 3000)
r = rows[-1] if rows else {}
check(len(rows) == 4, f"{len(rows)} samples")
check(r.get("wander") == 0, "Wander is still off")
check(r.get("piece") == rows[0].get("piece") and r.get("genre") == rows[0].get("genre"), "the piece stays")
moved = r.get("wmute", 0) != 0 or any(r.get(f"wovr{i}", -1) >= 0 for i in range(8)) or any(r.get(f"wtgt{i}", 0) != 0 for i in range(11))
check(moved, "the dice changed sounds, knobs or layers")
check(any(r.get(f"enpar{p}") != r.get(f"par{p}") for p in KNOB_P + SND_P) or r.get("wmute", 0) != 0, "and the engine plays them")

print()
print("RESULT: %s" % ("FAIL, %d" % len(fails) if fails else "0"))
sys.exit(1 if fails else 0)
