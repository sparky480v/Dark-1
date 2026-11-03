# DARK-1

**Dark-music firmware for the M-VAVE FM-1: a modification of [PERF-1](https://github.com/deamonSLINKY/PERF-1) by deamonSLINKY (GPL-3.0-only).**
The instrument is PERF-1's (plus the Wander pages): its pages and screens, buttons and knobs, sequencer and tracks, Auto-DJ, effects and project saving. The music is new:
twenty-one dark genres (dark techno, horror electronic, dark EDM, industrial, dark rave, dark cinematic, dark electro, dark trance, EBM, dystopian techno,
industrial horror, dark synthwave, dark drum and bass, dark dubstep, ritual, experimental, dungeon synth, witch house, darkwave, power noise, dark ambient) with ten factory pieces each, and the engine behind them (drum
kits and patterns, bass, FM voices, chord habits, scales, builds and breakdowns, latched effects) rewritten per genre.

* **Play it in your browser:** <https://sparky480v.github.io/Dark-1/play/> (or open `DARK-1.html` from the release in Chrome). It is the real engine compiled to WebAssembly, with a preset browser above the panel.
* **Install it on an FM-1:** download `DARK-1-1.0.1.fwsc` from the [latest release](https://github.com/sparky480v/Dark-1/releases/latest) and flash it with an FM-1 installer page (it carries its own update loader, so you can leave it again). Installed and tried on one real FM-1 by the author; that is the only hardware it has been tested on.
* **Dice, Wander kept at power-off, two more visualizers, four more scales, eight more FM voices and four more drum kits (new in 1.0.2).** SEL + PLAY rolls the dice; set Wander to ON+KEEP to have it on at every power-on.
* **Wander (new in 1.0.1):** a pair of settings pages (SEL + FX, then the dial to WANDER and WANDER 2) that make the music evolve by itself: it moves between pieces and genres, swaps sounds, drifts knobs and mutes layers, on a schedule and by an amount you set. It never touches the kick or snare, never changes your stored pieces, and its settings are not kept at power-off.
* **What changed, how it is wired, how it was tested, what is not verified:** [docs/DARK-1.md](docs/DARK-1.md).
* **Build and test:** `tools/release.sh VERSION` for the hardware package (JieLi toolchain and SDK, see BUILDING.md), `web/emu/build_clang.sh` (clang with the wasm32 target and wasm-ld), `tools/make_standalone.py`, `tests/dark/run_dark_tests.sh`.
* **Credit:** DARK-1 is a modification of PERF-1 by deamonSLINKY, itself a fork of HYP-1 by RedtailWorks and AMB-1 by Charles Vestal, on the Felucca platform. GPL-3.0-only; see `LICENSE` and `LICENSING.md`. DARK-1 is by Polystellarflow.

## What is in the repository

* `firmware/` the FM-1 firmware: the engine (`src/dsp/`), the app and screens (`src/app/`), the platform layer.
* `web/` the browser version, the installer page and the editor; `docs/play/` is the playable page published on GitHub Pages.
* `docs/MANUAL.md` the manual (its screenshots are made from the firmware itself in the simulator); `docs/DARK-1.md` what was changed and how it was tested; `docs/CHANGELOG.md`.
* `tests/` the automated tests (`tests/dark/run_dark_tests.sh`, and the scenarios in `tests/scenarios/`).
* `BUILDING.md` how to build the hardware package (JieLi's toolchain and SDK, on Linux or WSL).

## Licence

GPL-3.0-only. See `LICENSE` and `LICENSING.md` for the full chain of credit: AMB-1 by Charles Vestal, HYP-1 by RedtailWorks, PERF-1 by deamonSLINKY, and the Felucca platform.
