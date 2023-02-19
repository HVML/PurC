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

// #undef NDEBUG

#include "rdrbox.h"
#include "rdrbox-internal.h"
#include "udom.h"
#include "page.h"

#define _ISOC99_SOURCE
#include <assert.h>
#include <math.h>

static const char *def_bar_marks = "━━";
static const char *def_mark_marks = " ▁▂▃▄▅▆▇█";

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
    /* XXX: The following two fields must be placed at the head of this struct.
       The candidate marks; */
    int         nr_marks;
    int         nr_wide;
    uint32_t   *marks;

    double d[0];
    double min;
    double max;
    double value;
    double low;
    double high;
    double optimum;

    foil_color color_info;
    foil_color color_prim;
    foil_color color_seco;
    foil_color color_warn;
    foil_color color_dang;
    foil_color color_succ;
};

static void
update_properties(purc_document_t doc, struct foil_rdrbox *box)
{
    const char *value;
    size_t len;

    for (size_t i = 0; i < PCA_TABLESIZE(meter_attr_info); i++) {
        if (pcdoc_element_get_attribute(doc, box->owner,
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
            if (box->ctrl_type == FOIL_RDRBOX_CTRL_METER_MARK) {
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

    v = css_computed_foil_color_info(box->computed_style,
            &box->tailor_data->color_info.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_info.specified = false;
    else
        box->tailor_data->color_info.specified = true;

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

    v = css_computed_foil_color_warning(box->computed_style,
            &box->tailor_data->color_warn.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_warn.specified = false;
    else
        box->tailor_data->color_warn.specified = true;

    v = css_computed_foil_color_danger(box->computed_style,
            &box->tailor_data->color_dang.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_dang.specified = false;
    else
        box->tailor_data->color_dang.specified = true;

    v = css_computed_foil_color_success(box->computed_style,
            &box->tailor_data->color_succ.argb);
    if (v == CSS_COLOR_DEFAULT)
        box->tailor_data->color_succ.specified = false;
    else
        box->tailor_data->color_succ.specified = true;

    return 0;
}

static int
tailor(struct foil_create_ctxt *ctxt, struct foil_rdrbox *box)
{
    box->tailor_data = calloc(1, sizeof(struct _tailor_data));
    update_properties(ctxt->udom->doc, box);
    update_style_properties(box);
    return 0;
}

static void cleaner(struct foil_rdrbox *box)
{
    assert(box->tailor_data);

    if (box->tailor_data->marks)
        free(box->tailor_data->marks);
    free(box->tailor_data);
}

static foil_color get_color(const struct _tailor_data *tailor_data)
{
    foil_color color = tailor_data->color_info;
    if (isnan(tailor_data->optimum)) {
        if (tailor_data->value > tailor_data->high) {
            color = tailor_data->color_warn;
        }
        else if (tailor_data->value < tailor_data->low) {
            color = tailor_data->color_warn;
        }
    }
    else {
        if (tailor_data->optimum < tailor_data->low) {
            if (tailor_data->value > tailor_data->high) {
                color = tailor_data->color_dang;
            }
            else if (tailor_data->value > tailor_data->optimum) {
                color = tailor_data->color_warn;
            }
        }
        else if (tailor_data->optimum > tailor_data->high) {
            if (tailor_data->value < tailor_data->low) {
                color = tailor_data->color_dang;
            }
            else if (tailor_data->value < tailor_data->optimum) {
                color = tailor_data->color_warn;
            }
        }
    }

    return color;
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

    foil_color bgc = get_color(box->tailor_data);

    double bar_ratio = box->tailor_data->value
        / (box->tailor_data->max - box->tailor_data->min);
    int bar_width = (int)(tray_width * bar_ratio);
    page_rc.right = page_rc.left + bar_width;

    foil_page_set_bgc(ctxt->udom->page, bgc);
    foil_page_erase_rect(ctxt->udom->page, &page_rc);
}

static void on_attr_changed(struct foil_update_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    double old_attrs[PCA_TABLESIZE(meter_attr_info)];
    memcpy(&old_attrs, box->tailor_data->d, sizeof(old_attrs));

    update_properties(ctxt->udom->doc, box);
    if (memcmp(old_attrs, box->tailor_data->d, sizeof(old_attrs))) {
        foil_udom_invalidate_rdrbox(ctxt->udom, box);
    }
}

struct foil_rdrbox_tailor_ops meter_ops_as_box = {
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

    foil_color fgc = get_color(box->tailor_data);
    int tray_width = foil_rect_width(&page_rc);
    int y = page_rc.top + foil_rect_height(&page_rc) / 2;

    double ratio = box->tailor_data->value
        / (box->tailor_data->max - box->tailor_data->min);
    assert(ratio >= 0 && ratio <= 1.0);

    if (box->ctrl_type == FOIL_RDRBOX_CTRL_METER_BAR) {
        foil_page_set_fgc(ctxt->udom->page, box->tailor_data->color_seco);
        foil_page_draw_uchar(ctxt->udom->page, page_rc.left, y,
                box->tailor_data->marks[0],
                box->tailor_data->nr_wide ? tray_width / 2 : tray_width);

        int bar_width = (int)(tray_width * ratio + 0.5);
        LOG_DEBUG("tray width: %d, ratio: %f, bar width: %d\n",
                tray_width, ratio, bar_width);
        if (bar_width > 0) {
            foil_page_set_fgc(ctxt->udom->page, fgc);
            foil_page_draw_uchar(ctxt->udom->page, page_rc.left, y,
                    box->tailor_data->marks[1],
                    box->tailor_data->nr_wide ? bar_width / 2 : bar_width);
        }
    }
    else {
        int mark_idx;
        mark_idx = (int)((box->tailor_data->nr_marks - 1) * ratio + 0.5);
        LOG_DEBUG("index of mark: %d; nr_marks: %d\n",
                mark_idx, box->tailor_data->nr_marks);
        assert(mark_idx >= 0 && mark_idx < box->tailor_data->nr_marks);

        foil_page_set_fgc(ctxt->udom->page, fgc);

        int x = page_rc.left + tray_width / 2;
        foil_page_draw_uchar(ctxt->udom->page, x, y,
                box->tailor_data->marks[mark_idx], 1);
    }
}

struct foil_rdrbox_tailor_ops meter_ops_as_ctrl = {
    .tailor = tailor,
    .cleaner = cleaner,
    .ctnt_painter = ctnt_painter,
    .on_attr_changed = on_attr_changed,
};

struct foil_rdrbox_tailor_ops *
foil_rdrbox_meter_tailor_ops(struct foil_create_ctxt *ctxt,
        struct foil_rdrbox *box)
{
    uint8_t v = css_computed_appearance(ctxt->style);
    assert(v != CSS_APPEARANCE_INHERIT);
    switch (v) {
        case CSS_APPEARANCE_AUTO:
        case CSS_APPEARANCE_METER:
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

