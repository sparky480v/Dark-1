# DARK-1

DARK-1 is a modification of **PERF-1** (by deamonSLINKY, GPL-3.0-only; itself a fork of HYP-1 by RedtailWorks and AMB-1 by Charles Vestal) for the
M-VAVE FM-1. The instrument is unchanged: every page, screen, button, knob, the sequencer and its tracks, the Auto-DJ, the effects, the workflow, the
navigation and the project saving are PERF-1's. What changed is the music: the twenty-one genres, the 210 factory pieces, and everything in the engine that
makes one genre sound unlike another.

Licence and attribution are kept: `LICENSE` (GPL-3.0), `LICENSING.md` (the full chain of credit, with DARK-1 added to it) and the SPDX headers.

## The genres (all in `firmware/src/dsp/dark_presets.h`)

| # | Name | Description (from `DK_GENRES`) | BPM | Kits used |
|---|------|------|------|------|
| 0 | DARKTECH | dark techno: pounding kicks, rumbling sub bass, hypnotic minor sequences | 126-140 | Anvil, Crush, Pound, Rumble |
| 1 | HORROR | horror electronic: dissonant drones, tritones and clusters, sparse bone-dry hits | 58-100 | Bone, Coffin, Ritual |
| 2 | DARKEDM | dark EDM: huge detuned leads, offbeat bass, eight-bar builds and drops | 124-150 | Crush, Hangar, Pound, Slam |
| 3 | INDUSTR | industrial: machine rhythms, metal hits, driven bass, abrasive FM | 100-135 | Anvil, Crush |
| 4 | DARKRAVE | dark rave: hard kicks, hoover and stab chords, phrygian dominant | 144-165 | Crush, Hangar, Slam |
| 5 | CINEMA | dark cinematic: slow suspense, taiko hits, choirs and strings, long builds | 56-96 | Coffin, Nave, Ritual, Void |
| 6 | DARKELEC | dark electro: broken 808 rhythms, FM bleeps and a gloomy bassline | 108-128 | Anvil, Crush, Machine, Rumble |
| 7 | DARKTRNC | dark trance: rolling off-beat bass, minor supersaws, long breakdowns | 136-148 | Crush, Hangar, Slam |
| 8 | EBM | EBM: sequenced sixteenth bass, gated snares, military pulse | 112-132 | Anvil, Crush, Machine |
| 9 | DYSTOPIA | dystopian techno: stripped, rusted, bleak arpeggios, worn tape | 122-136 | Crush, Pound, Ritual, Rumble, Void |
| 10 | INDHORR | industrial horror: slow mechanical dread, noise and metal against a drone | 70-110 | Anvil, Bone, Coffin, Crush |
| 11 | DARKSYNT | dark synthwave: gated drums, minor arpeggios, andalusian progressions | 92-120 | Machine, Pound, Rumble, Slam |
| 12 | DARKDNB | dark drum and bass: neuro reese bass, tight two-step breaks | 168-176 | Anvil, Bone, Neuro |
| 13 | DARKSTEP | dark dubstep: half-time weight, sub kick, wobbling bass | 138-146 | Coffin, Dread, Ritual |
| 14 | RITUAL | ritual: tribal toms against polymeter, drones, chants | 60-110 | Bone, Coffin, Ritual |
| 15 | EXPERIM | experimental electronic: odd lane lengths, whole-tone FM, glitch | 70-140 | Anvil, Bone, Void |

Ten pieces in each. The exact figures live in the file, not here.

## Where the preset path goes (the map)

```
UI genre dial / the page's preset list
  -> ui_select_preset / genre_step          (firmware/src/app/ui.c)
  -> project_set_genre -> take(piece)       (firmware/src/app/project.c)   (only genre 0 keeps edited pieces; the image magic is "DRK1")
  -> recipe: en_piece_g, en_piece_pats_g, en_piece_mods_g                   (firmware/src/dsp/amb.c, from dark_presets.h)
  -> project_apply -> en_set(p, v)          (every parameter into the engine)
  -> en_genre(g)                            (chord habits, section behaviour, bass modes, pad stab rhythm, ghosts: DK_*_TABLE)
  -> genre_fx(g) -> en_genre_fx(g)          (the latched tape / vinyl / pump / phaser)
  -> the voices: KITS, PAD_SOUND, DRONE_SOUND, ACID_SOUND, CM_SOUND + FM6_DK (fm6_dark.h)
```

**One source of truth:** `firmware/src/dsp/dark_presets.h` defines the genre names (`DK_GENRES`), the 210 piece rows (`DK_ROWS`), each genre's base
parameters (`DK_B_*`) and every per-genre engine table (`DK_CHT_TABLE`, `DK_SECT_TABLE`, `DK_BL_*`, `DK_COMP_TABLE`, `DK_GHOST_*`, `DK_PLUCK_RATE_TABLE`,
`DK_MOD_TABLE`, `DK_FX_TABLE`, `DK_PAT_TABLE`). The firmware, the host simulator, the wasm build and the page all read it through `amb.c`; the page has no list
of names of its own (it asks the engine: `web_genre_name`, `web_piece_name`).

## Why the earlier attempt did not work (measured against the real source)

Compared recipe by recipe with the original (`tests/dark/recipe_dump.c` dumps what the engine's own recipe functions return):

* Of the 176 parameters, **159 never changed in any of the 160 recipes**; drum patterns changed in 20 of 160 recipes, step tricks in none.
* Genres 4-15 (120 recipes) still used the original patterns; one genre's patterns were the house patterns.
* The kits, the pad / drone / acid sound banks, the FM voices and every per-genre table (chord habits, sections, bass, comping, ghosts, latched effects) were
  the original ones: the change was names and a flattening "dark factory" step that made recipes more alike, not more different.
* `web/` was untouched: the browser kept its own storage key (`hyp1-emu-store`), so a browser that had run PERF-1 brought the PERF-1 pieces back, and the
  legacy "HYP2/HYP1" bank loader could resurrect old saves on the firmware too.

In this version: 65.7 of 176 parameters change per recipe on average (at least 53), all 160 drum-pattern sets and 140 of 160 step-trick sets differ from the
original, and every one of the 160 recipes is different from every other and from every PERF-1 recipe (`tests/dark/test_recipes.py`).

## What changed in the code

* `firmware/src/dsp/dark_presets.h` (new): the single source of truth above.
* `firmware/src/dsp/amb.c`: the recipe block replaced (`en_piece_g`, `en_piece_pats_g`, `en_piece_mods_g`, `en_genre_fx`, names); six new scales
  (Locrian, harmonic minor, Phrygian dominant, double harmonic, whole tone, blues minor; 1.0.2: Hungarian minor, Neapolitan minor, diminished, Persian); three new chord kinds (diminished 7th, minor-major 7th, cluster); the
  tempo floor lowered from 70 to 56; fourteen new drum kits (kick sweep, decay, click, drive, rumble; snare tones and noise; clap; hat metal; perc FM); the
  pad, drone and acid banks renamed and re-tuned darker (cutoffs, drive, levels); the thirty FM voices replaced; the FM gains re-measured (`tests/host/fmcal.c`).
  Drum pattern lists for pieces 8 and 9 are shifted so no two pieces of a genre share drum patterns.
* `firmware/src/dsp/fm6.inc`, `fm6_dark.h` (new, generated by `tools/gen_dark_fm.py`): thirty original six-operator FM voices (128-byte DX7 records); the
  old Melodee / DX / DY voice banks removed from the tree.
* `firmware/src/app/ui.c`: genre names and latched effects come from the engine; `ui_select_preset` (the page's entry).
* `firmware/src/app/project.c`: image magic "DRK1", no legacy bank loading; the PERF-1 power-on nudge of a start piece's knobs removed, so a factory piece is
  exactly its recipe.
* `web/emu/`: DARK-1 page text and manual; its own storage keys; a preset browser (21 genres x 10 pieces) that talks to the engine; new wasm exports
  (`web_genre_name`, `web_piece_name`, `web_select`, `web_cur_*`, `web_par`, `web_recipe_par`, `web_preset_hash`, ...); the worklet retries a selection that the firmware
  refuses while it is storing.
* `tools/make_standalone.py`: one self-contained HTML (wasm embedded, worklet from a data URL: a Blob URL fails on `file://`).
* `tests/dark/` (new) and the PERF-1 scenarios re-pinned (below).

## Wander (the first feature added on top of the music)

Two settings pages after SET (`WANDER`, `WANDER 2`; **SEL** + **FX**, then the dial) and a strip on the browser page. When on, every 4 to 64 bars, on the bar and in a groove, it does one to three of: a new piece (this genre or any, never one of the last four), a swapped drum kit or part sound, knobs drifting (a step every 220 ms, the tempo a quarter as fast), layers muted (never the kick or the snare). It is all laid on the piece like Mood (`mood_val` adds Wander's override or offset on the way to the engine), so a stored piece is never changed and a new piece starts clean; a knob the player turns, and the player's own mutes, win. Its settings are session-only (not written to the settings object).

Code: `firmware/src/app/ui.c` (`wander_*`, `W_KNOB`, `W_SND`, the two `FXP` pages, `uiv` 18-25), `firmware/src/app/runner.c` (the mute mask), `web/emu/` (exports `web_wander_*`, the page strip). Memory: about 60 bytes of `ui` state; the page adds no table.

## Tests

`tests/dark/run_dark_tests.sh [--no-browser] [--legacy]` builds everything from source and runs:

| Check | What it proves |
|---|---|
| `test_recipes.py` | 160 recipes; none equal to any PERF-1 recipe; unique names; ten different drum sets and parameter sets in every genre; no two presets within 20 values of each other |
| `test_web.py` | the standalone page embeds exactly `build/emu/amb.wasm`; the wasm holds the DARK-1 names and none of the PERF-1 genre, kit or FM voice names; the page uses `dark1-emu-store-v1` and never reads an old key |
| `render_all.mjs` + `test_audio.py` | the real wasm in Node: every genre x piece selected through `web_select`, the recipe's values read back from the running project (0 mismatches), rendered; no silence, NaN or clipping; 160 distinct audio signatures; genres differ (horror vs dark techno, industrial vs dark EDM) |
| `test_wander.py` | Wander in the real app (the host simulator, driven through the panel): off changes nothing; on moves pieces and genres, swaps sounds, drifts knobs and mutes layers, and the engine's own values show it; never the kick or snare; never a recent piece; GENRE and STAY keep their word; off puts everything back |
| `wander_wasm.mjs` | the same in the browser's engine through the page's entry points, minutes of audio: no silence, NaN or clipping while it wanders |
| `browser_test.py` | headless Chromium on the standalone file: switches on, clicks all 16 genre and 10 piece buttons, checks the page and the engine agree, and measures the audio leaving the AudioContext. A PERF-1 save is planted in localStorage first and must not be read |

`--legacy` also runs PERF-1's own suite (unit tests and 69 scenarios). Its value expectations (`par0 == 45`, `tempo == 128`, ...) described the old factory
recipes; they were re-pinned from a run of the real app by `tests/dark/rebase_scenarios.py`, and the timing scenarios (sections, key drop, Auto-DJ) were
retimed for the dark genres' longer builds. The unit test `amb_test.c` hardcoded old genres ("house loops", "lofi never goes back"); its genre choices were
moved to DARK-1 genres with the same intent.

## What is and is not verified

* **Verified by running the real engine:** every preset selectable, its values in the engine, audio produced and measurably different, in Node and in headless
  Chromium (including through the AudioContext), with no JavaScript synthesis anywhere.
* **Not verified:** how it *sounds* to a person. No one has listened; the audio checks are measurements (level, spectrum, distinctness). Whether the dark
  genres are good music, and whether e.g. DARKEDM and DARKTRNC feel different enough (their spectral signatures are the closest pair), needs ears.
* **Not built:** the hardware firmware. It needs JieLi's pi32v2 toolchain and the AC79 SDK; both are unreachable from the build environment (HTTP 403 through
  its proxy). The C sources compile with gcc (host simulator) and clang (wasm32). Flash and RAM use on the FM-1 (new tables, the FM voices) is therefore unmeasured.
* Android Chrome was not available; the page was tested in desktop (headless) Chromium. It uses only AudioWorklet and WebAssembly, as PERF-1's page did.
