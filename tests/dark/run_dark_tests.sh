#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# DARK-1: build everything from the source tree and run the DARK-1 checks.
#   tests/dark/run_dark_tests.sh [--no-browser] [--legacy]
# --legacy also runs the PERF-1 suite (tests/run_tests.sh: unit tests and the 69 scenarios, re-pinned to the DARK-1 presets).
set -u
cd "$(dirname "$0")/../.."
FAIL=0; BROWSER=1; LEGACY=0
for a in "$@"; do case $a in --no-browser) BROWSER=0;; --legacy) LEGACY=1;; esac; done
mkdir -p build/dark
step() { name=$1; shift; if "$@" > build/dark/$name.log 2>&1; then echo "  ok   $name: $(tail -1 build/dark/$name.log | cut -c1-150)"; else echo "  FAIL $name"; tail -8 build/dark/$name.log | sed 's/^/       /'; FAIL=1; fi; }
step build-host    sh host/build_host.sh
step build-tools   sh tests/dark/build_tools.sh
step build-wasm    env EN_VERSION=DARK-1 sh web/emu/build_clang.sh
step build-html    python3 -I tools/make_standalone.py build/emu build/emu/DARK-1.html
step fm-voices     sh -c 'python3 -I tools/gen_dark_fm.py build/dark/fm6_dark_check.h && cmp build/dark/fm6_dark_check.h firmware/src/dsp/fm6_dark.h'
step recipes       sh -c 'build/dark/recipe_dump > build/dark/recipes.jsonl && python3 -I tests/dark/test_recipes.py build/dark/recipes.jsonl'
step web-static    python3 -I tests/dark/test_web.py build/emu
step wander-app    python3 -I tests/dark/test_wander.py
step wander-wasm   node tests/dark/wander_wasm.mjs build/emu/amb.wasm build/dark/wander_wasm.json 3
step engine-render sh -c 'node tests/dark/render_all.mjs build/emu/amb.wasm build/dark/render_stats.json 6 && python3 -I tests/dark/test_audio.py build/dark/render_stats.json'
[ $BROWSER = 1 ] && step browser python3 tests/dark/browser_test.py build/emu/DARK-1.html build/dark/browser.json 700
[ $LEGACY = 1 ] && step legacy-suite sh tests/run_tests.sh
[ $FAIL = 0 ] && echo "DARK-1 tests passed" || echo "DARK-1 tests FAILED"
exit $FAIL
