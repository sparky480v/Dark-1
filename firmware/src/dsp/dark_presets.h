/* SPDX-License-Identifier: GPL-3.0-only */
/* DARK-1 factory content: the ONE place that defines the genres, their pieces and their per-genre engine tables.
 *
 * Everything that differs between genres and pieces lives here and is read by every build (firmware, host
 * simulator, browser/WebAssembly): amb.c (recipes, drum patterns, chord habits, sections, bass figures, ghosts,
 * step tricks), app/ui.c (the genre line on the screen, the genre's latched effects) and the web page (genre and
 * piece lists via the web_genre_name / web_piece_name exports; tools/gen_dark_docs.py writes the page's text from
 * the DK_GENRE lines below).  No other file may carry a genre name or a factory recipe.
 *
 * Based on PERF-1 by deamonSLINKY (GPL-3.0-only).  This file is part of the DARK-1 modification.
 *
 * Layout of a piece: the genre's BASE list (parameter, value pairs, -1 ends: levels, effects, envelopes, LFOs,
 * drum levels, polymeter) and then one dk_row_t (key, mode, chord, the sounds, the kits, the bass mode and the
 * levels).  Both are applied over the engine's defaults by en_piece_g(). */
#pragma once
#include <stdint.h>

/* ----------------------------------------------------------------------------------------- the genres --- */
/* DK_GENRE(index, "SCREEN NAME" (8 chars at most), 'piece letter', "what it is") */
#define DK_GENRES \
    DK_GENRE(0, "DARKTECH", 'T', "dark techno: pounding kicks, rumbling sub bass, hypnotic minor sequences, 126-140 BPM") \
    DK_GENRE(1, "HORROR", 'H', "horror electronic: dissonant drones, tritones and clusters, sparse bone-dry hits, 56-100 BPM") \
    DK_GENRE(2, "DARKEDM", 'E', "dark EDM: huge detuned leads, offbeat bass, eight-bar builds and drops, 124-150 BPM") \
    DK_GENRE(3, "INDUSTR", 'I', "industrial: machine rhythms, metal hits, driven bass, abrasive FM, 100-135 BPM") \
    DK_GENRE(4, "DARKRAVE", 'R', "dark rave: hard kicks, hoover and stab chords, phrygian dominant, 146-165 BPM") \
    DK_GENRE(5, "CINEMA", 'C', "dark cinematic: slow suspense, taiko hits, choirs and strings, long builds, 56-96 BPM") \
    DK_GENRE(6, "DARKELEC", 'L', "dark electro: broken 808 rhythms, FM bleeps and a gloomy bassline, 108-128 BPM") \
    DK_GENRE(7, "DARKTRNC", 'N', "dark trance: rolling off-beat bass, minor supersaws, long breakdowns, 136-148 BPM") \
    DK_GENRE(8, "EBM", 'B', "EBM: sequenced sixteenth bass, gated snares, military pulse, 112-132 BPM") \
    DK_GENRE(9, "DYSTOPIA", 'Y', "dystopian techno: stripped, rusted, bleak arpeggios, worn tape, 124-136 BPM") \
    DK_GENRE(10, "INDHORR", 'Z', "industrial horror: slow mechanical dread, noise and metal against a drone, 70-110 BPM") \
    DK_GENRE(11, "DARKSYNT", 'S', "dark synthwave: gated drums, minor arpeggios, andalusian progressions, 92-120 BPM") \
    DK_GENRE(12, "DARKDNB", 'D', "dark drum and bass: neuro reese bass, tight two-step breaks, 168-176 BPM") \
    DK_GENRE(13, "DARKSTEP", 'U', "dark dubstep: half-time weight, sub kick, wobbling bass, 138-146 BPM") \
    DK_GENRE(14, "RITUAL", 'A', "ritual: tribal toms against polymeter, drones, chants, 60-110 BPM") \
    DK_GENRE(15, "EXPERIM", 'X', "experimental electronic: odd lane lengths, whole-tone FM, glitch, 70-140 BPM")

#define DK_NGENRE 16
#define DK_NPIECE 10

static const char *const DK_GENRE_NAME[DK_NGENRE] = {
#define DK_GENRE(i, name, letter, desc) name,
    DK_GENRES
#undef DK_GENRE
};
static const char DK_PIECE_LETTER[DK_NGENRE] = {
#define DK_GENRE(i, name, letter, desc) letter,
    DK_GENRES
#undef DK_GENRE
};

/* ------------------------------------------------------------------------------------------ the rows --- */
typedef struct {
    uint8_t tempo, root, mode, chord;          /* BPM, key (0 = C), mode (EN_MODE_*), chord kind (EN_CH_*) */
    uint8_t pad, drone, acid, pluck;           /* the sounds: the pad's, the bass's, the lead line's, the pluck's (0..11 plain, 12.. FM voices) */
    uint8_t kk, ks, kh, kp;                    /* the drum kits: kick, snare + clap, hats, perc */
    uint8_t bass;                              /* the bass line mode (P_BL_MODE) */
    uint8_t plvl, alvl, blvl;                  /* levels: pad, lead line, bass */
    uint8_t drive, rev, echo;                  /* drum drive, reverb, echo */
    uint8_t adens, cdens, evolve;              /* the lead line's density, the pluck's density, Evolve */
} dk_row_t;
#define FMV(n) (12 + (n))                      /* an FM voice by its number in the pluck's list */
#define R(...) {__VA_ARGS__}

/* tempo,root,mode,chord | pad,drone,acid,pluck | kk,ks,kh,kp | bass | plvl,alvl,blvl | drive,rev,echo | adens,cdens,evolve */
static const dk_row_t DK_ROWS[DK_NGENRE][DK_NPIECE] = {
/* 0 DARKTECH: sub-heavy, 4/4, minor, little melody; kits Pound / Rumble / Crush */
{
    R(130, 4, 5, 5,   0, 8, 7, 2,            0, 0, 0, 0,   6,   26, 30, 86,   55, 28, 30,   35, 20, 45),   /* T01 Basement: E aeolian, m9 chords, riff bass */
    R(132, 6, 2, 6,   5, 9, 2, FMV(10),      1, 0, 2, 1,   6,   22, 38, 88,   60, 24, 35,   45, 25, 50),   /* T02 Hypno Pit: F# phrygian, fourths */
    R(134, 9, 1, 5,   11, 8, 14, FMV(0),     0, 2, 2, 3,   3,   30, 34, 84,   52, 34, 40,   40, 30, 55),   /* T03 Vault: A dorian, crypt bell */
    R(128, 2, 9, 6,   7, 20, 17, FMV(5),     1, 1, 0, 1,   4,   34, 42, 90,   58, 40, 30,   50, 20, 60),   /* T04 Smoke Stack: D harmonic minor, offbeat gnarl */
    R(136, 0, 5, 1,   0, 3, 0, FMV(16),      2, 2, 2, 2,   6,   20, 46, 80,   72, 22, 28,   55, 35, 65),   /* T05 Foundry: C aeolian, crush kit */
    R(133, 7, 2, 5,   5, 8, 13, 0,           0, 0, 0, 3,   2,   28, 28, 92,   50, 30, 45,   30, 15, 40),   /* T06 Tunnel: G phrygian, kick-locked bass */
    R(138, 11, 1, 6,  11, 9, 7, FMV(24),     1, 2, 0, 0,   6,   24, 40, 84,   64, 26, 38,   48, 28, 58),   /* T07 Concrete: B dorian */
    R(126, 5, 5, 5,   7, 21, 17, 4,          0, 1, 1, 1,   1,   38, 24, 94,   46, 46, 34,   25, 40, 35),   /* T08 Cold Room: F aeolian, long rumble */
    R(135, 3, 8, 4,   0, 20, 18, FMV(18),    2, 0, 2, 3,   6,   30, 36, 82,   68, 32, 26,   52, 22, 70),   /* T09 Undertow: Eb locrian, sus chords */
    R(140, 10, 9, 8,  5, 3, 21, FMV(12),     1, 2, 1, 2,   3,   18, 50, 86,   76, 20, 22,   60, 30, 75),   /* T10 Red Line: Bb harmonic minor, minmaj chords */
},
/* 1 HORROR: no steady pulse; dissonant scales and chords; bone/coffin kits; long reverbs */
{
    R(72, 6, 8, 7,    7, 23, 18, FMV(8),    11, 11, 11, 11,  0,   70, 24, 60,   20, 70, 55,   12, 14, 30),   /* H01 Attic: F# locrian, dim7 */
    R(60, 1, 11, 9,   1, 21, 16, FMV(22),    5, 11, 11, 4,   0,   78, 18, 62,   18, 78, 60,   10, 10, 25),   /* H02 Choir of Teeth: C# double harmonic, clusters */
    R(84, 9, 2, 8,    2, 8, 23, 3,           5, 5, 11, 11,   6,   62, 30, 76,   30, 60, 45,   14, 18, 45),   /* H03 Stairwell: A phrygian, minmaj */
    R(66, 4, 12, 9,   23, 23, 22, FMV(28),  11, 5, 5, 11,   0,   72, 20, 58,   10, 80, 62,   8, 16, 35),   /* H04 Séance: whole tone, clusters */
    R(96, 3, 8, 7,    3, 21, 19, FMV(25),    5, 11, 11, 4,   6,   56, 36, 80,   34, 58, 40,   24, 24, 55),   /* H05 Hallway Run: Eb locrian, pursuit riff */
    R(58, 8, 13, 9,   11, 22, 20, FMV(17),  11, 11, 5, 11,   0,   80, 16, 66,   12, 84, 66,   6, 8, 28),   /* H06 Drowned: G# blues minor, hollow flute */
    R(78, 0, 9, 7,    1, 8, 16, FMV(9),      5, 5, 11, 4,    6,   60, 28, 82,   36, 54, 38,   18, 20, 50),   /* H07 Mirror: C harmonic minor, iron */
    R(90, 11, 10, 9,  7, 3, 23, FMV(21),     4, 11, 5, 11,   2,   50, 40, 84,   40, 50, 36,   30, 22, 60),   /* H08 Pale Hands: B phrygian dominant */
    R(64, 2, 11, 8,   23, 23, 18, FMV(11),  11, 5, 11, 11,   0,   76, 22, 64,   14, 76, 58,   8, 12, 32),   /* H09 Cellar Door: D double harmonic */
    R(100, 5, 8, 7,   2, 20, 17, FMV(19),    5, 11, 5, 4,    6,   54, 42, 86,   44, 48, 34,   36, 26, 70),   /* H10 Last Light: F locrian, metal hit */
},
/* 2 DARKEDM: big offbeat bass, huge detuned leads, eight-bar builds, drops */
{
    R(128, 9, 5, 2,   6, 11, 24, FMV(15),    6, 6, 6, 6,   4,   40, 62, 82,   52, 32, 28,   55, 25, 60),   /* E01 Black Strobe: A aeolian, supersaw lead */
    R(140, 4, 9, 2,   6, 16, 27, FMV(8),     6, 6, 6, 2,   4,   36, 70, 84,   58, 28, 30,   62, 20, 70),   /* E02 Doom Drop: E harmonic minor, hyper lead */
    R(150, 6, 5, 1,   14, 16, 24, 9,         6, 7, 6, 2,   15,  34, 66, 80,   62, 26, 24,   58, 24, 72),   /* E03 Hellfire: F# aeolian, gallop bass */
    R(124, 0, 2, 5,   9, 20, 30, FMV(1),     6, 6, 0, 6,   4,   46, 56, 88,   46, 36, 34,   50, 28, 55),   /* E04 Midnight Rush: C phrygian */
    R(145, 2, 10, 2,  14, 11, 25, FMV(15),   6, 6, 6, 2,   4,   38, 64, 78,   60, 30, 28,   60, 22, 68),   /* E05 Venom Bloom: D phrygian dominant */
    R(132, 11, 5, 9,  20, 16, 27, 4,         6, 7, 6, 6,   13,  42, 60, 90,   54, 34, 30,   56, 26, 62),   /* E06 Neon Cathedral: B aeolian, cluster stabs */
    R(138, 7, 9, 2,   9, 17, 24, FMV(26),    6, 6, 7, 2,   4,   34, 72, 82,   64, 24, 26,   64, 18, 76),   /* E07 Wolf Moon: G harmonic minor */
    R(126, 10, 1, 1,  20, 11, 28, FMV(0),    6, 6, 6, 6,   15,  44, 52, 86,   48, 38, 32,   48, 30, 52),   /* E08 Smoke Signal: Bb dorian */
    R(148, 3, 5, 5,   14, 20, 27, 8,         6, 6, 6, 2,   4,   30, 68, 84,   66, 26, 24,   66, 20, 80),   /* E09 Siren Song: Eb aeolian */
    R(135, 5, 2, 8,   6, 16, 30, FMV(24),    6, 7, 6, 6,   4,   36, 60, 80,   56, 30, 30,   54, 26, 64),   /* E10 Apex Predator: F phrygian, minmaj */
},
/* 3 INDUSTR: mechanical, metal, driven, abrasive FM */
{
    R(120, 4, 5, 6,   10, 13, 19, FMV(19),   2, 3, 3, 3,   3,   22, 52, 78,   80, 20, 24,   58, 24, 40),   /* I01 Assembly Line */
    R(130, 9, 2, 5,   10, 3, 17, FMV(9),     2, 2, 2, 3,   3,   18, 56, 80,   85, 16, 20,   62, 26, 45),   /* I02 Slag */
    R(112, 6, 8, 6,   11, 19, 23, FMV(25),   3, 3, 3, 3,   4,   26, 44, 76,   76, 22, 28,   50, 30, 50),   /* I03 Press Floor */
    R(126, 2, 5, 4,   10, 20, 21, FMV(18),   2, 2, 3, 2,   3,   20, 60, 82,   88, 18, 22,   66, 22, 55),   /* I04 Rust Belt */
    R(104, 11, 9, 5,  7, 13, 18, FMV(12),    3, 3, 2, 3,   8,   30, 40, 84,   72, 28, 30,   44, 34, 35),   /* I05 Smelter: two-step bass */
    R(135, 0, 2, 6,   10, 3, 19, FMV(28),    2, 3, 2, 3,   3,   16, 62, 86,   92, 14, 18,   70, 20, 60),   /* I06 Drop Forge */
    R(116, 7, 1, 4,   13, 19, 17, 8,         3, 2, 3, 2,   4,   24, 48, 74,   78, 24, 26,   52, 28, 48),   /* I07 Conveyor */
    R(108, 3, 5, 5,   10, 13, 23, FMV(5),    2, 3, 3, 3,   3,   28, 42, 80,   82, 26, 34,   46, 32, 42),   /* I08 Rivet */
    R(124, 5, 8, 6,   11, 20, 21, FMV(10),   3, 3, 2, 3,   6,   20, 58, 82,   86, 18, 22,   64, 24, 58),   /* I09 Gearbox */
    R(100, 10, 11, 9, 10, 3, 19, FMV(21),    2, 2, 3, 2,   3,   32, 36, 88,   90, 30, 36,   40, 36, 50),   /* I10 Meltdown */
},
/* 4 DARKRAVE: hard kicks, stabs, hoover, phrygian dominant */
{
    R(150, 9, 10, 1,  14, 11, 10, 8,          7, 7, 7, 7,   4,   44, 60, 84,   70, 26, 22,   58, 20, 60),   /* R01 Warehouse 9 */
    R(155, 4, 2, 1,   14, 11, 27, FMV(15),    7, 7, 7, 2,   15,  40, 64, 80,   74, 22, 20,   62, 18, 66),   /* R02 Hardcore Dream */
    R(146, 2, 10, 2,  6, 16, 10, 4,           7, 6, 7, 7,   4,   46, 58, 86,   68, 28, 26,   54, 22, 56),   /* R03 Strobe Room */
    R(160, 7, 5, 1,   14, 11, 24, FMV(8),     7, 7, 7, 7,   15,  36, 70, 82,   78, 20, 18,   66, 16, 70),   /* R04 Laser Cage */
    R(148, 11, 9, 5,  9, 20, 10, FMV(26),     7, 6, 6, 2,   4,   42, 56, 84,   66, 30, 24,   52, 26, 54),   /* R05 Basement Mass */
    R(152, 0, 10, 1,  14, 11, 30, 8,          7, 7, 7, 7,   4,   38, 66, 80,   76, 24, 20,   60, 20, 62),   /* R06 Hoover Ritual */
    R(165, 5, 5, 1,   6, 16, 27, FMV(15),     7, 7, 7, 2,   15,  34, 72, 78,   82, 18, 16,   68, 14, 76),   /* R07 Black Light */
    R(144, 10, 10, 6, 10, 20, 10, FMV(1),     6, 6, 7, 7,   13,  48, 52, 90,   64, 32, 28,   46, 30, 50),   /* R08 Night Shift */
    R(156, 3, 2, 1,   14, 11, 24, 4,          7, 7, 7, 7,   4,   40, 62, 82,   72, 22, 22,   60, 18, 64),   /* R09 Hellraiser */
    R(150, 8, 9, 2,   9, 16, 27, FMV(28),     7, 6, 7, 2,   15,  42, 68, 84,   70, 24, 24,   64, 20, 72),   /* R10 Afterhours Fever */
},
/* 5 CINEMA: slow, huge, strings and choir, taiko, long builds */
{
    R(72, 9, 5, 9,    2, 22, 22, FMV(1),      9, 9, 10, 9,    0,  78, 14, 74,   14, 82, 50,   8, 12, 40),   /* C01 Opening Titles */
    R(60, 2, 9, 7,    1, 21, 16, FMV(17),     9, 4, 10, 9,    0,  84, 12, 70,   10, 86, 54,   6, 14, 34),   /* C02 Fall of the City */
    R(84, 0, 5, 3,    22, 8, 15, FMV(22),     9, 9, 4, 4,     6,  70, 18, 80,   24, 70, 42,   10, 18, 55),   /* C03 The Chase */
    R(66, 4, 2, 9,    7, 23, 18, FMV(13),     9, 5, 10, 9,    0,  80, 16, 68,   12, 84, 48,   6, 12, 30),   /* C04 Cold Open */
    R(92, 7, 9, 7,    2, 8, 22, FMV(3),       9, 9, 4, 4,     6,  66, 22, 82,   28, 66, 40,   14, 20, 60),   /* C05 War Drums */
    R(56, 11, 8, 8,   16, 23, 17, FMV(29),    5, 9, 10, 9,    0,  86, 10, 64,   8, 90, 56,   4, 10, 28),   /* C06 Requiem for Lights */
    R(76, 5, 5, 3,    1, 21, 23, FMV(20),     9, 9, 9, 4,     6,  74, 18, 78,   20, 76, 46,   10, 16, 46),   /* C07 Descent */
    R(88, 3, 9, 7,    2, 8, 15, FMV(27),      9, 4, 10, 9,    6,  68, 20, 84,   30, 64, 36,   16, 22, 62),   /* C08 Interrogation */
    R(64, 10, 2, 9,   22, 23, 18, FMV(4),     9, 5, 10, 9,    0,  82, 14, 72,   10, 88, 52,   6, 12, 32),   /* C09 Monolith */
    R(96, 8, 5, 3,    2, 21, 22, FMV(6),      9, 9, 4, 4,     6,  72, 24, 84,   32, 60, 34,   18, 24, 68),   /* C10 Final Hour */
},
/* 6 DARKELEC: broken 808 electro, FM bleeps, gloomy bass */
{
    R(122, 9, 5, 1,   15, 13, 23, FMV(15),    8, 8, 8, 8,    12,  28, 52, 76,   46, 22, 34,   62, 30, 50),   /* L01 Cold Wire */
    R(116, 4, 2, 6,   17, 3, 9, FMV(16),      8, 8, 8, 1,    12,  24, 56, 78,   50, 24, 30,   58, 34, 55),   /* L02 Lab Rat */
    R(128, 2, 9, 1,   15, 13, 23, FMV(29),    8, 8, 3, 8,    12,  30, 50, 74,   44, 20, 36,   66, 28, 48),   /* L03 Signal Loss */
    R(110, 0, 5, 2,   10, 19, 9, FMV(8),      8, 8, 8, 3,    13,  26, 54, 80,   52, 26, 32,   54, 36, 52),   /* L04 Black Circuit */
    R(124, 7, 1, 5,   15, 3, 23, FMV(24),     8, 1, 8, 8,    12,  32, 48, 78,   40, 28, 38,   60, 30, 46),   /* L05 Droid */
    R(108, 5, 2, 1,   17, 13, 21, FMV(23),    8, 8, 2, 8,    13,  22, 58, 82,   56, 22, 28,   68, 26, 60),   /* L06 Blackout Grid */
    R(120, 11, 5, 6,  10, 19, 9, FMV(25),     8, 8, 8, 3,    12,  34, 46, 76,   48, 30, 34,   56, 32, 50),   /* L07 Machine Tears */
    R(126, 3, 9, 1,   15, 13, 23, FMV(11),    8, 8, 8, 8,    13,  28, 56, 78,   54, 24, 30,   64, 28, 56),   /* L08 Radiation */
    R(112, 10, 5, 2,  17, 3, 9, FMV(5),       8, 8, 2, 8,    12,  26, 50, 80,   50, 26, 32,   60, 34, 48),   /* L09 Dead Channel */
    R(128, 8, 2, 8,   15, 13, 23, FMV(15),    8, 2, 8, 3,    12,  30, 60, 74,   58, 22, 28,   70, 24, 64),   /* L10 Red Phosphor */
},
/* 7 DARKTRNC: rolling bass, minor supersaws, breakdowns */
{
    R(140, 9, 5, 1,   9, 21, 8, FMV(15),      7, 7, 7, 7,    15,  48, 56, 80,   54, 36, 34,   72, 26, 55),   /* N01 Midnight Express */
    R(142, 4, 9, 1,   14, 21, 24, FMV(8),     7, 6, 7, 7,    15,  44, 60, 82,   50, 32, 30,   70, 24, 60),   /* N02 Black Sun */
    R(138, 2, 2, 2,   9, 11, 27, FMV(26),     7, 7, 7, 2,    15,  52, 52, 78,   56, 38, 36,   66, 30, 50),   /* N03 Eclipse */
    R(144, 7, 5, 1,   20, 21, 24, 8,          7, 6, 6, 7,    15,  46, 62, 84,   52, 34, 32,   74, 22, 62),   /* N04 Dark Horizon */
    R(136, 0, 1, 2,   9, 16, 8, FMV(15),      7, 7, 7, 2,    15,  50, 54, 80,   58, 40, 38,   68, 28, 52),   /* N05 Ashes */
    R(146, 11, 9, 1,  14, 21, 27, FMV(1),     7, 7, 7, 7,    15,  42, 66, 82,   60, 30, 28,   76, 20, 68),   /* N06 Void Pilgrim */
    R(138, 5, 5, 5,   9, 21, 24, 4,           7, 6, 7, 7,    15,  54, 50, 86,   48, 42, 40,   64, 32, 48),   /* N07 Nightfall */
    R(148, 10, 2, 1,  20, 11, 27, FMV(24),    7, 7, 6, 2,    15,  40, 68, 78,   62, 28, 26,   78, 18, 72),   /* N08 Cold Fusion */
    R(140, 3, 9, 2,   9, 21, 8, FMV(0),       7, 7, 7, 7,    15,  46, 58, 82,   54, 36, 34,   70, 26, 58),   /* N09 Crimson Gate */
    R(144, 6, 5, 1,   14, 16, 24, FMV(8),     7, 6, 7, 2,    15,  44, 64, 84,   58, 32, 30,   74, 22, 64),   /* N10 Last Train */
},
/* 8 EBM: sequenced sixteenths, gated snare, military */
{
    R(120, 9, 5, 1,   10, 13, 2, 2,           8, 8, 8, 3,     3,  24, 54, 84,   62, 18, 20,   60, 26, 45),   /* B01 Steel Parade */
    R(126, 4, 2, 1,   13, 3, 17, FMV(9),      8, 2, 8, 3,     3,  20, 58, 86,   68, 16, 18,   64, 22, 50),   /* B02 Iron Discipline */
    R(114, 2, 5, 6,   10, 13, 2, FMV(19),     8, 8, 2, 8,     7,  26, 50, 82,   58, 20, 24,   56, 28, 40),   /* B03 Barbed Wire */
    R(130, 7, 9, 1,   13, 19, 17, 2,          8, 8, 8, 3,     3,  18, 62, 88,   72, 14, 16,   68, 20, 56),   /* B04 Conscript */
    R(118, 0, 5, 2,   10, 13, 21, FMV(25),    2, 8, 8, 8,     3,  28, 48, 80,   56, 22, 26,   58, 30, 44),   /* B05 Gas Mask */
    R(132, 11, 2, 1,  13, 3, 2, FMV(9),       8, 8, 3, 3,     3,  16, 64, 90,   76, 12, 14,   72, 18, 62),   /* B06 Rivet Gun */
    R(112, 5, 5, 5,   10, 19, 17, 8,          8, 2, 8, 8,     7,  30, 46, 78,   54, 24, 28,   52, 32, 38),   /* B07 Night Patrol */
    R(124, 3, 9, 1,   13, 13, 2, FMV(19),     8, 8, 8, 3,     3,  22, 56, 84,   64, 18, 22,   62, 24, 52),   /* B08 Gestapo Disco */
    R(116, 10, 5, 4,  10, 3, 21, FMV(28),     2, 8, 8, 8,     3,  26, 52, 82,   60, 20, 26,   58, 28, 46),   /* B09 Cell Block */
    R(128, 8, 2, 1,   13, 19, 17, FMV(9),     8, 8, 8, 3,     3,  20, 60, 86,   70, 16, 18,   66, 22, 58),   /* B10 Firing Line */
},
/* 9 DYSTOPIA: stripped, rusted, bleak arpeggios, tape */
{
    R(128, 9, 5, 6,   11, 8, 22, FMV(10),     10, 10, 10, 10,  4,  30, 40, 78,   42, 34, 40,   46, 36, 50),   /* Y01 Ration Card */
    R(132, 4, 2, 5,   0, 21, 18, FMV(16),     10, 0, 10, 1,    3,  26, 46, 80,   46, 30, 36,   52, 32, 55),   /* Y02 Dead Mall */
    R(126, 2, 8, 6,   23, 9, 22, FMV(24),     10, 10, 4, 10,   4,  34, 36, 76,   38, 38, 44,   40, 40, 46),   /* Y03 Checkpoint */
    R(134, 7, 5, 4,   11, 8, 14, FMV(5),      0, 10, 10, 1,    1,  28, 44, 84,   50, 32, 38,   50, 34, 58),   /* Y04 Quiet Hour */
    R(124, 0, 2, 5,   0, 21, 18, FMV(18),     10, 10, 10, 10,  4,  32, 38, 82,   44, 36, 42,   44, 38, 52),   /* Y05 Grey Zone */
    R(130, 5, 9, 6,   7, 9, 22, FMV(28),      10, 0, 10, 4,    3,  24, 48, 78,   48, 28, 34,   56, 30, 60),   /* Y06 Surveillance */
    R(136, 11, 5, 5,  11, 8, 18, 4,           10, 10, 2, 10,   4,  36, 42, 86,   40, 40, 46,   42, 34, 48),   /* Y07 Burnt Sky */
    R(122, 3, 2, 6,   23, 21, 14, FMV(0),     0, 10, 10, 4,    1,  28, 34, 74,   36, 42, 48,   38, 42, 44),   /* Y08 Hollow Tower */
    R(133, 10, 5, 4,  0, 9, 22, FMV(23),      10, 10, 10, 10,  3,  26, 50, 80,   52, 26, 32,   58, 28, 62),   /* Y09 Dust Storm */
    R(129, 6, 8, 6,   11, 8, 18, FMV(10),     10, 1, 10, 4,    4,  34, 40, 82,   44, 34, 40,   46, 36, 54),   /* Y10 Last Broadcast */
},
/* 10 INDHORR: slow, mechanical, noisy dread */
{
    R(84, 6, 8, 7,    10, 3, 19, FMV(19),     2, 11, 11, 3,    3,  50, 36, 76,   74, 50, 36,   22, 20, 40),   /* Z01 Slaughterhouse */
    R(72, 1, 11, 9,   11, 22, 18, FMV(25),    5, 3, 11, 11,    0,  64, 26, 70,   60, 66, 44,   14, 16, 35),   /* Z02 Meat Hook */
    R(96, 9, 2, 8,    10, 20, 23, FMV(28),    2, 3, 3, 3,      3,  44, 44, 82,   80, 40, 32,   30, 24, 50),   /* Z03 Furnace Door */
    R(78, 4, 8, 7,    23, 3, 17, FMV(9),      5, 11, 3, 11,   14,  58, 30, 78,   66, 54, 38,   18, 20, 45),   /* Z04 Rendering Plant */
    R(104, 2, 10, 9,  10, 13, 19, FMV(12),    2, 3, 2, 3,      3,  40, 48, 84,   84, 36, 30,   36, 26, 58),   /* Z05 Bone Mill */
    R(70, 8, 13, 9,   11, 21, 20, FMV(21),    5, 11, 11, 5,    0,  68, 22, 66,   56, 70, 46,   12, 14, 30),   /* Z06 Cold Storage */
    R(92, 0, 8, 7,    10, 19, 23, FMV(5),     2, 3, 11, 3,     8,  46, 40, 80,   76, 44, 34,   26, 22, 48),   /* Z07 Scrapyard */
    R(110, 11, 9, 8,  13, 3, 17, 8,           3, 2, 3, 2,      3,  38, 54, 86,   88, 32, 28,   40, 28, 62),   /* Z08 Compactor */
    R(76, 3, 11, 9,   23, 22, 18, FMV(28),    5, 11, 11, 3,    0,  62, 28, 72,   64, 60, 42,   16, 18, 38),   /* Z09 Ward Nine */
    R(100, 5, 8, 7,   10, 13, 19, FMV(19),    2, 11, 3, 3,     3,  42, 46, 82,   82, 38, 30,   34, 24, 56),   /* Z10 Terminal Case */
},
/* 11 DARKSYNT: gated drums, minor arpeggios, andalusian */
{
    R(104, 9, 5, 2,   20, 21, 26, FMV(24),    8, 6, 1, 8,      3,  54, 52, 78,   48, 54, 36,   62, 24, 55),   /* S01 Night Drive */
    R(96, 4, 9, 2,    9, 11, 25, FMV(8),      8, 6, 0, 8,      3,  58, 48, 80,   42, 58, 40,   56, 28, 50),   /* S02 Neon Coffin */
    R(112, 2, 5, 1,   20, 21, 26, FMV(15),    8, 8, 1, 8,      3,  50, 56, 76,   52, 50, 34,   66, 22, 60),   /* S03 Vice Alley */
    R(92, 0, 2, 2,    23, 11, 25, FMV(16),    6, 6, 0, 8,      4,  62, 44, 82,   38, 62, 42,   50, 30, 48),   /* S04 Rain on Chrome */
    R(108, 7, 5, 6,   9, 21, 30, FMV(1),      8, 6, 1, 8,      3,  52, 58, 80,   50, 52, 36,   68, 24, 58),   /* S05 Laser Grid */
    R(100, 11, 9, 2,  20, 11, 26, FMV(24),    8, 8, 0, 8,      3,  56, 50, 78,   46, 56, 38,   60, 26, 52),   /* S06 Dead Mall Dreams */
    R(118, 5, 5, 1,   9, 21, 25, FMV(8),      6, 6, 1, 8,      3,  48, 60, 82,   54, 48, 32,   70, 20, 64),   /* S07 Chase Scene */
    R(94, 3, 2, 2,    23, 11, 30, FMV(0),     8, 6, 0, 8,      4,  60, 46, 80,   40, 60, 42,   52, 30, 46),   /* S08 Midnight Motel */
    R(110, 10, 5, 1,  20, 21, 26, FMV(15),    8, 8, 1, 8,      3,  52, 54, 76,   50, 50, 34,   64, 24, 56),   /* S09 Wasteland Cruise */
    R(120, 8, 9, 2,   9, 11, 25, FMV(24),     6, 6, 0, 8,      3,  50, 62, 84,   52, 46, 32,   72, 20, 66),   /* S10 Final Reel */
},
/* 12 DARKDNB: neuro reese, tight breaks */
{
    R(172, 9, 5, 1,   2, 4, 2, 4,             12, 12, 12, 12,   8,  24, 50, 86,   50, 26, 24,   54, 22, 55),   /* D01 Neuro Hunt */
    R(174, 4, 2, 6,   5, 20, 17, FMV(8),      12, 12, 12, 11,   8,  20, 54, 88,   56, 22, 20,   58, 20, 60),   /* D02 Concrete Jungle */
    R(170, 2, 9, 1,   11, 4, 2, 2,            12, 12, 12, 12,   8,  26, 46, 84,   46, 28, 26,   50, 24, 50),   /* D03 Dark Matter */
    R(176, 7, 5, 5,   2, 20, 21, FMV(25),     12, 3, 12, 12,    8,  18, 58, 90,   60, 20, 18,   62, 18, 64),   /* D04 Phantom Limb */
    R(168, 0, 2, 1,   5, 4, 18, FMV(10),      12, 12, 12, 11,   8,  22, 48, 86,   52, 24, 22,   52, 26, 52),   /* D05 Sewer Bass */
    R(174, 11, 10, 2, 11, 20, 2, FMV(15),     12, 12, 12, 12,   8,  20, 56, 88,   58, 22, 20,   60, 20, 62),   /* D06 Skinwalker */
    R(172, 5, 5, 6,   2, 4, 17, 8,            12, 12, 3, 12,    8,  26, 44, 84,   48, 30, 26,   48, 28, 48),   /* D07 Lockdown */
    R(176, 3, 9, 1,   5, 20, 2, FMV(9),       12, 12, 12, 11,   8,  18, 60, 90,   62, 18, 18,   64, 16, 68),   /* D08 Razor Wire */
    R(170, 10, 2, 5,  11, 4, 21, FMV(8),      12, 12, 12, 12,   8,  24, 50, 86,   54, 24, 22,   56, 22, 56),   /* D09 Static Cell */
    R(174, 8, 5, 1,   2, 20, 17, FMV(25),     12, 3, 12, 11,    8,  22, 54, 88,   56, 22, 20,   58, 22, 60),   /* D10 Dead Signal */
},
/* 13 DARKSTEP: half-time, sub kick, wobble bass */
{
    R(140, 4, 5, 5,   7, 16, 14, 2,           13, 13, 13, 13,   0,  34, 30, 90,   62, 34, 32,   30, 20, 50),   /* U01 Subterranean */
    R(142, 9, 2, 6,   5, 16, 0, FMV(10),      13, 13, 13, 5,    0,  30, 34, 92,   66, 30, 30,   34, 22, 55),   /* U02 Wobble Cult */
    R(138, 2, 9, 5,   11, 20, 14, FMV(25),    13, 13, 4, 13,    0,  36, 28, 88,   58, 38, 36,   28, 26, 48),   /* U03 Heavy Water */
    R(146, 6, 5, 6,   7, 16, 0, 4,            13, 13, 13, 13,   0,  28, 38, 94,   70, 28, 28,   36, 18, 60),   /* U04 Dread Engine */
    R(140, 0, 2, 5,   5, 20, 14, FMV(18),     13, 13, 13, 13,   0,  32, 32, 90,   64, 32, 32,   32, 24, 52),   /* U05 Tectonic */
    R(144, 11, 8, 6,  11, 16, 0, FMV(10),     13, 13, 13, 5,    0,  26, 40, 92,   72, 26, 26,   38, 20, 62),   /* U06 Nine Gates */
    R(138, 7, 5, 5,   7, 21, 14, 2,           13, 13, 13, 13,   0,  38, 30, 88,   60, 36, 34,   26, 28, 46),   /* U07 Hollow Earth */
    R(142, 5, 9, 6,   5, 16, 0, FMV(25),      13, 13, 4, 13,    0,  30, 36, 94,   68, 30, 30,   34, 22, 58),   /* U08 Blood Ritual */
    R(146, 10, 2, 5,  11, 20, 14, FMV(18),    13, 13, 13, 13,   0,  28, 34, 90,   70, 28, 28,   32, 22, 60),   /* U09 Gravedigger */
    R(140, 3, 5, 6,   7, 16, 0, 4,            13, 13, 13, 5,    0,  32, 40, 92,   64, 32, 30,   36, 20, 54),   /* U10 Black Tide */
},
/* 14 RITUAL: tribal toms, polymeter, drones, chants */
{
    R(78, 9, 7, 4,    1, 8, 18, FMV(20),      4, 4, 11, 4,     6,  68, 24, 76,   22, 60, 46,   16, 16, 40),   /* A01 Fire Circle */
    R(90, 4, 10, 4,   16, 22, 16, FMV(17),    4, 4, 11, 4,     2,  64, 28, 80,   26, 56, 42,   18, 18, 45),   /* A02 Bone Drum */
    R(66, 2, 2, 4,    11, 8, 22, FMV(18),     4, 5, 11, 4,     6,  72, 20, 72,   18, 66, 50,   12, 14, 35),   /* A03 Moon Offering */
    R(100, 7, 9, 7,   1, 21, 18, FMV(29),     4, 4, 11, 4,     2,  58, 30, 82,   30, 52, 38,   22, 20, 55),   /* A04 Procession */
    R(60, 11, 13, 4,  23, 23, 22, FMV(22),    5, 4, 11, 4,     0,  76, 16, 70,   14, 72, 54,   8, 12, 30),   /* A05 Bonfire Chant */
    R(84, 0, 7, 9,    16, 8, 16, FMV(17),     4, 4, 4, 4,      6,  66, 26, 78,   24, 58, 44,   16, 18, 42),   /* A06 Sacrifice */
    R(108, 5, 10, 4,  1, 21, 18, FMV(20),     4, 4, 11, 4,     2,  60, 32, 84,   34, 48, 36,   26, 22, 58),   /* A07 Dance of Masks */
    R(70, 3, 2, 7,    11, 22, 22, FMV(18),    4, 5, 11, 4,     6,  70, 22, 74,   20, 62, 48,   14, 16, 38),   /* A08 Ancestors */
    R(94, 10, 9, 4,   16, 8, 18, FMV(29),     4, 4, 11, 4,     2,  62, 28, 80,   28, 54, 40,   20, 20, 50),   /* A09 Smoke Rite */
    R(110, 8, 13, 9,  1, 21, 16, FMV(22),     4, 4, 4, 4,      6,  56, 34, 86,   36, 46, 34,   28, 24, 62),   /* A10 Last Dance */
},
/* 15 EXPERIM: odd lengths, whole-tone FM, glitch */
{
    R(110, 4, 12, 6,  12, 13, 16, FMV(28),    10, 10, 11, 10,  11,  40, 46, 70,   40, 44, 48,   44, 40, 60),   /* X01 Glass Teeth */
    R(96, 9, 13, 9,   18, 9, 23, FMV(16),     10, 11, 10, 11,  12,  36, 50, 72,   36, 48, 52,   48, 44, 66),   /* X02 Broken Clock */
    R(124, 2, 12, 4,  3, 17, 19, FMV(29),     11, 10, 10, 10,  11,  44, 42, 68,   44, 40, 44,   40, 36, 56),   /* X03 Kernel Panic */
    R(80, 6, 11, 9,   23, 6, 22, FMV(23),     10, 11, 10, 11,  0,   48, 38, 74,   30, 56, 58,   36, 40, 52),   /* X04 Ghost in the Gear */
    R(132, 0, 8, 7,   12, 13, 16, FMV(28),    10, 10, 11, 3,   11,  38, 52, 66,   50, 38, 40,   52, 38, 72),   /* X05 Bit Rot */
    R(70, 11, 12, 6,  3, 18, 20, FMV(5),      11, 11, 10, 10,  12,  50, 40, 76,   26, 60, 60,   30, 44, 48),   /* X06 Slow Collapse */
    R(118, 7, 13, 4,  18, 9, 23, FMV(25),     10, 10, 10, 11,  11,  42, 48, 70,   42, 42, 46,   46, 40, 62),   /* X07 Hex Dump */
    R(140, 5, 11, 9,  12, 17, 19, FMV(28),    10, 3, 10, 10,   12,  34, 56, 66,   52, 34, 36,   58, 36, 76),   /* X08 Seizure */
    R(88, 3, 12, 7,   23, 6, 22, FMV(18),     11, 10, 11, 10,  0,   46, 36, 72,   28, 58, 56,   34, 42, 50),   /* X09 Memory Leak */
    R(104, 10, 8, 9,  3, 13, 16, FMV(29),     10, 11, 10, 10,  11,  44, 44, 68,   46, 46, 50,   42, 38, 64),   /* X10 Last Packet */
},
};

/* ------------------------------------------------------------------------------- the genres' base lists --- */
/* Parameter, value pairs applied over the engine's defaults before the row.  LV(l): lane l's level (kick, snare, clap,
 * hat, open hat, perc).  KIT-free: the kits come from the rows. */
#define DK_B_TECH \
    P_CH_LOOP, 2, P_BL_LEN, 70, P_BL_MOVE, 12, P_BL_GLIDE, 0, P_BASSOCT, -1, P_PULSE, 80, P_SWING, 0, P_NOISE, 8, P_GHOST, 20, P_HUMAN, 8, P_BRIGHT, 14, P_DETUNE, 18, \
    P_PE_ATK, 70, P_PE_REL, 75, P_PL_RATE, 2, P_PL_DEPTH, 30, P_PL_CHO, 18, P_DR_CUT, 38, P_DR_RES, 52, P_AC_CUT, 22, P_AC_RES, 66, P_AC_ENV, 48, P_AC_DEC, 38, \
    P_AC_LEN, 16, P_AC_RANGE, 1, P_AC_MUT, 18, P_AC_OCT, -1, P_AC_RESP, 13, P_ETIME, 3, P_RV_DAMP, 62, P_RV_PRE, 60, P_MOTION, 55, P_CM_LVL, 22, P_CM_OCT, 0, P_CM_DEC, 50, \
    P_MC_RATIO, 4, P_MC_THR, 44, P_MC_MAKE, 36, P_VOICING, 1, P_PAD_COMP, 0, LV(0), 96, LV(1), 40, LV(2), 62, LV(3), 48, LV(4), 54, LV(5), 56
#define DK_B_HORROR \
    P_CH_LOOP, 0, P_BL_LEN, 95, P_BL_MOVE, 4, P_BL_GLIDE, 55, P_BASSOCT, -2, P_PULSE, 55, P_SWING, 0, P_NOISE, 22, P_NZ_COLOR, 5, P_GHOST, 0, P_HUMAN, 55, P_BRIGHT, 8, P_DETUNE, 62, \
    P_PE_ATK, 88, P_PE_HOLD, 70, P_PE_REL, 90, P_PE_REST, 35, P_PL_RATE, 1, P_PL_DEPTH, 62, P_PL_SHAPE, 4, P_PL_DEST, 3, P_PL_CHO, 40, P_DR_CUT, 25, P_DR_RES, 58, \
    P_DL_RATE, 1, P_DL_DEPTH, 70, P_DL_SHAPE, 5, P_DL_DEST, 1, P_AC_CUT, 20, P_AC_RES, 78, P_AC_ENV, 70, P_AC_DEC, 70, P_AC_LEN, 9, P_AC_RANGE, 3, P_AC_MUT, 55, P_AC_OCT, 0, P_AC_SLIDE, 60, \
    P_ECHO, 55, P_ETIME, 4, P_RV_TYPE, 1, P_RV_X, 45, P_RV_DAMP, 74, P_RV_PRE, 90, P_MOTION, 85, P_EVOLVE, 55, P_CM_LVL, 30, P_CM_OCT, 1, P_CM_DEC, 80, P_CM_VU, 35, P_CM_LOOP, 1, P_CM_RANGE, 3, \
    P_MC_RATIO, 2, P_MC_THR, 52, P_MC_MAKE, 34, P_DD_CHANGE, 0, P_PROB, 70, P_PROB1, 60, P_PROB3, 75, P_PROB5, 80, P_LLEN3, 13, P_LLEN5, 11, \
    LV(0), 80, LV(1), 38, LV(2), 30, LV(3), 30, LV(4), 38, LV(5), 70
#define DK_B_EDM \
    P_CH_LOOP, 2, P_BL_LEN, 55, P_BL_MOVE, 12, P_BL_GLIDE, 0, P_BASSOCT, -1, P_PULSE, 88, P_SWING, 0, P_NOISE, 0, P_GHOST, 10, P_HUMAN, 4, P_BRIGHT, 62, P_DETUNE, 62, \
    P_PE_ATK, 30, P_PE_REL, 50, P_PL_RATE, 3, P_PL_DEPTH, 20, P_PL_CHO, 72, P_DR_CUT, 55, P_DR_RES, 60, P_DE_TRIG, 5, P_AC_CUT, 55, P_AC_RES, 55, P_AC_ENV, 55, P_AC_DEC, 45, \
    P_AC_LEN, 16, P_AC_RANGE, 2, P_AC_MUT, 24, P_AC_OCT, 0, P_ECHO, 25, P_ETIME, 3, P_RV_DAMP, 40, P_RV_PRE, 40, P_MOTION, 50, P_EVOLVE, 60, P_CM_LVL, 28, P_CM_OCT, 1, P_CM_DEC, 35, \
    P_MC_RATIO, 5, P_MC_THR, 40, P_MC_MAKE, 42, P_VOICING, 0, P_PAD_COMP, 0, LV(0), 100, LV(1), 55, LV(2), 80, LV(3), 60, LV(4), 66, LV(5), 40
#define DK_B_INDUSTRIAL \
    P_CH_LOOP, 1, P_BL_LEN, 38, P_BL_MOVE, 6, P_BL_GLIDE, 0, P_BASSOCT, -1, P_PULSE, 92, P_SWING, 0, P_NOISE, 14, P_NZ_COLOR, 6, P_GHOST, 0, P_HUMAN, 0, P_BRIGHT, 34, P_DETUNE, 40, \
    P_PE_ATK, 20, P_PE_REL, 35, P_PL_RATE, 5, P_PL_DEPTH, 40, P_PL_SHAPE, 3, P_DR_CUT, 55, P_DR_RES, 66, P_AC_CUT, 46, P_AC_RES, 62, P_AC_ENV, 70, P_AC_DEC, 28, P_AC_LEN, 16, \
    P_AC_RANGE, 1, P_AC_MUT, 10, P_AC_OCT, 0, P_ECHO, 20, P_ETIME, 2, P_RV_DAMP, 30, P_RV_PRE, 30, P_RV_TYPE, 2, P_RV_X, 55, P_MOTION, 30, P_EVOLVE, 40, P_CM_LVL, 30, P_CM_OCT, 0, P_CM_DEC, 25, \
    P_MC_RATIO, 5, P_MC_THR, 36, P_MC_MAKE, 44, P_VOICING, 1, P_PAD_COMP, 5, LV(0), 98, LV(1), 70, LV(2), 62, LV(3), 66, LV(4), 55, LV(5), 78
#define DK_B_RAVE \
    P_CH_LOOP, 1, P_BL_LEN, 52, P_BL_MOVE, 10, P_BL_GLIDE, 0, P_BASSOCT, -1, P_PULSE, 92, P_SWING, 0, P_NOISE, 0, P_GHOST, 6, P_HUMAN, 0, P_BRIGHT, 70, P_DETUNE, 70, \
    P_PE_ATK, 12, P_PE_REL, 30, P_PL_RATE, 4, P_PL_DEPTH, 18, P_PL_CHO, 60, P_DR_CUT, 58, P_DR_RES, 62, P_AC_CUT, 62, P_AC_RES, 60, P_AC_ENV, 60, P_AC_DEC, 34, P_AC_LEN, 16, \
    P_AC_RANGE, 2, P_AC_MUT, 24, P_AC_OCT, 0, P_ECHO, 24, P_ETIME, 3, P_RV_DAMP, 38, P_RV_PRE, 30, P_MOTION, 45, P_EVOLVE, 55, P_CM_LVL, 30, P_CM_OCT, 1, P_CM_DEC, 28, \
    P_MC_RATIO, 5, P_MC_THR, 38, P_MC_MAKE, 44, P_PAD_COMP, 0, LV(0), 100, LV(1), 55, LV(2), 82, LV(3), 62, LV(4), 70, LV(5), 50
#define DK_B_CINEMA \
    P_CH_LOOP, 0, P_BL_LEN, 98, P_BL_MOVE, 4, P_BL_GLIDE, 25, P_BASSOCT, -2, P_PULSE, 62, P_SWING, 0, P_NOISE, 0, P_GHOST, 0, P_HUMAN, 30, P_BRIGHT, 28, P_DETUNE, 48, \
    P_PE_ATK, 85, P_PE_HOLD, 60, P_PE_REL, 92, P_PE_REST, 15, P_PL_RATE, 1, P_PL_DEPTH, 36, P_PL_SHAPE, 0, P_PL_DEST, 0, P_PL_CHO, 70, P_DR_CUT, 28, P_DR_RES, 40, \
    P_AC_CUT, 28, P_AC_RES, 55, P_AC_ENV, 40, P_AC_DEC, 80, P_AC_LEN, 8, P_AC_RANGE, 2, P_AC_MUT, 30, P_AC_OCT, 0, P_ECHO, 30, P_ETIME, 4, P_RV_TYPE, 1, P_RV_X, 35, P_RV_DAMP, 55, P_RV_PRE, 85, P_MOTION, 75, P_EVOLVE, 55, \
    P_CM_LVL, 30, P_CM_OCT, 0, P_CM_DEC, 80, P_CM_VU, 30, P_MC_RATIO, 2, P_MC_THR, 55, P_MC_MAKE, 30, P_PAD_COMP, 1, P_VOICING, 1, \
    LV(0), 100, LV(1), 70, LV(2), 40, LV(3), 36, LV(4), 44, LV(5), 82
#define DK_B_ELECTRO \
    P_CH_LOOP, 2, P_BL_LEN, 40, P_BL_MOVE, 14, P_BL_GLIDE, 10, P_BASSOCT, -1, P_PULSE, 84, P_SWING, 4, P_NOISE, 4, P_GHOST, 30, P_HUMAN, 6, P_BRIGHT, 40, P_DETUNE, 30, \
    P_PE_ATK, 30, P_PE_REL, 50, P_PL_RATE, 3, P_PL_DEPTH, 20, P_PL_CHO, 40, P_DR_CUT, 44, P_DR_RES, 56, P_AC_CUT, 42, P_AC_RES, 72, P_AC_ENV, 62, P_AC_DEC, 30, P_AC_LEN, 16, \
    P_AC_RANGE, 2, P_AC_MUT, 28, P_AC_OCT, 0, P_ECHO, 35, P_ETIME, 3, P_RV_TYPE, 2, P_RV_X, 35, P_RV_DAMP, 45, P_MOTION, 50, P_EVOLVE, 45, P_CM_LVL, 32, P_CM_OCT, 1, P_CM_DEC, 30, \
    P_MC_RATIO, 4, P_MC_THR, 42, P_MC_MAKE, 38, P_PAD_COMP, 0, LV(0), 92, LV(1), 64, LV(2), 60, LV(3), 56, LV(4), 50, LV(5), 62
#define DK_B_TRANCE \
    P_CH_LOOP, 2, P_BL_LEN, 42, P_BL_MOVE, 8, P_BL_GLIDE, 0, P_BASSOCT, -1, P_PULSE, 88, P_SWING, 0, P_NOISE, 0, P_GHOST, 4, P_HUMAN, 4, P_BRIGHT, 58, P_DETUNE, 68, \
    P_PE_ATK, 40, P_PE_REL, 62, P_PL_RATE, 3, P_PL_DEPTH, 24, P_PL_CHO, 78, P_DR_CUT, 50, P_DR_RES, 55, P_AC_CUT, 56, P_AC_RES, 58, P_AC_ENV, 58, P_AC_DEC, 30, P_AC_LEN, 16, \
    P_AC_RANGE, 2, P_AC_MUT, 20, P_AC_OCT, 0, P_ECHO, 32, P_ETIME, 3, P_RV_DAMP, 42, P_RV_PRE, 45, P_MOTION, 55, P_EVOLVE, 62, P_CM_LVL, 36, P_CM_OCT, 1, P_CM_DEC, 32, \
    P_MC_RATIO, 4, P_MC_THR, 42, P_MC_MAKE, 38, P_PAD_COMP, 0, LV(0), 98, LV(1), 50, LV(2), 74, LV(3), 60, LV(4), 72, LV(5), 46
#define DK_B_EBM \
    P_CH_LOOP, 2, P_BL_LEN, 30, P_BL_MOVE, 8, P_BL_GLIDE, 0, P_BASSOCT, -1, P_PULSE, 90, P_SWING, 0, P_NOISE, 4, P_GHOST, 0, P_HUMAN, 0, P_BRIGHT, 36, P_DETUNE, 28, \
    P_PE_ATK, 18, P_PE_REL, 30, P_PL_RATE, 4, P_PL_DEPTH, 20, P_DR_CUT, 52, P_DR_RES, 64, P_AC_CUT, 44, P_AC_RES, 66, P_AC_ENV, 64, P_AC_DEC, 26, P_AC_LEN, 16, P_AC_RANGE, 1, \
    P_AC_MUT, 8, P_AC_OCT, 0, P_ECHO, 14, P_ETIME, 2, P_RV_TYPE, 2, P_RV_X, 45, P_RV_DAMP, 35, P_MOTION, 30, P_EVOLVE, 38, P_CM_LVL, 26, P_CM_OCT, 0, P_CM_DEC, 22, \
    P_MC_RATIO, 5, P_MC_THR, 38, P_MC_MAKE, 42, P_PAD_COMP, 5, LV(0), 96, LV(1), 72, LV(2), 66, LV(3), 60, LV(4), 50, LV(5), 70
#define DK_B_DYSTOPIA \
    P_CH_LOOP, 2, P_BL_LEN, 60, P_BL_MOVE, 10, P_BL_GLIDE, 6, P_BASSOCT, -1, P_PULSE, 74, P_SWING, 0, P_NOISE, 26, P_NZ_COLOR, 5, P_GHOST, 14, P_HUMAN, 20, P_BRIGHT, 24, P_DETUNE, 36, \
    P_PE_ATK, 62, P_PE_REL, 70, P_PL_RATE, 2, P_PL_DEPTH, 34, P_PL_CHO, 26, P_DR_CUT, 36, P_DR_RES, 50, P_AC_CUT, 30, P_AC_RES, 60, P_AC_ENV, 50, P_AC_DEC, 36, P_AC_LEN, 12, P_AC_RANGE, 2, \
    P_AC_MUT, 22, P_AC_OCT, 0, P_ECHO, 42, P_ETIME, 3, P_RV_DAMP, 66, P_RV_PRE, 70, P_MOTION, 60, P_EVOLVE, 50, P_CM_LVL, 34, P_CM_OCT, 1, P_CM_DEC, 44, P_CM_VU, 20, \
    P_MC_RATIO, 3, P_MC_THR, 46, P_MC_MAKE, 34, P_PAD_COMP, 0, LV(0), 90, LV(1), 44, LV(2), 50, LV(3), 44, LV(4), 46, LV(5), 62
#define DK_B_INDHORR \
    P_CH_LOOP, 0, P_BL_LEN, 80, P_BL_MOVE, 6, P_BL_GLIDE, 30, P_BASSOCT, -2, P_PULSE, 80, P_SWING, 0, P_NOISE, 30, P_NZ_COLOR, 7, P_GHOST, 0, P_HUMAN, 18, P_BRIGHT, 20, P_DETUNE, 58, \
    P_PE_ATK, 75, P_PE_HOLD, 55, P_PE_REL, 84, P_PL_RATE, 2, P_PL_DEPTH, 55, P_PL_SHAPE, 4, P_PL_CHO, 34, P_DR_CUT, 32, P_DR_RES, 64, P_DL_RATE, 2, P_DL_DEPTH, 55, P_DL_SHAPE, 3, P_DL_DEST, 3, \
    P_AC_CUT, 34, P_AC_RES, 72, P_AC_ENV, 66, P_AC_DEC, 46, P_AC_LEN, 10, P_AC_RANGE, 2, P_AC_MUT, 40, P_AC_OCT, 0, P_ECHO, 38, P_ETIME, 3, P_RV_TYPE, 2, P_RV_X, 60, P_RV_DAMP, 52, P_RV_PRE, 60, \
    P_MOTION, 70, P_EVOLVE, 50, P_CM_LVL, 32, P_CM_OCT, 0, P_CM_DEC, 40, P_CM_VU, 25, P_MC_RATIO, 4, P_MC_THR, 40, P_MC_MAKE, 40, P_LLEN4, 13, P_LLEN5, 10, \
    LV(0), 96, LV(1), 60, LV(2), 44, LV(3), 48, LV(4), 54, LV(5), 80
#define DK_B_SYNTHWAVE \
    P_CH_LOOP, 2, P_BL_LEN, 44, P_BL_MOVE, 10, P_BL_GLIDE, 0, P_BASSOCT, -1, P_PULSE, 80, P_SWING, 0, P_NOISE, 2, P_GHOST, 14, P_HUMAN, 10, P_BRIGHT, 44, P_DETUNE, 56, \
    P_PE_ATK, 50, P_PE_REL, 66, P_PL_RATE, 3, P_PL_DEPTH, 30, P_PL_CHO, 85, P_DR_CUT, 46, P_DR_RES, 52, P_AC_CUT, 44, P_AC_RES, 60, P_AC_ENV, 46, P_AC_DEC, 34, P_AC_LEN, 16, P_AC_RANGE, 2, \
    P_AC_MUT, 14, P_AC_OCT, 0, P_ECHO, 40, P_ETIME, 3, P_RV_TYPE, 2, P_RV_X, 62, P_RV_DAMP, 48, P_RV_PRE, 35, P_MOTION, 50, P_EVOLVE, 42, P_CM_LVL, 34, P_CM_OCT, 1, P_CM_DEC, 36, \
    P_MC_RATIO, 3, P_MC_THR, 46, P_MC_MAKE, 32, P_PAD_COMP, 0, LV(0), 94, LV(1), 66, LV(2), 56, LV(3), 56, LV(4), 50, LV(5), 60
#define DK_B_DNB \
    P_CH_LOOP, 1, P_BL_LEN, 70, P_BL_MOVE, 10, P_BL_GLIDE, 20, P_BASSOCT, -1, P_PULSE, 84, P_SWING, 0, P_NOISE, 6, P_GHOST, 40, P_HUMAN, 8, P_BRIGHT, 28, P_DETUNE, 40, \
    P_PE_ATK, 55, P_PE_REL, 65, P_PL_RATE, 3, P_PL_DEPTH, 28, P_PL_CHO, 30, P_DR_CUT, 42, P_DR_RES, 66, P_DL_RATE, 4, P_DL_DEPTH, 40, P_DL_SHAPE, 0, P_DL_DEST, 0, P_AC_CUT, 30, P_AC_RES, 64, \
    P_AC_ENV, 52, P_AC_DEC, 32, P_AC_LEN, 12, P_AC_RANGE, 1, P_AC_MUT, 22, P_AC_OCT, 0, P_ECHO, 24, P_ETIME, 3, P_RV_DAMP, 55, P_RV_PRE, 50, P_MOTION, 50, P_EVOLVE, 50, P_CM_LVL, 24, P_CM_OCT, 1, \
    P_CM_DEC, 30, P_MC_RATIO, 4, P_MC_THR, 40, P_MC_MAKE, 40, P_DE_TRIG, 2, LV(0), 92, LV(1), 74, LV(2), 40, LV(3), 50, LV(4), 44, LV(5), 50
#define DK_B_DUBSTEP \
    P_CH_LOOP, 1, P_BL_LEN, 92, P_BL_MOVE, 6, P_BL_GLIDE, 30, P_BASSOCT, -1, P_PULSE, 84, P_SWING, 0, P_NOISE, 6, P_GHOST, 0, P_HUMAN, 4, P_BRIGHT, 20, P_DETUNE, 44, \
    P_PE_ATK, 62, P_PE_REL, 72, P_PL_RATE, 2, P_PL_DEPTH, 24, P_PL_CHO, 28, P_DR_CUT, 38, P_DR_RES, 72, P_DL_RATE, 2, P_DL_DEPTH, 90, P_DL_SHAPE, 1, P_DL_DEST, 0, P_AC_CUT, 28, P_AC_RES, 70, \
    P_AC_ENV, 58, P_AC_DEC, 42, P_AC_LEN, 8, P_AC_RANGE, 1, P_AC_MUT, 20, P_AC_OCT, -1, P_ECHO, 36, P_ETIME, 3, P_RV_DAMP, 60, P_RV_PRE, 80, P_MOTION, 45, P_EVOLVE, 50, P_CM_LVL, 18, P_CM_OCT, 0, \
    P_CM_DEC, 40, P_MC_RATIO, 5, P_MC_THR, 36, P_MC_MAKE, 44, P_DE_TRIG, 2, LV(0), 100, LV(1), 74, LV(2), 62, LV(3), 44, LV(4), 46, LV(5), 56
#define DK_B_RITUAL \
    P_CH_LOOP, 0, P_BL_LEN, 85, P_BL_MOVE, 8, P_BL_GLIDE, 15, P_BASSOCT, -2, P_PULSE, 78, P_SWING, 6, P_NOISE, 10, P_NZ_COLOR, 4, P_GHOST, 20, P_HUMAN, 40, P_BRIGHT, 16, P_DETUNE, 50, \
    P_PE_ATK, 78, P_PE_HOLD, 55, P_PE_REL, 84, P_PL_RATE, 1, P_PL_DEPTH, 40, P_PL_CHO, 40, P_DR_CUT, 30, P_DR_RES, 48, P_AC_CUT, 26, P_AC_RES, 62, P_AC_ENV, 44, P_AC_DEC, 60, P_AC_LEN, 12, \
    P_AC_RANGE, 2, P_AC_MUT, 36, P_AC_OCT, 0, P_ECHO, 48, P_ETIME, 4, P_RV_DAMP, 60, P_RV_PRE, 80, P_MOTION, 70, P_EVOLVE, 52, P_CM_LVL, 28, P_CM_OCT, 0, P_CM_DEC, 60, \
    P_MC_RATIO, 2, P_MC_THR, 50, P_MC_MAKE, 32, P_LLEN, 16, P_LLEN1, 12, P_LLEN3, 15, P_LLEN4, 9, P_LLEN5, 12, P_PAD_COMP, 1, \
    LV(0), 94, LV(1), 70, LV(2), 34, LV(3), 42, LV(4), 38, LV(5), 88
#define DK_B_EXPERIMENTAL \
    P_CH_LOOP, 0, P_BL_LEN, 50, P_BL_MOVE, 40, P_BL_GLIDE, 20, P_BASSOCT, -1, P_PULSE, 76, P_SWING, 14, P_NOISE, 6, P_GHOST, 30, P_HUMAN, 30, P_BRIGHT, 46, P_DETUNE, 50, \
    P_PE_ATK, 50, P_PE_REL, 60, P_PL_RATE, 5, P_PL_DEPTH, 55, P_PL_SHAPE, 4, P_PL_CHO, 50, P_DR_CUT, 44, P_DR_RES, 58, P_AC_CUT, 46, P_AC_RES, 64, P_AC_ENV, 60, P_AC_DEC, 34, P_AC_LEN, 11, \
    P_AC_RANGE, 3, P_AC_MUT, 55, P_AC_OCT, 0, P_ECHO, 44, P_ETIME, 4, P_RV_TYPE, 1, P_RV_X, 40, P_RV_DAMP, 44, P_MOTION, 80, P_EVOLVE, 66, P_CM_LVL, 40, P_CM_OCT, 1, P_CM_DEC, 26, P_CM_VU, 15, \
    P_MC_RATIO, 3, P_MC_THR, 48, P_MC_MAKE, 30, P_LLEN, 16, P_LLEN1, 7, P_LLEN2, 5, P_LLEN3, 11, P_LLEN4, 13, P_LLEN5, 9, \
    LV(0), 88, LV(1), 60, LV(2), 50, LV(3), 56, LV(4), 50, LV(5), 78

/* ---------------------------------------------------------------------------------- per-genre engine tables --- */
typedef struct { uint8_t from, to, w; } dk_edge_t;
typedef struct { uint8_t tonic, cad_every, cad_to, back, hr, dom; dk_edge_t e[8]; } dk_cht_t;
typedef struct { uint8_t kick, hat, build, pad, kret; } dk_sect_t;
typedef struct { uint16_t mask; int8_t iv[4]; } dk_riff_t;
typedef struct { uint16_t k[8], s[4], c[4], h[8], o[4], p[8]; } dk_pat_t;

/* chord habits on a 7-note scale: degrees 0..6 = i, ii, III, IV, V, VI, VII (see CHT in amb.c for what each field does) */
#define DK_CHT_TABLE { \
    /* 0 DARKTECH:  i, bVI, bVII, iv: slow (eight bars), home pulls strongly */ \
    {6, 8, 5, 1, 8, 0, {{0,5,3},{0,6,3},{5,0,6},{6,0,6},{0,3,2},{3,0,5},{0,0,0},{0,0,0}}}, \
    /* 1 HORROR:  i, bII (phrygian), the tritone degree, falling by half-steps; Drift's clock, nothing resolves */ \
    {1, 8, 1, 1, 0, 0, {{0,1,8},{1,0,6},{0,4,6},{4,1,6},{1,5,3},{5,4,4},{0,6,3},{6,1,4}}}, \
    /* 2 DARKEDM:  the epic i-VI-III-VII, two bars a chord */ \
    {4, 4, 6, 0, 2, 0, {{0,5,9},{5,2,9},{2,6,9},{6,0,9},{5,6,2},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 3 INDUSTR:  one chord and its flat second, eight bars */ \
    {8, 8, 1, 1, 8, 0, {{0,1,6},{1,0,6},{0,6,4},{6,0,4},{0,3,2},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 4 DARKRAVE:  i-bII-i-bVII (phrygian dominant), two bars */ \
    {5, 4, 6, 1, 2, 0, {{0,1,9},{1,0,9},{0,6,5},{6,0,6},{0,5,3},{5,6,3},{0,0,0},{0,0,0}}}, \
    /* 5 CINEMA:  i-VI-III-VII and the iv; the dominant in minor raised; four bars */ \
    {4, 4, 6, 0, 4, 1, {{0,5,6},{5,2,6},{2,6,6},{6,0,7},{0,3,5},{3,0,5},{5,4,3},{4,0,5}}}, \
    /* 6 DARKELEC:  i-iv-i-VII, two bars */ \
    {5, 4, 6, 1, 2, 0, {{0,3,6},{3,0,6},{0,6,5},{6,0,5},{0,5,2},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 7 DARKTRNC:  i-VI-III-VII, two bars (trance's loop) */ \
    {4, 4, 6, 0, 2, 0, {{0,5,9},{5,2,9},{2,6,9},{6,0,9},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 8 EBM:  i and iv, four bars; static */ \
    {6, 8, 3, 1, 4, 0, {{0,3,6},{3,0,6},{0,6,3},{6,0,3},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 9 DYSTOPIA:  i-VI-VII, bleak, four bars */ \
    {6, 4, 6, 0, 4, 0, {{0,5,8},{5,6,6},{6,0,8},{0,1,3},{5,0,4},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 10 INDHORR:  i, bII, tritone; Drift's clock */ \
    {3, 8, 1, 1, 0, 0, {{0,1,7},{1,0,6},{0,4,6},{4,1,5},{1,5,3},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 11 DARKSYNT:  the andalusian i-VII-VI-V with the raised leading tone, two bars */ \
    {3, 4, 4, 0, 2, 1, {{0,6,9},{6,5,9},{5,4,9},{4,0,9},{0,5,2},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 12 DARKDNB:  i-VI-VII-i, four bars, harmonic-minor dominant */ \
    {5, 4, 4, 0, 4, 1, {{0,5,8},{5,6,6},{6,0,8},{0,3,5},{5,4,3},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 13 DARKSTEP:  i and bVI, eight bars (sparse) */ \
    {6, 8, 5, 0, 8, 0, {{0,5,6},{5,0,8},{0,1,4},{1,0,6},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 14 RITUAL:  i and v, Drift's clock */ \
    {4, 8, 4, 1, 0, 0, {{0,4,6},{4,0,8},{0,3,3},{3,0,3},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}}, \
    /* 15 EXPERIM:  every move about equal (the walk of the original genre, but with the whole-tone and blues pools) */ \
    {1, 8, 4, 0, 0, 0, {{0,3,4},{0,5,4},{0,2,4},{0,6,4},{3,6,4},{2,5,4},{4,1,4},{6,2,4}}}, \
}
/* what a breakdown keeps (kick, hats), the build's bars, how far the pad rises (percent), whether the kick returns for the build's last bar */
#define DK_SECT_TABLE { \
    {1,0,8,60,1}, {0,0,4,125,0}, {0,0,8,90,1}, {1,1,4,40,0}, {0,0,4,70,1}, {0,0,8,130,0}, {1,1,4,70,0}, {0,0,8,100,1}, \
    {1,0,4,40,1}, {1,0,4,70,0}, {0,0,2,100,0}, {0,0,4,90,1}, {0,1,4,90,0}, {0,0,2,80,0}, {0,0,4,110,0}, {0,1,2,100,0} }
/* the bass line: the root on the genre's rhythm (mode 1), and the genre's riff (mode 6) */
#define DK_BL_ROOT_TABLE { 0x6666, 0x0001, 0x4444, 0x5555, 0x4444, 0x0001, 0x0509, 0x6666, 0x5555, 0x4401, 0x0101, 0x5555, 0x0441, 0x0401, 0x0001, 0x2105 }
#define DK_BL_RIFF_TABLE { \
    {0x6666, {0, 0, 12, 0}}, {0x0201, {0, -1, 6, 1}}, {0x4444, {0, 0, 12, 7}}, {0x5555, {0, 0, 7, 0}}, {0x4A4A, {0, 12, 0, 7}}, {0x0101, {0, 7, 5, 0}}, \
    {0x0509, {0, 0, 12, 7}}, {0x6666, {0, 0, 0, 12}}, {0x5757, {0, 0, 12, 0}}, {0x0449, {0, 7, 0, 12}}, {0x0201, {0, 1, 0, 6}}, {0x5555, {0, 0, 12, 0}}, \
    {0x0441, {0, 0, 12, 7}}, {0x0401, {0, 0, -2, 0}}, {0x0549, {0, 7, 5, 0}}, {0x4A25, {0, 6, 1, 13}} }
/* the pad's own stab rhythm when its sound stabs (Comp: Auto) */
#define DK_COMP_TABLE { 0x0404, 0x0001, 0x4444, 0x0A0A, 0x4949, 0x0001, 0x2448, 0x4444, 0x1111, 0x0401, 0x0001, 0x2222, 0x0408, 0x0401, 0x0001, 0x2105 }
/* the genre's ghost notes: snare and kick steps where a soft hit may sound that the pattern lacks */
#define DK_GHOST_SN_TABLE { 0, 0, 0, 0, 0x0200, 0, 0x8480, 0, 0, 0x0200, 0, 0, 0xC288, 0, 0x0220, 0x8A12 }
#define DK_GHOST_KK_TABLE { 0x0400, 0, 0, 0, 0, 0, 0x2200, 0, 0, 0x4400, 0, 0, 0x2200, 0, 0, 0x0810 }
/* the pluck's rate by genre: 1/4 (0), 3/16 (1), 1/8 (2), 1/16 (3) */
#define DK_PLUCK_RATE_TABLE { 3, 0, 2, 3, 3, 0, 3, 3, 3, 2, 0, 2, 2, 0, 0, 1 }

/* The genre's step tricks (en_mod, a byte a lane step: ratchet | push << 3 | level << 6).  Flags per genre: */
enum { DKM_SOFT_OFFHAT = 1, DKM_ACCENT_BACKBEAT = 2, DKM_ACCENT_ONE = 4, DKM_LATE_SNARE = 8, DKM_HAT_ROLL = 16, DKM_GHOST_HATS = 32, DKM_FLAM_PERC = 64, DKM_ACCENT_OFFHAT = 128 };
#define DK_MOD_TABLE { \
    DKM_SOFT_OFFHAT | DKM_ACCENT_ONE, DKM_FLAM_PERC | DKM_LATE_SNARE, DKM_ACCENT_ONE | DKM_ACCENT_BACKBEAT | DKM_HAT_ROLL, DKM_ACCENT_BACKBEAT | DKM_FLAM_PERC, \
    DKM_ACCENT_ONE | DKM_ACCENT_OFFHAT, DKM_ACCENT_ONE | DKM_FLAM_PERC | DKM_LATE_SNARE, DKM_ACCENT_BACKBEAT | DKM_SOFT_OFFHAT, DKM_ACCENT_OFFHAT | DKM_HAT_ROLL, \
    DKM_ACCENT_BACKBEAT | DKM_ACCENT_ONE, DKM_SOFT_OFFHAT | DKM_LATE_SNARE, DKM_FLAM_PERC | DKM_LATE_SNARE | DKM_GHOST_HATS, DKM_ACCENT_BACKBEAT | DKM_SOFT_OFFHAT, \
    DKM_ACCENT_BACKBEAT | DKM_GHOST_HATS, DKM_ACCENT_BACKBEAT | DKM_LATE_SNARE, DKM_FLAM_PERC | DKM_GHOST_HATS, DKM_GHOST_HATS | DKM_HAT_ROLL }

/* The genres' latched effects (ui.c genre_fx): tape wow, flutter, drive, dull; vinyl level, crackle, hiss, tone; pump depth, release, every, target
 * (255: the engine's default), and what is latched on (1 vinyl, 2 tape, 4 pump, 8 phaser). */
#define DK_FX_TABLE { \
    {10, 10, 45, 35, 255, 255, 255, 255, 255, 255, 255, 255}, {38, 34, 30, 60, 30, 36, 46, 30, 255, 255, 255, 255}, \
    {255, 255, 255, 255, 255, 255, 255, 255, 55, 45, 1, 0x70}, {8, 6, 85, 28, 255, 255, 255, 255, 255, 255, 255, 255}, \
    {255, 255, 255, 255, 255, 255, 255, 255, 42, 40, 1, 0x70}, {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, \
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, {255, 255, 255, 255, 255, 255, 255, 255, 38, 45, 1, 0x70}, \
    {6, 4, 65, 22, 255, 255, 255, 255, 255, 255, 255, 255}, {24, 22, 30, 50, 34, 30, 40, 34, 255, 255, 255, 255}, \
    {40, 36, 55, 55, 36, 34, 50, 28, 255, 255, 255, 255}, {255, 255, 255, 255, 255, 255, 255, 255, 30, 45, 1, 0x70}, \
    {255, 255, 255, 255, 255, 255, 255, 255, 34, 32, 1, 0xF0}, {255, 255, 255, 255, 255, 255, 255, 255, 50, 58, 0, 0x50}, \
    {28, 30, 22, 52, 22, 28, 40, 30, 255, 255, 255, 255}, {20, 18, 60, 40, 255, 255, 255, 255, 255, 255, 255, 255} }
#define DK_FX_LOCK_TABLE { 2, 3, 4, 10, 4, 0, 8, 4, 2, 3, 11, 4, 4, 4, 3, 10 }

/* ------------------------------------------------------------------------------------------ drum patterns --- */
/* Each genre owns a bank of masks per lane (bit s = a hit on sixteenth s).  A piece's six patterns take their masks from the banks by
 * (piece, pattern); the first pattern is the genre's core, the last is a fill/build variation (see dk_pats in amb.c). */
#define DK_PAT_TABLE { \
/* 0 DARKTECH: four on the floor, off-beat open hats, a rolling perc */ \
 { {0x1111,0x1111,0x9111,0x1151,0x1115,0x1111,0x5111,0x1191}, {0x0000,0x1000,0x0010,0x1010}, {0x1010,0x1000,0x1210,0x0010}, \
   {0x5555,0x7575,0xDDDD,0x5757,0xFFFF,0x4545,0x5D5D,0x7777}, {0x4444,0x4040,0x4444,0x0404}, {0x2424,0x0A0A,0x4888,0x2828,0x8421,0x1249,0x0820,0x4242} }, \
/* 1 HORROR: nearly empty: a lone kick, a bone tick here and there, irregular perc */ \
 { {0x0001,0x0081,0x0201,0x4001,0x0000,0x0801,0x0011,0x2001}, {0x0000,0x0100,0x0000,0x1100}, {0x0000,0x0100,0x0000,0x0000}, \
   {0x0000,0x1010,0x0000,0x4004,0x0200,0x0000,0x2020,0x0808}, {0x0000,0x0000,0x4000,0x0400}, {0x0420,0x2002,0x0804,0x8208,0x1804,0x0000,0x4100,0x2142} }, \
/* 2 DARKEDM: the floor kick, clap on 2 and 4, off-beat open hats; a roll for the build */ \
 { {0x1111,0x1111,0x1111,0x1119,0x1151,0x1111,0x9111,0x1115}, {0x0000,0x1010,0x0000,0x5050}, {0x1010,0x1010,0x1210,0x5010}, \
   {0x5555,0xDDDD,0xFFFF,0x5555,0x7777,0xFFFF,0xEEEE,0x5D5D}, {0x4444,0x4444,0x4040,0x4444}, {0x0000,0x2222,0x0A0A,0x8888,0x0000,0x2424,0x4242,0x0808} }, \
/* 3 INDUSTR: machine; kick on the beat plus an off-grid hit, metallic perc */ \
 { {0x1111,0x1011,0x1131,0x5111,0x1515,0x9191,0x1111,0x1311}, {0x1010,0x0808,0x1000,0x1818}, {0x0000,0x1010,0x0808,0x1010}, \
   {0xFFFF,0xEEEE,0xBBBB,0xDDDD,0x7777,0xFFFF,0xAAAA,0x5555}, {0x0000,0x0404,0x0808,0x0202}, {0x8888,0x4242,0xA5A5,0x2D2D,0x0A0A,0x9494,0x1B1B,0x6666} }, \
/* 4 DARKRAVE: hard four on the floor, clap on 2 and 4, a stab-rhythm perc */ \
 { {0x1111,0x1151,0x9111,0x1119,0x1111,0x1115,0x5111,0x1131}, {0x0000,0x1010,0x5050,0x1010}, {0x1010,0x1010,0x5050,0x1210}, \
   {0x5555,0xDDDD,0xFFFF,0x7575,0x5D5D,0xF5F5,0x5555,0xDDDD}, {0x4444,0x4444,0x4040,0x4444}, {0x4949,0x2492,0x0924,0x4A4A,0x2924,0x0000,0x8888,0x5252} }, \
/* 5 CINEMA: taiko hits on the big beats; the last two patterns are the snare roll for the build */ \
 { {0x0001,0x0101,0x0011,0x1001,0x0000,0x0141,0x0801,0x0111}, {0x0000,0x0100,0x1100,0x1111}, {0x0000,0x0100,0x0100,0x5555}, \
   {0x0000,0x1010,0x0000,0x4444,0x0000,0x0101,0x0000,0x5555}, {0x0000,0x0000,0x4000,0x0400}, {0x0001,0x8008,0x0100,0x2002,0x0000,0x1001,0x4004,0x0A0A} }, \
/* 6 DARKELEC: broken electro; syncopated kick, snare on 2 and 4, a clap ghost */ \
 { {0x0105,0x0409,0x2449,0x0501,0x0485,0x0209,0x0141,0x0805}, {0x1010,0x1810,0x1090,0x1010}, {0x0000,0x1010,0x0000,0x0210}, \
   {0x5555,0x5D5D,0xFFFF,0x7777,0x5555,0xDDDD,0x55D5,0x5D55}, {0x0000,0x4000,0x0040,0x0400}, {0x0000,0x2020,0x8484,0x4040,0x0A0A,0x8004,0x2200,0x4284} }, \
/* 7 DARKTRNC: four on the floor, claps, off-beat open hats, sixteenth hats */ \
 { {0x1111,0x1111,0x1111,0x1115,0x1111,0x9111,0x1111,0x1151}, {0x0000,0x1010,0x0000,0x1010}, {0x1010,0x1010,0x1210,0x1010}, \
   {0x5555,0x5555,0xFFFF,0x7777,0x5D5D,0x5555,0xDDDD,0x5555}, {0x4444,0x4444,0x4444,0x4040}, {0x0000,0x2222,0x0808,0x2424,0x0000,0x8888,0x0A0A,0x4242} }, \
/* 8 EBM: straight kick, gated snare, a clanking off-beat perc */ \
 { {0x1111,0x1151,0x1131,0x1113,0x1111,0x1511,0x1191,0x1115}, {0x1010,0x1010,0x1090,0x1050}, {0x1010,0x0000,0x1010,0x1210}, \
   {0x5555,0xFFFF,0x7777,0x5555,0xDDDD,0x5D5D,0xFFFF,0x5555}, {0x0000,0x4444,0x4040,0x0404}, {0x4444,0x2222,0x4A4A,0x8888,0x4242,0x2424,0x0000,0x4949} }, \
/* 9 DYSTOPIA: stripped; broken kick, sparse perc, no snare */ \
 { {0x1111,0x1101,0x1011,0x1110,0x1001,0x0111,0x1100,0x1010}, {0x0000,0x0000,0x1000,0x0010}, {0x0010,0x1000,0x0200,0x0000}, \
   {0x4444,0x2222,0x5555,0x4545,0x0000,0x6666,0x4141,0x7474}, {0x0000,0x0400,0x0040,0x0004}, {0x0A0A,0x0505,0x4210,0x2108,0x0000,0x4444,0x1248,0x0820} }, \
/* 10 INDHORR: slow, clanking, off-grid */ \
 { {0x0001,0x0101,0x0081,0x0011,0x1001,0x0201,0x0041,0x0801}, {0x0000,0x0100,0x1100,0x0110}, {0x0000,0x0100,0x0000,0x1010}, \
   {0x5555,0xAAAA,0x0000,0xA0A0,0x5050,0x3333,0x0F0F,0x6969}, {0x0000,0x0404,0x2020,0x0000}, {0x0420,0x8208,0x1804,0x2142,0x4A10,0x0A50,0x9201,0x6420} }, \
/* 11 DARKSYNT: four on the floor, gated snare, tom fills */ \
 { {0x1111,0x1151,0x1131,0x1115,0x1111,0x9111,0x1111,0x5111}, {0x1010,0x1010,0x1090,0x1010}, {0x1010,0x0000,0x1010,0x1210}, \
   {0x5555,0xFFFF,0x5D5D,0x5555,0xDDDD,0xFFFF,0x7777,0x5555}, {0x0000,0x4444,0x0000,0x4040}, {0x0000,0x0200,0x0000,0x4400,0x0000,0x2200,0x0000,0x0600} }, \
/* 12 DARKDNB: two-step; kick on the one and the and-of-three, snare on 2 and 4 */ \
 { {0x0401,0x0405,0x0481,0x0421,0x0409,0x0441,0x4401,0x0801}, {0x1010,0x1010,0x1090,0x9010}, {0x0000,0x1010,0x0000,0x0010}, \
   {0x5555,0xFFFF,0x5D5D,0x5555,0x5757,0xFFFF,0xDDDD,0x7777}, {0x0000,0x4000,0x0040,0x0400}, {0x8008,0x0220,0x4080,0x2008,0x0000,0x8808,0x1004,0x0A02} }, \
/* 13 DARKSTEP: half time; kick on one, snare on three */ \
 { {0x0001,0x0401,0x0041,0x0201,0x0801,0x0021,0x0A01,0x0081}, {0x0100,0x0100,0x4100,0x0110}, {0x0100,0x0100,0x0000,0x0100}, \
   {0x5555,0x4444,0x7777,0x5D5D,0x1515,0x5555,0xFFFF,0x4545}, {0x0000,0x4000,0x0040,0x0400}, {0x0000,0x0220,0x2000,0x0010,0x0000,0x0A0A,0x8004,0x0240} }, \
/* 14 RITUAL: low toms against hand drums; polymeter does the rest (lane lengths in the base list) */ \
 { {0x0001,0x0009,0x0041,0x0111,0x0801,0x0205,0x0049,0x1001}, {0x0004,0x0210,0x0804,0x0120}, {0x0000,0x0002,0x0000,0x0040}, \
   {0x2492,0x4925,0x1249,0x0A52,0x5555,0x2222,0x0000,0x4444}, {0x0000,0x0000,0x0200,0x2000}, {0x2492,0x4925,0x1249,0x0924,0x5249,0x2924,0x0A52,0x4A4A} }, \
/* 15 EXPERIM: off-grid kicks, snares, hats; lane lengths in the base list make them drift against each other */ \
 { {0x4801,0x0245,0x1021,0x2085,0x0803,0x4204,0x1181,0x8421}, {0x0210,0x4004,0x0A00,0x1081}, {0x0000,0x0802,0x4010,0x0000}, \
   {0xA5A5,0x6969,0xD2D2,0x3C3C,0x5A5A,0xF00F,0x0FF0,0x9696}, {0x0000,0x0404,0x2020,0x8080}, {0x3C3C,0x0F0F,0x5959,0xA00A,0x6C36,0x1B1B,0x7007,0x8E31} } }
