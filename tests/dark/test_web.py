#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DARK-1: static checks that the page is the new build and cannot bring old presets back.
#   test_web.py build/emu
import base64, re, sys, os, hashlib
d = sys.argv[1]
fails = []
def check(c, m):
    if not c: fails.append(m)
wasm = open(os.path.join(d, 'amb.wasm'), 'rb').read()
page = open(os.path.join(d, 'index.html')).read()
solo = open(os.path.join(d, 'DARK-1.html')).read()
# 1. the standalone page embeds exactly this wasm
m = re.search(r'const DARK_WASM_B64 = "([A-Za-z0-9+/=]+)"', solo)
check(m and base64.b64decode(m.group(1)) == wasm, 'DARK-1.html embeds a different wasm than build/emu/amb.wasm')
# 2. the engine in the wasm carries the DARK-1 names and none of the PERF-1 genre / kit / sound names
strs = set(re.findall(rb'[A-Za-z0-9 .#+\-]{3,}', wasm))
for n in [b'DARKTECH', b'HORROR', b'DARKEDM', b'INDUSTR', b'DARKRAVE', b'CINEMA', b'DARKELEC', b'DARKTRNC', b'EBM', b'DYSTOPIA', b'INDHORR', b'DARKSYNT', b'DARKDNB', b'DARKSTEP', b'RITUAL', b'EXPERIM',
          b'Pound', b'Coffin', b'Anvil', b'GHOST PAD', b'DREAD BRSS', b'SUB PULSE']:
    check(n in strs, f'wasm lacks {n.decode()}')
for n in [b'HYPNO', b'LOFI', b'AMAPIANO', b'TRIPHOP', b'HOUSE', b'DUBSTEP', b'GARAGE', b'MF EPRHOD1', b'WINEGLASS', b'DX Tine EP', b'DX Glass Bell', b'Boom Bap']:
    check(n not in strs, f'wasm still carries the PERF-1 name {n.decode()}')
# 3. storage: a DARK-1 key of its own; the PERF-1 / HYP-1 key is only named in a comment, never read
keys = re.findall(r'localStorage\.(?:get|set|remove)Item\((\w+)', page)
check('STORE_KEY' in keys, 'the page does not use STORE_KEY')
check('const STORE_KEY = "dark1-emu-store-v1"' in page, 'STORE_KEY is not the DARK-1 key')
check(not re.search(r'getItem\(\s*"hyp1', page), 'the page reads an old hyp1 key')
# 4. the page gets its preset names from the engine, and has no list of its own
check('web_genre_name' in open(os.path.join(d, 'worklet.js')).read(), 'the worklet does not read the genre names from the engine')
for n in ['HYPNO', 'LOFI', 'AMAPIANO']:
    check(n not in page.replace('hyp1-emu-store', ''), f'index.html names {n}')
# 5. the page really loads this build (no stale cache)
check('cache: "no-store"' in page or 'DARK_WASM_B64' in solo, 'neither a cache-busting fetch nor an embedded wasm')
print(f'web: wasm {len(wasm)} B sha256 {hashlib.sha256(wasm).hexdigest()[:16]}, standalone {len(solo)} B, {len(fails)} failures')
for f in fails: print('  FAIL', f)
sys.exit(1 if fails else 0)
