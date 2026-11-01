#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# DARK-1 browser test: the standalone page in headless Chromium.  Switches on, clicks every genre and piece in the preset browser, checks the page and
# the engine agree on what is selected, and measures the audio that actually leaves the AudioContext (an AnalyserNode on the worklet's output).
# A PERF-1 style save is planted in localStorage first: it must not be read.
#   browser_test.py build/emu/DARK-1.html out.json [ms-per-preset]
import json, sys, time
from playwright.sync_api import sync_playwright
page_file, out = sys.argv[1], sys.argv[2]
MS = int(sys.argv[3]) if len(sys.argv) > 3 else 1200
INIT = """
window.__tap = null;
const _AWN = window.AudioWorkletNode;
window.AudioWorkletNode = class extends _AWN { constructor(ctx, name, o) { super(ctx, name, o);
  const an = ctx.createAnalyser(); an.fftSize = 2048; this.connect(an); window.__tap = an; window.__ctx = ctx; } };
try { localStorage.setItem('hyp1-emu-store', JSON.stringify({0: 'AAAA'})); localStorage.setItem('hyp1-emu-midimap', '{}'); } catch (e) {}
"""
res, fails = [], []
with sync_playwright() as p:
    b = p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required', '--no-sandbox'])
    pg = b.new_page(viewport={'width': 1100, 'height': 1300})
    errs = []
    pg.on('pageerror', lambda e: errs.append(str(e)))
    pg.on('console', lambda m: errs.append(m.text) if m.type == 'error' else None)
    pg.add_init_script(INIT)
    pg.goto('file://' + __import__('os').path.abspath(page_file))
    assert 'DARK-1' in pg.title(), pg.title()
    pg.click('#go')
    pg.wait_for_function('window.darkNow && window.__tap', timeout=30000)
    pg.wait_for_timeout(4500)                      # the boot cards
    ng = pg.eval_on_selector_all('#pr-genres button', 'e => e.length'); npc = pg.eval_on_selector_all('#pr-pieces button', 'e => e.length')
    print('genre buttons', ng, 'piece buttons', npc)
    if ng != 16 or npc != 10: fails.append(f'buttons {ng}x{npc}')
    keys = pg.evaluate('Object.keys(localStorage)')
    pg.screenshot(path=out.replace('.json', '_start.png'))
    probe = """() => { const an = window.__tap, a = new Float32Array(an.fftSize); let pk = 0, s = 0, n = 0;
      return new Promise(res => { const t0 = performance.now(); const f = () => { an.getFloatTimeDomainData(a); for (const v of a) { pk = Math.max(pk, Math.abs(v)); s += v*v; n++; }
        if (performance.now() - t0 < %d) setTimeout(f, 30); else res({pk, rms: Math.sqrt(s/n)}); }; f(); }); }""" % MS
    for g in range(ng):
        pg.click(f'#pr-genres button[data-g="{g}"]')
        for i in range(npc):
            pg.click(f'#pr-pieces button[data-i="{i}"]')
            try:
                pg.wait_for_function(f'window.darkNow && window.darkNow.g === {g} && window.darkNow.i === {i}', timeout=5000)
            except Exception:
                fails.append(f'select {g}/{i} -> {pg.evaluate("JSON.stringify(window.darkNow)")}; screen: {pg.inner_text("#status")}; log {pg.evaluate("JSON.stringify([ambLast.sw, ambLast.sl, ambLast.ring])")}')
                continue
            a = pg.evaluate(probe)
            nm = pg.evaluate('window.darkNow')
            res.append({'g': g, 'i': i, 'genre': nm['genre'], 'piece': nm['piece'], **a})
            if a['pk'] < 0.005: fails.append(f'silent {g}/{i}')
        if g in (0, 5, 15): pg.screenshot(path=out.replace('.json', f'_g{g}.png'))
    # Wander: switched on from the page's own strip (every 4 bars, wild, any genre, all four kinds); pieces must move by themselves, the audio must go on, and off puts it all back
    pg.click('#wd-on'); pg.select_option('#wd-every', '0'); pg.select_option('#wd-amt', '2'); pg.select_option('#wd-piece', '2')
    for k in ('kits', 'voices', 'knobs', 'layers'):
        if not pg.is_checked(f'#wd-{k}'): pg.check(f'#wd-{k}')
    pg.wait_for_timeout(800)
    if pg.evaluate('ambLast.wd[0]') != 1 or pg.evaluate('ambLast.wd[2]') != 2 or pg.evaluate('ambLast.wd[3]') != 2: fails.append('wander settings did not reach the engine: ' + pg.evaluate('JSON.stringify(ambLast.wd)'))
    seen, wdsilent, t_end = set(), 0, time.time() + (30 if MS < 400 else 60)
    while time.time() < t_end:
        a = pg.evaluate(probe); nm = pg.evaluate('window.darkNow'); seen.add((nm['g'], nm['i']))
        if a['pk'] < 0.005: wdsilent += 1
    wi = pg.evaluate('ambLast.wi'); wtxt = pg.inner_text('#wd-info')
    print('wander: pieces seen', len(seen), 'info', wi, wtxt)
    if len(seen) < 3: fails.append(f'wander: only {len(seen)} different pieces in the minute')
    if wdsilent: fails.append(f'wander: {wdsilent} silent windows')
    if 'Wander is on' not in wtxt: fails.append('wander: the page does not say it is on: ' + wtxt)
    pg.click('#wd-on'); pg.wait_for_timeout(1500)
    wi = pg.evaluate('ambLast.wi')
    if pg.evaluate('ambLast.wd[0]') != 0 or wi[1] or wi[2] or wi[3]: fails.append('wander: not cleared when switched off: ' + str(wi))
    pg.screenshot(path=out.replace('.json', '_wander.png'))
    keys2 = pg.evaluate('Object.keys(localStorage)')
    info = pg.inner_text('#pr-info')
    b.close()
json.dump({'localStorageKeysAtStart': keys, 'localStorageKeysAtEnd': keys2, 'info': info, 'errors': errs[:10], 'results': res, 'fails': fails}, open(out, 'w'), indent=1)
print('presets measured', len(res), 'fails', fails[:10], 'errors', errs[:5]); print(info)
sys.exit(1 if fails or len(res) != 160 else 0)
