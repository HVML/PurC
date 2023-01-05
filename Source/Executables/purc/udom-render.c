/*
** @file rdrbox-render.c
** @author Vincent Wei
** @date 2022/12/29
** @brief The implementation of rendering boxes.
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

#include <stdio.h>
#include <assert.h>

static void rdrbox_render_before_file(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
    /* print title */
    if (level == 0) {
        if (ctxt->udom->title_ucs) {
            for (size_t i = 0; i < ctxt->udom->title_len; i++) {
                char utf8[10];
                unsigned len = pcutils_unichar_to_utf8(ctxt->udom->title_ucs[i],
                        (unsigned char *)utf8);
                utf8[len] = 0;
                fputs(utf8, ctxt->fp);
            }
            fputs("\n", ctxt->fp);
        }
    }
    (void)box;
}

static void render_ucs(FILE *fp, const uint32_t *ucs, size_t nr_ucs)
{
    for (size_t i = 0; i < nr_ucs; i++) {
        char utf8[16];
        unsigned len = pcutils_unichar_to_utf8(ucs[i],
                (unsigned char *)utf8);
        utf8[len] = 0;
        fputs(utf8, fp);
    }
}

static void rdrbox_render_content_file(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
    (void)ctxt;
    (void)level;

    if (box->type == FOIL_RDRBOX_TYPE_LIST_ITEM) {
        if (box->list_item_data->marker_box) {
            foil_rdrbox *marker = box->list_item_data->marker_box;
            render_ucs(ctxt->fp,
                    marker->marker_data->ucs, marker->marker_data->nr_ucs);
        }
    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
        struct _inline_box_data *inline_data = box->inline_data;
        struct text_paragraph *p;
        list_for_each_entry(p, &inline_data->paras, ln) {
            render_ucs(ctxt->fp, p->ucs, p->nr_ucs);
        }
    }
}

static void rdrbox_render_after_file(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
    (void)ctxt;
    (void)level;

    if (box->is_block_level && box->first && box->first->is_inline_level) {
        fputs("\n", ctxt->fp);
    }
}

static void
render_rdrtree_file(struct foil_render_ctxt *ctxt, struct foil_rdrbox *ancestor,
        unsigned level)
{
    rdrbox_render_before_file(ctxt, ancestor, level);
    rdrbox_render_content_file(ctxt, ancestor, level);

    /* travel children */
    foil_rdrbox *child = ancestor->first;
    while (child) {

        render_rdrtree_file(ctxt, child, level + 1);

        child = child->next;
    }

    rdrbox_render_after_file(ctxt, ancestor, level);
}

void foil_udom_render_to_file(pcmcth_udom *udom, FILE *fp)
{
    /* render the whole tree */
    foil_render_ctxt render_ctxt = { udom, { fp } };

    LOG_DEBUG("Calling render_rdrtree...\n");
    render_rdrtree_file(&render_ctxt, udom->initial_cblock, 0);
}

static inline int width_to_cols(int width)
{
    assert(width % FOIL_PX_GRID_CELL_W == 0);
    return width / FOIL_PX_GRID_CELL_W;
}

static inline int height_to_rows(int height)
{
    assert(height % FOIL_PX_GRID_CELL_H == 0);
    return height / FOIL_PX_GRID_CELL_H;
}

static void
rdrbox_draw_background(struct foil_render_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
}

static void
rdrbox_draw_border(struct foil_render_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
}

static void
render_rdrbox_with_stacking_ctxt(struct foil_render_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    assert(box->stacking_ctxt);

    if (box->is_root) {
        // background color of element over the entire canvas.
    }

    if (box->is_block_level) {
        if (box->type == FOIL_RDRBOX_TYPE_TABLE) {
            // TODO: table
        }
        else {
            // background color of element unless it is the root element.
            if (!box->is_root)
                rdrbox_draw_background(ctxt, box);

            // border of element.
            rdrbox_draw_border(ctxt, box);
        }
    }

    // Stacking contexts formed by positioned descendants
    // with negative z-indices (excluding 0) in z-index order
    // (most negative first) then tree order.
    size_t n = sorted_array_count(box->stacking_ctxt->zidx2child);
    for (size_t i = 0; i < n; i++) {
        int zidx;
        struct list_head *head;
        zidx = (int)(int64_t)sorted_array_get(box->stacking_ctxt->zidx2child,
                i, (void **)&head);

        if (zidx >= 0)
            break;

        foil_stacking_context *p;
        list_for_each_entry(p, head, list) {
            render_rdrbox_with_stacking_ctxt(ctxt, p->creator);
        }
    }

    foil_rdrbox *child = box->first;
    while (child) {

        // For all its in-flow, non-positioned, block-level descendants
        // in tree order
        if (child->is_in_flow && !child->position && child->is_block_level) {
            if (child->type == FOIL_RDRBOX_TYPE_TABLE) {
                // TODO: table
            }
            else {
                rdrbox_draw_background(ctxt, child);
                rdrbox_draw_border(ctxt, child);
            }
        }

        // All non-positioned floating descendants, in tree order.
        if (!child->position && child->floating) {
        }

        child = child->next;
    }

    if (box->is_inline_level) {
    }
}

void foil_udom_render_to_page(pcmcth_udom *udom, pcmcth_page *page)
{
    (void)udom;
    (void)page;

    foil_render_ctxt render_ctxt = { udom, { page } };

    /* continue for the children */
    foil_rdrbox *root = udom->initial_cblock->first;
    assert(root->is_root && root->stacking_ctxt);

    render_rdrbox_with_stacking_ctxt(&render_ctxt, root);
}

