/*
** @file rdrbox-layout.c
** @author Vincent Wei
** @date 2022/11/23
** @brief The implementation of layout of rendering box.
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

static float normalize_used_length(foil_layout_ctxt *ctxt, foil_rdrbox *box,
        css_unit unit, css_fixed length)
{
    float v = 0;

    switch (unit) {
    case CSS_UNIT_PCT:
        v = foil_rect_width(&box->cblock_creator->ctnt_rect);
        v = v * FIXTOFLT(length);
        break;

    case CSS_UNIT_PX:
        v = FIXTOFLT(length);
        break;

    /* font-relative lengths */
    case CSS_UNIT_EX:
        // The x-height is so called because it is often
        // equal to the height of the lowercase "x".
        v = FIXTOFLT(length) * FOIL_PX_GRID_CELL_W;
        break;

    case CSS_UNIT_EM:
    case CSS_UNIT_CH:
    case CSS_UNIT_REM:
        // Equal to the used advance measure of the "0" glyph
        v = FIXTOFLT(length) * FOIL_PX_GRID_CELL_H;
        break;

    /* absolute lengths */
    case CSS_UNIT_CM:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/2.54;
        break;
    case CSS_UNIT_IN:
        v = FIXTOFLT(length) * FOIL_DEF_DPI;
        break;
    case CSS_UNIT_MM:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/2.54/10;
        break;
    case CSS_UNIT_PC:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/6.0;
        break;
    case CSS_UNIT_PT:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/72.0;
        break;
    case CSS_UNIT_Q:
        v = FIXTOFLT(length) * FOIL_DEF_DPI/2.54/40;
        break;

    /* viewport-relative lengths */
    case CSS_UNIT_VW:
        v = FIXTOFLT(length) * ctxt->udom->vw / 100;
        break;
    case CSS_UNIT_VH:
        v = FIXTOFLT(length) * ctxt->udom->vh / 100;
        break;
    case CSS_UNIT_VMAX:
        if (ctxt->udom->vh > ctxt->udom->vw)
            v = FIXTOFLT(length) * ctxt->udom->vh / 100;
        else
            v = FIXTOFLT(length) * ctxt->udom->vw / 100;
        break;
    case CSS_UNIT_VMIN:
        if (ctxt->udom->vh > ctxt->udom->vw)
            v = FIXTOFLT(length) * ctxt->udom->vw / 100;
        else
            v = FIXTOFLT(length) * ctxt->udom->vh / 100;
        break;

    default:
        // TODO: support more unit
        LOG_WARN("TODO: not supported unit: %d\n", unit);
        break;
    }

    return v;
}

static int round_width(float w)
{
    if (w > 0)
        return (int)(w / FOIL_PX_GRID_CELL_W + 0.5) * FOIL_PX_GRID_CELL_W;

    return (int)(w / FOIL_PX_GRID_CELL_W - 0.5) * FOIL_PX_GRID_CELL_W;
}

static int calc_used_value_width(foil_layout_ctxt *ctxt, foil_rdrbox *box,
        css_unit unit, css_fixed length)
{
    return round_width(normalize_used_length(ctxt, box, unit, length));
}

static int round_height(float h)
{
    if (h > 0)
        return (int)(h / FOIL_PX_GRID_CELL_H + 0.5) * FOIL_PX_GRID_CELL_H;
    return (int)(h / FOIL_PX_GRID_CELL_H - 0.5) * FOIL_PX_GRID_CELL_H;
}

static int calc_used_value_height(foil_layout_ctxt *ctxt, foil_rdrbox *box,
        css_unit unit, css_fixed length)
{
    return round_height(normalize_used_length(ctxt, box, unit, length));
}

static void
dtrm_margin_left_right(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    uint8_t value;
    css_fixed length;
    css_unit unit;
    value = css_computed_margin_left(box->computed_style, &length, &unit);
    assert(value != CSS_MARGIN_INHERIT);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->ml = 0;
            break;
        case CSS_MARGIN_SET:
            box->ml = calc_used_value_width(ctxt, box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }

    value = css_computed_margin_right(box->computed_style, &length, &unit);
    assert(value != CSS_MARGIN_INHERIT);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mr = 0;
            break;
        case CSS_MARGIN_SET:
            box->mr = calc_used_value_width(ctxt, box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }
}

static void
dtrm_margin_top_bottom(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    uint8_t value;
    css_fixed length;
    css_unit unit;

    value = css_computed_margin_top(box->computed_style, &length, &unit);
    assert(value != CSS_MARGIN_INHERIT);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mt = 0;
            break;
        case CSS_MARGIN_SET:
            box->mr = calc_used_value_height(ctxt, box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }

    value = css_computed_margin_bottom(box->computed_style, &length, &unit);
    assert(value != CSS_MARGIN_INHERIT);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mb = 0;
            break;
        case CSS_MARGIN_SET:
            box->mb = calc_used_value_height(ctxt, box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }
}

static int
get_intrinsic_width(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    int l = 0;
    const char *value;
    size_t len;

    if (pcdoc_element_get_attribute(ctxt->udom->doc, box->owner,
            "width", &value, &len) == 0) {

        char *v = strndup(value, len);
        l = strtol(v, NULL, 10);
        free(v);
    }

    return (int)l;
}

static int
get_intrinsic_height(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    int l = 0;
    const char *value;
    size_t len;

    if (pcdoc_element_get_attribute(ctxt->udom->doc, box->owner,
            "height", &value, &len) == 0) {

        char *v = strndup(value, len);
        l = strtol(v, NULL, 10);
        free(v);
    }

    return (int)l;
}

static int
get_intrinsic_ratio(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;

    return 2.0f; // always assume the instrinsic ratio is 2:1
}

static uint8_t
real_computed_width(foil_rdrbox *box, css_fixed *l, css_unit *u)
{
    uint8_t v = CSS_WIDTH_AUTO;
    switch (box->prop_for_width) {
    case FOIL_RDRBOX_USE_WIDTH:
        v = css_computed_width(box->computed_style, l, u);
        assert(v != CSS_WIDTH_INHERIT);
        break;

    case FOIL_RDRBOX_USE_MAX_WIDTH:
        assert(box->max_width > 0);
        v = CSS_WIDTH_SET;
        *l = INTTOFIX(box->max_width);
        *u = CSS_UNIT_PX;
        break;

    case FOIL_RDRBOX_USE_MIN_WIDTH:
        assert(box->min_width >= 0);
        v = CSS_WIDTH_SET;
        *l = INTTOFIX(box->min_width);
        *u = CSS_UNIT_PX;
        break;

    default:
        assert(0);
        break;
    }

    return v;
}

static uint8_t
real_computed_height(foil_rdrbox *box, css_fixed *l, css_unit *u)
{
    uint8_t v = CSS_HEIGHT_AUTO;
    switch (box->prop_for_height) {
    case FOIL_RDRBOX_USE_HEIGHT:
        v = css_computed_height(box->computed_style, l, u);
        assert(v != CSS_HEIGHT_INHERIT);
        break;

    case FOIL_RDRBOX_USE_MAX_HEIGHT:
        assert(box->max_height > 0);
        v = CSS_HEIGHT_SET;
        *l = INTTOFIX(box->max_height);
        *u = CSS_UNIT_PX;
        break;

    case FOIL_RDRBOX_USE_MIN_HEIGHT:
        assert(box->min_height >= 0);
        v = CSS_HEIGHT_SET;
        *l = INTTOFIX(box->min_height);
        *u = CSS_UNIT_PX;
        break;

    default:
        assert(0);
        break;
    }

    return v;
}

static uint8_t
dtrm_width_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v, height_v;
    css_fixed width_l, height_l;
    css_unit width_u, height_u;

    assert(box->is_replaced);

    width_v = real_computed_width(box, &width_l, &width_u);
    if (width_v != CSS_WIDTH_AUTO) {
        box->width = calc_used_value_width(ctxt, box, width_u, width_l);
        return width_v;
    }

    height_v = real_computed_height(box, &height_l, &height_u);
    assert(height_v != CSS_HEIGHT_INHERIT);

    int intrinsic_width = get_intrinsic_width(ctxt, box);
    int intrinsic_height = get_intrinsic_height(ctxt, box);
    float intrinsic_ratio = get_intrinsic_ratio(ctxt, box);

    if (width_v == CSS_WIDTH_AUTO && height_v == CSS_HEIGHT_AUTO
            && intrinsic_width > 0) {
        box->width = round_width(intrinsic_width);
    }
    else if (width_v == CSS_WIDTH_AUTO && height_v == CSS_HEIGHT_AUTO
            && intrinsic_height > 0) {
        box->width = round_width(intrinsic_height * intrinsic_ratio);
    }
    else if (width_v == CSS_WIDTH_AUTO && height_v != CSS_HEIGHT_AUTO) {
        int height = 0;
        height = calc_used_value_height(ctxt, box, height_u, height_l);
        box->width = round_width(height * intrinsic_ratio);
    }
    else if (width_v == CSS_WIDTH_AUTO && height_v == CSS_HEIGHT_AUTO
            && intrinsic_ratio > 0) {
        LOG_WARN("`width` is undefined\n");
    }
    else if (width_v == CSS_WIDTH_AUTO && intrinsic_width > 0) {
        box->width = round_width(intrinsic_width);
    }
    else if (width_v == CSS_WIDTH_AUTO) {
        box->width = FOIL_PX_REPLACED_W;
    }

    return width_v;
}

static uint8_t
dtrm_height_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v, height_v;
    css_fixed width_l, height_l;
    css_unit width_u, height_u;

    assert(box->is_replaced);

    height_v = real_computed_height(box, &height_l, &height_u);
    assert(height_v != CSS_HEIGHT_INHERIT);
    if (height_v != CSS_WIDTH_AUTO) {
        box->height = calc_used_value_height(ctxt, box, height_u, height_l);

        return height_v;
    }

    width_v = real_computed_width(box, &width_l, &width_u);

    int intrinsic_height = get_intrinsic_height(ctxt, box);
    float intrinsic_ratio = get_intrinsic_ratio(ctxt, box);

    if (width_v == CSS_WIDTH_AUTO && height_v == CSS_HEIGHT_AUTO
            && intrinsic_height > 0) {
        box->height = round_height(intrinsic_height);
    }
    else if (height_v == CSS_HEIGHT_AUTO && intrinsic_ratio > 0) {
        box->height = round_height(box->width / intrinsic_ratio);
    }
    else if (height_v == CSS_HEIGHT_AUTO && intrinsic_height > 0) {
        box->height = round_height(intrinsic_height);
    }
    else if (height_v == CSS_WIDTH_AUTO) {
        box->height = FOIL_PX_REPLACED_H;
    }

    return width_v;
}

static int
dtrm_width_shrink_to_fit(foil_layout_ctxt *ctxt, foil_rdrbox *box);

static void
dtrm_margin_left_right_block_normal(foil_layout_ctxt *ctxt,
        foil_rdrbox *box, uint8_t width_v)
{
    (void)ctxt;
    int nr_autos = 0;
    int cblock_width = foil_rect_width(&box->cblock_creator->ctnt_rect);

    if (width_v == CSS_WIDTH_AUTO)
        nr_autos++;

    uint8_t margin_left_v;
    css_fixed margin_left_l;
    css_unit margin_left_u;
    margin_left_v = css_computed_margin_left(box->computed_style,
            &margin_left_l, &margin_left_u);
    assert(margin_left_v != CSS_MARGIN_INHERIT);

    if (margin_left_v == CSS_MARGIN_AUTO) {
        nr_autos++;
    }
    else {
        box->ml = calc_used_value_width(ctxt, box, margin_left_u, margin_left_l);
    }

    uint8_t margin_right_v;
    css_fixed margin_right_l;
    css_unit margin_right_u;
    margin_right_v = css_computed_margin_right(box->computed_style,
            &margin_right_l, &margin_right_u);
    assert(margin_right_v != CSS_MARGIN_INHERIT);
    if (margin_right_v == CSS_MARGIN_AUTO) {
        nr_autos++;
    }
    else {
        box->mr = calc_used_value_width(ctxt, box, margin_right_u, margin_right_l);
    }

    if (width_v != CSS_WIDTH_AUTO) {
        int tmp = box->ml + box->bl + box->pl + box->width +
            box->pr + box->br + box->mr;
        if (tmp > cblock_width) {
            if (margin_left_v == CSS_MARGIN_AUTO) {
                box->ml = 0;
            }

            if (margin_right_v == CSS_MARGIN_AUTO) {
                box->mr = 0;
            }
        }
    }

    if (nr_autos == 0) {
        if (box->width < 0) {
            LOG_WARN("Computed width is negative: %d\n", box->width);
            box->width = 0;
        }

        if (box->cblock_creator->direction == FOIL_RDRBOX_DIRECTION_LTR) {
            box->mr = cblock_width - box->width - box->ml -
                box->pl - box->bl - box->pr - box->br;
        }
        else {
            box->ml = cblock_width - box->width -
                box->pl - box->bl - box->pr - box->br - box->mr;
        }
    }
    else if (nr_autos == 1) {
        if (width_v == CSS_WIDTH_AUTO)
            box->width = cblock_width -
                box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
        else if (margin_left_v == CSS_MARGIN_AUTO)
            box->ml = cblock_width - box->width -
                box->bl - box->pl - box->pr - box->br - box->mr;
        else if (margin_right_v == CSS_MARGIN_AUTO)
            box->mr = cblock_width - box->width -
                box->bl - box->pl - box->pr - box->br - box->ml;
    }

    if (width_v == CSS_WIDTH_AUTO) {
        if (margin_left_v == CSS_MARGIN_AUTO)
            box->ml = 0;

        if (margin_right_v == CSS_MARGIN_AUTO)
            box->mr = 0;

        box->width = cblock_width -
            box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
    }

    if (margin_left_v == CSS_MARGIN_AUTO &&
            margin_right_v == CSS_MARGIN_AUTO) {
        int margins = cblock_width -
                box->width - box->bl - box->pl - box->pr - box->br;
        box->ml = margins >> 1;
        box->ml = round_width(box->ml);
        box->mr = margins - box->ml;
    }
}

static void
dtrm_width_abspos_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t left_v, right_v, margin_left_v, margin_right_v;
    css_fixed left_l, right_l, margin_left_l, margin_right_l;
    css_unit left_u, right_u, margin_left_u, margin_right_u;

    left_v = css_computed_left(box->computed_style, &left_l, &left_u);
    assert(left_v != CSS_LEFT_INHERIT);

    right_v = css_computed_right(box->computed_style, &right_l, &right_u);
    assert(right_v != CSS_RIGHT_INHERIT);

    if (left_v == CSS_LEFT_AUTO && right_v == CSS_RIGHT_AUTO) {
        if (box->cblock_creator->direction == FOIL_RDRBOX_DIRECTION_LTR) {
            box->left = 0;      // TODO: use the static position.
        }
        else {
            box->right = 0;     // TODO: use the static position.
        }
    }
    else {
        if (left_v == CSS_LEFT_SET) {
            box->left = round_width(normalize_used_length(ctxt, box,
                        left_u, left_l));
        }

        if (right_v == CSS_RIGHT_SET) {
            box->right = round_width(normalize_used_length(ctxt, box,
                        right_u, right_l));
        }
    }

    margin_left_v = css_computed_margin_left(box->computed_style,
            &margin_left_l, &margin_left_u);
    assert(margin_left_v != CSS_MARGIN_INHERIT);

    margin_right_v = css_computed_margin_right(box->computed_style,
            &margin_right_l, &margin_right_u);
    assert(margin_right_v != CSS_MARGIN_INHERIT);

    if (margin_left_v != CSS_MARGIN_AUTO) {
        box->ml = round_width(normalize_used_length(ctxt, box,
                    margin_left_u, margin_left_l));
    }
    if (margin_right_v != CSS_MARGIN_AUTO) {
        box->mr = round_width(normalize_used_length(ctxt, box,
                    margin_right_u, margin_right_l));
    }

    int cblock_width = foil_rect_width(&box->cblock_creator->ctnt_rect);
    if (left_v == CSS_LEFT_AUTO || right_v == CSS_RIGHT_AUTO) {
        if (margin_left_v == CSS_MARGIN_AUTO) {
            box->ml = 0;
            margin_left_v = CSS_MARGIN_SET;
        }
        if (margin_right_v == CSS_MARGIN_AUTO) {
            box->mr = 0;
            margin_right_v = CSS_MARGIN_SET;
        }
    }

    if (margin_left_v == CSS_MARGIN_AUTO &&
            margin_right_v == CSS_MARGIN_AUTO) {

        int margin = (cblock_width - box->left - box->bl - box->pl -
                box->width - box->pr - box->br - box->right) / 2;

        if (margin >= 0)
            box->ml = box->mr = round_width(margin);
        else {
            if (box->cblock_creator->direction ==
                    FOIL_RDRBOX_DIRECTION_LTR) {
                box->ml = 0;
                box->mr = cblock_width - box->left - box->bl - box->pl -
                    box->width - box->pr - box->br - box->right;
            }
            else {
                box->mr = 0;
                box->ml = cblock_width - box->left - box->bl - box->pl -
                    box->width - box->pr - box->br - box->right;
            }
        }

        margin_left_v = CSS_MARGIN_SET;
        margin_right_v = CSS_MARGIN_SET;
    }

    if (margin_left_v == CSS_MARGIN_AUTO) {
        box->ml = cblock_width - box->left - box->bl - box->pl -
            box->width - box->pr - box->br - box->mr - box->right;
    }
    else if (margin_right_v == CSS_MARGIN_AUTO) {
        box->mr = cblock_width - box->left - box->ml - box->bl - box->pl -
            box->width - box->pr - box->br - box->right;
    }
    else if (left_v == CSS_LEFT_SET && right_v == CSS_RIGHT_SET) {
        if (box->cblock_creator->direction ==
                FOIL_RDRBOX_DIRECTION_LTR) {
            box->left = cblock_width - box->ml - box->bl - box->pl -
                box->width - box->pr - box->br - box->mr - box->right;
        }
        else {
            box->right = cblock_width - box->left -
                box->ml - box->bl - box->pl - box->width -
                box->pr - box->br - box->mr;
        }
    }

}

static void
dtrm_width_abspos_non_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t left_v, width_v, right_v, margin_left_v, margin_right_v;
    css_fixed left_l, width_l, right_l, margin_left_l, margin_right_l;
    css_unit left_u, width_u, right_u, margin_left_u, margin_right_u;

    left_v = css_computed_left(box->computed_style, &left_l, &left_u);
    assert(left_v != CSS_LEFT_INHERIT);

    width_v = real_computed_width(box, &width_l, &width_u);
    assert(width_v != CSS_WIDTH_INHERIT);

    right_v = css_computed_right(box->computed_style, &right_l, &right_u);
    assert(right_v != CSS_RIGHT_INHERIT);

    margin_left_v = css_computed_margin_left(box->computed_style,
            &margin_left_l, &margin_left_u);
    assert(margin_left_v != CSS_MARGIN_INHERIT);

    margin_right_v = css_computed_margin_right(box->computed_style,
            &margin_right_l, &margin_right_u);
    assert(margin_right_v != CSS_MARGIN_INHERIT);

    int cblock_width = foil_rect_width(&box->cblock_creator->ctnt_rect);

    if (left_v == CSS_LEFT_AUTO && width_v == CSS_WIDTH_AUTO &&
            right_v == CSS_RIGHT_AUTO) {
        if (margin_left_v == CSS_MARGIN_AUTO)
            box->ml = 0;
        else
            box->ml = round_width(normalize_used_length(ctxt, box,
                    margin_left_u, margin_left_l));
        if (margin_right_v == CSS_MARGIN_AUTO)
            box->mr = 0;
        else
            box->mr = round_width(normalize_used_length(ctxt, box,
                    margin_right_u, margin_right_l));

        if (box->cblock_creator->direction ==
                FOIL_RDRBOX_DIRECTION_LTR) {
            box->left = 0;      // TODO: use the static position.
            box->width = dtrm_width_shrink_to_fit(ctxt, box);
            box->right = cblock_width - box->left -
                    box->ml - box->bl - box->pl -
                    box->width - box->pr - box->br - box->mr;
        }
        else {
            box->right = 0;     // TODO: use the static position.
            box->width = dtrm_width_shrink_to_fit(ctxt, box);
            box->left = cblock_width - box->right -
                    box->ml - box->bl - box->pl -
                    box->width - box->pr - box->br - box->mr;
        }
    }
    else if (left_v == CSS_LEFT_SET && width_v == CSS_WIDTH_SET &&
            right_v == CSS_RIGHT_SET) {
        box->left = round_width(normalize_used_length(ctxt, box,
                    left_u, left_l));
        box->width = round_width(normalize_used_length(ctxt, box,
                    width_u, width_l));
        box->right = round_width(normalize_used_length(ctxt, box,
                    right_u, right_l));

        if (margin_left_v == CSS_MARGIN_AUTO &&
                margin_right_v == CSS_MARGIN_AUTO) {
            int margin = (cblock_width - box->left - box->bl - box->pl -
                box->width - box->pr - box->br - box->right) / 2;
            if (margin >= 0)
                box->ml = box->mr = round_width(margin);
            else {
                if (box->cblock_creator->direction ==
                        FOIL_RDRBOX_DIRECTION_LTR) {
                    box->ml = 0;
                    box->mr = cblock_width - box->left - box->bl - box->pl -
                        box->width - box->pr - box->br - box->right;
                }
                else {
                    box->mr = 0;
                    box->ml = cblock_width - box->left - box->bl - box->pl -
                        box->width - box->pr - box->br - box->right;
                }
            }
        }
        else if (margin_left_v == CSS_MARGIN_AUTO) {
            box->mr = round_width(normalize_used_length(ctxt, box,
                        margin_right_u, margin_right_l));
            box->ml = cblock_width - box->left - box->bl - box->pl -
                box->width - box->pr - box->br - box->right - box->mr;
        }
        else if (margin_right_v == CSS_MARGIN_AUTO) {
            box->ml = round_width(normalize_used_length(ctxt, box,
                        margin_left_u, margin_left_l));
            box->mr = cblock_width - box->left - box->bl - box->pl -
                box->width - box->pr - box->br - box->right - box->ml;
        }
        else {
            box->ml = round_width(normalize_used_length(ctxt, box,
                        margin_left_u, margin_left_l));
            box->mr = round_width(normalize_used_length(ctxt, box,
                        margin_right_u, margin_right_l));
            if (box->cblock_creator->direction ==
                    FOIL_RDRBOX_DIRECTION_LTR) {
                box->left = cblock_width - box->ml - box->bl - box->pl -
                    box->width - box->pr - box->br - box->mr - box->right;
            }
            else {
                box->right = cblock_width - box->left -
                    box->ml - box->bl - box->pl -
                    box->width - box->pr - box->br - box->mr;
            }
        }
    }
    else {
        if (margin_left_v == CSS_MARGIN_AUTO)
            box->ml = 0;
        else
            box->ml = round_width(normalize_used_length(ctxt, box,
                    margin_left_u, margin_left_l));
        if (margin_right_v == CSS_MARGIN_AUTO)
            box->mr = 0;
        else
            box->mr = round_width(normalize_used_length(ctxt, box,
                    margin_right_u, margin_right_l));

        if (left_v == CSS_MARGIN_AUTO && width_v == CSS_WIDTH_AUTO &&
                right_v != CSS_MARGIN_AUTO) {
            box->right = round_width(normalize_used_length(ctxt, box,
                        right_u, right_l));
            box->width = dtrm_width_shrink_to_fit(ctxt, box);
            box->left = cblock_width - box->right -
                    box->ml - box->bl - box->pl -
                    box->width - box->pr - box->br - box->mr;
        }
        else if (left_v == CSS_MARGIN_AUTO && width_v != CSS_WIDTH_AUTO &&
                right_v == CSS_MARGIN_AUTO) {
            box->width = round_width(normalize_used_length(ctxt, box,
                        width_u, width_l));
            if (box->cblock_creator->direction == FOIL_RDRBOX_DIRECTION_LTR) {
                box->left = 0;      // TODO: the static position.
                box->right = cblock_width - box->width -
                    box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
            }
            else {
                box->right = 0;      // TODO: the static position.
                box->left = cblock_width - box->width -
                    box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
            }
        }
        else if (left_v != CSS_MARGIN_AUTO && width_v == CSS_WIDTH_AUTO &&
                right_v == CSS_MARGIN_AUTO) {
            box->left = round_width(normalize_used_length(ctxt, box,
                        left_u, left_l));
            box->width = dtrm_width_shrink_to_fit(ctxt, box);
            box->right = cblock_width - box->left -
                    box->ml - box->bl - box->pl -
                    box->width - box->pr - box->br - box->mr;
        }
        else if (left_v == CSS_MARGIN_AUTO && width_v != CSS_WIDTH_AUTO &&
                right_v != CSS_MARGIN_AUTO) {
            box->width = round_width(normalize_used_length(ctxt, box,
                        width_u, width_l));
            box->right = round_width(normalize_used_length(ctxt, box,
                        right_u, right_l));
            box->left = cblock_width - box->right - box->width -
                box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
        }
        else if (left_v != CSS_MARGIN_AUTO && width_v == CSS_WIDTH_AUTO &&
                right_v != CSS_MARGIN_AUTO) {
            box->left = round_width(normalize_used_length(ctxt, box,
                        left_u, left_l));
            box->right = round_width(normalize_used_length(ctxt, box,
                        right_u, right_l));
            box->width = cblock_width - box->left - box->right -
                box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
        }
        else if (left_v != CSS_MARGIN_AUTO && width_v != CSS_WIDTH_AUTO &&
                right_v == CSS_MARGIN_AUTO) {
            box->left = round_width(normalize_used_length(ctxt, box,
                        left_u, left_l));
            box->width = round_width(normalize_used_length(ctxt, box,
                        width_u, width_l));
            box->right = cblock_width - box->width - box->left -
                box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
        }
        else {
            assert(0); // never reach here
        }
    }
}

static void
dtrm_left_right_relpos(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t left_v, right_v;
    css_fixed left_l, right_l;
    css_unit left_u, right_u;

    left_v = css_computed_left(box->computed_style, &left_l, &left_u);
    assert(left_v != CSS_LEFT_INHERIT);

    right_v = css_computed_right(box->computed_style, &right_l, &right_u);
    assert(right_v != CSS_RIGHT_INHERIT);
    if (left_v == CSS_LEFT_AUTO && right_v == CSS_RIGHT_AUTO) {
        box->left = box->right = 0;
    }
    else if (left_v == CSS_LEFT_AUTO) {
        box->right = round_width(normalize_used_length(ctxt, box,
                    right_u, right_l));
        box->left = -box->right;
    }
    else if (right_v == CSS_RIGHT_AUTO) {
        box->left = round_width(normalize_used_length(ctxt, box,
                    left_u, left_l));
        box->right = -box->left;
    }
    else {
        if (box->cblock_creator->direction == FOIL_RDRBOX_DIRECTION_LTR) {
            box->left = round_width(normalize_used_length(ctxt, box,
                        left_u, left_l));
            box->right = -box->left;
        }
        else {
            box->right = round_width(normalize_used_length(ctxt, box,
                        right_u, right_l));
            box->left = -box->right;
        }
    }
}

/* calculate widths and margins according to CSS 2.2 Section 10.3 */
static void
calc_width_margins(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
        if (box->is_replaced) {
            dtrm_width_replaced(ctxt, box);
        }
        else {
            box->width = 0; // not apply
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else if (box->is_block_level && box->is_in_normal_flow) {
        uint8_t width_v;

        if (box->is_replaced) {
            width_v = dtrm_width_replaced(ctxt, box);
        }
        else {
            css_fixed width_l;
            css_unit width_u;
            width_v = real_computed_width(box, &width_l, &width_u);
            assert(width_v != CSS_WIDTH_INHERIT);
            if (width_v == CSS_WIDTH_AUTO) {
                box->width = 0;
            }
            else {
                box->width = calc_used_value_width(ctxt, box,
                        width_u, width_l);
            }
        }

        dtrm_margin_left_right_block_normal(ctxt, box, width_v);
    }
    else if (box->floating != FOIL_RDRBOX_FLOAT_NONE) {
        if (box->is_replaced) {
            dtrm_width_replaced(ctxt, box);
        }
        else {
            box->width = dtrm_width_shrink_to_fit(ctxt, box);
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else if (box->is_abs_positioned) {
        if (box->is_replaced) {
            dtrm_width_replaced(ctxt, box);
            dtrm_width_abspos_replaced(ctxt, box);
        }
        else {
            dtrm_width_abspos_non_replaced(ctxt, box);
        }
    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK &&
            box->is_in_normal_flow) {
        if (box->is_replaced) {
            dtrm_width_replaced(ctxt, box);
        }
        else {
            box->width = dtrm_width_shrink_to_fit(ctxt, box);
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else {
        LOG_ERROR("Should not be here\n");
        assert(0);
    }

    /* determine left and right according to CSS 2.2 Section 9.4.3 */
    if (box->position == FOIL_RDRBOX_POSITION_RELATIVE) {
        dtrm_left_right_relpos(ctxt, box);
    }
}

static void
dtrm_top_bottom_relpos(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t top_v, bottom_v;
    css_fixed top_l, bottom_l;
    css_unit top_u, bottom_u;

    top_v = css_computed_top(box->computed_style, &top_l, &top_u);
    assert(top_v != CSS_LEFT_INHERIT);

    bottom_v = css_computed_bottom(box->computed_style, &bottom_l, &bottom_u);
    assert(bottom_v != CSS_RIGHT_INHERIT);
    if (top_v == CSS_LEFT_AUTO && bottom_v == CSS_RIGHT_AUTO) {
        box->top = box->bottom = 0;
    }
    else if (top_v == CSS_LEFT_AUTO) {
        box->bottom = round_width(normalize_used_length(ctxt, box,
                    bottom_u, bottom_l));
        box->top = -box->bottom;
    }
    else if (bottom_v == CSS_RIGHT_AUTO) {
        box->top = round_width(normalize_used_length(ctxt, box,
                    top_u, top_l));
        box->bottom = -box->top;
    }
    else {
        box->top = round_width(normalize_used_length(ctxt, box,
                    top_u, top_l));
        box->bottom = -box->top;
    }
}

static void
dtrm_heights_abspos_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t top_v, bottom_v, margin_top_v, margin_bottom_v;
    css_fixed top_l, bottom_l, margin_top_l, margin_bottom_l;
    css_unit top_u, bottom_u, margin_top_u, margin_bottom_u;

    top_v = css_computed_top(box->computed_style, &top_l, &top_u);
    assert(top_v != CSS_TOP_INHERIT);

    bottom_v = css_computed_bottom(box->computed_style, &bottom_l, &bottom_u);
    assert(bottom_v != CSS_BOTTOM_INHERIT);

    if (top_v == CSS_TOP_AUTO && bottom_v == CSS_BOTTOM_AUTO) {
        box->top = 0;      // TODO: the static position.
    }
    else if (top_v == CSS_TOP_SET) {
        box->top = round_height(normalize_used_length(ctxt, box,
                    top_u, top_l));
    }
    else if (bottom_v == CSS_BOTTOM_SET) {
        box->bottom = round_height(normalize_used_length(ctxt, box,
                    bottom_u, bottom_l));
    }

    margin_top_v = css_computed_margin_top(box->computed_style,
            &margin_top_l, &margin_top_u);
    assert(margin_top_v != CSS_MARGIN_INHERIT);

    margin_bottom_v = css_computed_margin_bottom(box->computed_style,
            &margin_bottom_l, &margin_bottom_u);
    assert(margin_bottom_v != CSS_MARGIN_INHERIT);

    if (margin_top_v != CSS_MARGIN_AUTO) {
        box->mt = round_height(normalize_used_length(ctxt, box,
                    margin_top_u, margin_top_l));
    }
    if (margin_bottom_v != CSS_MARGIN_AUTO) {
        box->mb = round_height(normalize_used_length(ctxt, box,
                    margin_bottom_u, margin_bottom_l));
    }

    int cblock_height = foil_rect_height(&box->cblock_creator->ctnt_rect);
    if (bottom_v == CSS_BOTTOM_AUTO) {
        if (margin_top_v == CSS_MARGIN_AUTO) {
            box->mt = 0;
            margin_top_v = CSS_MARGIN_SET;
        }
        if (margin_bottom_v != CSS_MARGIN_AUTO) {
            box->mb = 0;
            margin_bottom_v = CSS_MARGIN_SET;
        }
    }

    if (margin_top_v == CSS_MARGIN_AUTO &&
            margin_bottom_v == CSS_MARGIN_AUTO) {
        int margin = (cblock_height - box->top - box->bt - box->pt -
            box->height - box->pb - box->bb - box->bottom) / 2;
        box->mt = box->mb = round_height(margin);
        margin_top_v = CSS_MARGIN_SET;
        margin_bottom_v = CSS_MARGIN_SET;
    }

    if (margin_top_v == CSS_MARGIN_AUTO) {
        assert(margin_bottom_v == CSS_MARGIN_SET);
        box->mt = cblock_height - box->top - box->bt - box->pt -
            box->height - box->pb - box->bb - box->mb - box->bottom;
        margin_top_v = CSS_MARGIN_SET;
    }
    else if (margin_bottom_v == CSS_MARGIN_AUTO) {
        assert(margin_top_v == CSS_MARGIN_SET);
        box->mb = cblock_height - box->top - box->mt - box->bt - box->pt -
            box->height - box->pb - box->bb - box->bottom;
        margin_bottom_v = CSS_MARGIN_SET;
    }
    else if (top_v == CSS_TOP_SET && bottom_v == CSS_BOTTOM_SET) {
        box->bottom = cblock_height - box->top - box->height -
            box->mt - box->bt - box->pt - box->pb - box->bb - box->mb;
    }
}

static int
calc_height_for_visible_non_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box);

static int
calc_height_for_block_fmt_ctxt_maker(foil_layout_ctxt *ctxt, foil_rdrbox *box);

static void
dtrm_heights_abspos_non_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t top_v, height_v, bottom_v, margin_top_v, margin_bottom_v;
    css_fixed top_l, height_l, bottom_l, margin_top_l, margin_bottom_l;
    css_unit top_u, height_u, bottom_u, margin_top_u, margin_bottom_u;

    top_v = css_computed_top(box->computed_style, &top_l, &top_u);
    assert(top_v != CSS_TOP_INHERIT);

    height_v = real_computed_height(box, &height_l, &height_u);
    assert(height_v != CSS_HEIGHT_INHERIT);

    bottom_v = css_computed_bottom(box->computed_style, &bottom_l, &bottom_u);
    assert(bottom_v != CSS_BOTTOM_INHERIT);

    margin_top_v = css_computed_margin_top(box->computed_style,
            &margin_top_l, &margin_top_u);
    assert(margin_top_v != CSS_MARGIN_INHERIT);

    margin_bottom_v = css_computed_margin_bottom(box->computed_style,
            &margin_bottom_l, &margin_bottom_u);
    assert(margin_bottom_v != CSS_MARGIN_INHERIT);

    int cblock_height = foil_rect_height(&box->cblock_creator->ctnt_rect);

    if (top_v == CSS_TOP_AUTO && height_v == CSS_HEIGHT_AUTO &&
            bottom_v == CSS_BOTTOM_AUTO) {
        if (margin_top_v == CSS_MARGIN_AUTO)
            box->mt = 0;
        else
            box->mt = round_height(normalize_used_length(ctxt, box,
                    margin_top_u, margin_top_l));
        if (margin_bottom_v == CSS_MARGIN_AUTO)
            box->mb = 0;
        else
            box->mb = round_height(normalize_used_length(ctxt, box,
                    margin_bottom_u, margin_bottom_l));

        box->top = 0;      // TODO: the static position.
        box->height = calc_height_for_block_fmt_ctxt_maker(ctxt, box);
        box->bottom = cblock_height - box->top - box->mt - box->bt - box->pt -
                box->height - box->pb - box->bb - box->mb;
    }
    else if (top_v == CSS_TOP_SET && height_v == CSS_HEIGHT_SET &&
            bottom_v == CSS_BOTTOM_SET) {
        box->top = round_height(normalize_used_length(ctxt, box,
                    top_u, top_l));
        box->height = round_height(normalize_used_length(ctxt, box,
                    height_u, height_l));
        box->bottom = round_height(normalize_used_length(ctxt, box,
                    bottom_u, bottom_l));

        if (margin_top_v == CSS_MARGIN_AUTO &&
                margin_bottom_v == CSS_MARGIN_AUTO) {
            int margin = (cblock_height - box->top - box->bt - box->pt -
                box->height - box->pb - box->bb - box->bottom) / 2;
            box->mt = box->mb = margin;
        }
        else if (margin_top_v == CSS_MARGIN_AUTO) {
            box->mb = round_height(normalize_used_length(ctxt, box,
                        margin_bottom_u, margin_bottom_l));
            box->mt = cblock_height - box->top - box->bt - box->pt -
                box->height - box->pb - box->bb - box->mb - box->bottom;
        }
        else if (margin_bottom_v == CSS_MARGIN_AUTO) {
            box->mt = round_height(normalize_used_length(ctxt, box,
                        margin_top_u, margin_top_l));
            box->mb = cblock_height - box->top -
                box->mt - box->bt - box->pt -
                box->height - box->pb - box->bb - box->bottom;
        }
        else {
            box->mt = round_height(normalize_used_length(ctxt, box,
                        margin_top_u, margin_top_l));
            box->mb = round_height(normalize_used_length(ctxt, box,
                        margin_bottom_u, margin_bottom_l));
            box->bottom = cblock_height - box->top -
                box->mt - box->bt - box->pt -
                box->height - box->pb - box->bb - box->mb;
        }
    }
    else {
        if (margin_top_v == CSS_MARGIN_AUTO)
            box->mt = 0;
        else
            box->mt = round_height(normalize_used_length(ctxt, box,
                    margin_top_u, margin_top_l));
        if (margin_bottom_v == CSS_MARGIN_AUTO)
            box->mb = 0;
        else
            box->mb = round_height(normalize_used_length(ctxt, box,
                    margin_bottom_u, margin_bottom_l));

        if (top_v == CSS_MARGIN_AUTO && height_v == CSS_HEIGHT_AUTO &&
                bottom_v != CSS_MARGIN_AUTO) {
            box->bottom = round_height(normalize_used_length(ctxt, box,
                        bottom_u, bottom_l));
            box->height = calc_height_for_block_fmt_ctxt_maker(ctxt, box);
            box->top = cblock_height - box->mt - box->bt - box->pt -
                box->height - box->pb - box->bb - box->mb - box->bottom;
        }
        else if (top_v == CSS_MARGIN_AUTO && height_v != CSS_HEIGHT_AUTO &&
                bottom_v == CSS_MARGIN_AUTO) {
            box->height = round_height(normalize_used_length(ctxt, box,
                        height_u, height_l));
            box->top = 0;      // TODO: the static position.
            box->bottom = cblock_height - box->top -
                box->mt - box->bt - box->pt -
                box->height - box->pb - box->bb - box->mb;
        }
        else if (top_v != CSS_MARGIN_AUTO && height_v == CSS_HEIGHT_AUTO &&
                bottom_v == CSS_MARGIN_AUTO) {
            box->top = round_height(normalize_used_length(ctxt, box,
                        top_u, top_l));
            box->height = calc_height_for_block_fmt_ctxt_maker(ctxt, box);
            box->bottom = cblock_height - box->top -
                box->mt - box->bt - box->pt -
                box->height - box->pb - box->bb - box->mb;
        }
        else if (top_v == CSS_MARGIN_AUTO && height_v != CSS_HEIGHT_AUTO &&
                bottom_v != CSS_MARGIN_AUTO) {
            box->height = round_height(normalize_used_length(ctxt, box,
                        height_u, height_l));
            box->bottom = round_height(normalize_used_length(ctxt, box,
                        bottom_u, bottom_l));
            box->top = cblock_height - box->mt - box->bt - box->pt -
                box->height - box->pb - box->bb - box->mb - box->bottom;
        }
        else if (top_v != CSS_MARGIN_AUTO && height_v == CSS_HEIGHT_AUTO &&
                bottom_v != CSS_MARGIN_AUTO) {
            box->top = round_height(normalize_used_length(ctxt, box,
                        top_u, top_l));
            box->bottom = round_height(normalize_used_length(ctxt, box,
                        bottom_u, bottom_l));
            box->height = cblock_height - box->top -
                box->mt - box->bt - box->pt -
                box->pb - box->bb - box->mb- box->bottom;
        }
        else if (top_v != CSS_MARGIN_AUTO && height_v != CSS_HEIGHT_AUTO &&
                bottom_v == CSS_MARGIN_AUTO) {
            box->top = round_height(normalize_used_length(ctxt, box,
                        top_u, top_l));
            box->height = round_height(normalize_used_length(ctxt, box,
                        height_u, height_l));
            box->bottom = cblock_height - box->top -
                box->mt - box->bt - box->pt -
                box->height - box->pb - box->bb - box->mb;
        }
        else {
            assert(0); // never reach here
        }
    }
}

/* calculate height and margins according to CSS 2.2 Section 10.4 */
static void
calc_height_margins(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE && !box->is_replaced) {
        box->height = 0; // not apply
    }
    else if (box->is_replaced && (box->type == FOIL_RDRBOX_TYPE_INLINE ||
                (box->is_block_level && box->is_in_normal_flow) ||
                (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK &&
                    box->is_in_normal_flow) ||
                box->floating != FOIL_RDRBOX_FLOAT_NONE)) {

        dtrm_margin_top_bottom(ctxt, box);
        dtrm_height_replaced(ctxt, box);
    }
    else if (box->is_block_level && !box->is_replaced &&
            box->is_in_normal_flow &&
            box->overflow_y == CSS_OVERFLOW_VISIBLE) {

        css_fixed height_l;
        css_unit height_u;
        uint8_t height_v = real_computed_height(box, &height_l, &height_u);
        assert(height_v != CSS_HEIGHT_INHERIT);

        if (height_v != CSS_WIDTH_AUTO) {
            box->height = calc_used_value_height(ctxt, box, height_u, height_l);
        }
        else {
            box->height = calc_height_for_visible_non_replaced(ctxt, box);
            dtrm_margin_top_bottom(ctxt, box);
        }
    }
    else if (box->is_abs_positioned) {
        if (box->is_replaced) {
            dtrm_height_replaced(ctxt, box);
            dtrm_heights_abspos_replaced(ctxt, box);
        }
        else {
            dtrm_heights_abspos_non_replaced(ctxt, box);
        }
    }
    else if (!box->is_replaced && ((box->is_block_level &&
                box->is_in_normal_flow &&
                box->overflow_y != CSS_OVERFLOW_VISIBLE) ||
            (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK) ||
            (box->floating != FOIL_RDRBOX_FLOAT_NONE))) {

        dtrm_margin_top_bottom(ctxt, box);

        css_fixed l;
        css_unit u;
        uint8_t v = real_computed_height(box, &l, &u);
        assert(v != CSS_HEIGHT_INHERIT);
        if (v != CSS_HEIGHT_AUTO) {
            box->height = calc_used_value_height(ctxt, box, u, l);
        }
        else {
            box->height = calc_height_for_block_fmt_ctxt_maker(ctxt, box);
        }
    }
    else {
        LOG_ERROR("Should not be here\n");
        assert(0);
    }

    /* determine top and bottom according to CSS 2.2 Section 9.4.3 */
    if (box->position == FOIL_RDRBOX_POSITION_RELATIVE) {
        dtrm_top_bottom_relpos(ctxt, box);
    }
}

#ifndef NDEBUG
static const char *literal_values_text_align[] = {
    "left",
    "right",
    "center",
    "justify",
};

static const char *literal_values_text_overflow[] = {
    "clip",
    "ellipsis",
};
#endif

static foil_rdrbox *find_enclosing_container(foil_rdrbox *box)
{
    foil_rdrbox *ancestor = box->parent;

    while (ancestor) {

        if (ancestor->is_block_container && !ancestor->is_anonymous)
            return ancestor;

        ancestor = ancestor->parent;
    }

    return NULL;
}

static void inherit_used_values(foil_rdrbox *box, const foil_rdrbox *from)
{
    const uint8_t *start  = (const uint8_t *)&from->__copy_start;
    const uint8_t *finish = (const uint8_t *)&from->__copy_finish;

    size_t nr_bytes = finish - start;
    memcpy(&box->__copy_start, start, nr_bytes);

    if (from->quotes) {
        box->quotes = foil_quotes_ref(from->quotes);
    }
}

static void dtmr_sizing_properties(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    css_fixed length;
    css_unit unit;

    /* determine letter-spacing */
    uint8_t v = css_computed_letter_spacing(box->computed_style,
            &length, &unit);
    assert(v != CSS_LETTER_SPACING_INHERIT);
    if (v == CSS_LETTER_SPACING_SET) {
        box->letter_spacing = calc_used_value_width(ctxt, box, unit, length);
    }
    /* CSS_LETTER_SPACING_NORMAL */

    if (box->letter_spacing < 0) {
        box->letter_spacing = 0;
    }

    LOG_DEBUG("\tletter-spacing: %d\n", box->letter_spacing);
    assert(box->letter_spacing >= 0);

    /* determine word-spacing */
    v = css_computed_word_spacing(box->computed_style, &length, &unit);
    assert(v != CSS_WORD_SPACING_INHERIT);
    if (v == CSS_WORD_SPACING_SET) {
        box->word_spacing = calc_used_value_width(ctxt, box, unit, length);
    }
    /* CSS_WORD_SPACING_NORMAL */

    if (box->word_spacing < 0) {
        box->word_spacing = 0;
    }

    LOG_DEBUG("\tword-spacing: %d\n", box->word_spacing);

    if (box->is_block_container) {
        /* determine text-indent */
        v = css_computed_text_indent(box->computed_style, &length, &unit);
        assert(v != CSS_TEXT_INDENT_INHERIT);
        box->text_indent = calc_used_value_width(ctxt, box, unit, length);

        if (box->text_indent < 0)
            box->text_indent = 0;

        LOG_DEBUG("\ttext-indent: %d\n", box->text_indent);

        /* determine text-align */
        v = css_computed_text_align(box->computed_style);
        assert(v != CSS_TEXT_ALIGN_INHERIT);
        switch (v) {
        case CSS_TEXT_ALIGN_RIGHT:
            box->text_align = FOIL_RDRBOX_TEXT_ALIGN_RIGHT;
            break;
        case CSS_TEXT_ALIGN_CENTER:
            box->text_align = FOIL_RDRBOX_TEXT_ALIGN_CENTER;
            break;
        case CSS_TEXT_ALIGN_JUSTIFY:
            box->text_align = FOIL_RDRBOX_TEXT_ALIGN_JUSTIFY;
            break;
        case CSS_TEXT_ALIGN_LEFT:
        default:
            box->text_align = FOIL_RDRBOX_TEXT_ALIGN_LEFT;
            break;
        }

        LOG_DEBUG("\ttext-align: %s\n",
                literal_values_text_align[box->text_align]);

        /* determine text-overflow.
           Note that CSSEng has a wrong interface for text-overflow */
        lwc_string *string;
        v = css_computed_text_overflow(
            box->computed_style, &string);
        if (v == CSS_TEXT_OVERFLOW_ELLIPSIS)
            box->text_overflow = FOIL_RDRBOX_TEXT_OVERFLOW_ELLIPSIS;
        else
            box->text_overflow = FOIL_RDRBOX_TEXT_OVERFLOW_CLIP;

        LOG_DEBUG("\ttext-overflow: %s\n",
                literal_values_text_overflow[box->text_overflow]);
    }

    /* paddings apply to all elements except
       table-row-group, table-header-group, table-footer-group, table-row,
       table-column-group and table-column */
    if (box->type >= FOIL_RDRBOX_TYPE_TABLE_ROW_GROUP &&
            box->type <= FOIL_RDRBOX_TYPE_TABLE_COLUMN) {
        uint8_t v;
        css_fixed l;
        css_unit u;

        v = css_computed_padding_left(box->computed_style, &l, &u);
        assert(v != CSS_PADDING_INHERIT);
        box->pl = calc_used_value_width(ctxt, box, u, l);

        v = css_computed_padding_right(box->computed_style, &l, &u);
        assert(v != CSS_PADDING_INHERIT);
        box->pr = calc_used_value_width(ctxt, box, u, l);

        v = css_computed_padding_top(box->computed_style, &l, &u);
        assert(v != CSS_PADDING_INHERIT);
        box->pt = calc_used_value_width(ctxt, box, u, l);

        v = css_computed_padding_bottom(box->computed_style, &l, &u);
        assert(v != CSS_PADDING_INHERIT);
        box->pb = calc_used_value_width(ctxt, box, u, l);
    }

    /* min-width and max-width:
     *
     * In CSS 2, the effect of min-width and max-width on tables, inline
     * tables, table cells, table columns, and column groups is undefined.
     */
    if (box->type != FOIL_RDRBOX_TYPE_TABLE &&
            box->type != FOIL_RDRBOX_TYPE_INLINE_TABLE &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_CELL &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_COLUMN &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_COLUMN_GROUP &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_ROW_GROUP &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_ROW &&
            !(box->type == FOIL_RDRBOX_TYPE_INLINE && !box->is_replaced)) {
        uint8_t v;
        css_fixed l;
        css_unit u;

        v = css_computed_min_width(box->computed_style, &l, &u);
        assert(v != CSS_MIN_WIDTH_INHERIT);
        if (v == CSS_MIN_WIDTH_SET) {
            box->min_width = calc_used_value_width(ctxt, box, u, l);
            if (box->min_width < 0)
                box->min_width = 0;
        }
        /* else keep initial 0 */

        v = css_computed_max_width(box->computed_style, &l, &u);
        assert(v != CSS_MAX_WIDTH_INHERIT);
        if (v == CSS_MAX_WIDTH_SET) {
            box->max_width = calc_used_value_width(ctxt, box, u, l);
        }
        /* else keep initial -1 */
    }

    /* min-height and max-height:
     *
     * In CSS 2, the effect of min-height and max-height on tables, inline
     * tables, table cells, table rows, and row groups is undefined.
     */
    if (box->type != FOIL_RDRBOX_TYPE_TABLE &&
            box->type != FOIL_RDRBOX_TYPE_INLINE_TABLE &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_CELL &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_ROW &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_ROW_GROUP &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_COLUMN_GROUP &&
            box->type != FOIL_RDRBOX_TYPE_TABLE_COLUMN &&
            !(box->type == FOIL_RDRBOX_TYPE_INLINE && !box->is_replaced)) {
        uint8_t v;
        css_fixed l;
        css_unit u;

        v = css_computed_min_height(box->computed_style, &l, &u);
        assert(v != CSS_MIN_HEIGHT_INHERIT);
        if (v == CSS_MIN_HEIGHT_SET) {
            box->min_height = calc_used_value_height(ctxt, box, u, l);
            if (box->min_height < 0)
                box->min_height = 0;
        }
        /* else keep initial 0 */

        v = css_computed_max_height(box->computed_style, &l, &u);
        assert(v != CSS_MAX_HEIGHT_INHERIT);
        if (v == CSS_MAX_HEIGHT_SET) {
            box->max_height = calc_used_value_height(ctxt, box, u, l);
        }
        /* else keep initial -1 */
    }

    /* determine line-height */
    v = css_computed_line_height(box->computed_style, &length, &unit);
    assert(v != CSS_LINE_HEIGHT_INHERIT);
    switch (v) {
    case CSS_LINE_HEIGHT_NORMAL:
        box->line_height = FOIL_PX_GRID_CELL_H;
        break;

    case CSS_LINE_HEIGHT_NUMBER: {
        float pct = FIXTOFLT(length);
        box->line_height = round_height(FOIL_PX_GRID_CELL_H * pct);
        break;
        }

    case CSS_LINE_HEIGHT_DIMENSION:
        if (unit == CSS_UNIT_PCT) {
            float pct = FIXTOFLT(length);
            box->line_height = round_height(FOIL_PX_GRID_CELL_H * pct);
        }
        else
            box->line_height = calc_used_value_height(ctxt, box, unit, length);
        break;
    }

    /* determine vertical-align */
    if (box->is_inline_level || box->type != FOIL_RDRBOX_TYPE_TABLE_CELL) {
        v = css_computed_vertical_align(box->computed_style, &length, &unit);
        assert(v != CSS_VERTICAL_ALIGN_INHERIT);
        switch (v) {
            case CSS_VERTICAL_ALIGN_BASELINE:
            case CSS_VERTICAL_ALIGN_SUB:
            case CSS_VERTICAL_ALIGN_BOTTOM:
            case CSS_VERTICAL_ALIGN_TEXT_BOTTOM:
                box->vertical_align = FOIL_RDRBOX_VALIGN_BOTTOM;
                break;
            case CSS_VERTICAL_ALIGN_SUPER:
            case CSS_VERTICAL_ALIGN_TOP:
            case CSS_VERTICAL_ALIGN_TEXT_TOP:
                box->vertical_align = FOIL_RDRBOX_VALIGN_TOP;
                break;

            case CSS_VERTICAL_ALIGN_MIDDLE:
                box->vertical_align = FOIL_RDRBOX_VALIGN_MIDDLE;
                break;

            case CSS_VERTICAL_ALIGN_SET:
                if (unit == CSS_UNIT_PCT) {
                    float pct = FIXTOFLT(length);
                    if (pct < 0.25f)
                        box->vertical_align = FOIL_RDRBOX_VALIGN_BOTTOM;
                    else if (pct > 0.75f)
                        box->vertical_align = FOIL_RDRBOX_VALIGN_TOP;
                    else
                        box->vertical_align = FOIL_RDRBOX_VALIGN_MIDDLE;
                }
                else {
                    int distance = calc_used_value_height(ctxt, box,
                            unit, length);
                    if (distance < (int)box->line_height / 4)
                        box->vertical_align = FOIL_RDRBOX_VALIGN_BOTTOM;
                    else if (distance > (int)box->line_height * 3 / 4)
                        box->vertical_align = FOIL_RDRBOX_VALIGN_TOP;
                    else
                        box->vertical_align = FOIL_RDRBOX_VALIGN_MIDDLE;
                }
                break;
        }
    }
}

static uint8_t normalize_border_style(uint8_t v)
{
    switch (v) {
    case CSS_BORDER_STYLE_NONE:
        v = FOIL_RDRBOX_BORDER_STYLE_NONE;
        break;
    case CSS_BORDER_STYLE_HIDDEN:
        v = FOIL_RDRBOX_BORDER_STYLE_HIDDEN;
        break;
    case CSS_BORDER_STYLE_DOTTED:
        v = FOIL_RDRBOX_BORDER_STYLE_DOTTED;
        break;
    case CSS_BORDER_STYLE_DASHED:
        v = FOIL_RDRBOX_BORDER_STYLE_DASHED;
        break;
    case CSS_BORDER_STYLE_SOLID:
        v = FOIL_RDRBOX_BORDER_STYLE_SOLID;
        break;
    case CSS_BORDER_STYLE_DOUBLE:
        v = FOIL_RDRBOX_BORDER_STYLE_DOUBLE;
        break;
    case CSS_BORDER_STYLE_GROOVE:
    case CSS_BORDER_STYLE_RIDGE:
    case CSS_BORDER_STYLE_INSET:
    case CSS_BORDER_STYLE_OUTSET:
    default:
        v = FOIL_RDRBOX_BORDER_STYLE_SOLID;
        break;
    }

    return v;
}

static uint8_t normalize_border_width_v(int w)
{
    uint8_t v;
    if (w <= 0) {
        v = FOIL_RDRBOX_BORDER_WIDTH_ZERO;
    }
    else if (w < FOIL_PX_GRID_CELL_H / 3) {
        v = FOIL_RDRBOX_BORDER_WIDTH_THIN;
    }
    else if (w < FOIL_PX_GRID_CELL_H * 2 / 3) {
        v = FOIL_RDRBOX_BORDER_WIDTH_MEDIUM;
    }
    else {
        v = FOIL_RDRBOX_BORDER_WIDTH_THICK;
    }

    return v;
}

static uint8_t normalize_border_width_h(int w)
{
    uint8_t v;
    if (w <= 0) {
        v = FOIL_RDRBOX_BORDER_WIDTH_ZERO;
    }
    else if (w < FOIL_PX_GRID_CELL_W / 3) {
        v = FOIL_RDRBOX_BORDER_WIDTH_THIN;
    }
    else if (w < FOIL_PX_GRID_CELL_W * 2 / 3) {
        v = FOIL_RDRBOX_BORDER_WIDTH_MEDIUM;
    }
    else {
        v = FOIL_RDRBOX_BORDER_WIDTH_THICK;
    }

    return v;
}

static void dtmr_border_properties(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t v;
    css_fixed length;
    css_unit unit;
    css_color color;
    int w;

    v = css_computed_border_top_style(box->computed_style);
    assert(v != CSS_BORDER_STYLE_INHERIT);
    box->border_top_style = normalize_border_style(v);
    if (box->border_top_style == FOIL_RDRBOX_BORDER_STYLE_NONE ||
            box->border_top_style == FOIL_RDRBOX_BORDER_STYLE_HIDDEN) {
        box->border_top_width = FOIL_RDRBOX_BORDER_WIDTH_ZERO;
    }
    else {
        v = css_computed_border_top_width(box->computed_style,
                &length, &unit);
        assert(v != CSS_BORDER_WIDTH_INHERIT);
        if (v == CSS_BORDER_WIDTH_WIDTH) {
            w = round_height(normalize_used_length(ctxt, box, unit, length));
            box->border_top_width = normalize_border_width_v(w);
        }
        else {
            box->border_top_width = v;
        }
    }

    if (box->border_top_width == FOIL_RDRBOX_BORDER_WIDTH_ZERO)
        box->bt = 0;
    else {
        box->bt = FOIL_PX_GRID_CELL_H;
        v = css_computed_border_top_color(box->computed_style, &color);
        assert(v != CSS_COLOR_INHERIT);
        box->border_top_color = foil_map_xrgb_to_16c(color);
    }

    v = css_computed_border_right_style(box->computed_style);
    assert(v != CSS_BORDER_STYLE_INHERIT);
    box->border_right_style = normalize_border_style(v);
    if (box->border_right_style == FOIL_RDRBOX_BORDER_STYLE_NONE ||
            box->border_right_style == FOIL_RDRBOX_BORDER_STYLE_HIDDEN) {
        box->border_right_width = FOIL_RDRBOX_BORDER_WIDTH_ZERO;
    }
    else {
        v = css_computed_border_right_width(box->computed_style,
                &length, &unit);
        assert(v != CSS_BORDER_WIDTH_INHERIT);
        if (v == CSS_BORDER_WIDTH_WIDTH) {
            w = round_height(normalize_used_length(ctxt, box, unit, length));
            box->border_right_width = normalize_border_width_h(w);
        }
        else {
            box->border_right_width = v;
        }
    }

    if (box->border_right_width == FOIL_RDRBOX_BORDER_WIDTH_ZERO)
        box->br = 0;
    else {
        box->br = FOIL_PX_GRID_CELL_W;
        v = css_computed_border_right_color(box->computed_style, &color);
        assert(v != CSS_COLOR_INHERIT);
        box->border_right_color = foil_map_xrgb_to_16c(color);
    }

    v = css_computed_border_bottom_style(box->computed_style);
    assert(v != CSS_BORDER_STYLE_INHERIT);
    box->border_bottom_style = normalize_border_style(v);
    if (box->border_bottom_style == FOIL_RDRBOX_BORDER_STYLE_NONE ||
            box->border_bottom_style == FOIL_RDRBOX_BORDER_STYLE_HIDDEN) {
        box->border_bottom_width = FOIL_RDRBOX_BORDER_WIDTH_ZERO;
    }
    else {
        v = css_computed_border_bottom_width(box->computed_style,
                &length, &unit);
        assert(v != CSS_BORDER_WIDTH_INHERIT);
        if (v == CSS_BORDER_WIDTH_WIDTH) {
            w = round_height(normalize_used_length(ctxt, box, unit, length));
            box->border_bottom_width = normalize_border_width_v(w);
        }
        else {
            box->border_bottom_width = v;
        }
    }

    if (box->border_bottom_width == FOIL_RDRBOX_BORDER_WIDTH_ZERO)
        box->br = 0;
    else {
        box->br = FOIL_PX_GRID_CELL_H;
        v = css_computed_border_bottom_color(box->computed_style, &color);
        assert(v != CSS_COLOR_INHERIT);
        box->border_bottom_color = foil_map_xrgb_to_16c(color);
    }

    v = css_computed_border_left_style(box->computed_style);
    assert(v != CSS_BORDER_STYLE_INHERIT);
    box->border_left_style = normalize_border_style(v);
    if (box->border_left_style == FOIL_RDRBOX_BORDER_STYLE_NONE ||
            box->border_left_style == FOIL_RDRBOX_BORDER_STYLE_HIDDEN) {
        box->border_left_width = FOIL_RDRBOX_BORDER_WIDTH_ZERO;
    }
    else {
        v = css_computed_border_left_width(box->computed_style,
                &length, &unit);
        assert(v != CSS_BORDER_WIDTH_INHERIT);
        if (v == CSS_BORDER_WIDTH_WIDTH) {
            w = round_height(normalize_used_length(ctxt, box, unit, length));
            box->border_left_width = normalize_border_width_h(w);
        }
        else {
            box->border_left_width = v;
        }
    }

    if (box->border_left_width == FOIL_RDRBOX_BORDER_WIDTH_ZERO)
        box->br = 0;
    else {
        box->br = FOIL_PX_GRID_CELL_W;
        v = css_computed_border_left_color(box->computed_style, &color);
        assert(v != CSS_COLOR_INHERIT);
        box->border_left_color = foil_map_xrgb_to_16c(color);
    }
}

void foil_rdrbox_pre_layout(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
#ifndef NDEBUG
    char *name = foil_rdrbox_get_name(ctxt->udom->doc, box);
    LOG_DEBUG("called for box %s\n", name);
    free(name);
#endif

    /* determine the container */
    if (purc_document_root(ctxt->udom->doc) == box->owner) {
        box->cblock_creator = ctxt->udom->initial_cblock;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_STATIC ||
            box->position == FOIL_RDRBOX_POSITION_RELATIVE) {
        /* For other elements, if the element’s position is relative or static,
           the containing block is formed by the content edge of the nearest
           block container ancestor box. */

        const foil_rdrbox *container;
        container = foil_rdrbox_find_container_for_relative(ctxt, box->parent);
        assert(container);
        box->cblock_creator = container;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_FIXED) {
        box->cblock_creator = ctxt->udom->initial_cblock;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_ABSOLUTE) {
        /* the containing block is established by the nearest ancestor
           with a 'position' of 'absolute', 'relative' or 'fixed',
           in the following way:

           In the case that the ancestor is an inline element,
           the containing block is the bounding box around the padding boxes
           of the first and the last inline boxes generated for that element.

           Otherwise, the containing block is formed by the padding edge of
           the ancestor.

           If there is no such ancestor, the containing block is
           the initial containing block. */

        const foil_rdrbox *container;
        container = foil_rdrbox_find_container_for_absolute(ctxt, box);
        if (container) {
            box->cblock_creator = container;
        }
        else {
            box->cblock_creator = ctxt->udom->initial_cblock;
        }
    }

    /* inherit properties for anonymous and pseudo box */
    /* FIXME: we might need to create a new css_computed_style object
       for the pseudo and anonymous box and compose the values... */
    if (box->is_pseudo) {
        assert(box->principal);

        inherit_used_values(box, box->principal);
    }
    else if (box->is_anonymous) {
        foil_rdrbox *from = NULL;
        if (box->type == FOIL_RDRBOX_TYPE_BLOCK) {
            from = find_enclosing_container(box);
        }
        else if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
            from = box->parent;
        }

        assert(from && from->type == FOIL_RDRBOX_TYPE_BLOCK);
        inherit_used_values(box, from);
    }
    else {
        assert(box->computed_style);
        dtmr_sizing_properties(ctxt, box);
        dtmr_border_properties(ctxt, box);
    }

    if (!box->is_abs_positioned && !box->floating) {
        box->is_in_normal_flow = 1;
        if (!box->is_root)
            box->is_in_flow = 1;
    }
    else {
        if (box->floating) {
            box->parent->nr_floating_children++;
        }

        if (box->is_abs_positioned) {
            box->parent->nr_abspos_children++;
        }
    }

    if (box->is_in_normal_flow) {
        if (box->is_inline_level) {
            box->parent->nr_inline_level_children++;
        }
        else if (box->is_block_level) {
            box->parent->nr_block_level_children++;
        }
    }

}

void foil_rdrbox_resolve_width(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    assert(box->is_width_resolved == 0);

    if (box->type == FOIL_RDRBOX_TYPE_MARKER) {
        box->width = box->marker_data->width;
        box->is_width_resolved = 1;
    }
    else if (box->computed_style) {
        /* calculate widths and margins */
        box->prop_for_width = FOIL_RDRBOX_USE_WIDTH;
        calc_width_margins(ctxt, box);
        if (box->max_width > 0 && box->width > box->max_width) {
            box->prop_for_width = FOIL_RDRBOX_USE_MAX_WIDTH;
            calc_width_margins(ctxt, box);
        }
        if (box->min_width > 0 && box->width < box->min_width) {
            box->prop_for_width = FOIL_RDRBOX_USE_MIN_WIDTH;
            calc_width_margins(ctxt, box);
        }
        box->is_width_resolved = 1;
    }
    else if (box->is_anonymous && box->is_block_level) {
        /* calculate width for anonymous block level box */
        int cblock_width = foil_rect_width(&box->cblock_creator->ctnt_rect);
        box->width = cblock_width - box->ml - box->bl - box->pl -
            box->pr - box->br - box->mr;
        box->is_width_resolved = 1;
    }

    if (box->floating || box->is_abs_positioned ||
            (box->is_block_container && !box->is_block_level) ||
            (box->is_block_level && box->overflow_y != CSS_OVERFLOW_VISIBLE)) {
        box->block_fmt_ctxt = foil_rdrbox_block_fmt_ctxt_new(
                &ctxt->udom->rgnrc_heap, box->width, -1);
    }
}

void foil_rdrbox_resolve_height(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    assert(box->is_height_resolved == 0);

    if (box->nr_inline_level_children > 0) {

        if (box->type == FOIL_RDRBOX_TYPE_BLOCK) {
            box->block_data->lfmt_ctxt = foil_rdrbox_inline_fmt_ctxt_new();
            box->cb_data_cleanup =
                (foil_data_cleanup_cb)foil_rdrbox_inline_fmt_ctxt_delete;
        }
        else if (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK) {
            box->inline_block_data->lfmt_ctxt =
                foil_rdrbox_inline_fmt_ctxt_new();
            box->cb_data_cleanup =
                (foil_data_cleanup_cb)foil_rdrbox_inline_fmt_ctxt_delete;
        }
        else {
            // never reach here
            assert(0);
        }
    }

    if (box->type == FOIL_RDRBOX_TYPE_MARKER) {
        box->height = FOIL_PX_GRID_CELL_H;
        box->is_height_resolved = 1;
    }
    else if (box->computed_style) {
        /* calculate height and margins */
        box->prop_for_height = FOIL_RDRBOX_USE_HEIGHT;
        calc_height_margins(ctxt, box);
        if (box->max_height > 0 && box->height > box->max_height) {
            box->prop_for_height = FOIL_RDRBOX_USE_MAX_HEIGHT;
            calc_height_margins(ctxt, box);
        }
        if (box->min_height > 0 && box->height > box->min_height) {
            box->prop_for_height = FOIL_RDRBOX_USE_MIN_HEIGHT;
            calc_height_margins(ctxt, box);
        }
        box->is_height_resolved = 1;

    }
    else if (box->is_anonymous && box->is_block_level) {
        if (box->nr_inline_level_children > 0) {
        }
        else {
            box->height = 0;
            box->is_height_resolved = 1;
        }
    }
}

/* adjust position */
static void
adjust_position(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
    // TODO
}

void foil_rdrbox_layout(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    /* adjust position of the box */
    adjust_position(ctxt, box);
}

void foil_rdrbox_containing_block(const foil_rdrbox *box, foil_rect *rc)
{
    assert(box->cblock_creator);

    if (box->position == FOIL_RDRBOX_POSITION_ABSOLUTE) {
        /* the containing block is established by the nearest ancestor
           with a 'position' of 'absolute', 'relative' or 'fixed',
           in the following way:

           In the case that the ancestor is an inline element,
           the containing block is the bounding box around the padding boxes
           of the first and the last inline boxes generated for that element.

           Otherwise, the containing block is formed by the padding edge of
           the ancestor.

           If there is no such ancestor, the containing block is
           the initial containing block. */

        if (box->cblock_creator->nr_inline_level_children) {
            foil_rdrbox_containing_block_from_inlines(box->cblock_creator, rc);
        }
        else {
            foil_rdrbox_padding_box(box->cblock_creator, rc);
        }
    }
    else {
        foil_rdrbox_content_box(box->cblock_creator, rc);
    }
}

void foil_rdrbox_containing_block_from_inlines(const foil_rdrbox *box,
        foil_rect *rc)
{
    if (box->first) {
        rc->left   = box->first->ctnt_rect.left  - box->first->pl;
        rc->top    = box->first->ctnt_rect.top   - box->first->pt;
        rc->right  = box->last->ctnt_rect.right  + box->last->pr;
        rc->bottom = box->last->ctnt_rect.bottom + box->last->pb;
    }
    else {
        *rc = box->ctnt_rect;
    }
}

void foil_rdrbox_content_box(const foil_rdrbox *box, foil_rect *rc)
{
    *rc = box->ctnt_rect;
}

void foil_rdrbox_padding_box(const foil_rdrbox *box, foil_rect *rc)
{
    assert(box->type != FOIL_RDRBOX_TYPE_INLINE);

    rc->left   = box->ctnt_rect.left   - box->pl;
    rc->top    = box->ctnt_rect.top    - box->pt;
    rc->right  = box->ctnt_rect.right  + box->pr;
    rc->bottom = box->ctnt_rect.bottom + box->pb;
}

void foil_rdrbox_border_box(const foil_rdrbox *box, foil_rect *rc)
{
    assert(box->type != FOIL_RDRBOX_TYPE_INLINE);

    rc->left   = box->ctnt_rect.left   - box->pl - box->bl;
    rc->top    = box->ctnt_rect.top    - box->pt - box->bt;
    rc->right  = box->ctnt_rect.right  + box->pr + box->br;
    rc->bottom = box->ctnt_rect.bottom + box->pb + box->bb;
}

void foil_rdrbox_margin_box(const foil_rdrbox *box, foil_rect *rc)
{
    assert(box->type != FOIL_RDRBOX_TYPE_INLINE);

    rc->left   = box->ctnt_rect.left   - box->pl - box->bl - box->ml;
    rc->top    = box->ctnt_rect.top    - box->pt - box->bt - box->mt;
    rc->right  = box->ctnt_rect.right  + box->pr + box->br + box->mr;
    rc->bottom = box->ctnt_rect.bottom + box->pb + box->bb + box->mb;
}

const foil_rdrbox *
foil_rdrbox_find_container_for_relative(foil_layout_ctxt *ctxt,
        const foil_rdrbox *box)
{
    assert(box->position == FOIL_RDRBOX_POSITION_RELATIVE ||
            box->position == FOIL_RDRBOX_POSITION_STATIC);

    /* the containing block is formed by the content edge of the nearest
       block container ancestor box. */
    const foil_rdrbox *parent = box->parent;
    while (parent) {
        if (parent->is_block_container)
            return parent;

        parent = box->parent;
    }

    return ctxt->initial_cblock;
}

const foil_rdrbox *
foil_rdrbox_find_container_for_absolute(foil_layout_ctxt *ctxt,
        const foil_rdrbox *box)
{
    (void)ctxt;

    assert(box->position == FOIL_RDRBOX_POSITION_ABSOLUTE);

    /* the containing block is established by the nearest ancestor
       with a 'position' of 'absolute', 'relative' or 'fixed'. */
    const foil_rdrbox *parent = box->parent;
    while (parent) {
        if (parent->position == FOIL_RDRBOX_POSITION_ABSOLUTE ||
                parent->position == FOIL_RDRBOX_POSITION_RELATIVE ||
                parent->position == FOIL_RDRBOX_POSITION_FIXED)
            return parent;

        parent = box->parent;
    }

    return NULL;
}

static int
dtrm_width_shrink_to_fit(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v;
    css_fixed width_l;
    css_unit width_u;

    int width = 0;
    width_v = real_computed_width(box, &width_l, &width_u);
    assert(width_v != CSS_WIDTH_INHERIT);
    if (width_v != CSS_WIDTH_AUTO) {
        width = calc_used_value_width(ctxt, box, width_u, width_l);
    }
    else if (box->nr_inline_level_children > 0) {
        int cblock_width = foil_rect_width(&box->cblock_creator->ctnt_rect);
        int avl_width = cblock_width - box->ml - box->bl - box->pl -
            box->pr - box->br - box->mr;

        foil_rdrbox *child = box->first;
        while (child) {
            assert(child->is_width_resolved == 0);

            int shrink_width;
            if (child->type == FOIL_RDRBOX_TYPE_INLINE) {
                int pref_width, min_width;
                pref_width = foil_rdrbox_inline_calc_preferred_width(child);
                min_width = foil_rdrbox_inline_calc_preferred_minimum_width(
                        child);

                shrink_width = MIN(MAX(min_width, avl_width), pref_width);
            }
            else if (child->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK) {
                shrink_width = dtrm_width_shrink_to_fit(ctxt, child);
            }
            else if (child->type == FOIL_RDRBOX_TYPE_INLINE_TABLE) {
                // TODO: inline-table
                shrink_width = FOIL_PX_GRID_CELL_W * 10;
            }

            if (shrink_width > width)
                width = shrink_width;

            child->width = shrink_width;
            dtrm_margin_left_right(ctxt, child);
            child->is_width_resolved = 1;

            child = child->next;
        }
    }
    else if (box->nr_block_level_children > 0) {
        foil_rdrbox *child = box->first;
        while (child) {
            assert(child->is_width_resolved == 0);

            int shrink_width = dtrm_width_shrink_to_fit(ctxt, child);
            if (shrink_width > width)
                width = shrink_width;

            child->width = shrink_width;
            dtrm_margin_left_right(ctxt, child);
            child->is_width_resolved = 1;

            child = child->next;
        }
    }

    return width;
}

static int
calc_height_for_visible_non_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
    // TODO:
    return 0;
}

static int
calc_height_for_block_fmt_ctxt_maker(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
    // TODO:
    return 0;
}

