#!/usr/bin/env python3
# DARK-1: generates firmware/src/dsp/fm6_dark.h: forty-eight original six-operator FM voices as 128-byte packed DX7 records.
# Written for DARK-1 (no third-party patch data).  Operators are given as op1..op6; the record stores op6 first.
import sys, os
ENV = {  # (rates R1-R4, levels L1-L4)
 'perc':  ((99, 62, 35, 50), (99, 70, 0, 0)),
 'pluck': ((99, 78, 45, 60), (99, 55, 0, 0)),
 'tail':  ((99, 40, 22, 38), (99, 75, 35, 0)),
 'sus':   ((70, 40, 50, 42), (99, 96, 90, 0)),
 'swell': ((32, 36, 50, 36), (99, 99, 94, 0)),
 'organ': ((99, 99, 99, 72), (99, 99, 99, 0)),
 'brass': ((46, 60, 50, 55), (99, 92, 80, 0)),
 'bell':  ((99, 38, 20, 36), (99, 72, 30, 0)),
 'bow':   ((50, 45, 50, 40), (99, 94, 85, 0)),
 'mod_dec': ((99, 50, 30, 50), (99, 45, 12, 0)),
 'mod_hold': ((75, 60, 50, 50), (99, 90, 85, 0)),
 'mod_fall': ((99, 35, 28, 45), (99, 40, 0, 0)),
}
def op(fc, ff=0, ol=99, env='sus', det=7, kvs=2, rs=2, ams=0):
    return dict(fc=fc, ff=ff, ol=ol, env=env, det=det, kvs=kvs, rs=rs, ams=ams)
def rec(name, alg, fb, ops, tr=24, lfo=(35, 0, 0, 0, 0, 0, 3), pms=3):
    assert len(ops) == 6 and 0 <= alg < 32
    b = []
    for n in range(6, 0, -1):               # op6 first
        o = ops[n - 1]
        r, l = ENV[o['env']]
        b += list(r) + list(l)
        b += [39, 0, 0]                      # breakpoint C3, no scaling
        b += [0]                             # curves
        b += [o['rs'] | (o['det'] << 3)]
        b += [o['ams'] | (o['kvs'] << 2)]
        b += [o['ol']]
        b += [(o['fc'] << 1) | 0]            # ratio mode
        b += [o['ff']]
    b += [99, 99, 99, 99, 50, 50, 50, 50]    # flat pitch EG
    b += [alg, fb | 8]                       # algorithm, feedback + osc key sync
    sp, de, pmd, amd, sy, wv, pm = lfo
    b += [sp, de, pmd, amd, (sy | (wv << 1) | (pm << 4))]
    b += [tr]
    nm = (name + ' ' * 10)[:10]
    b += [ord(c) for c in nm]
    assert len(b) == 128, len(b)
    return name, b
Z = lambda: op(1, 0, 0)                      # a silent operator
V = []
# algorithm numbers below are 0-based (alg 5 = 4)
V.append(rec('TINE EP', 4, 4, [op(1,0,92,'tail'), op(1,0,70,'mod_dec'), op(1,0,90,'tail'), op(14,0,62,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('GLS TOLL', 4, 0, [op(1,0,92,'bell'), op(35,40,72,'mod_fall'), op(2,0,70,'bell'), op(7,70,66,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('SUB PULSE', 21, 5, [op(1,0,99,'sus'), op(1,0,88,'mod_hold'), op(1,0,70,'mod_hold'), op(1,0,0), op(1,0,0), op(2,0,60,'mod_dec')], tr=12))
V.append(rec('DREAD BRS', 17, 6, [op(1,0,96,'brass'), op(1,0,86,'brass'), op(1,0,78,'brass'), op(1,0,0), op(1,0,0), op(1,2,74,'brass')], tr=12))
V.append(rec('GHOST PAD', 31, 6, [op(1,0,70,'swell',det=5), op(1,1,66,'swell',det=9), op(2,0,48,'swell',det=6), op(1,0,62,'swell',det=8), op(3,0,40,'swell'), op(1,0,64,'swell',det=4)]))
V.append(rec('LOG SKIN', 4, 6, [op(0,50,96,'perc'), op(0,40,78,'mod_fall'), op(1,0,70,'perc'), op(3,50,58,'mod_fall'), op(1,0,0), op(1,0,0)], tr=12))
V.append(rec('CLD ORGAN', 31, 4, [op(1,0,86,'organ'), op(2,0,76,'organ'), op(3,0,52,'organ'), op(4,0,44,'organ'), op(6,0,36,'organ'), op(1,0,70,'organ')]))
V.append(rec('BONE PICK', 4, 5, [op(1,0,92,'pluck'), op(3,0,72,'mod_fall'), op(1,0,86,'pluck'), op(5,0,64,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('BLK BRASS', 17, 7, [op(1,0,96,'brass'), op(1,0,84,'brass'), op(1,0,82,'brass'), op(1,0,0), op(1,0,0), op(1,0,76,'brass')], tr=12))
V.append(rec('IRON BASS', 21, 7, [op(1,0,99,'sus'), op(1,0,84,'mod_hold'), op(1,0,0), op(1,0,0), op(1,0,0), op(1,0,82,'mod_dec')], tr=12))
V.append(rec('CATHEDRAL', 4, 1, [op(1,0,90,'bell'), op(14,0,70,'mod_fall'), op(1,2,86,'bell'), op(7,0,62,'mod_fall'), op(2,0,60,'bell'), op(2,45,44,'mod_fall')]))
V.append(rec('HOLLOW WD', 4, 2, [op(1,0,94,'perc'), op(2,0,74,'mod_fall'), op(1,0,82,'pluck'), op(5,0,60,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('CLAV BITE', 4, 7, [op(1,0,92,'pluck'), op(2,0,80,'mod_fall'), op(1,0,86,'pluck'), op(5,0,72,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('FUNRL STR', 31, 4, [op(1,0,74,'bow',det=5), op(1,0,70,'bow',det=9), op(1,0,66,'bow',det=7), op(2,0,48,'bow'), op(1,0,64,'bow',det=10), op(1,0,56,'bow',det=4)]))
V.append(rec('SHARD PAD', 4, 3, [op(1,0,82,'swell',det=6), op(5,0,60,'mod_hold'), op(1,0,80,'swell',det=8), op(9,0,58,'mod_hold'), op(2,0,66,'swell'), op(4,0,52,'mod_hold')]))
V.append(rec('WRAITH LD', 22, 7, [op(1,0,92,'sus'), op(1,0,84,'mod_hold'), op(2,0,70,'mod_hold'), op(1,0,0), op(1,0,0), op(2,0,66,'mod_hold')], tr=36))
V.append(rec('DIRGE HRP', 4, 1, [op(1,0,94,'pluck'), op(2,0,70,'mod_fall'), op(1,0,86,'tail'), op(3,0,58,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('DEAD KAL', 4, 0, [op(1,0,94,'pluck'), op(5,40,66,'mod_fall'), op(1,0,0), op(1,0,0), op(1,0,0), op(1,0,0)]))
V.append(rec('BRTH FLUT', 4, 6, [op(1,0,90,'bow'), op(1,0,64,'mod_hold'), op(1,0,0), op(1,0,0), op(1,0,0), op(1,0,0)]))
V.append(rec('RUST PANS', 4, 3, [op(1,0,92,'tail'), op(3,40,76,'mod_fall'), op(2,0,80,'tail'), op(5,20,60,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('REESE BAS', 21, 7, [op(1,0,99,'sus',det=6), op(1,0,70,'mod_hold'), op(1,0,98,'sus',det=8), op(1,0,0), op(1,0,0), op(1,0,76,'mod_dec')], tr=12))
V.append(rec('DOOM TUB', 4, 2, [op(1,0,92,'bell'), op(35,0,76,'mod_fall'), op(1,0,84,'bell'), op(14,50,66,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('SLD MOAN', 22, 6, [op(1,0,96,'swell'), op(1,0,0), op(1,0,0), op(1,0,0), op(1,0,0), op(1,0,0)], tr=12))
V.append(rec('WIRE GTR', 4, 5, [op(1,0,92,'pluck'), op(2,0,76,'mod_fall'), op(1,0,84,'tail'), op(4,0,64,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('DEAD MUTE', 4, 4, [op(1,0,92,'perc'), op(1,0,70,'mod_fall'), op(1,0,0), op(1,0,0), op(1,0,0), op(1,0,0)], tr=12))
V.append(rec('DRIVE GTR', 4, 7, [op(1,0,92,'tail'), op(1,0,88,'mod_hold'), op(1,0,90,'tail'), op(2,0,80,'mod_hold'), op(1,0,0), op(1,0,0)], tr=12))
V.append(rec('ROT SLAP', 21, 7, [op(1,0,99,'pluck'), op(2,0,86,'mod_fall'), op(1,0,0), op(1,0,0), op(1,0,0), op(3,0,80,'mod_fall')], tr=12))
V.append(rec('SIREN BRS', 17, 6, [op(1,0,94,'brass'), op(1,0,90,'brass'), op(1,0,80,'brass'), op(1,0,0), op(1,0,0), op(1,3,78,'brass')]))
V.append(rec('COLD SAX', 22, 7, [op(1,0,90,'bow'), op(1,0,76,'mod_hold'), op(3,0,50,'mod_hold'), op(1,0,0), op(1,0,0), op(1,0,68,'mod_hold')]))
V.append(rec('MUTE HORN', 17, 5, [op(1,0,92,'brass'), op(1,0,78,'brass'), op(1,0,68,'brass'), op(1,0,0), op(1,0,0), op(2,0,60,'brass')]))
V.append(rec('ASH PIANO', 4, 5, [op(1,0,94,'tail'), op(1,0,66,'mod_fall'), op(1,0,86,'tail'), op(14,0,58,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('VOID BELL', 4, 0, [op(1,0,90,'bell'), op(7,0,70,'mod_fall'), op(2,0,76,'bell'), op(11,30,58,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('GRAVE VOX', 31, 5, [op(1,0,72,'swell',det=6), op(2,0,60,'swell',det=8), op(3,0,50,'swell',det=5), op(1,0,66,'swell',det=9), op(4,0,38,'swell'), op(1,0,58,'swell',det=4)]))
V.append(rec('TAR STAB', 17, 6, [op(1,0,94,'brass'), op(1,0,80,'brass'), op(1,0,72,'pluck'), op(1,0,0), op(1,0,0), op(1,1,70,'brass')]))
V.append(rec('HEX BASS', 21, 6, [op(1,0,99,'sus'), op(2,0,80,'mod_hold'), op(1,0,0), op(1,0,0), op(1,0,0), op(3,0,74,'mod_dec')], tr=12))
V.append(rec('MOLD WIND', 4, 3, [op(1,0,88,'bow'), op(2,0,52,'mod_hold'), op(1,0,0), op(1,0,0), op(1,0,0), op(1,0,0)]))
V.append(rec('CRYPT HRP', 4, 2, [op(1,0,94,'pluck'), op(3,0,66,'mod_fall'), op(1,0,84,'tail'), op(2,0,56,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('NEON KNL', 4, 1, [op(1,0,92,'bell'), op(14,0,72,'mod_fall'), op(1,2,80,'bell'), op(9,0,60,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('TOMB GONG', 4, 0, [op(1,0,92,'bell'), op(1,41,72,'mod_fall'), op(2,76,66,'bell'), op(3,0,56,'mod_fall'), op(1,0,0), op(1,0,0)], tr=12))
V.append(rec('CLD HARPS', 4, 4, [op(1,0,94,'pluck'), op(3,0,74,'mod_fall'), op(1,0,86,'pluck'), op(7,0,66,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('RAVEN CLL', 31, 6, [op(1,0,76,'swell',det=4), op(3,0,64,'swell',det=10), op(5,0,52,'swell',det=6), op(1,0,70,'swell',det=9), op(2,0,44,'swell'), op(1,0,60,'swell',det=3)]))
V.append(rec('ACID RAIN', 4, 3, [op(1,0,90,'pluck'), op(9,0,70,'mod_fall'), op(2,0,76,'pluck'), op(13,0,62,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('COAL DRUM', 4, 7, [op(0,50,96,'perc'), op(0,30,74,'mod_fall'), op(1,0,66,'perc'), op(2,50,52,'mod_fall'), op(1,0,0), op(1,0,0)], tr=0))
V.append(rec('WIDOW FLT', 4, 1, [op(1,0,88,'bow'), op(2,0,44,'mod_hold'), op(1,0,0), op(1,0,0), op(1,0,0), op(1,0,0)]))
V.append(rec('GRIM STR', 31, 3, [op(1,0,80,'bow',det=5), op(1,0,72,'bow',det=9), op(2,0,54,'bow',det=6), op(1,0,68,'bow',det=10), op(1,0,56,'bow',det=3), op(3,0,40,'bow')]))
V.append(rec('HOLLOW BL', 4, 2, [op(1,0,90,'bell'), op(2,76,70,'mod_fall'), op(3,0,74,'bell'), op(5,10,58,'mod_fall'), op(1,0,0), op(1,0,0)]))
V.append(rec('SLAG LEAD', 22, 7, [op(1,0,92,'bow'), op(1,0,80,'mod_hold'), op(2,0,60,'mod_hold'), op(1,0,0), op(1,0,0), op(1,2,72,'mod_hold')]))
V.append(rec('DUSK CHR', 31, 5, [op(1,0,74,'swell',det=3), op(2,0,62,'swell',det=11), op(3,0,52,'swell',det=7), op(1,0,66,'swell',det=12), op(4,0,40,'swell',det=5), op(1,0,56,'swell',det=2)]))
assert len(V) == 48
out = ['/* SPDX-License-Identifier: GPL-3.0-only */',
       '/* DARK-1: forty-eight original six-operator FM voices (tools/gen_dark_fm.py): 128-byte packed DX7 records, op6 first. Generated: do not edit. */',
       '#pragma once', '#include <stdint.h>', '#define FM6_NDK 48',
       'static const char *const FM6_DK_NAME[FM6_NDK] = {' + ', '.join('"%s"' % n for n, _ in V) + '};',
       'static const uint8_t FM6_DK[FM6_NDK][128] = {']
for i, (n, b) in enumerate(V):
    out.append('    {/* %d: %s */' % (i, n))
    for k in range(0, 128, 16):
        out.append('      ' + ', '.join('%3d' % x for x in b[k:k + 16]) + ',')
    out.append('    },')
out.append('};')
dst = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'firmware', 'src', 'dsp', 'fm6_dark.h')
open(dst, 'w').write('\n'.join(out) + '\n')
print('wrote', dst)
