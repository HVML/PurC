/*
** @file rdrbox-layout-helpers.c
** @author Vincent Wei
** @date 2022/12/21
** @brief The implementation of layout helpers.
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

#undef NDEBUG

#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "udom.h"

#include "unicode/unicode.h"

#include <stdio.h>
#include <assert.h>

struct _block_fmt_ctxt *foil_rdrbox_block_fmt_ctxt_new(foil_block_heap *heap,
        int width, int height)
{
    struct _block_fmt_ctxt *ctxt = calloc(1, sizeof(*ctxt));
    if (ctxt) {
        foil_region_init(&ctxt->region, heap);
        foil_rect rc = { 0, 0, width, height };
        if (height < 0)
            rc.bottom = INT_MAX;
        foil_region_set(&ctxt->region, &rc);

        ctxt->max_height = rc.bottom;
    }

    return ctxt;
}

void foil_rdrbox_block_fmt_ctxt_delete(struct _block_fmt_ctxt *ctxt)
{
    foil_region_empty(&ctxt->region);
    free(ctxt);
}

struct _inline_fmt_ctxt *foil_rdrbox_inline_fmt_ctxt_new(
        foil_block_heap *heap, int width, int height)
{
    (void)width;
    (void)height;

    struct _inline_fmt_ctxt *ctxt = calloc(1, sizeof(*ctxt));
    if (ctxt) {
        foil_region_init(&ctxt->region, heap);
    }

    return ctxt;
}

void foil_rdrbox_inline_fmt_ctxt_delete(struct _inline_fmt_ctxt *ctxt)
{
    foil_region_empty(&ctxt->region);
    free(ctxt);
}

#define SZ_IN_STACK_BUFF    128

int foil_rdrbox_inline_calc_preferred_width(struct _preferred_width_ctxt *ctxt,
        foil_rdrbox *box)
{
    assert(box->is_inline_box);

    const uint32_t render_flags =
        FOIL_GRF_WRITING_MODE_HORIZONTAL_TB |
        FOIL_GRF_TEXT_ORIENTATION_UPRIGHT |
        FOIL_GRF_SPACES_REMOVE_START |
        FOIL_GRF_OVERFLOW_WRAP_NORMAL;
    const int max_extent = -1;

    foil_glyph_extinfo  gei_in_stack[SZ_IN_STACK_BUFF];
    foil_glyph_pos      gps_in_stack[SZ_IN_STACK_BUFF];

    struct _inline_box_data *inline_data = box->inline_data;
    struct text_paragraph *p;
    list_for_each_entry(p, &inline_data->paras, ln) {
        assert(p->nr_ucs > 0);

        foil_glyph_extinfo *gei = gei_in_stack;
        foil_glyph_pos     *gps = gps_in_stack;
        if (p->nr_ucs > SZ_IN_STACK_BUFF) {
            gei = malloc(sizeof(*gei) * p->nr_ucs);
            gps = malloc(sizeof(*gps) * p->nr_ucs);
            if (gei == NULL || gps == NULL) {
                if (gei) free(gei);
                if (gps) free(gps);
                goto failed;
            }
        }

        foil_ustr_get_glyphs_extent_simple(p->ucs, p->nr_ucs,
                p->break_oppos, render_flags,
                ctxt->x, ctxt->y, box->letter_spacing, box->word_spacing, 0,
                max_extent, NULL, gei, gps);

        ctxt->x = gps[p->nr_ucs - 1].x;
        ctxt->y = gps[p->nr_ucs - 1].y;
        if (p->break_oppos[p->nr_ucs] == FOIL_BOV_LB_MANDATORY) {
            ctxt->x = 0;
            ctxt->y += box->line_height;
        }
        else {
            ctxt->x += gps[p->nr_ucs - 1].advance;
        }

        if (gei != gei_in_stack)
            free(gei);
        if (gps != gps_in_stack)
            free(gps);
    }

    return 0;

failed:
    return -1;
}

int foil_rdrbox_inline_calc_preferred_minimum_width(foil_rdrbox *box)
{
    assert(box->is_inline_box);

    const uint32_t render_flags =
        FOIL_GRF_WRITING_MODE_HORIZONTAL_TB |
        FOIL_GRF_TEXT_ORIENTATION_UPRIGHT |
        FOIL_GRF_SPACES_REMOVE_START |
        FOIL_GRF_OVERFLOW_WRAP_ANYWHERE;
    const int max_extent = FOIL_PX_GRID_CELL_W;

    int width = 0;
    foil_size line_size;
    foil_glyph_extinfo  gei_in_stack[SZ_IN_STACK_BUFF];
    foil_glyph_pos      gps_in_stack[SZ_IN_STACK_BUFF];

    struct _inline_box_data *inline_data = box->inline_data;
    struct text_paragraph *p;
    list_for_each_entry(p, &inline_data->paras, ln) {
        foil_glyph_extinfo *gei = gei_in_stack;
        foil_glyph_pos     *gps = gps_in_stack;
        if (p->nr_ucs > SZ_IN_STACK_BUFF) {
            gei = malloc(sizeof(*gei) * p->nr_ucs);
            gps = malloc(sizeof(*gps) * p->nr_ucs);
            if (gei == NULL || gps == NULL) {
                if (gei) free(gei);
                if (gps) free(gps);
                goto failed;
            }
        }

        foil_ustr_get_glyphs_extent_simple(p->ucs, p->nr_ucs,
                p->break_oppos, render_flags,
                0, 0, box->letter_spacing, box->word_spacing, 0,
                max_extent, &line_size, gei, gps);

        if (line_size.cx > width)
            width = line_size.cx;

        if (gei != gei_in_stack)
            free(gei);
        if (gps != gps_in_stack)
            free(gps);
    }

    return width;

failed:
    return -1;
}


int foil_rdrbox_calc_shrink_to_fit_width(foil_layout_ctxt *ctxt,
        foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
    return 0;
}

bool foil_rdrbox_layout_inline(foil_layout_ctxt *ctxt,
        foil_rdrbox *block, foil_rdrbox *box)
{
    (void)ctxt;
    (void)block;
    (void)box;
    return true;
}

int foil_rdrbox_calc_height_only_inlines(foil_layout_ctxt *ctxt,
        foil_rdrbox *box)
{
    assert(box->nr_inline_level_children > 0 && box->is_width_resolved);

    (void)ctxt;
    (void)box;
    return 0;
}

