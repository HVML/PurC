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

// #undef NDEBUG

#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "udom.h"
#include "page.h"
#include "timer.h"

#include <assert.h>

#define TIMER_NAME      "progress"
#define TIMER_INTERVAL  100
#define INDICATOR_STEPS 10

struct _tailor_data {
    /* the max value, which must be larger than 0.0 */
    double max;

    /* in indeterminate state if the value is negative. */
    double value;

    /* the current indicator percent for indeterminate state */
    int     indicator;

    /* the indicator steps for indeterminate state */
    int     ind_steps;

    /* the handle of the timer for indeterminate status */
    pcmcth_timer_t    timer;
};

static int
timer_expired(const char *name, void *ctxt)
{
    (void)name;
    struct foil_rdrbox *box = ctxt;
    struct _tailor_data *tailor_data = box->tailor_data;

    tailor_data->indicator += tailor_data->ind_steps;
    if (tailor_data->ind_steps > 0 &&
            tailor_data->indicator >= 100) {
        tailor_data->indicator = 100;
        tailor_data->ind_steps = -INDICATOR_STEPS;
    }
    else if (tailor_data->ind_steps < 0 &&
            tailor_data->indicator <= 0) {
        tailor_data->indicator = 0;
        tailor_data->ind_steps = INDICATOR_STEPS;
    }

    foil_udom_invalidate_rdrbox(foil_udom_from_rdrbox(box), box);
    return 0;
}

static void
update_properties(purc_document_t doc, struct foil_rdrbox *box)
{
    const char *value;
    size_t len;

    if (pcdoc_element_get_attribute(doc, box->owner,
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

    pcmcth_renderer* rdr = foil_get_renderer();
    if (pcdoc_element_get_attribute(doc, box->owner,
            "value", &value, &len) == 0 && len > 0) {
        char buff[len + 1];
        strncpy(buff, value, len);
        buff[len] = 0;
        box->tailor_data->value = atof(buff);

        if (box->tailor_data->value < 0)
            box->tailor_data->value = 0;
        else if (box->tailor_data->value > box->tailor_data->max)
            box->tailor_data->value = box->tailor_data->max;

        /* uninstall the timer */
        if (box->tailor_data->timer) {
            foil_timer_delete(rdr, box->tailor_data->timer);
            box->tailor_data->timer = NULL;
        }
    }
    else {
        /* indeterminate */
        box->tailor_data->value = -1.0;
        box->tailor_data->indicator = 0;
        box->tailor_data->ind_steps = INDICATOR_STEPS;

        /* TODO: set a timer for indeterminate state */
        if (box->tailor_data->timer == NULL) {
            box->tailor_data->timer = foil_timer_new(rdr,
                    TIMER_NAME, timer_expired, TIMER_INTERVAL, box);
        }
    }
}

static int
tailor(struct foil_create_ctxt *ctxt, struct foil_rdrbox *box)
{
    box->tailor_data = calloc(1, sizeof(struct _tailor_data));
    update_properties(ctxt->udom->doc, box);
    return 0;
}

static void cleaner(struct foil_rdrbox *box)
{
    assert(box->tailor_data);
    if (box->tailor_data->timer) {
        pcmcth_renderer* rdr = foil_get_renderer();
        foil_timer_delete(rdr, box->tailor_data->timer);
    }
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
    foil_page_set_bgc(ctxt->udom->page, box->background_color);
    foil_page_erase_rect(ctxt->udom->page, &page_rc);

    if (box->tailor_data->value < 0) {
        /* in indeterminate state */
        foil_rect bar_rc = page_rc;

        bar_rc.left += tray_width * box->tailor_data->indicator / 100;
        bar_rc.right = bar_rc.left + tray_width / 10;

        if (foil_rect_intersect(&bar_rc, &bar_rc, &page_rc)) {
            LOG_DEBUG("Update PROGRESS bar: from %d to %d (%d)\n",
                    bar_rc.left, bar_rc.right, box->tailor_data->indicator);
            foil_page_set_bgc(ctxt->udom->page, FOIL_BGC_PROGRESS_BAR);
            foil_page_erase_rect(ctxt->udom->page, &bar_rc);
        }
    }
    else {
        double bar_ratio = box->tailor_data->value / box->tailor_data->max;
        assert(bar_ratio > 0 && bar_ratio < 1.0);
        int bar_width = (int)(tray_width * bar_ratio);

        page_rc.right = page_rc.left + bar_width;
        foil_page_set_bgc(ctxt->udom->page, FOIL_BGC_PROGRESS_BAR);
        foil_page_erase_rect(ctxt->udom->page, &page_rc);
    }
}

static void on_attr_changed(struct foil_update_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    double old_max = box->tailor_data->max;
    double old_val = box->tailor_data->value;
    update_properties(ctxt->udom->doc, box);

    if (old_max != box->tailor_data->max ||
            old_val != box->tailor_data->value) {
        foil_udom_invalidate_rdrbox(ctxt->udom, box);
    }
}

struct foil_rdrbox_tailor_ops _foil_rdrbox_progress_ops = {
    .tailor = tailor,
    .cleaner = cleaner,
    .bgnd_painter = bgnd_painter,
    .on_attr_changed = on_attr_changed,
};

