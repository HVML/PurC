/*
** @file rdrbox-internal.h
** @author Vincent Wei
** @date 2022/10/10
** @brief The internal interface for renderring box.
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

#ifndef purc_foil_rdrbox_internal_h
#define purc_foil_rdrbox_internal_h

#include <purc/purc-utils.h>
#include <stdint.h>

#include "unicode/unicode.h"
#include "region/region.h"
#include "util/list.h"

#define LEN_BUF_INTEGER 128

struct text_paragraph {
    struct list_head ln;

    /* the code points of text in Unicode (logical order) */
    uint32_t *ucs;
    size_t nr_ucs;

    /* the break opportunities of the characters */
    foil_break_oppo_t *break_oppos;

    /* the glyph positions */
    foil_glyph_pos *glyph_poses;
};

struct _inline_box_data {
    foil_langcode_t lang;
    unsigned nr_paras;

    /* the text segments */
    struct list_head paras;
};

struct _inline_runbox {
    /* the box generating this inline run */
    foil_rdrbox *box;

    /* the rectangle of this inline run */
    foil_rect rc;

    /* the text span if the box is an inline box */
    const struct text_paragraph *span;
    /* the index of the first character of this segment in the text span */
    size_t first_uc;
    /* the number of characters fits in this segment */
    size_t nr_ucs;
};

struct _line_info {
    /* the bounding rectangle of this line */
    foil_rect rc;

    /* the actual width and height of this line */
    int width, height;

    /* the position to lay the new segment */
    int x, y;

    /* the left extent of the current line */
    int left_extent;

    /* the number of inline runs in this line */
    size_t nr_runs;

    /* the array of inline segments fit in this line */
    struct _inline_runbox *runs;
};

struct _inline_fmt_ctxt {
    /* the bounding rectangle of all inlines */
    foil_rect rc;

    /* the possible/maximum extent of a line */
    int poss_extent;

    /* number of total lines */
    size_t nr_lines;

    /* pointer to the array of lines */
    struct _line_info *lines;
};

struct _block_box_data {
    /* not NULL if the block contains inline level boxes */
    struct _inline_fmt_ctxt *lfmt_ctxt;
};

struct _inline_block_data {
    /* not NULL if the block contains inline level boxes */
    struct _inline_fmt_ctxt *lfmt_ctxt;
};

struct _list_item_data {
    /* not NULL if the block contains inline level boxes */
    struct _inline_fmt_ctxt *lfmt_ctxt;
    /* index in the parent box */
    unsigned index;
    /* NULL for no marker */
    foil_rdrbox *marker_box;
};

struct _marker_box_data {
    uint32_t *ucs;
    size_t    nr_ucs;
    int       width;
};

struct _block_fmt_ctxt {
    /* < 0 for no limit */
    int max_height;
    int allocated_height;

    /* the available region to lay out floats and inline boxes. */
    foil_region region;
};

/* not used so far */
struct _preferred_width_ctxt {
    int x, y;
};

#ifdef __cplusplus
extern "C" {
#endif

struct _block_fmt_ctxt *foil_rdrbox_block_fmt_ctxt_new(foil_block_heap *heap,
        int width, int height);
void foil_rdrbox_block_fmt_ctxt_delete(struct _block_fmt_ctxt *ctxt);

struct _inline_fmt_ctxt *foil_rdrbox_inline_fmt_ctxt_new(void);
void foil_rdrbox_block_box_cleanup(struct _block_box_data *data);
void foil_rdrbox_list_item_cleanup(struct _list_item_data *data);
void foil_rdrbox_inline_block_box_cleanup(struct _inline_block_data *data);

static inline struct _inline_fmt_ctxt *
foil_rdrbox_inline_fmt_ctxt(foil_rdrbox *box)
{
    if (box->type == FOIL_RDRBOX_TYPE_BLOCK)
        return box->block_data->lfmt_ctxt;
    else if (box->type == FOIL_RDRBOX_TYPE_LIST_ITEM)
        return box->list_item_data->lfmt_ctxt;
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK)
        return box->inline_block_data->lfmt_ctxt;
    return NULL;
}

static inline void
foil_rdrbox_line_set_size(struct _line_info *line,
        int width, int height)
{
    line->width += width;
    line->rc.right += width;
    if (height > line->height) {
        line->height = height;
        line->rc.bottom = line->rc.top + line->height;
    }
}

struct _inline_runbox *
foil_rdrbox_line_allocate_new_run(struct _inline_fmt_ctxt *fmt_ctxt);

struct _line_info *foil_rdrbox_block_allocate_new_line(foil_layout_ctxt *ctxt,
        foil_rdrbox *block);

int foil_rdrbox_inline_calc_preferred_width(foil_rdrbox *box);
int foil_rdrbox_inline_calc_preferred_minimum_width(foil_rdrbox *box);

struct _line_info *foil_rdrbox_layout_inline(foil_layout_ctxt *ctxt,
        foil_rdrbox *block, foil_rdrbox *box);

extern struct foil_rdrbox_tailor_ops _foil_rdrbox_replaced_ops;
extern struct foil_rdrbox_tailor_ops _foil_rdrbox_progress_ops;
extern struct foil_rdrbox_tailor_ops _foil_rdrbox_meter_ops;

#ifdef __cplusplus
}
#endif

#endif /* not defined purc_foil_rdrbox_internal_h */

