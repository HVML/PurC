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
#include "timer.h"
#include "foil.h"

#include <assert.h>

#define TIMER_NAME      "progress"
#define TIMER_INTERVAL  100
#define INDICATOR_STEPS 10

static const char *def_bar_marks = "━━";
static const char *def_mark_marks = "⣾⣷⣯⣟⡿⢿⣻⣽⣿";
// static const char *def_mark_marks = "⣿⣾⣶⣦⣆⡆⠆⠂⢁⠁⠉⠙⠹⢹⣹⣽⣿";
// static const char *def_mark_marks = "⠁⠈⠐⠠⢀⡀⠄⠂";

struct _tailor_data {
    /* XXX: The following two fields must be placed at the head of this struct.
       The candidate marks; */
    int         nr_marks;
    int         mark_width;
    uint32_t   *marks;

    /* the max value, which must be larger than 0.0 */
    double          max;

    /* in indeterminate state if the value is negative. */
    double          value;

    /* the current indicator percent for indeterminate state */
    int             indicator;

    /* the indicator steps for indeterminate state */
    int             ind_steps;

    /* the handle of the timer for indeterminate status */
    pcmcth_timer_t  timer;

    /* colors */
    foil_color      color_prim;
    foil_color      color_seco;
};

static int
timer_expired(const char *name, void *ctxt)
{
    (void)name;
    struct foil_rdrbox *box = ctxt;
    struct _tailor_data *tailor_data = box->tailor_data;

    if (box->ctrl_type == CSS_APPEARANCE_PROGRESS_MARK) {
        tailor_data->indicator += tailor_data->ind_steps;
        if (tailor_data->indicator >= 100) {
            tailor_data->indicator = 0;
        }
    }
    else {
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

static int update_style_properties(struct foil_rdrbox *box)
{
    uint8_t v;
    if (box->is_control) {
        const char *marks = NULL;
        size_t marks_len;

        lwc_string *str;
        v = css_computed_foil_candidate_marks(box->computed_style, &str);
        if (v != CSS_FOIL_CANDIDATE_MARKS_AUTO) {
            assert(str);

            marks = lwc_string_data(str);
            marks_len = lwc_string_length(str);
            if (foil_validate_marks(box->tailor_data, marks, marks_len)) {
                // bad value
                v = CSS_FOIL_CANDIDATE_MARKS_AUTO;
            }
        }

        if (v == CSS_FOIL_CANDIDATE_MARKS_AUTO) {
            if (box->ctrl_type == FOIL_RDRBOX_CTRL_PROGRESS_MARK) {
                marks = def_mark_marks;
            }
            else {
                marks = def_bar_marks;
            }

            marks_len = strlen(marks);
            int r = foil_validate_marks(box->tailor_data, marks, marks_len);
            assert(r == 0);
            (void)r;
        }
    }

    v = css_computed_foil_color_primary(box->computed_style,
            &box->tailor_data->color_prim.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_prim.specified = false;
    else
        box->tailor_data->color_prim.specified = true;

    v = css_computed_foil_color_secondary(box->computed_style,
            &box->tailor_data->color_seco.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_seco.specified = false;
    else
        box->tailor_data->color_seco.specified = true;

    LOG_DEBUG("\t-foil-color-primary: %s, 0x%08x\n",
            box->tailor_data->color_prim.specified ? "specified" : "default",
            box->tailor_data->color_prim.argb);

    LOG_DEBUG("\t-foil-color-secondary: %s, 0x%08x\n",
            box->tailor_data->color_seco.specified ? "specified" : "default",
            box->tailor_data->color_seco.argb);
    return 0;
}

static int tailor(struct foil_create_ctxt *ctxt, struct foil_rdrbox *box)
{
    box->tailor_data = calloc(1, sizeof(struct _tailor_data));
    update_properties(ctxt->udom->doc, box);
    update_style_properties(box);
    return 0;
}

static void cleaner(struct foil_rdrbox *box)
{
    assert(box->tailor_data);
    if (box->tailor_data->timer) {
        pcmcth_renderer* rdr = foil_get_renderer();
        foil_timer_delete(rdr, box->tailor_data->timer);
    }

    if (box->tailor_data->marks)
        free(box->tailor_data->marks);
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
            foil_page_set_bgc(ctxt->udom->page, box->tailor_data->color_prim);
            foil_page_erase_rect(ctxt->udom->page, &bar_rc);
        }
    }
    else {
        double bar_ratio = box->tailor_data->value / box->tailor_data->max;
        assert(bar_ratio >= 0 && bar_ratio <= 1.0);
        int bar_width = (int)(tray_width * bar_ratio);

        page_rc.right = page_rc.left + bar_width;
        foil_page_set_bgc(ctxt->udom->page, box->tailor_data->color_prim);
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

struct foil_rdrbox_tailor_ops progress_ops_as_box = {
    .tailor = tailor,
    .cleaner = cleaner,
    .bgnd_painter = bgnd_painter,
    .on_attr_changed = on_attr_changed,
};

static void
ctnt_painter(struct foil_render_ctxt *ctxt, struct foil_rdrbox *box)
{
    foil_rect page_rc;
    foil_rdrbox_map_rect_to_page(&box->ctnt_rect, &page_rc);

    if (foil_rect_is_empty(&page_rc))
        return;

    int tray_width = foil_rect_width(&page_rc);
    int y = page_rc.top + foil_rect_height(&page_rc) / 2;
    if (box->ctrl_type == FOIL_RDRBOX_CTRL_PROGRESS_BAR) {
        foil_page_set_fgc(ctxt->udom->page, box->tailor_data->color_seco);
        foil_page_draw_uchar(ctxt->udom->page, page_rc.left, y,
                box->tailor_data->marks[0],
                tray_width / box->tailor_data->mark_width);

        if (box->tailor_data->value < 0) {
            /* in indeterminate state */

            foil_rect bar_rc = page_rc;
            bar_rc.left += tray_width * box->tailor_data->indicator / 100;
            bar_rc.right = bar_rc.left + tray_width / 10;

            int bar_width = foil_rect_width(&bar_rc);
            if (foil_rect_intersect(&bar_rc, &bar_rc, &page_rc)) {
                foil_page_set_fgc(ctxt->udom->page,
                        box->tailor_data->color_prim);
                foil_page_draw_uchar(ctxt->udom->page, bar_rc.left, y,
                    box->tailor_data->marks[1],
                    bar_width / box->tailor_data->mark_width);
            }
        }
        else {
            double bar_ratio = box->tailor_data->value / box->tailor_data->max;
            assert(bar_ratio >= 0 && bar_ratio <= 1.0);
            int bar_width = (int)(tray_width * bar_ratio);

            if (bar_width > 0) {
                foil_page_set_fgc(ctxt->udom->page,
                        box->tailor_data->color_prim);
                foil_page_draw_uchar(ctxt->udom->page, page_rc.left, y,
                        box->tailor_data->marks[1],
                        bar_width / box->tailor_data->mark_width);
            }
        }
    }
    else {
        double ratio;

        if (box->tailor_data->value < 0) {
            /* in indeterminate state */
            ratio = box->tailor_data->indicator / 100.0;
        }
        else {
            ratio = box->tailor_data->value / box->tailor_data->max;
        }

        int mark_idx;
        mark_idx = (int)((box->tailor_data->nr_marks - 1) * ratio + 0.5);
        assert(mark_idx >= 0 && mark_idx < box->tailor_data->nr_marks);

        foil_page_set_fgc(ctxt->udom->page, box->color);

        int x = page_rc.left + (tray_width - box->tailor_data->mark_width) / 2;
        LOG_DEBUG("value: %f, max: %f; left: %d, tray width: %d, x: %d\n",
                box->tailor_data->value, box->tailor_data->max,
                page_rc.left, tray_width, x);
        foil_page_draw_uchar(ctxt->udom->page, x, y,
                box->tailor_data->marks[mark_idx], 1);
    }
}

static struct foil_rdrbox_tailor_ops progress_ops_as_ctrl = {
    .tailor = tailor,
    .cleaner = cleaner,
    .ctnt_painter = ctnt_painter,
    .on_attr_changed = on_attr_changed,
};

struct foil_rdrbox_tailor_ops *
foil_rdrbox_progress_tailor_ops(struct foil_create_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    uint8_t v = css_computed_appearance(ctxt->style);
    assert(v != CSS_DIRECTION_INHERIT);
    switch (v) {
        case CSS_APPEARANCE_AUTO:
        case CSS_APPEARANCE_PROGRESS_BAR:
        default:
            box->is_control = 1;
            box->ctrl_type = FOIL_RDRBOX_CTRL_PROGRESS_BAR;
            break;

        case CSS_APPEARANCE_PROGRESS_MARK:
            box->is_control = 1;
            box->ctrl_type = FOIL_RDRBOX_CTRL_PROGRESS_MARK;
            break;

        case CSS_APPEARANCE_PROGRESS_BKGND:
            box->is_control = 0;
            break;
    }

    if (box->is_control)
        return &progress_ops_as_ctrl;

    return &progress_ops_as_box;
}

