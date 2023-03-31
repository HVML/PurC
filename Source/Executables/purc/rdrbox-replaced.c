/*
** @file rdrbox-replaced.c
** @author Vincent Wei
** @date 2023/01/31
** @brief The implementation of tailored operations for replaced box.
**
** Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

// #undef NDEBUG

#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "udom.h"
#include "page.h"

#include <assert.h>

struct _tailor_data {
    /* the code points of text in Unicode (logical order) */
    uint32_t *ucs;
    size_t nr_ucs;

    /* the break opportunities of the characters */
    foil_break_oppo_t *break_oppos;
};

static int
tailor(struct foil_create_ctxt *ctxt, struct foil_rdrbox *box)
{
    const char *text;
    size_t len;

    if (pcdoc_element_get_attribute(ctxt->udom->doc, box->owner,
            "alt", &text, &len)) {
        /* if no alt attribute, use tag name instead */
        text = ctxt->tag_name;
        len = strlen(ctxt->tag_name);
    }

    if (len > 0) {
#if 0
        box->tailor_data = calloc(1, sizeof(struct _tailor_data));

        size_t consumed = foil_ustr_from_utf8_until_paragraph_boundary(text,
                len, box->white_space,
                &box->tailor_data->ucs, &box->tailor_data->nr_ucs);

        if (consumed > 0 && box->tailor_data->nr_ucs > 0) {

            // break oppos
            uint8_t lbp = box->line_break;
            if (lbp == FOIL_RDRBOX_LINE_BREAK_AUTO)
                lbp = FOIL_RDRBOX_LINE_BREAK_NORMAL;

            foil_ustr_get_breaks(box->lang_code,
                    box->text_transform,
                    box->word_break, lbp,
                    box->tailor_data->ucs, box->tailor_data->nr_ucs,
                    &box->tailor_data->break_oppos);
        }
#endif

        box->inline_data = calloc(1, sizeof(*box->inline_data));
        INIT_LIST_HEAD(&box->inline_data->paras);
        foil_rdrbox_init_inline_data(ctxt, box, text, len);
    }

    return 0;
}

static void cleaner(struct foil_rdrbox *box)
{
    (void) box;
#if 0
    assert(box->tailor_data);
    if (box->tailor_data->ucs)
        free(box->tailor_data->ucs);
    if (box->tailor_data->break_oppos)
        free(box->tailor_data->break_oppos);
    free(box->tailor_data);
#endif
}

static inline int width_to_cols(int width)
{
    assert(width % FOIL_PX_GRID_CELL_W == 0);
    return width / FOIL_PX_GRID_CELL_W;
}

static void paint_alt(struct foil_render_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    struct _inline_box_data *inline_data = box->inline_data;

    if (inline_data->nr_paras == 0) {
        return;
    }


    foil_rect *rc_dest = &box->ctnt_rect;
    int x = rc_dest->left;
    int y = rc_dest->top;
    int bottom = rc_dest->bottom;
    int left_extent = rc_dest->right - rc_dest->left;
    if (left_extent <= 0 || y >= bottom) {
        goto failed;
    }

    foil_rect page_rc;
    foil_rdrbox_map_rect_to_page(rc_dest, &page_rc);

    const uint32_t render_flags =
        FOIL_GRF_WRITING_MODE_HORIZONTAL_TB |
        FOIL_GRF_TEXT_ORIENTATION_UPRIGHT |
        FOIL_GRF_OVERFLOW_WRAP_NORMAL;

    struct text_paragraph *p;
    list_for_each_entry(p, &inline_data->paras, ln) {
        assert(p->nr_ucs > 0);

        if (p->glyph_poses == NULL) {
            p->glyph_poses = malloc(sizeof(foil_glyph_pos) * p->nr_ucs);
            if (p->glyph_poses == NULL)
                goto failed;
        }

        size_t nr_laid = 0;
        while (nr_laid < p->nr_ucs) {
            foil_size seg_size;
            size_t n =
                foil_ustr_get_glyphs_extent_simple(p->ucs + nr_laid,
                        p->nr_ucs - nr_laid,
                        p->break_oppos + nr_laid + 1, render_flags,
                        0, 0,
                        box->letter_spacing, box->word_spacing, 0,
                        left_extent, &seg_size, NULL,
                        p->glyph_poses + nr_laid);
            assert(n > 0);
            if (seg_size.cx > left_extent) {
                /* new line */
                y += box->line_height;
                if (y >= bottom) {
                    goto paint_overflow;
                }
                x = rc_dest->left;
                left_extent = rc_dest->right - rc_dest->left;
                continue;
            }

            foil_page_set_fgc(ctxt->udom->page, box->color);

            uint32_t *ucs = p->ucs + nr_laid;
            foil_glyph_pos *poses = p->glyph_poses + nr_laid;

            int px = x / FOIL_PX_GRID_CELL_W;
            int py = y / FOIL_PX_GRID_CELL_H;

            for (size_t i = 0; i < n; i++) {
                if (poses[i].suppressed) {
                    continue;
                }

                foil_page_draw_uchar(ctxt->udom->page,
                        px + width_to_cols(poses[i].x), py, ucs[i], 1);
            }

            nr_laid += n;
            if (seg_size.cx > left_extent) {
                y += box->line_height;
                if (y >= bottom) {
                    goto paint_overflow;
                }
                x = rc_dest->left;
                left_extent = rc_dest->right - rc_dest->left;
            }
            else {
                left_extent -= seg_size.cx;
                x += seg_size.cx;
            }
        }

        if ((p->break_oppos[p->nr_ucs] & FOIL_BOV_LB_MASK) == FOIL_BOV_LB_MANDATORY) {
            y += box->line_height;
            if (y >= bottom) {
                goto paint_overflow;
            }
            x = rc_dest->left;
            left_extent = rc_dest->right - rc_dest->left;
        }
    }
    return;

paint_overflow:
    if (rc_dest->right - x < 3) {
        x = rc_dest->right - 3;
    }

    if (x <= rc_dest->left) {
        return;
    }

    uint32_t ucs = '.';
    y = y - box->line_height;
    int px = x / FOIL_PX_GRID_CELL_W;
    int py = y / FOIL_PX_GRID_CELL_H;
    for (size_t i = 0; i < 3; i++) {
        foil_page_draw_uchar(ctxt->udom->page, px + i, py, ucs, 1);
    }


failed:
    return;
}

static void
ctnt_painter(struct foil_render_ctxt *ctxt, struct foil_rdrbox *box)
{
    paint_alt(ctxt, box);
}

static struct foil_rdrbox_tailor_ops replaced_ops = {
    .tailor = tailor,
    .cleaner = cleaner,
    .ctnt_painter = ctnt_painter,
};

struct foil_rdrbox_tailor_ops *
foil_rdrbox_replaced_tailor_ops(struct foil_create_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    // TODO
    (void)ctxt;
    (void)box;
    return &replaced_ops;
}

