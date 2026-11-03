#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DARK-1: checks on the audio statistics tests/dark/render_all.mjs measured from the real engine (the wasm), 210 presets.
#   test_audio.py render_stats.json
import json, sys, itertools, math, statistics as st
d = json.load(open(sys.argv[1])); R = d['results']
fails = []
def check(c, m):
    if not c: fails.append(m)
check(len(R) == 210, f'{len(R)} presets rendered')
check(not d['fails'], f'render failures: {d["fails"][:3]}')
check(all(r['paramMismatch'] == 0 for r in R), 'every selected preset reached the engine with exactly its recipe: ' + str([(r["g"], r["i"], r["paramMismatch"]) for r in R if r['paramMismatch']][:5]))
check(all(not r['nan'] and 0.01 < r['peak'] < 0.999 for r in R), 'no preset is silent, clipped or NaN')
check(all(0.02 < r['rms'] < 0.5 for r in R), 'every preset has a sane loudness: ' + str([(r["g"], r["i"], r["rms"]) for r in R if not 0.02 < r['rms'] < 0.5][:5]))
def feat(r): return [r['rms'] * 4, math.log(max(r['centroid'], 20)) / 2, *[b for b in r['bands']]]
def dist(a, b): return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))
# 1. presets sound different from one another: within a genre, ten distinct renderings
for g in range(21):
    F = [feat(r) for r in R if r['g'] == g]
    dm = min(dist(a, b) for a, b in itertools.combinations(F, 2))
    check(dm > 0.001, f'genre {g}: two presets render the same ({dm:.4f})')
sig = {(round(r['rms'], 3), round(r['centroid'] / 20), tuple(round(b, 2) for b in r['bands'])) for r in R}
check(len(sig) >= 200, f'only {len(sig)} of 210 presets have a distinct audio signature')
# 2. genres sound different from one another: genre means
M = {g: [st.mean(c) for c in zip(*[feat(r) for r in R if r['g'] == g])] for g in range(21)}
pairs = sorted((dist(M[a], M[b]), a, b) for a, b in itertools.combinations(range(21), 2))
check(pairs[0][0] > 0.01, f'genres {pairs[0][1]} and {pairs[0][2]} sound the same ({pairs[0][0]:.4f})')
name = {g: next(r['genre'] for r in R if r['g'] == g) for g in range(21)}
for a, b, why in [(1, 0, 'horror vs dark techno'), (3, 2, 'industrial vs dark EDM')]:
    check(dist(M[a], M[b]) > 0.04, f'{why}: too alike ({dist(M[a], M[b]):.3f})')
spread = (st.pstdev([r['rms'] for r in R]), st.pstdev([r['centroid'] for r in R]))
check(spread[1] > 40, f'the centroid barely varies across the bank ({spread[1]:.0f} Hz)')
print(f'audio: 210 presets, {len(sig)} distinct signatures, closest genre pair {name[pairs[0][1]]}/{name[pairs[0][2]]} at {pairs[0][0]:.3f}, '
      f'horror-vs-darktech {dist(M[1], M[0]):.3f}, industrial-vs-darkedm {dist(M[3], M[2]):.3f}, {len(fails)} failures')
for f in fails: print('  FAIL', f)
sys.exit(1 if fails else 0)
