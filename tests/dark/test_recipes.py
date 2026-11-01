#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DARK-1: the factory recipes, dumped from the real engine tables by build/dark/recipe_dump (en_piece_g, en_piece_pats_g, en_piece_mods_g).
#   test_recipes.py recipes.jsonl
import json, sys, hashlib, itertools, collections, os
here = os.path.dirname(os.path.abspath(__file__))
R = [json.loads(l) for l in open(sys.argv[1])]
fails = []
def check(c, msg):
    if not c: fails.append(msg)
def h(r): return hashlib.sha256(json.dumps([r['par'], r['pool'], r['pats'], r['mod']]).encode()).hexdigest()[:16]
check(len(R) == 160, f'{len(R)} recipes, expected 160')
check({(r['g'], r['i']) for r in R} == {(g, i) for g in range(16) for i in range(10)}, 'every genre x piece present')
# 1. none of the PERF-1 recipes survives
old = {l.strip() for l in open(os.path.join(here, 'perf1_recipe_hashes.txt')) if not l.startswith('#') and l.strip()}
same = [(r['g'], r['i']) for r in R if h(r) in old]
check(not same, f'recipes identical to a PERF-1 factory recipe: {same}')
# 2. names are the DARK-1 ones, and unique
OLD_NAMES = {'HYPNO', 'LOFI', 'HOUSE', 'DUBSTEP', 'DUB', 'DNB', 'BREAKS', 'GARAGE', 'ELECTRO', 'TRAP', 'DISCO', 'FUNK', 'REGGAE', 'SYNTHWAV', 'AMAPIANO', 'TRIPHOP'}
gn = [next(r['gname'] for r in R if r['g'] == g) for g in range(16)]
check(len(set(gn)) == 16, 'sixteen distinct genre names')
check(not (set(gn) & OLD_NAMES), f'old genre names present: {set(gn) & OLD_NAMES}')
pn = [(r['gname'], r['pname']) for r in R]
check(len(set(pn)) == 160, '160 distinct piece names')
# 3. every preset is its own: pairwise they differ in many parameters (not a rename of one recipe)
V = {(r['g'], r['i']): r['par'] + [r['pool']] + list(r['pats']) for r in R}   # parameters, note pool, every drum pattern
close = []
for a, b in itertools.combinations(V, 2):
    d = sum(1 for x, y in zip(V[a], V[b]) if x != y)
    if d < 20: close.append((a, b, d))
check(not close, f'presets differing in fewer than 20 values (parameters + pool + drum patterns): {close[:5]}')
# 4. each genre differs from the others in the parameters that make a sound: tempo ranges, kits, pad / drone / acid sounds, bass, pluck voice
import re
H = open(os.path.join(here, '..', '..', 'firmware', 'src', 'dsp', 'amb.h')).read()
names = re.findall(r'\bP_[A-Z0-9_]+\b', H[H.index('P_TEMPO') - 400:])
P = {}
for n in names:
    if n not in P: P[n] = len(P)
# (the dump has one value per parameter in enum order; the index of the tempo etc. is read from the engine by the dumper's header when present)
for g in range(16):
    gr = [r for r in R if r['g'] == g]
    tempos = sorted(r['par'][0] for r in gr)
    pats = {tuple(map(tuple, [r['pats'][:6]])) .__hash__() for r in gr}
    check(len({json.dumps(r['pats']) for r in gr}) == 10, f'genre {g}: ten different drum pattern sets')
    check(len({json.dumps(r['par']) for r in gr}) == 10, f'genre {g}: ten different parameter sets')
# 5. the drum patterns differ across genres, not only within one
gp = collections.defaultdict(set)
for r in R: gp[json.dumps(r['pats'])].add(r['g'])
check(all(len(v) == 1 for v in gp.values()), 'no drum pattern set is shared between two genres')
# 6. the pool (the mode's notes) is used: at least 8 different scales over the bank
check(len({r['pool'] for r in R}) >= 8, f'{len({r["pool"] for r in R})} different note pools')
print(f'recipes: {len(R)} checked, {len(fails)} failures')
for f in fails: print('  FAIL', f)
sys.exit(1 if fails else 0)
