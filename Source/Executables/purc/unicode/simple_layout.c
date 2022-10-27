/*
** @file unicode.c
** @author Vincent Wei
** @date 2022/10/27
** @brief Implemetation of Unicode-related interface.
**  Note that we copied most of code from GPL'd MiniGUI:
**
**      <https://github.com/VincentWei/MiniGUI/>
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <glib.h>

#include "foil.h"
#include "unicode.h"

// internal use
typedef struct my_glyph_info {
    uint32_t uc;
    uint8_t gc;
    uint8_t suppressed:1;
    uint8_t whitespace:1;
    uint8_t orientation:2;
    uint8_t hanged:2;
    uint8_t justify_word:1;
    uint8_t justify_char:1;
} my_glyph_info;

typedef struct my_glyph_args {
    const uint32_t* ucs;
    const uint16_t* bos;
    uint32_t* gvs;
    uint32_t rf;
    size_t nr_ucs;

    int lw;
    ssize_t hanged_start;
    ssize_t hanged_end;
} my_glyph_args;

typedef struct my_bbox {
    int x, y;
    int w, h;
} my_bbox;

static int
get_glyph_width(uint32_t gv)
{
    int width;

    /* TODO: handle emoji */
    if (g_unichar_iswide(gv)) {
        width = FOIL_PX_GRID_CELL_W * 2;
    }
    else {
        width = FOIL_PX_GRID_CELL_W;
    }

    return width;
}

static int
get_glyph_metrics(uint32_t gv, int* adv_x, int* adv_y, my_bbox* bbox)
{
    int tmp_x = 0;
    int tmp_y = 0;
    int bbox_w = 0, bbox_h = 0;
    int width;;

    bbox_h = FOIL_PX_GRID_CELL_H;
    bbox_w = get_glyph_width(gv);
    width = bbox_w;
    tmp_x = bbox_w;
    tmp_y = bbox_h;

    if (bbox) {
        bbox->x = 0;
        bbox->y = 0;
        bbox->w = bbox_w;
        bbox->h = bbox_h;
    }

    if (adv_x) *adv_x = tmp_x;
    if (adv_y) *adv_y = tmp_y;

    return width;
}

static void normalize_glyph_metrics(uint32_t render_flags,
        int* adv_x, int* adv_y, int* line_adv, int* line_width)
{
    switch (render_flags & FOIL_GRF_WRITING_MODE_MASK) {
    case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
    case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
        if (*adv_x > *line_width)
            *line_width = *adv_x;

        *adv_x = 0;
        *adv_y = FOIL_PX_GRID_CELL_H;
        *line_adv = FOIL_PX_GRID_CELL_H;
        break;

    case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
    default:
        *line_adv = *adv_x;
        if (FOIL_PX_GRID_CELL_H > *line_width)
            *line_width = FOIL_PX_GRID_CELL_H;
        break;
    }
}

static void set_extra_spacing(my_glyph_args* args, int extra_spacing,
        foil_glyph_extinfo* ges)
{
    switch (args->rf & FOIL_GRF_WRITING_MODE_MASK) {
    case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
    case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
        ges->extra_x = 0;
        ges->extra_y = extra_spacing;
        break;

    case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
    default:
        ges->extra_x = extra_spacing;
        ges->extra_y = 0;
        break;
    }
}

static void increase_extra_spacing(my_glyph_args* args, int extra_spacing,
        foil_glyph_extinfo* ges)
{
    switch (args->rf & FOIL_GRF_WRITING_MODE_MASK) {
    case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
    case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
        ges->extra_y += extra_spacing;
        break;

    case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
    default:
        ges->extra_x += extra_spacing;
        break;
    }
}

static ssize_t find_breaking_pos_normal(my_glyph_args* args, size_t n)
{
    ssize_t i;

    for (i = n - 1; i >= 0; i--) {
        if ((args->bos[i] & FOIL_BOV_LB_MASK) == FOIL_BOV_LB_ALLOWED)
            return i;
    }

    return -1;
}

static ssize_t find_breaking_pos_any(my_glyph_args* args, size_t n)
{
    int i;

    for (i = n - 1; i >= 0; i--) {
        if (args->bos[i] & FOIL_BOV_GB_CHAR_BREAK)
            return i;
    }

    return -1;
}

static ssize_t find_breaking_pos_word(my_glyph_args* args, size_t n)
{
    int i;

    for (i = n - 1; i >= 0; i--) {
        if (args->bos[i] & FOIL_BOV_WB_WORD_BOUNDARY)
            return i;
    }

    return -1;
}

static inline bool is_whitespace_glyph(const my_glyph_args* args,
        const my_glyph_info* gis, int i)
{
    (void)gis;
    return (args->bos[i] & FOIL_BOV_WHITESPACE);
}

static inline bool is_zero_width_glyph(const my_glyph_args* args,
        const my_glyph_info* gis, int i)
{
    (void)gis;
    return (args->bos[i] & FOIL_BOV_ZERO_WIDTH);
}

static inline bool is_word_separator(const my_glyph_args* args,
        const my_glyph_info* gis, int i)
{
    (void)args;
#if 0
    return (args->bos[i] & FOIL_BOV_WB_WORD_BOUNDARY);
#else
    return (
        gis[i].uc == 0x0020 || gis[i].uc == 0x00A0 ||
        gis[i].uc == 0x1361 ||
        gis[i].uc == 0x10100 || gis[i].uc == 0x10101 ||
        gis[i].uc == 0x1039F || gis[i].uc == 0x1091F
    );
#endif
}

/*
 * TODO: scripts and spacing:
 * https://www.w3.org/TR/css-text-3/#script-groups
 */
static inline bool is_typographic_char(const my_glyph_args* args,
        const my_glyph_info* gis, int i)
{
    (void)gis;
    return (args->bos[i - 1] & FOIL_BOV_GB_CHAR_BREAK);
}

static void justify_glyphs_inter_word(my_glyph_args* args,
        my_glyph_info* gis, foil_glyph_extinfo* ges, int n, int error)
{
    int i;
    int nr_words = 0;
    int err_per_unit;
    int left;

    for (i = 0; i < n; i++) {
        if (gis[i].suppressed == 0 && gis[i].hanged == 0 &&
                is_word_separator(args, gis, i)) {
            nr_words++;
            gis[i].justify_word = 1;
        }
        else {
            gis[i].justify_word = 0;
        }
    }

    if (nr_words <= 0)
        return;

    err_per_unit = error / nr_words;
    left = error % nr_words;
    if (err_per_unit > 0) {

        i = 0;
        do {
            if (gis[i].justify_word) {
                increase_extra_spacing(args, err_per_unit, ges + i);
                nr_words--;
            }

            i++;
        } while (nr_words > 0);
    }

    if (left > 0) {
        i = 0;
        do {
            if (gis[i].justify_word) {
                increase_extra_spacing(args, 1, ges + i);
                if (--left == 0)
                    break;
            }

            i++;
        } while (1);
    }
}

static void justify_glyphs_inter_char(my_glyph_args* args,
        my_glyph_info* gis, foil_glyph_extinfo* ges, int n, int error)
{
    int i;
    int nr_chars = 0;
    int err_per_unit;
    int left;

    for (i = 0; i < n; i++) {
        if (gis[i].suppressed == 0 && gis[i].hanged == 0 &&
                !is_word_separator(args, gis, i) &&
                is_typographic_char(args, gis, i)) {
            nr_chars++;
            gis[i].justify_char = 1;
        }
        else {
            gis[i].justify_char = 0;
        }
    }

    nr_chars--;
    if (nr_chars <= 0)
        return;

    err_per_unit = error / nr_chars;
    left = error % nr_chars;
    if (err_per_unit > 0) {
        i = 0;
        do {
            if (gis[i].justify_char) {
                increase_extra_spacing(args, err_per_unit, ges + i);
                nr_chars--;
            }

            i++;
        } while (nr_chars > 0);
    }

    if (left > 0) {
        for (i = 0; i < n; i++) {
            if (gis[i].justify_char) {
                increase_extra_spacing(args, 1, ges + i);
                if (--left == 0)
                    break;
            }
        }
    }
}

/*
 * For auto justification, we use the following policy:
 * Primarily expanding word separators and between CJK typographic
 * letter units along with secondarily expanding between other
 * typographic character units.
 */
static void justify_glyphs_auto(my_glyph_args* args,
        my_glyph_info* gis, foil_glyph_extinfo* ges, int n, int error)
{
    int i;
    int total_error = error;
    int nr_words = 0;
    int nr_chars = 0;
    int err_per_unit;
    int compensated = 0;
    int left;

    for (i = 0; i < n; i++) {
        gis[i].justify_word = 0;
        gis[i].justify_char = 0;
        if (gis[i].suppressed == 0 && gis[i].hanged == 0) {
            if ((is_word_separator(args, gis, i) && i != 0) ||
                    g_unichar_iswide_cjk(gis[i].uc)) {
                nr_words++;
                gis[i].justify_word = 1;
            }
            else if (is_typographic_char(args, gis, i)) {
                nr_chars++;
                gis[i].justify_char = 1;
            }
        }
    }

    LOG_DEBUG("nr_words(%d), nr_chars(%d)\n", nr_words, nr_chars);

    nr_chars--;

    /* most error for words and CJK letters */
    if (nr_chars > 0)
        error = error * 2 / 3;

    if (nr_words > 0) {
        err_per_unit = error / nr_words;
        left = error % nr_words;
        if (err_per_unit > 0) {

            i = 0;
            do {
                if (gis[i].justify_word) {
                    increase_extra_spacing(args, err_per_unit, ges + i);
                    compensated += err_per_unit;
                    nr_words--;
                }

                i++;
            } while (nr_words > 0);
        }

        if (nr_chars <= 0 && left > 0) {
            for (i = 0; i < n; i++) {
                if (gis[i].justify_word) {
                    increase_extra_spacing(args, 1, ges + i);
                    if (--left == 0)
                        break;
                }
            }

            return;
        }
    }

    if (nr_chars > 0) {
        /* left error for other chars */
        error = total_error - compensated;
        err_per_unit = error / nr_chars;
        left = error % nr_chars;
        if (err_per_unit > 0) {
            i = 0;
            do {
                if (gis[i].justify_char) {
                    increase_extra_spacing(args, err_per_unit, ges + i);
                    nr_chars--;
                }

                i++;
            } while (nr_chars > 0);
        }

        if (left > 0) {
            for (i = 0; i < n; i++) {
                if (gis[i].justify_char) {
                    increase_extra_spacing(args, 1, ges + i);
                    if (--left == 0)
                        break;
                }
            }
        }
    }
}

static void adjust_glyph_position(my_glyph_args* args,
        int x, int y, const my_glyph_info* gi, const foil_glyph_extinfo* ge,
        foil_glyph_pos* pos)
{
    switch (args->rf & FOIL_GRF_WRITING_MODE_MASK) {
    case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
        if (gi->orientation == FOIL_GLYPH_ORIENT_UPRIGHT) {
            x -= (args->lw + ge->bbox_w) / 2;
            x -= ge->bbox_x;
        }
        break;

    case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
        x += (args->lw - ge->bbox_w) / 2;
        x -= ge->bbox_x;
        break;

    case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
    default:
        break;
    }

    pos->x += x;
    pos->y += y;
}

static void calc_unhanged_glyph_positions(my_glyph_args* args,
        const my_glyph_info* gis, foil_glyph_extinfo* ges, int n,
        int x, int y, foil_glyph_pos* pos)
{
    int i;
    int first = 0, stop = n;

    if (args->hanged_start >= 0)
        first = args->hanged_start + 1;
    if (args->hanged_end < n)
        stop = args->hanged_end;

    for (i = first; i < stop; i++) {
        if (i == first) {
            pos[i].x = 0;
            pos[i].y = 0;
        }
        else {
            pos[i].x = pos[i - 1].x + ges[i - 1].adv_x;
            pos[i].y = pos[i - 1].y + ges[i - 1].adv_y;
            pos[i].x += ges[i - 1].extra_x;
            pos[i].y += ges[i - 1].extra_y;
        }
    }

    for (i = first; i < stop; i++) {
        adjust_glyph_position(args, x, y, gis + i, ges + i, pos + i);

        ges[i].suppressed = gis[i].suppressed;
        ges[i].whitespace = gis[i].whitespace;
        ges[i].orientation = gis[i].orientation;

        pos[i].suppressed = gis[i].suppressed;
        pos[i].whitespace = gis[i].whitespace;
        pos[i].orientation = gis[i].orientation;
        pos[i].hanged = gis[i].hanged;
    }
}

static int calc_hanged_glyphs_extent(my_glyph_args* args,
        const foil_glyph_extinfo* ges, int n)
{
    int i;
    int hanged_extent = 0;

    if (args->hanged_start >= 0) {
        for (i = 0; i <= args->hanged_start; i++) {
            hanged_extent += ges[i].line_adv;
        }
    }

    if (args->hanged_end < n) {
        for (i = args->hanged_end; i < n; i++) {
            hanged_extent += ges[i].line_adv;
        }
    }

    LOG_DEBUG("hanged_start(%d) hanged_end(%d) n(%d) hanged_extent(%d)\n",
        args->hanged_start, args->hanged_end, n, hanged_extent);

    return hanged_extent;
}

static int calc_hanged_glyphs_start(my_glyph_args* args,
        const my_glyph_info* gis, foil_glyph_extinfo* ges,
        foil_glyph_pos* pos, int n, int x, int y)
{
    (void)n;
    int i;
    int hanged_extent = 0;

    switch (args->rf & FOIL_GRF_WRITING_MODE_MASK) {
    case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
    case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
        for (i = 0; i <= args->hanged_start; i++) {
            hanged_extent += ges[i].line_adv;
        }

        for (i = 0; i <= args->hanged_start; i++) {
            if (i == 0) {
                pos[i].x = 0;
                pos[i].y = -hanged_extent;
            }
            else {
                pos[i].x = pos[i - 1].x + ges[i - 1].adv_x;
                pos[i].y = pos[i - 1].y + ges[i - 1].adv_y;
                pos[i].x += ges[i - 1].extra_x;
                pos[i].y += ges[i - 1].extra_y;
            }
        }
        break;

    case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
    default:
        for (i = 0; i <= args->hanged_start; i++) {
            hanged_extent += ges[i].line_adv;
        }

        for (i = 0; i <= args->hanged_start; i++) {
            if (i == 0) {
                pos[i].x = -hanged_extent;
                pos[i].y = 0;
            }
            else {
                pos[i].x = pos[i - 1].x + ges[i - 1].adv_x;
                pos[i].y = pos[i - 1].y + ges[i - 1].adv_y;
                pos[i].x += ges[i - 1].extra_x;
                pos[i].y += ges[i - 1].extra_y;
            }
        }
        break;
    }

    for (i = 0; i <= args->hanged_start; i++) {
        adjust_glyph_position(args, x, y, gis + i, ges + i, pos + i);

        ges[i].suppressed = gis[i].suppressed;
        ges[i].whitespace = gis[i].whitespace;
        ges[i].orientation = gis[i].orientation;

        pos[i].suppressed = gis[i].suppressed;
        pos[i].whitespace = gis[i].whitespace;
        pos[i].orientation = gis[i].orientation;
        pos[i].hanged = gis[i].hanged;
    }

    return hanged_extent;
}

static int calc_hanged_glyphs_end(my_glyph_args* args,
        const my_glyph_info* gis, foil_glyph_extinfo* ges,
        foil_glyph_pos* pos, int n, int x, int y, int extent)
{
    int i;
    int hanged_extent = 0;

    switch (args->rf & FOIL_GRF_WRITING_MODE_MASK) {
    case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
    case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
        for (i = args->hanged_end; i < n; i++) {
            if (i == args->hanged_end) {
                pos[i].x = 0;
                pos[i].y = extent;
            }
            else {
                pos[i].x = pos[i - 1].x + ges[i - 1].adv_x;
                pos[i].y = pos[i - 1].y + ges[i - 1].adv_y;
                pos[i].x += ges[i - 1].extra_x;
                pos[i].y += ges[i - 1].extra_y;
            }

            hanged_extent += ges[i].line_adv;
        }
        break;

    case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
    default:
        for (i = args->hanged_end; i < n; i++) {
            if (i == args->hanged_end) {
                pos[i].x = extent;
                pos[i].y = 0;
            }
            else {
                pos[i].x = pos[i - 1].x + ges[i - 1].adv_x;
                pos[i].y = pos[i - 1].y + ges[i - 1].adv_y;
                pos[i].x += ges[i - 1].extra_x;
                pos[i].y += ges[i - 1].extra_y;
            }

            hanged_extent += ges[i].line_adv;
        }
        break;
    }

    for (i = args->hanged_end; i < n; i++) {
        adjust_glyph_position(args, x, y, gis + i, ges + i, pos + i);

        ges[i].suppressed = gis[i].suppressed;
        ges[i].whitespace = gis[i].whitespace;
        ges[i].orientation = gis[i].orientation;

        pos[i].suppressed = gis[i].suppressed;
        pos[i].whitespace = gis[i].whitespace;
        pos[i].orientation = gis[i].orientation;
        pos[i].hanged = gis[i].hanged;
    }

    LOG_DEBUG("hanged_start(%d) hanged_end(%d) n(%d) hanged_extent(%d)\n",
        args->hanged_start, args->hanged_end, n, hanged_extent);

    return hanged_extent;
}

static void offset_unhanged_glyph_positions(my_glyph_args* args,
        foil_glyph_pos* pos, int n, int offset)
{
    int i;
    int first = 0, stop = n;

    if (args->hanged_start >= 0)
        first = args->hanged_start + 1;
    if (args->hanged_end < n)
        stop = args->hanged_end;

    LOG_DEBUG("offset(%d), first(%d), stop(%d)\n",
            offset, first, stop);

    switch (args->rf & FOIL_GRF_WRITING_MODE_MASK) {
    case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
    case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
        for (i = first; i < stop; i++) {
            pos[i].y += offset;
        }
        break;

    case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
    default:
        for (i = first; i < stop; i++) {
            pos[i].x += offset;
        }
        break;
    }
}

static void align_unhanged_glyphs(my_glyph_args* args,
        foil_glyph_pos* pos, int n, int gap)
{
    LOG_DEBUG("args->rf(0x%08X), gap(%d)\n", args->rf, gap);
    switch (args->rf & FOIL_GRF_ALIGN_MASK) {
    case FOIL_GRF_ALIGN_RIGHT:
    case FOIL_GRF_ALIGN_END:
        offset_unhanged_glyph_positions(args, pos, n, gap);
        break;

    case FOIL_GRF_ALIGN_CENTER:
        offset_unhanged_glyph_positions(args, pos, n, gap/2);
        break;

    case FOIL_GRF_ALIGN_LEFT:
    case FOIL_GRF_ALIGN_START:
    case FOIL_GRF_ALIGN_JUSTIFY:
    default:
        break;
    }
}

static inline bool is_opening_punctation(const my_glyph_info* gi)
{
    return (
        gi->gc == G_UNICODE_OPEN_PUNCTUATION ||
        gi->gc == G_UNICODE_FINAL_PUNCTUATION ||
        gi->gc == G_UNICODE_INITIAL_PUNCTUATION ||
        gi->uc == 0x0027 ||
        gi->uc == 0x0022
    );
}

static inline bool is_closing_punctation(const my_glyph_info* gi)
{
    return (
        gi->gc == G_UNICODE_CLOSE_PUNCTUATION ||
        gi->gc == G_UNICODE_FINAL_PUNCTUATION ||
        gi->gc == G_UNICODE_INITIAL_PUNCTUATION ||
        gi->uc == 0x0027 ||
        gi->uc == 0x0022
    );
}

static inline bool is_stop_or_common(const my_glyph_info* gi)
{
    return (
        gi->uc == 0x002C || //  ,   COMMA
        gi->uc == 0x002E || //  .   FULL STOP
        gi->uc == 0x060C || //  ،   ARABIC COMMA
        gi->uc == 0x06D4 || //  ۔   ARABIC FULL STOP
        gi->uc == 0x3001 || //  、  IDEOGRAPHIC COMMA
        gi->uc == 0x3002 || //  。  IDEOGRAPHIC FULL STOP
        gi->uc == 0xFF0C || //  ，  FULLWIDTH COMMA
        gi->uc == 0xFF0E || //  ．  FULLWIDTH FULL STOP
        gi->uc == 0xFE50 || //  ﹐  SMALL COMMA
        gi->uc == 0xFE51 || //  ﹑  SMALL IDEOGRAPHIC COMMA
        gi->uc == 0xFE52 || //  ﹒  SMALL FULL STOP
        gi->uc == 0xFF61 || //  ｡   HALFWIDTH IDEOGRAPHIC FULL STOP
        gi->uc == 0xFF64    //  ､   HALFWIDTH IDEOGRAPHIC COMMA
    );
}

static void init_glyph_info(my_glyph_args* args, int i,
        my_glyph_info* gi)
{
    args->gvs[i] = args->ucs[i];

    gi->uc = args->ucs[i];
    gi->gc = g_unichar_type(gi->uc);
    gi->whitespace = 0;
    gi->suppressed = 0;
    gi->hanged = FOIL_GLYPH_HANGED_NONE;
    gi->orientation = FOIL_GLYPH_ORIENT_UPRIGHT;
}

static inline int shrink_total_extent(my_glyph_args* args, int total_extent,
        const foil_glyph_extinfo* ges)
{
    (void)args;
    return total_extent - ges->line_adv;

}

static int get_glyph_extent_info(my_glyph_args* args, uint32_t gv,
        my_glyph_info* gi, foil_glyph_extinfo* ges)
{
    (void)gi;

    my_bbox bbox;
    int adv_x = 0, adv_y = 0;
    int line_adv;

    get_glyph_metrics(gv, &adv_x, &adv_y, &bbox);
    normalize_glyph_metrics(args->rf,
            &adv_x, &adv_y, &line_adv, &args->lw);

    ges->bbox_x = bbox.x;
    ges->bbox_y = bbox.y;
    ges->bbox_w = bbox.w;
    ges->bbox_h = bbox.h;
    ges->adv_x = adv_x;
    ges->adv_y = adv_y;

    return line_adv;
}

static int get_first_normal_glyph(const my_glyph_args* args,
        const my_glyph_info* gis, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (gis[i].suppressed == 0 && gis[i].hanged == 0
                && (args->bos[i - 1] & FOIL_BOV_GB_CHAR_BREAK))
            return i;
    }

    return -1;
}

static int get_last_normal_glyph(const my_glyph_args* args,
        const my_glyph_info* gis, int n)
{
    int i;
    for (i = n - 1; i >= 0; i--) {
        if (gis[i].suppressed == 0 && gis[i].hanged == 0
                && (args->bos[i - 1] & FOIL_BOV_GB_CHAR_BREAK))
            return i;
    }

    return -1;
}

size_t foil_ustr_get_glyphs_extent_simple(const uint32_t* ucs, size_t nr_ucs,
        const foil_break_oppo_t* break_oppos, uint32_t render_flags,
        int x, int y, int letter_spacing, int word_spacing, int tab_size,
        int max_extent, foil_size* line_size, uint32_t* glyphs,
        foil_glyph_extinfo* glyph_ext_info,
        foil_glyph_pos* glyph_pos)
{
    size_t n = 0;
    int total_extent = 0;
    ssize_t breaking_pos = -1;
    int gap;

    my_glyph_args  args;
    my_glyph_info* gis = NULL;
    foil_glyph_extinfo* ges = NULL;
    bool test_overflow = TRUE;

    if (glyph_ext_info == NULL) {
        ges = (foil_glyph_extinfo*)calloc(sizeof(foil_glyph_extinfo), nr_ucs);
        if (ges == NULL)
            goto error;
    }
    else {
        ges = glyph_ext_info;
        memset(ges, 0, sizeof(foil_glyph_extinfo) * nr_ucs);
    }

    gis = (my_glyph_info*)calloc(sizeof(my_glyph_info), nr_ucs);
    if (gis == NULL)
        goto error;

    args.ucs = ucs;
    args.gvs = glyphs;
    args.bos = break_oppos;
    args.nr_ucs = nr_ucs;
    args.rf = render_flags;
    args.lw = FOIL_PX_GRID_CELL_H;

    while (n < nr_ucs) {
        int extra_spacing;

        init_glyph_info(&args, n, gis + n);

        /*
         * NOTE: The collapsible spaces should be handled in GetGlyphsByRules.
         */
        if (gis[n].uc == FOIL_UCHAR_TAB) {
            if (tab_size > 0) {
                int tabstops = total_extent / tab_size + 1;
                ges[n].line_adv = tabstops * tab_size- total_extent;

                // If this distance is less than 0.5ch, then the
                // subsequent tab stop is used instead.
                if (ges[n].line_adv < FOIL_PX_GRID_CELL_W) {
                    tabstops++;
                    ges[n].line_adv = tabstops * tab_size- total_extent;
                }

                switch (render_flags & FOIL_GRF_WRITING_MODE_MASK) {
                case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
                case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
                    ges[n].adv_y = ges[n].line_adv;
                    break;
                case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
                default:
                    ges[n].adv_x = ges[n].line_adv;
                    break;
                }

                gis[n].whitespace = 1;
            }
            else {
                gis[n].suppressed = 1;
                ges[n].line_adv = 0;
            }
        }
        else if (is_whitespace_glyph(&args, gis, n)) {
            uint32_t space_gv = FOIL_UCHAR_SPACE;

            gis[n].whitespace = 1;
            ges[n].line_adv = get_glyph_width(space_gv);

            switch (render_flags & FOIL_GRF_WRITING_MODE_MASK) {
            case FOIL_GRF_WRITING_MODE_VERTICAL_RL:
            case FOIL_GRF_WRITING_MODE_VERTICAL_LR:
                ges[n].adv_y = ges[n].line_adv;
                break;
            case FOIL_GRF_WRITING_MODE_HORIZONTAL_TB:
            default:
                ges[n].adv_x = ges[n].line_adv;
                break;
            }
        }
        else if (is_zero_width_glyph(&args, gis, n)) {
            gis[n].suppressed = 1;
            ges[n].line_adv = 0;
        }
        else {
            ges[n].line_adv = get_glyph_extent_info(&args, glyphs[n], gis + n,
                    ges + n);
        }

        // extra space for word and letter
        extra_spacing = 0;
        if (gis[n].suppressed == 0 && is_word_separator(&args, gis, n)) {
            extra_spacing = word_spacing;
        }
        else if (gis[n].suppressed == 0 && is_typographic_char(&args, gis, n)) {
            extra_spacing = letter_spacing;
        }

        if (extra_spacing > 0) {
            ges[n].line_adv += extra_spacing;
            set_extra_spacing(&args, extra_spacing, ges + n);
        }

        if (test_overflow && max_extent > 0
                && (total_extent + ges[n].line_adv) > max_extent) {
            // overflow
            switch (render_flags & FOIL_GRF_OVERFLOW_WRAP_MASK) {
            case FOIL_GRF_OVERFLOW_WRAP_BREAK_WORD:
                breaking_pos = find_breaking_pos_word(&args, n);
                break;
            case FOIL_GRF_OVERFLOW_WRAP_ANYWHERE:
                breaking_pos = find_breaking_pos_any(&args, n);
                break;
            case FOIL_GRF_OVERFLOW_WRAP_NORMAL:
            default:
                breaking_pos = find_breaking_pos_normal(&args, n);
                break;
            }

            if (breaking_pos >= 0) {
                // a valid breaking position found before current glyph
                break;
            }

            breaking_pos = -1;
            test_overflow = FALSE;
        }

        total_extent += ges[n].line_adv;
        if ((break_oppos[n] & FOIL_BOV_LB_MASK) == FOIL_BOV_LB_MANDATORY) {
            // hard line breaking
            n++;
            break;
        }

        if (!test_overflow && max_extent > 0
                && ((break_oppos[n] & FOIL_BOV_LB_MASK) == FOIL_BOV_LB_ALLOWED)) {
            n++;
            break;
        }

        n++;
    }

    if (breaking_pos >= 0 && (size_t)breaking_pos != n) {
        // wrapped due to overflow
        size_t i;

        n = breaking_pos + 1;

        total_extent = 0;
        for (i = 0; i < n; i++) {
            total_extent += ges[i].line_adv;
        }
    }

    args.hanged_start = -1;
    args.hanged_end = n + 1;

    // Trimming spaces at the start of the line
    if (render_flags & FOIL_GRF_SPACES_REMOVE_START) {
        size_t i = 0;
        while (i < n && gis[i].uc == FOIL_UCHAR_SPACE) {
            total_extent -= ges[i].line_adv;
            memset(ges + i, 0, sizeof(foil_glyph_extinfo));
            gis[i].suppressed = 1;
            i++;
        }
    }

    // Trimming or hanging spaces at the end of the line
    if (render_flags & FOIL_GRF_SPACES_REMOVE_END) {
        ssize_t i = n - 1;
        while (i > 0 &&
                (gis[i].uc == FOIL_UCHAR_SPACE || gis[i].uc == FOIL_UCHAR_IDSPACE)) {
            total_extent -= ges[i].line_adv;
            memset(ges + i, 0, sizeof(foil_glyph_extinfo));
            gis[i].suppressed = 1;
            i--;
        }
    }
    else if (render_flags & FOIL_GRF_SPACES_HANGE_END) {
        ssize_t i = n - 1;
        while (i > 0 &&
                (gis[i].uc == FOIL_UCHAR_SPACE || gis[i].uc == FOIL_UCHAR_IDSPACE)) {

            gis[i].hanged = FOIL_GLYPH_HANGED_END;
            if (i < args.hanged_end) args.hanged_end = i;
            i--;
        }
    }

    if (n < nr_ucs) {
        init_glyph_info(&args, n, gis + n);
    }

    if (render_flags & FOIL_GRF_HANGING_PUNC_OPEN) {
        int first = get_first_normal_glyph(&args, gis, n);
        if (first >= 0 && is_opening_punctation(gis + first)) {
            gis[first].hanged = FOIL_GLYPH_HANGED_START;
            if (first > args.hanged_start) args.hanged_start = first;
        }
    }

    if (n > 1 && render_flags & FOIL_GRF_HANGING_PUNC_CLOSE) {
        int last = get_last_normal_glyph(&args, gis, n);
        if (last > 0 && is_closing_punctation(gis + last)) {
            gis[last].hanged = FOIL_GLYPH_HANGED_END;
            if (last < args.hanged_end) args.hanged_end = last;
        }
#if 0
        else if (n < nr_ucs && is_closing_punctation(gis + n)) {
            gis[n].hanged = FOIL_GLYPH_HANGED_END;
            if (n < args.hanged_end) args.hanged_end = n;
            ges[n].line_adv = get_glyph_extent_info(&args, glyphs[n], gis + n,
                    ges + n);
            total_extent += ges[n].line_adv;
            n++;
        }
#endif
    }

    if (render_flags & FOIL_GRF_HANGING_PUNC_FORCE_END) {
        // A stop or comma at the end of a line hangs.
        int last = get_last_normal_glyph(&args, gis, n);
        if (last > 0 && is_stop_or_common(gis + last)) {
            gis[last].hanged = FOIL_GLYPH_HANGED_END;
            if (last < args.hanged_end) args.hanged_end = last;
        }
#if 0
        else if (n < nr_ucs && is_stop_or_common(gis + n)) {
            gis[n].hanged = FOIL_GLYPH_HANGED_END;
            if (n < args.hanged_end) args.hanged_end = n;
            ges[n].line_adv = get_glyph_extent_info(&args, glyphs[n], gis + n,
                    ges + n);
            total_extent += ges[n].line_adv;
            n++;
        }
#endif
    }
    else if (render_flags & FOIL_GRF_HANGING_PUNC_ALLOW_END) {
        // A stop or comma at the end of a line hangs
        // if it does not otherwise fit prior to justification.
        if (n < nr_ucs && is_stop_or_common(gis + n)) {
            gis[n].hanged = FOIL_GLYPH_HANGED_END;
            if (n < (size_t)args.hanged_end) args.hanged_end = n;
            ges[n].line_adv = get_glyph_extent_info(&args, glyphs[n], gis + n,
                    ges + n);
            total_extent += ges[n].line_adv;
            n++;
        }
    }

    total_extent -= calc_hanged_glyphs_extent(&args, ges, n);

    // calc positions of hanged glyphs
    if (args.hanged_start >= 0) {
        calc_hanged_glyphs_start(&args, gis, ges,
                glyph_pos, n, x, y);
    }

    if ((size_t)args.hanged_end < n) {
        if (max_extent > 0) {
            calc_hanged_glyphs_end(&args, gis, ges,
                        glyph_pos, n, x, y, MAX(max_extent, total_extent));
        }
        else {
            calc_hanged_glyphs_end(&args, gis, ges,
                    glyph_pos, n, x, y, total_extent);
        }
    }

    gap = max_extent - total_extent;
    // justify the unhanged glyphs
    if ((render_flags & FOIL_GRF_ALIGN_MASK) == FOIL_GRF_ALIGN_JUSTIFY
            && gap > 0) {
        switch (render_flags & FOIL_GRF_TEXT_JUSTIFY_MASK) {
        case FOIL_GRF_TEXT_JUSTIFY_INTER_WORD:
            justify_glyphs_inter_word(&args, gis, ges, n, gap);
            break;
        case FOIL_GRF_TEXT_JUSTIFY_INTER_CHAR:
            justify_glyphs_inter_char(&args, gis, ges, n, gap);
            break;
        case FOIL_GRF_TEXT_JUSTIFY_AUTO:
        default:
            justify_glyphs_auto(&args, gis, ges, n, gap);
            break;
        }

    }

    // calcualte unhanged glyph positions according to the base point
    calc_unhanged_glyph_positions(&args, gis, ges, n, x, y, glyph_pos);

    // align unhanged glyphs
    align_unhanged_glyphs(&args, glyph_pos, n, gap);

    if (line_size) {
        if ((render_flags & FOIL_GRF_WRITING_MODE_MASK)
                == FOIL_GRF_WRITING_MODE_HORIZONTAL_TB) {
            line_size->cx = glyph_pos[n - 1].x - glyph_pos[0].x
                + ges[n - 1].adv_x + ges[n - 1].extra_x;
            line_size->cy = args.lw;
        }
        else {
            line_size->cy = glyph_pos[n - 1].y - glyph_pos[0].y
                + ges[n - 1].adv_y + ges[n - 1].extra_y;
            line_size->cx = args.lw;
        }
    }

    free(gis);
    if (glyph_ext_info == NULL)
        free(ges);

    return n;

error:
    if (ges) free(ges);
    if (gis) free(gis);

    return 0;
}

