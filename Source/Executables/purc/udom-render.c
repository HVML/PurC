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

// #undef NDEBUG

#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "udom.h"
#include "page.h"

#include <stdio.h>
#include <assert.h>

static const uint32_t border_dotted_row = 0x2504;
static const uint32_t border_dotted_col = 0x2506;

static const uint32_t border_dash_row = 0x254C;
static const uint32_t border_dash_col = 0x254E;

static const uint32_t border_solid_row = 0x2500;
static const uint32_t border_solid_col = 0x2502;

static const uint32_t border_double_row = 0x2550;
static const uint32_t border_double_col = 0x2551;

static const uint32_t border_corner_down_right = 0x250C;
static const uint32_t border_corner_up_right = 0x2514;
static const uint32_t border_corner_down_left = 0x2510;
static const uint32_t border_corner_up_left = 0x2518;

static const uint32_t border_corner_double_down_right = 0x2554;
static const uint32_t border_corner_double_up_right = 0x255A;
static const uint32_t border_corner_double_down_left = 0x2557;
static const uint32_t border_corner_double_up_left = 0x255D;

static const uint32_t border_corner_down_single_right_double = 0x2552;
static const uint32_t border_corner_up_single_right_double = 0x2558;
static const uint32_t border_corner_down_single_left_double = 0x2555;
static const uint32_t border_corner_up_single_left_double = 0x255B;

static const uint32_t border_corner_down_double_right_single= 0x2553;
static const uint32_t border_corner_up_double_right_single= 0x2559;
static const uint32_t border_corner_down_double_left_single= 0x2556;
static const uint32_t border_corner_up_double_left_single= 0x255C;


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
    foil_render_ctxt rdr_ctxt = { .udom = udom, .fp = fp };

    render_rdrtree_file(&rdr_ctxt, udom->initial_cblock, 0);
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

#if 0
static inline void
map_rdrbox_rect_to_page(const foil_rect *rdrbox_rc, foil_rect *page_rc)
{
    assert(rdrbox_rc->left % FOIL_PX_GRID_CELL_W == 0);
    page_rc->left = rdrbox_rc->left / FOIL_PX_GRID_CELL_W;

    assert(rdrbox_rc->right % FOIL_PX_GRID_CELL_W == 0);
    page_rc->right = rdrbox_rc->right / FOIL_PX_GRID_CELL_W;

    assert(rdrbox_rc->top % FOIL_PX_GRID_CELL_H == 0);
    page_rc->top = rdrbox_rc->top / FOIL_PX_GRID_CELL_H;

    assert(rdrbox_rc->bottom % FOIL_PX_GRID_CELL_H == 0);
    page_rc->bottom = rdrbox_rc->bottom / FOIL_PX_GRID_CELL_H;
}
#endif

static void
render_marker_box(struct foil_render_ctxt *ctxt, struct foil_rdrbox *box)
{
    assert(box->type == FOIL_RDRBOX_TYPE_MARKER);

    foil_rect page_rc;
    foil_rdrbox_map_rect_to_page(&box->ctnt_rect, &page_rc);

    foil_page_set_fgc(ctxt->udom->page, box->color);
    foil_page_draw_ustring(ctxt->udom->page, page_rc.left, page_rc.top,
            box->marker_data->ucs, box->marker_data->nr_ucs);
}

static uint32_t
get_border_row_uc(int style)
{
    uint32_t uc;
    switch (style) {
    case FOIL_RDRBOX_BORDER_STYLE_DOTTED:
        uc = border_dotted_row;
        break;

    case FOIL_RDRBOX_BORDER_STYLE_DASHED:
        uc = border_dash_row;
        break;

    case FOIL_RDRBOX_BORDER_STYLE_SOLID:
        uc = border_solid_row;
        break;

    case FOIL_RDRBOX_BORDER_STYLE_DOUBLE:
        uc = border_double_row;
        break;

    default:
        uc = 0;
    }
    return uc;
}

static uint32_t
get_border_col_uc(int style)
{
    uint32_t uc;
    switch (style) {
    case FOIL_RDRBOX_BORDER_STYLE_DOTTED:
        uc = border_dotted_col;
        break;

    case FOIL_RDRBOX_BORDER_STYLE_DASHED:
        uc = border_dash_col;
        break;

    case FOIL_RDRBOX_BORDER_STYLE_SOLID:
        uc = border_solid_col;
        break;

    case FOIL_RDRBOX_BORDER_STYLE_DOUBLE:
        uc = border_double_col;
        break;

    default:
        uc = 0;
    }
    return uc;
}

static uint32_t
get_border_corner_lt(struct foil_rdrbox *box)
{
    uint32_t uc = 0;
    if (box->bl && box->bt) {
        if ((box->border_left_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) &&
            (box->border_top_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE)) {
            uc = border_corner_double_down_right;
        }
        else if (box->border_left_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) {
            uc = border_corner_down_double_right_single;
        }
        else if (box->border_top_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) {
            uc = border_corner_down_single_right_double;
        }
        else {
            uc = border_corner_down_right;
        }
    }
    else if (box->bl) {
        uc = get_border_col_uc(box->border_left_style);
    }
    else if (box->bt) {
        uc = get_border_row_uc(box->border_top_style);
    }
    return uc;
}

static uint32_t
get_border_corner_lb(struct foil_rdrbox *box)
{
    uint32_t uc = 0;
    if (box->bl && box->bb) {
        if ((box->border_left_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) &&
            (box->border_bottom_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE)) {
            uc = border_corner_double_up_right;
        }
        else if (box->border_left_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) {
            uc = border_corner_up_double_right_single;
        }
        else if (box->border_bottom_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) {
            uc = border_corner_up_single_right_double;
        }
        else {
            uc = border_corner_up_right;
        }
    }
    else if (box->bl) {
        uc = get_border_col_uc(box->border_left_style);
    }
    else if (box->bb) {
        uc = get_border_row_uc(box->border_bottom_style);
    }
    return uc;
}

static uint32_t
get_border_corner_rt(struct foil_rdrbox *box)
{
    uint32_t uc = 0;
    if (box->br && box->bt) {
        if ((box->border_right_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) &&
            (box->border_top_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE)) {
            uc = border_corner_double_down_left;
        }
        else if (box->border_right_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) {
            uc = border_corner_down_double_left_single;
        }
        else if (box->border_top_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) {
            uc = border_corner_down_single_left_double;
        }
        else {
            uc = border_corner_down_left;
        }
    }
    else if (box->br) {
        uc = get_border_col_uc(box->border_right_style);
    }
    else if (box->bt) {
        uc = get_border_row_uc(box->border_top_style);
    }
    return uc;
}

static uint32_t
get_border_corner_rb(struct foil_rdrbox *box)
{
    uint32_t uc = 0;
    if (box->br && box->bb) {
        if ((box->border_right_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) &&
            (box->border_bottom_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE)) {
            uc = border_corner_double_up_left;
        }
        else if (box->border_right_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) {
            uc = border_corner_up_double_left_single;
        }
        else if (box->border_bottom_style == FOIL_RDRBOX_BORDER_STYLE_DOUBLE) {
            uc = border_corner_up_single_left_double;
        }
        else {
            uc = border_corner_up_left;
        }
    }
    else if (box->br) {
        uc = get_border_col_uc(box->border_right_style);
    }
    else if (box->bb) {
        uc = get_border_row_uc(box->border_bottom_style);
    }
    return uc;
}

static void
render_rdrbox_part(struct foil_render_ctxt *ctxt,
        struct foil_rdrbox *box, foil_box_part_k part)
{
    switch (part) {
    case FOIL_BOX_PART_BACKGROUND:
        if (box->tailor_ops && box->tailor_ops->bgnd_painter) {
            box->tailor_ops->bgnd_painter(ctxt, box);
            break;
        }
        else {
            foil_rect page_rc;
            const foil_rect *rc;
            if (box->is_root) {
                rc = NULL;
            }
            else {
                foil_rdrbox_map_rect_to_page(&box->ctnt_rect, &page_rc);
                rc = &page_rc;
            }
            foil_page_set_bgc(ctxt->udom->page, box->background_color);
            foil_page_erase_rect(ctxt->udom->page, rc);
        }
        break;

    case FOIL_BOX_PART_BORDER:
        // TODO: draw border
        if (box->bt || box->br || box->bb || box->bl) {
            foil_rect border_rc;
            foil_rect destrc;
            const foil_rect *rc;
            uint32_t uc;

            foil_rdrbox_border_box(box, &border_rc);
            foil_rdrbox_map_rect_to_page(&border_rc, &destrc);
            rc = &destrc;

            if (box->bt) {
                uc = get_border_row_uc(box->border_top_style);
                ctxt->udom->page->bgc = box->border_top_color;
                int x = rc->left + 1;
                int y = rc->top;
                int count = rc->right - x;
                foil_page_draw_uchar(ctxt->udom->page, x, y, uc, count);
            }

            if (box->br) {
                uc = get_border_col_uc(box->border_right_style);
                ctxt->udom->page->bgc = box->border_right_color;
                int x = rc->right;
                int begin = rc->top + 1;
                int end = rc->bottom - 1;

                for (int i = begin; i < end; i++) {
                    foil_page_draw_uchar(ctxt->udom->page, x, i, uc, 1);
                }
            }

            if (box->bb) {
                uc = get_border_row_uc(box->border_bottom_style);
                ctxt->udom->page->bgc = box->border_bottom_color;
                int x = rc->left + 1;
                int y = rc->bottom - 1;
                int count = rc->right - x;
                foil_page_draw_uchar(ctxt->udom->page, x, y, uc, count);
            }

            if (box->bl) {
                uc = get_border_col_uc(box->border_left_style);
                ctxt->udom->page->bgc = box->border_right_color;
                int x = rc->left;
                int begin = rc->top + 1;
                int end = rc->bottom - 1;

                for (int i = begin; i < end; i++) {
                    foil_page_draw_uchar(ctxt->udom->page, x, i, uc, 1);
                }
            }

            /* top left corner */
            uc = get_border_corner_lt(box);
            if (uc) {
                foil_page_draw_uchar(ctxt->udom->page, rc->left, rc->top, uc, 1);
            }

            /* bottom left corner */
            uc = get_border_corner_lb(box);
            if (uc) {
                foil_page_draw_uchar(ctxt->udom->page, rc->left, rc->bottom - 1, uc, 1);
            }

            /* top right corner */
            uc = get_border_corner_rt(box);
            if (box->br) {
                foil_page_draw_uchar(ctxt->udom->page, rc->right, rc->top, uc, 1);
            }

            /* bottom right corner */
            uc = get_border_corner_rb(box);
            if (box->br) {
                foil_page_draw_uchar(ctxt->udom->page, rc->right, rc->bottom - 1, uc, 1);
            }
        }
        break;

    case FOIL_BOX_PART_CONTENT:
        if (box->tailor_ops && box->tailor_ops->ctnt_painter) {
            box->tailor_ops->ctnt_painter(ctxt, box);
            break;
        }
        break;
    }

    // default handler.
}

static void
render_runbox_part(struct foil_render_ctxt *ctxt, struct _line_info *line,
        struct _inline_runbox *run, foil_box_part_k part)
{
    switch (part) {
    case FOIL_BOX_PART_BACKGROUND:
        // do not draw bkgnd for inline text
        if (0 && !foil_rect_is_empty(&run->rc)) {
            foil_rect page_rc;
            foil_rdrbox_map_rect_to_page(&run->rc, &page_rc);
            foil_page_set_bgc(ctxt->udom->page, run->box->background_color);
            foil_page_erase_rect(ctxt->udom->page, &page_rc);
        }
        break;

    case FOIL_BOX_PART_BORDER:
        // TODO: draw border
        break;

    case FOIL_BOX_PART_CONTENT:
        if (!foil_rect_is_empty(&run->rc) && run->nr_ucs > 0) {
            foil_rect page_rc;
            foil_rdrbox_map_rect_to_page(&run->rc, &page_rc);

            uint32_t *ucs = run->span->ucs + run->first_uc;
            foil_glyph_pos *poses = run->span->glyph_poses + run->first_uc;
            for (size_t i = 0; i < run->nr_ucs; i++) {
                if (poses[i].suppressed) {
                    continue;
                }

                int x = page_rc.left + width_to_cols(poses[i].x);
                int y = page_rc.top;
                LOG_DEBUG("Draw char 0x%04x at (%d, %d), line (%d, %d)\n",
                        ucs[i], x, y, line->rc.left, line->rc.top);
                (void)line;
                foil_page_draw_uchar(ctxt->udom->page, x, y, ucs[i], 1);
            }
        }
        break;
    }

    // default handler.
}

static void
render_rdrbox_in_line(struct foil_render_ctxt *ctxt, struct _line_info *line,
        struct foil_rdrbox *box);
static void
render_rdrbox_with_stacking_ctxt(struct foil_render_ctxt *rdr_ctxt,
        struct foil_stacking_context *stk_ctxt,
        struct foil_rdrbox *box);

static void
render_runbox(struct foil_render_ctxt *ctxt, struct _line_info *line,
        struct _inline_runbox *run)
{
    struct foil_rdrbox *box = run->box;

    if (run->span) {
        render_runbox_part(ctxt, line, run, FOIL_BOX_PART_BACKGROUND);
        render_runbox_part(ctxt, line, run, FOIL_BOX_PART_BORDER);
    }
    else {
        render_rdrbox_part(ctxt, box, FOIL_BOX_PART_BACKGROUND);
        render_rdrbox_part(ctxt, box, FOIL_BOX_PART_BORDER);
    }

    if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
        if (run->span) {
            render_runbox_part(ctxt, line, run, FOIL_BOX_PART_CONTENT);
        }
        else if (box->is_in_flow && !box->position && box->is_inline_level) {
            render_rdrbox_in_line(ctxt, line, box);
        }

    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK) {
        render_rdrbox_with_stacking_ctxt(ctxt, NULL, box);
    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE_TABLE) {
        // TODO: table
    }
    else if (box->is_inline_level && box->is_replaced) {
        render_rdrbox_part(ctxt, box, FOIL_BOX_PART_CONTENT);
    }
}

static void
render_rdrbox_in_line(struct foil_render_ctxt *ctxt, struct _line_info *line,
        struct foil_rdrbox *box)
{
    for (size_t i = 0; i < line->nr_runs; i++) {
        struct _inline_runbox *run = line->runs + i;

        if (run->box == box) {
            foil_page_set_fgc(ctxt->udom->page, run->box->color);
            render_runbox(ctxt, line, run);
        }
    }
}

static void
render_lines(struct foil_render_ctxt *ctxt, struct foil_rdrbox *box)
{
    struct _inline_fmt_ctxt *lfmt_ctxt;
    lfmt_ctxt = foil_rdrbox_inline_fmt_ctxt(box);
    if (lfmt_ctxt) {
        for (size_t i = 0; i < lfmt_ctxt->nr_lines; i++) {
            struct _line_info *line = lfmt_ctxt->lines + i;
            for (size_t j = 0; j < line->nr_runs; j++) {
                struct _inline_runbox *run = line->runs + j;

                if (run->box->parent == box) {
                    foil_page_set_fgc(ctxt->udom->page, run->box->color);
                    render_runbox(ctxt, line, run);
                }
            }
        }
    }
}

static void
render_normal_boxes_in_tree_order(struct foil_render_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    if (box->is_control || box->is_replaced) {
        render_rdrbox_part(ctxt, box, FOIL_BOX_PART_BACKGROUND);
        render_rdrbox_part(ctxt, box, FOIL_BOX_PART_BORDER);
        render_rdrbox_part(ctxt, box, FOIL_BOX_PART_CONTENT);
        return;
    }
    else {
        render_rdrbox_part(ctxt, box, FOIL_BOX_PART_BACKGROUND);
        render_rdrbox_part(ctxt, box, FOIL_BOX_PART_BORDER);

        render_lines(ctxt, box);
    }

    if (box->type == FOIL_RDRBOX_TYPE_LIST_ITEM &&
            box->list_item_data->marker_box) {
        // render marker
        render_marker_box(ctxt, box->list_item_data->marker_box);
    }

    foil_rdrbox *child = box->first;
    while (child) {

        // For all its in-flow, non-positioned, block-level descendants
        // in tree order
        if (child->is_in_flow && !child->position && child->is_block_level) {
            render_normal_boxes_in_tree_order(ctxt, child);
        }

        child = child->next;
    }
}

static void
render_rdrbox_with_stacking_ctxt(struct foil_render_ctxt *rdr_ctxt,
        struct foil_stacking_context *stk_ctxt,
        struct foil_rdrbox *box)
{
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
                render_rdrbox_part(rdr_ctxt, box, FOIL_BOX_PART_BACKGROUND);

            // border of element.
            render_rdrbox_part(rdr_ctxt, box, FOIL_BOX_PART_BORDER);
        }
    }

    // Stacking contexts formed by positioned descendants
    // with negative z-indices (excluding 0) in z-index order
    // (most negative first) then tree order.
    if (stk_ctxt) {
        size_t n = sorted_array_count(stk_ctxt->zidx2child);
        for (size_t i = 0; i < n; i++) {
            int zidx;
            struct list_head *head;
            zidx = (int)(int64_t)sorted_array_get(stk_ctxt->zidx2child,
                    i, (void **)&head);

            if (zidx >= 0)
                break;

            foil_stacking_context *p;
            list_for_each_entry(p, head, list) {
                render_rdrbox_with_stacking_ctxt(rdr_ctxt, p, p->creator);
            }
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
                render_rdrbox_part(rdr_ctxt, child, FOIL_BOX_PART_BACKGROUND);
                render_rdrbox_part(rdr_ctxt, child, FOIL_BOX_PART_BORDER);
            }

        }

        // All non-positioned floating descendants, in tree order.
        // For each one of these, treat the element as if it created a new
        // stacking context, but any positioned descendants and descendants
        // which actually create a new stacking context should be considered
        // part of the parent stacking context, not this new one.
        if (!child->position && child->floating) {
            render_rdrbox_with_stacking_ctxt(rdr_ctxt, NULL, child);
        }

        child = child->next;
    }

    // If the element is an inline element that generates a stacking context
    if (box->type == FOIL_RDRBOX_TYPE_INLINE && box->stacking_ctxt) {
        assert(box->parent);

        struct _inline_fmt_ctxt *lfmt_ctxt;
        lfmt_ctxt = foil_rdrbox_inline_fmt_ctxt(box->parent);
        if (lfmt_ctxt) {
            for (size_t i = 0; i < lfmt_ctxt->nr_lines; i++) {
                struct _line_info *line = lfmt_ctxt->lines + i;
                render_rdrbox_in_line(rdr_ctxt, line, box);
            }
        }
    }
    else {
        // Otherwise: first for the element, then for all its in-flow,
        // non-positioned, block-level descendants in tree order:
        render_normal_boxes_in_tree_order(rdr_ctxt, box);
    }

    // All positioned descendants with 'z-index: auto' or 'z-index: 0',
    // in tree order.
    child = box->first;
    while (child) {

        if (child->position && child->z_index == 0) {
            if (child->is_zidx_auto) {
                render_rdrbox_with_stacking_ctxt(rdr_ctxt, NULL, child);
            }
            else {
                assert(child->stacking_ctxt);
                render_rdrbox_with_stacking_ctxt(rdr_ctxt,
                        child->stacking_ctxt, child);
            }
        }

        child = child->next;
    }

    // Stacking contexts formed by positioned descendants with z-indices
    // greater than or equal to 1 in z-index order (smallest first)
    // then tree order.
    if (stk_ctxt) {
        size_t n = sorted_array_count(stk_ctxt->zidx2child);
        for (size_t i = 0; i < n; i++) {
            int zidx;
            struct list_head *head;
            zidx = (int)(int64_t)sorted_array_get(stk_ctxt->zidx2child,
                    i, (void **)&head);

            if (zidx <= 0)
                continue;

            foil_stacking_context *p;
            list_for_each_entry(p, head, list) {
                render_rdrbox_with_stacking_ctxt(rdr_ctxt, p, p->creator);
            }
        }
    }
}

void foil_udom_render_to_page(pcmcth_udom *udom)
{
    (void)udom;

    foil_render_ctxt rdr_ctxt = { .udom = udom, .fp = NULL };

    /* continue for the children */
    foil_rdrbox *root = udom->initial_cblock->first;
    assert(root->is_root && root->stacking_ctxt);

    render_rdrbox_with_stacking_ctxt(&rdr_ctxt, root->stacking_ctxt, root);
}

void foil_udom_invalidate_rdrbox(pcmcth_udom *udom, foil_rdrbox *box)
{
    struct foil_stacking_context *stacking_ctxt = NULL;
    foil_rdrbox *parent = box;

    do {
        if (parent->stacking_ctxt)
            stacking_ctxt = parent->stacking_ctxt;

        parent = parent->parent;
    } while (parent);

    assert(stacking_ctxt);

    foil_rect invrc;
    foil_rdrbox_border_box(box, &invrc);

    if (!foil_rect_is_empty(&invrc)) {
        foil_render_ctxt rdr_ctxt = { .udom = udom, .invrc = &invrc };

        render_rdrbox_with_stacking_ctxt(&rdr_ctxt, stacking_ctxt, box);
        foil_page_expose(udom->page);
    }
}

