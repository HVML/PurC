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
};

struct _inline_box_data {
    foil_langcode_t lang;
    unsigned nr_paras;

    /* the text segments */
    struct list_head paras;
};

struct _block_box_data {
    int text_indent;

    uint8_t text_align:3;
};

struct _inline_block_data {
    int foo, bar;

    uint8_t text_align:3;
};

struct _list_item_data {
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
};

struct _inline_fmt_ctxt {
    /* the available region to lay out floats and inline boxes. */
    foil_region region;
};

#ifdef __cplusplus
extern "C" {
#endif

struct _block_fmt_ctxt *foil_rdrbox_block_fmt_ctxt_new(int height);
void foil_rdrbox_block_fmt_ctxt_delete(struct _block_fmt_ctxt *ctxt);

struct _inline_fmt_ctxt *foil_rdrbox_inline_fmt_ctxt_new(
        foil_block_heap *heap, int width, int height);
void foil_rdrbox_inline_fmt_ctxt_delete(struct _inline_fmt_ctxt *ctxt);

int foil_rdrbox_width_shrink_to_fit(
        foil_layout_ctxt *ctxt, foil_rdrbox *box);

int foil_rdrbox_height_for_visible_non_replaced(
        foil_layout_ctxt *ctxt, foil_rdrbox *box);

int foil_rdrbox_height_for_block_fmt_ctxt_maker(
        foil_layout_ctxt *ctxt, foil_rdrbox *box);

#ifdef __cplusplus
}
#endif

#endif /* not defined purc_foil_rdrbox_internal_h */

