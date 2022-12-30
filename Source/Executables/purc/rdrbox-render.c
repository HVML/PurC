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

void foil_rdrbox_render_before(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
#ifndef NDEBUG
    /* print title */
    if (level == 0) {
        if (ctxt->udom->title_ucs) {
            for (size_t i = 0; i < ctxt->udom->title_len; i++) {
                char utf8[10];
                unsigned len = pcutils_unichar_to_utf8(ctxt->udom->title_ucs[i],
                        (unsigned char *)utf8);
                utf8[len] = 0;
                fputs(utf8, stdout);
            }
            fputs("\n", stdout);
        }
    }
    (void)box;
#else
    (void)ctxt;
    (void)box;
    (void)level;
#endif
}

static void render_ucs(const uint32_t *ucs, size_t nr_ucs)
{
    for (size_t i = 0; i < nr_ucs; i++) {
        char utf8[16];
        unsigned len = pcutils_unichar_to_utf8(ucs[i],
                (unsigned char *)utf8);
        utf8[len] = 0;
        fputs(utf8, stdout);
    }
}

void foil_rdrbox_render_content(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
    (void)ctxt;
    (void)level;

    if (box->type == FOIL_RDRBOX_TYPE_LIST_ITEM) {
        if (box->list_item_data->marker_box) {
            foil_rdrbox *marker = box->list_item_data->marker_box;
            render_ucs(marker->marker_data->ucs, marker->marker_data->nr_ucs);
        }
    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
        struct _inline_box_data *inline_data = box->inline_data;
        struct text_paragraph *p;
        list_for_each_entry(p, &inline_data->paras, ln) {
            render_ucs(p->ucs, p->nr_ucs);
        }
    }
}

void foil_rdrbox_render_after(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level)
{
    (void)ctxt;
    (void)level;

    if (box->is_block_level && box->first && box->first->is_inline_level) {
        fputs("\n", stdout);
    }
}

