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

#include <stdio.h>
#include <assert.h>

struct _block_fmt_ctxt *foil_rdrbox_block_fmt_ctxt_new(int height)
{
    struct _block_fmt_ctxt *ctxt = calloc(1, sizeof(*ctxt));
    if (ctxt) {
        if (height < 0)
            ctxt->max_height = INT_MAX;
        else
            ctxt->max_height = height;
    }

    return ctxt;
}

void foil_rdrbox_block_fmt_ctxt_delete(struct _block_fmt_ctxt *ctxt)
{
    free(ctxt);
}

struct _inline_fmt_ctxt *foil_rdrbox_inline_fmt_ctxt_new(
        foil_block_heap *heap, int width, int height)
{
    assert(width > 0);

    struct _inline_fmt_ctxt *ctxt = calloc(1, sizeof(*ctxt));
    if (ctxt) {
        foil_region_init(&ctxt->region, heap);
        foil_rect rc = { 0, 0, width, height };
        if (height < 0)
            rc.bottom = INT_MAX;

        foil_region_set(&ctxt->region, &rc);
    }

    return ctxt;
}

void foil_rdrbox_inline_fmt_ctxt_delete(struct _inline_fmt_ctxt *ctxt)
{
    foil_region_empty(&ctxt->region);
    free(ctxt);
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
    (void)ctxt;
    (void)box;
    return 0;
}

