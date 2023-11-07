/*
** @file rdrbox-inline.c
** @author Vincent Wei
** @date 2022/10/29
** @brief The implementation of inline box.
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
#include "util/list.h"

#include <assert.h>
#include <stdlib.h>

static void inline_data_cleaner(void *data)
{
    struct _inline_box_data *inline_data = (struct _inline_box_data *)data;

    struct text_paragraph *p, *n;
    list_for_each_entry_safe(p, n, &inline_data->paras, ln) {
        list_del(&p->ln);
        free(p->ucs);
        if (p->break_oppos)
            free(p->break_oppos);
        if (p->glyph_poses)
            free(p->glyph_poses);
        free(p);
    }
}

bool foil_rdrbox_init_inline_data(foil_create_ctxt *ctxt,
        foil_rdrbox *box, const char *text, size_t len)
{
    (void)ctxt;

    size_t consumed;
    size_t left = len;

    struct _inline_box_data *inline_data = box->inline_data;
    assert(inline_data && inline_data->nr_paras == 0);

    while (left > 0) {
        uint32_t *ucs;
        size_t nr_ucs;

        consumed = foil_ustr_from_utf8_until_paragraph_boundary(text, left,
                box->white_space, &ucs, &nr_ucs);

        if (consumed == 0)
            break;

        if (nr_ucs > 0) {
            assert(ucs);

            struct text_paragraph *seg;
            seg = calloc(1, sizeof(*seg));
            if (seg == NULL)
                goto failed;

            seg->ucs = ucs;
            seg->nr_ucs = nr_ucs;

            // break oppos
            uint8_t lbp = box->line_break;
            if (lbp == FOIL_RDRBOX_LINE_BREAK_AUTO)
                lbp = FOIL_RDRBOX_LINE_BREAK_NORMAL;

            foil_ustr_get_breaks(box->lang_code, box->text_transform,
                    box->word_break, lbp, ucs, nr_ucs, &seg->break_oppos);
            if (seg->break_oppos == NULL) {
                LOG_ERROR("failed when getting break opportunities\n");
                goto failed;
            }

            list_add_tail(&seg->ln, &inline_data->paras);
            inline_data->nr_paras++;
        }

        left -= consumed;
        text += consumed;
    }

    if (inline_data->nr_paras > 0)
        box->extra_data_cleaner = inline_data_cleaner;

    return true;

failed:
    return false;
}

