/* SPDX-License-Identifier: GPL-3.0-only */
/* Dump every factory recipe (genre x piece) straight from the engine's own en_piece_g / en_piece_pats_g /
 * en_piece_mods_g, as JSON lines.  Used to compare firmware trees and by the DARK-1 tests.
 *   recipe_dump > recipes.jsonl     (build: tests/dark/build_tools.sh) */
#include <stdio.h>
#include <stdint.h>
#include "amb.h"
void en_midi_out(uint32_t st, uint32_t d1, uint32_t d2) { (void)st; (void)d1; (void)d2; }
int main(void)
{
    int g, i, p, l, s;
    int16_t par[P_NPARAMS];
    uint16_t pool, pats[EN_NPAT][EN_NLANE];
    uint8_t mod[EN_NLANE][16];
    for (g = 0; g < EN_NGENRE; g++)
        for (i = 0; i < EN_NPIECES; i++) {
            en_piece_g(g, i, par, &pool);
            en_piece_pats_g(g, i, pats);
            en_piece_mods_g(g, i, mod);
            printf("{\"g\":%d,\"i\":%d,\"gname\":\"%s\",\"pname\":\"%s\",\"pool\":%u,\"par\":[", g, i, en_genre_name(g), en_piece_name_g(g, i), pool);
            for (p = 0; p < P_NPARAMS; p++)
                printf("%s%d", p ? "," : "", par[p]);
            printf("],\"pats\":[");
            for (p = 0; p < EN_NPAT; p++)
                for (l = 0; l < EN_NLANE; l++)
                    printf("%s%u", (p || l) ? "," : "", pats[p][l]);
            printf("],\"mod\":[");
            for (l = 0; l < EN_NLANE; l++)
                for (s = 0; s < 16; s++)
                    printf("%s%u", (l || s) ? "," : "", mod[l][s]);
            printf("]}\n");
        }
    return 0;
}
