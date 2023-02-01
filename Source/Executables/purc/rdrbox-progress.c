/*
** @file rdrbox-progress.c
** @author Vincent Wei
** @date 2023/01/31
** @brief The implementation of tailored operations for progress box.
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
#include "page.h"

#include <assert.h>

struct _tailor_data {
    double max;
    /* in indeterminate state if the value is negative. */
    double value;

    /* the current indicator position for indeterminate state */
    int    indicator;
};

static int
tailor(struct foil_create_ctxt *ctxt, struct foil_rdrbox *box)
{
    const char *value;
    size_t len;

    box->tailor_data = calloc(1, sizeof(struct _tailor_data));

    if (pcdoc_element_get_attribute(ctxt->udom->doc, box->owner,
            "max", &value, &len) == 0 && len > 0) {
        char buff[len + 1];
        strncpy(buff, value, len);
        buff[len] = 0;
        box->tailor_data->max = atof(buff);

        if (box->tailor_data->max < 0)
            box->tailor_data->max = 1.0;
    }
    else {
        box->tailor_data->max = 1.0;
    }

    if (pcdoc_element_get_attribute(ctxt->udom->doc, box->owner,
            "value", &value, &len) == 0 && len > 0) {
        char buff[len + 1];
        strncpy(buff, value, len);
        buff[len] = 0;
        box->tailor_data->value = atof(buff);

        if (box->tailor_data->value < 0)
            box->tailor_data->value = 0;
        else if (box->tailor_data->value > box->tailor_data->max)
            box->tailor_data->value = box->tailor_data->max;
    }
    else {
        /* indeterminate */
        box->tailor_data->value = -1.0;
        box->tailor_data->indicator = 0;

        /* TODO: set a timer for indeterminate state */
    }

    return 0;
}

static void cleaner(struct foil_rdrbox *box)
{
    assert(box->tailor_data);
    free(box->tailor_data);
}

static void
bgnd_painter(struct foil_render_ctxt *ctxt, struct foil_rdrbox *box)
{
    foil_rect page_rc;
    foil_rdrbox_map_rect_to_page(&box->ctnt_rect, &page_rc);

    if (foil_rect_is_empty(&page_rc))
        return;

    int tray_width = foil_rect_width(&page_rc);
    foil_page_set_bgc(ctxt->page, box->background_color);
    foil_page_erase_rect(ctxt->page, &page_rc);

    if (box->tailor_data->value < 0) {
        /* in indeterminate state */
        page_rc.left  += box->tailor_data->indicator;
        page_rc.right = page_rc.left + 1;

        box->tailor_data->indicator++;
        if (box->tailor_data->indicator >= tray_width)
            box->tailor_data->indicator = 0;

        foil_page_set_bgc(ctxt->page, FOIL_BGC_PROGRESS_BAR);
        foil_page_erase_rect(ctxt->page, &page_rc);
    }
    else {
        double bar_ratio = box->tailor_data->value / box->tailor_data->max;
        assert(bar_ratio > 0 && bar_ratio < 1.0);
        int bar_width = (int)(tray_width * bar_ratio);

        page_rc.right = page_rc.left + bar_width;
        foil_page_set_bgc(ctxt->page, FOIL_BGC_PROGRESS_BAR);
        foil_page_erase_rect(ctxt->page, &page_rc);
    }
}

struct foil_rdrbox_tailor_ops _foil_rdrbox_progress_ops = {
    tailor,
    cleaner,
    bgnd_painter,
    NULL,
};

