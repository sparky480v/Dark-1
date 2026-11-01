#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DARK-1: the PERF-1 scenarios pin the old factory presets' values (par0 == 45, tempo == 128 ...).  Those that changed because the presets are now the
# DARK-1 ones are re-pinned here from a run of the real app in the simulator.  Only plain value names are touched; a scenario that fails on a timing or
# a behaviour (sec, keyon, djphase ...) is left alone and listed, to be looked at by hand.
import re, sys, os, glob
VALUE = re.compile(r'^(par\d+|pool|tempo|lane\d+|pats\d+_\d+|moodacid|chordiv|chordroot|fxlock)$')
import subprocess
changed, manual = 0, []
for path in sorted(glob.glob('tests/scenarios/*.amb')):
    name = os.path.basename(path)[:-4]
    for rounds in range(6):                      # a re-pinned value can move what follows it: run again
        os.makedirs(f'build/scenarios/{name}', exist_ok=True)
        out = subprocess.run(['build/host/amb_host', path, f'build/scenarios/{name}'], capture_output=True, text=True).stdout.split('\n')
        src = open(path).read().split('\n')
        fixed = left = 0
        for k, l in enumerate(out):
            m = re.match(r'FAIL expect (\S+) == (\S+), got (-?\d+)', l)
            if not m: continue
            ln = re.match(r'\s*\(scenario line (\d+)\)', out[k + 1]) if k + 1 < len(out) else None
            if not ln or not VALUE.match(m.group(1)): left += 1; continue
            q = src[int(ln.group(1)) - 1].split()
            if len(q) < 3 or q[0] != 'expect': left += 1; continue
            q[2] = hex(int(m.group(3))).upper().replace('0X', '0x') if q[2].lower().startswith('0x') else m.group(3)
            src[int(ln.group(1)) - 1] = ' '.join(q); fixed += 1
        if fixed: open(path, 'w').write('\n'.join(src)); changed += fixed
        if not fixed: break
    if left: manual.append((name, f'{left} behavioural failures left'))
print('re-pinned', changed, 'values'); [print(' manual:', m) for m in manual]
