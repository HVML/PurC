/*
** @file rdrbox-meter.c
** @author Vincent Wei
** @date 2023/01/31
** @brief The implementation of tailored operations for meter box.
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

#define _ISOC99_SOURCE
#include <assert.h>
#include <math.h>

static struct attr_info {
    const char *name;
    double def_value;
} meter_attr_info[] = {
    { "min",     0.0 },
    { "max",     1.0 },
    { "value",   0.0 },
    { "low",     NAN },
    { "high",    NAN },
    { "optimum", NAN },
};

struct _tailor_data {
    double d[0];
    double min;
    double max;
    double value;
    double low;
    double high;
    double optimum;

    foil_color color_info;
    foil_color color_prim;
    foil_color color_warn;
    foil_color color_dang;
    foil_color color_succ;
};

static int
tailor(struct foil_create_ctxt *ctxt, struct foil_rdrbox *box)
{
    const char *value;
    size_t len;

    box->tailor_data = calloc(1, sizeof(struct _tailor_data));

    for (size_t i = 0; i < PCA_TABLESIZE(meter_attr_info); i++) {
        if (pcdoc_element_get_attribute(ctxt->udom->doc, box->owner,
                    meter_attr_info[i].name, &value, &len) == 0 && len > 0) {
            char buff[len + 1];
            strncpy(buff, value, len);
            buff[len] = 0;
            box->tailor_data->d[i] = atof(buff);
        }
        else {
            box->tailor_data->d[i] = meter_attr_info[i].def_value;
        }
    }

    /* normalize the values */
    if (box->tailor_data->min >= box->tailor_data->max) {
        box->tailor_data->min = 0.0;
        box->tailor_data->max = 1.0;
    }

    if (box->tailor_data->value < box->tailor_data->min) {
        box->tailor_data->value = box->tailor_data->min;
    }

    if (box->tailor_data->value > box->tailor_data->max) {
        box->tailor_data->value = box->tailor_data->max;
    }

    if (isnan(box->tailor_data->low)
            || box->tailor_data->low < box->tailor_data->min) {
        box->tailor_data->low = box->tailor_data->min;
    }

    if (isnan(box->tailor_data->high)
            || box->tailor_data->high > box->tailor_data->max) {
        box->tailor_data->high = box->tailor_data->max;
    }

    uint8_t v;
    v = css_computed_foil_color_info(ctxt->style,
            &box->tailor_data->color_info.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_info.specified = false;
    else
        box->tailor_data->color_info.specified = true;

    v = css_computed_foil_color_primary(ctxt->style,
            &box->tailor_data->color_prim.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_prim.specified = false;
    else
        box->tailor_data->color_prim.specified = true;

    v = css_computed_foil_color_warning(ctxt->style,
            &box->tailor_data->color_warn.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_warn.specified = false;
    else
        box->tailor_data->color_warn.specified = true;

    v = css_computed_foil_color_danger(ctxt->style,
            &box->tailor_data->color_dang.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_dang.specified = false;
    else
        box->tailor_data->color_dang.specified = true;

    v = css_computed_foil_color_success(ctxt->style,
            &box->tailor_data->color_succ.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_succ.specified = false;
    else
        box->tailor_data->color_succ.specified = true;

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
    foil_page_set_bgc(ctxt->udom->page, box->background_color);
    foil_page_erase_rect(ctxt->udom->page, &page_rc);

    foil_color bgc = box->tailor_data->color_prim;
    if (isnan(box->tailor_data->optimum)) {
        if (box->tailor_data->value > box->tailor_data->high) {
            bgc = box->tailor_data->color_warn;
        }
        else if (box->tailor_data->value < box->tailor_data->low) {
            bgc = box->tailor_data->color_warn;
        }
    }
    else {
        if (box->tailor_data->optimum < box->tailor_data->low) {
            if (box->tailor_data->value > box->tailor_data->high) {
                bgc = box->tailor_data->color_dang;
            }
            else {
                bgc = box->tailor_data->color_warn;
            }
        }
        else if (box->tailor_data->optimum > box->tailor_data->high) {
            if (box->tailor_data->value < box->tailor_data->low) {
                bgc = box->tailor_data->color_dang;
            }
            else {
                bgc = box->tailor_data->color_warn;
            }
        }
    }

    double bar_ratio = box->tailor_data->value
        / (box->tailor_data->max - box->tailor_data->min);
    int bar_width = (int)(tray_width * bar_ratio);
    page_rc.right = page_rc.left + bar_width;

    foil_page_set_bgc(ctxt->udom->page, bgc);
    foil_page_erase_rect(ctxt->udom->page, &page_rc);
}

struct foil_rdrbox_tailor_ops meter_ops_as_box = {
    .tailor = tailor,
    .cleaner = cleaner,
    .bgnd_painter = bgnd_painter,
};

static void
ctnt_painter(struct foil_render_ctxt *ctxt, struct foil_rdrbox *box)
{
    foil_rect page_rc;
    foil_rdrbox_map_rect_to_page(&box->ctnt_rect, &page_rc);

    if (foil_rect_is_empty(&page_rc))
        return;

    int tray_width = foil_rect_width(&page_rc);
    foil_page_set_bgc(ctxt->udom->page, box->background_color);
    foil_page_erase_rect(ctxt->udom->page, &page_rc);

    foil_color bgc = box->tailor_data->color_prim;
    if (isnan(box->tailor_data->optimum)) {
        if (box->tailor_data->value > box->tailor_data->high) {
            bgc = box->tailor_data->color_warn;
        }
        else if (box->tailor_data->value < box->tailor_data->low) {
            bgc = box->tailor_data->color_warn;
        }
    }
    else {
        if (box->tailor_data->optimum < box->tailor_data->low) {
            if (box->tailor_data->value > box->tailor_data->high) {
                bgc = box->tailor_data->color_dang;
            }
            else {
                bgc = box->tailor_data->color_warn;
            }
        }
        else if (box->tailor_data->optimum > box->tailor_data->high) {
            if (box->tailor_data->value < box->tailor_data->low) {
                bgc = box->tailor_data->color_dang;
            }
            else {
                bgc = box->tailor_data->color_warn;
            }
        }
    }

    double bar_ratio = box->tailor_data->value
        / (box->tailor_data->max - box->tailor_data->min);
    int bar_width = (int)(tray_width * bar_ratio);
    page_rc.right = page_rc.left + bar_width;

    foil_page_set_bgc(ctxt->udom->page, bgc);
    foil_page_erase_rect(ctxt->udom->page, &page_rc);
}

struct foil_rdrbox_tailor_ops meter_ops_as_ctrl = {
    .tailor = tailor,
    .cleaner = cleaner,
    .ctnt_painter = ctnt_painter,
};

struct foil_rdrbox_tailor_ops *
foil_rdrbox_meter_tailor_ops(struct foil_create_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    uint8_t v = css_computed_appearance(ctxt->style);
    assert(v != CSS_APPEARANCE_INHERIT);
    switch (v) {
        case CSS_APPEARANCE_AUTO:
        case CSS_APPEARANCE_METER_BAR:
        default:
            box->is_control = 1;
            box->ctrl_type = FOIL_RDRBOX_CTRL_METER_BAR;
            break;

        case CSS_APPEARANCE_METER_MARK:
            box->is_control = 1;
            box->ctrl_type = FOIL_RDRBOX_CTRL_METER_MARK;
            break;

        case CSS_APPEARANCE_METER_BKGND:
            box->is_control = 0;
            break;
    }

    if (box->is_control)
        return &meter_ops_as_ctrl;
    return &meter_ops_as_box;
}

