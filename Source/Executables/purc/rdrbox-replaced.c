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

#undef NDEBUG

#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "udom.h"

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
    }

    return 0;
}

static void cleaner(struct foil_rdrbox *box)
{
    assert(box->tailor_data);
    if (box->tailor_data->ucs)
        free(box->tailor_data->ucs);
    if (box->tailor_data->break_oppos)
        free(box->tailor_data->break_oppos);
    free(box->tailor_data);
}

static void
ctnt_painter(struct foil_render_ctxt *ctxt, struct foil_rdrbox *box)
{
    // TODO
    (void)ctxt;
    (void)box;
}

struct foil_rdrbox_tailor_ops _foil_rdrbox_replaced_ops = {
    tailor,
    cleaner,
    NULL,
    ctnt_painter,
};

