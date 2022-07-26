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

static int round_width(float w)
{
    if (w > 0)
        return (int)(w / FOIL_PX_GRID_CELL_W + 0.5) * FOIL_PX_GRID_CELL_W;

    return (int)(w / FOIL_PX_GRID_CELL_W - 0.5) * FOIL_PX_GRID_CELL_W;
}

static int round_height(float h)
{
    if (h > 0)
        return (int)(h / FOIL_PX_GRID_CELL_H + 0.5) * FOIL_PX_GRID_CELL_H;
    return (int)(h / FOIL_PX_GRID_CELL_H - 0.5) * FOIL_PX_GRID_CELL_H;
}

static int calc_used_value_widths(foil_rdrbox *box,
        css_unit unit, css_fixed length)
{
    int v = 0;

    switch (unit) {
    case CSS_UNIT_PCT:
        v = foil_rect_width(&box->cblock_rect);
        v = round_width(v * FIXTOFLT(length));
        break;

    case CSS_UNIT_PX:
        v = round_width(FIXTOFLT(length));
        break;

    case CSS_UNIT_EX:
        v = round_width(FIXTOFLT(length) * FOIL_PX_GRID_CELL_W);
        break;

    case CSS_UNIT_EM:
        v = round_width(FIXTOFLT(length) * FOIL_PX_GRID_CELL_H);
        break;

    default:
        // TODO: support more unit
        LOG_WARN("TODO: support unit: %d\n", unit);
        break;
    }

    return v;
}

static int calc_used_value_heights(foil_rdrbox *box,
        css_unit unit, css_fixed length)
{
    int v = 0;

    switch (unit) {
    case CSS_UNIT_PCT:
        v = foil_rect_height(&box->cblock_rect);
        v = round_height(v * FIXTOFLT(length));
        break;

    case CSS_UNIT_PX:
        v = round_height(FIXTOFLT(length));
        break;

    case CSS_UNIT_EX:
        v = round_height(FIXTOFLT(length) * FOIL_PX_GRID_CELL_W);
        break;

    case CSS_UNIT_EM:
        v = round_height(FIXTOFLT(length) * FOIL_PX_GRID_CELL_H);
        break;

    default:
        // TODO: support more unit
        LOG_WARN("TODO: support unit: %d\n", unit);
        break;
    }

    return v;
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
            box->ml = calc_used_value_widths(box, unit, length);
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
            box->mr = calc_used_value_widths(box, unit, length);
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
            box->mr = calc_used_value_heights(box, unit, length);
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
            box->mb = calc_used_value_heights(box, unit, length);
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
dtrm_width_replaced(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v, height_v;
    css_fixed width_l, height_l;
    css_unit width_u, height_u;

    assert(box->is_replaced);

    width_v = css_computed_width(box->computed_style, &width_l, &width_u);
    assert(width_v != CSS_WIDTH_INHERIT);

    if (width_v != CSS_WIDTH_AUTO) {
        box->width = calc_used_value_widths(box, width_u, width_l);

        return width_v;
    }

    height_v = css_computed_height(box->computed_style, &height_l, &height_u);
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
        height = calc_used_value_heights(box, height_u, height_l);
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

    height_v = css_computed_height(box->computed_style, &height_l, &height_u);
    assert(height_v != CSS_HEIGHT_INHERIT);
    if (height_v != CSS_WIDTH_AUTO) {
        box->height = calc_used_value_heights(box, height_u, height_l);

        return height_v;
    }

    width_v = css_computed_width(box->computed_style, &width_l, &width_u);

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

static uint8_t
dtrm_width_shrink_to_fit(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    uint8_t width_v;
    css_fixed width_l;
    css_unit width_u;

    width_v = css_computed_width(box->computed_style, &width_l, &width_u);
    assert(width_v != CSS_WIDTH_INHERIT);

    if (width_v != CSS_WIDTH_AUTO) {
        box->width = calc_used_value_widths(box, width_u, width_l);

        return width_v;
    }

    /* TODO */
    LOG_WARN("Not implemented\n");
    box->width = FOIL_PX_GRID_CELL_W * 10;
    return CSS_WIDTH_SET;
}

static void
dtrm_margin_left_right_block_normal(foil_layout_ctxt *ctxt,
        foil_rdrbox *box, uint8_t width_v)
{
    (void)ctxt;
    int nr_autos = 0;
    int cblock_width = foil_rect_width(&box->cblock_rect);

    if (width_v == CSS_WIDTH_AUTO)
        nr_autos++;

    uint8_t v;
    css_fixed padding_left_l;
    css_unit padding_left_u;
    v = css_computed_padding_left(box->computed_style,
            &padding_left_l, &padding_left_u);
    assert(v != CSS_PADDING_INHERIT);
    box->pl = calc_used_value_widths(box, padding_left_u, padding_left_l);

    css_fixed padding_right_l;
    css_unit padding_right_u;
    v = css_computed_padding_right(box->computed_style,
            &padding_right_l, &padding_right_u);
    assert(v != CSS_PADDING_INHERIT);
    box->pr = calc_used_value_widths(box, padding_right_u, padding_right_l);
    (void)v;

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
        box->ml = calc_used_value_widths(box, margin_left_u, margin_left_l);
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
        box->mr = calc_used_value_widths(box, margin_right_u, margin_right_l);
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
            box->mr = cblock_width -
                box->width - box->pl - box->bl - box->pr - box->br - box->ml;
        }
        else {
            box->ml = cblock_width -
                box->width - box->pl - box->bl - box->pr - box->br - box->mr;
        }
    }
    else if (nr_autos == 1) {
        if (width_v == CSS_WIDTH_AUTO)
            box->width = cblock_width -
                box->ml - box->bl - box->pl - box->pr - box->br - box->mr;
        else if (margin_left_v == CSS_MARGIN_AUTO)
            box->ml = cblock_width -
                box->width - box->bl - box->pl - box->pr - box->br - box->mr;
        else if (margin_right_v == CSS_MARGIN_AUTO)
            box->mr = cblock_width -
                box->width - box->bl - box->pl - box->pr - box->br - box->ml;
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

/* calculate widths and margins */
static void
calc_widths_margins(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE) {
        if (box->is_replaced) {
            /* CSS 2.2 Section 10.3.2 */

            dtrm_width_replaced(ctxt, box);
        }
        else {
            /* CSS 2.2 Section 10.3.1 */
            box->width = -1; // not apply
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else if (box->type == FOIL_RDRBOX_TYPE_BLOCK && ctxt->in_normal_flow) {
        uint8_t width_v;

        if (box->is_replaced) {
            width_v = dtrm_width_replaced(ctxt, box);
        }
        else {
            css_fixed width_l;
            css_unit width_u;
            width_v = css_computed_width(box->computed_style, &width_l, &width_u);
            assert(width_v != CSS_WIDTH_INHERIT);
            if (width_v == CSS_WIDTH_AUTO) {
                box->width = 0;
            }
            else {
                box->width = calc_used_value_widths(box, width_u, width_l);
            }
        }

        dtrm_margin_left_right_block_normal(ctxt, box, width_v);
    }
    else if (ctxt->pos_schema == FOIL_RDRBOX_POSSCHEMA_FLOATS) {
        if (box->is_replaced) {
            dtrm_width_replaced(ctxt, box);
        }
        else {
            dtrm_width_shrink_to_fit(ctxt, box);
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else if (ctxt->pos_schema == FOIL_RDRBOX_POSSCHEMA_ABSOLUTE) {
        LOG_WARN("Not implemented for absolutely positioned\n");
    }
    else if (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK &&
            ctxt->in_normal_flow) {
        if (box->is_replaced) {
            dtrm_width_replaced(ctxt, box);
        }
        else {
            dtrm_width_shrink_to_fit(ctxt, box);
        }

        dtrm_margin_left_right(ctxt, box);
    }
    else {
        LOG_ERROR("Should not be here\n");
    }
}

/* calculate heights and margins */
static void
calc_heights_margins(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE && !box->is_replaced) {
        box->height = -1; // not apply
    }
    else if (box->is_replaced && (box->type == FOIL_RDRBOX_TYPE_INLINE ||
                (box->type == FOIL_RDRBOX_TYPE_BLOCK &&
                    ctxt->in_normal_flow) ||
                (box->type == FOIL_RDRBOX_TYPE_INLINE_BLOCK &&
                    ctxt->in_normal_flow) ||
                ctxt->pos_schema == FOIL_RDRBOX_POSSCHEMA_FLOATS)) {

        dtrm_margin_top_bottom(ctxt, box);
        dtrm_height_replaced(ctxt, box);
    }
    else if (box->type == FOIL_RDRBOX_TYPE_BLOCK && !box->is_replaced &&
            ctxt->in_normal_flow) {

        css_fixed height_l;
        css_unit height_u;
        uint8_t height_v = css_computed_height(box->computed_style,
                &height_l, &height_u);
        assert(height_v != CSS_HEIGHT_INHERIT);

        if (height_v != CSS_WIDTH_AUTO) {
            box->height = calc_used_value_heights(box, height_u, height_l);
        }
        else if (box->overflow_y == CSS_OVERFLOW_VISIBLE) {
            dtrm_margin_top_bottom(ctxt, box);

            // set height is pending.
            box->height_pending = 1;
        }
    }
}

/* adjust position according to 'vertical-align' */
static void
adjust_position_vertically(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
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

static void dtmr_sizing_properties(foil_rdrbox *box)
{
    css_fixed length;
    css_unit unit;

    /* determine letter-spacing */
    uint8_t v = css_computed_letter_spacing(
            box->computed_style,
            &length, &unit);
    assert(v != CSS_LETTER_SPACING_INHERIT);
    if (v == CSS_LETTER_SPACING_SET) {
        box->letter_spacing = calc_used_value_widths(box, unit, length);
    }
    /* CSS_LETTER_SPACING_NORMAL */

    if (box->letter_spacing < 0) {
        box->letter_spacing = 0;
    }

    LOG_DEBUG("\tletter-spacing: %d\n", box->letter_spacing);
    assert(box->letter_spacing >= 0);

    /* determine word-spacing */
    v = css_computed_word_spacing(
            box->computed_style,
            &length, &unit);
    assert(v != CSS_WORD_SPACING_INHERIT);
    if (v == CSS_WORD_SPACING_SET) {
        box->word_spacing = calc_used_value_widths(box, unit, length);
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
        box->text_indent = calc_used_value_widths(box, unit, length);

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
}

void foil_rdrbox_determine_geometry(foil_layout_ctxt *ctxt, foil_rdrbox *box)
{
#ifndef NDEBUG
    char *name = foil_rdrbox_get_name(ctxt->udom->doc, box);
    LOG_DEBUG("called for box %s\n", name);
    free(name);
#endif

    /* determine the containing block */
    if (purc_document_root(ctxt->udom->doc) == box->owner) {
        box->cblock_rect.left = 0;
        box->cblock_rect.top = 0;
        box->cblock_rect.right = ctxt->udom->initial_cblock->width;
        box->cblock_rect.bottom = ctxt->udom->initial_cblock->height;
        box->cblock_creator = ctxt->udom->initial_cblock;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_STATIC ||
            box->position == FOIL_RDRBOX_POSITION_RELATIVE) {
        /* the containing block is formed by the content edge of
           the nearest ancestor box that is a block container or
           which establishes a formatting context. */

        const foil_rdrbox *container;
        container = foil_rdrbox_find_container_for_relative(ctxt,
                box->parent);
        assert(container);
        foil_rdrbox_content_box(container, &box->cblock_rect);
        box->cblock_creator = container;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_FIXED) {
        box->cblock_rect.left = 0;
        box->cblock_rect.top = 0;
        box->cblock_rect.right = ctxt->udom->initial_cblock->width;
        box->cblock_rect.bottom = ctxt->udom->initial_cblock->height;
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
        container = foil_rdrbox_find_container_for_absolute(ctxt, box->parent);
        if (container) {
            if (container->type == FOIL_RDRBOX_TYPE_INLINE)
                foil_rdrbox_form_containing_block(container,
                        &box->cblock_rect);
            else
                foil_rdrbox_padding_box(container, &box->cblock_rect);
        }
        else {
            box->cblock_rect.left = 0;
            box->cblock_rect.top = 0;
            box->cblock_rect.right = ctxt->udom->initial_cblock->width;
            box->cblock_rect.bottom = ctxt->udom->initial_cblock->height;
        }
        box->cblock_creator = ctxt->udom->initial_cblock;
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
        dtmr_sizing_properties(box);
    }

    /* calculate widths and margins */
    calc_widths_margins(ctxt, box);

    /* calculate heights and margins */
    calc_heights_margins(ctxt, box);

    /* adjust position according to 'vertical-align' */
    adjust_position_vertically(ctxt, box);
}

bool foil_rdrbox_content_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->cblock_rect.left   + box->left +
        box->ml + box->bl + box->pl;
    rc->top    = box->cblock_rect.top    + box->top  +
        box->mt + box->bt + box->pt;
    rc->right  = box->cblock_rect.right  + box->left +
        box->mr + box->br + box->pr;
    rc->bottom = box->cblock_rect.bottom + box->top  +
        box->mb + box->bb + box->pb;
    return true;
}

bool foil_rdrbox_padding_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->cblock_rect.left   + box->left + box->ml + box->bl;
    rc->top    = box->cblock_rect.top    + box->top  + box->mt + box->bt;
    rc->right  = box->cblock_rect.right  + box->left + box->mr + box->br;
    rc->bottom = box->cblock_rect.bottom + box->top  + box->mb + box->bb;

    return true;
}

bool foil_rdrbox_border_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->cblock_rect.left   + box->left + box->ml;
    rc->top    = box->cblock_rect.top    + box->top  + box->mt;
    rc->right  = box->cblock_rect.right  + box->left + box->mr;
    rc->bottom = box->cblock_rect.bottom + box->top  + box->mb;

    return true;
}

bool foil_rdrbox_margin_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->cblock_rect.left   + box->left;
    rc->top    = box->cblock_rect.top    + box->top;
    rc->right  = box->cblock_rect.right  + box->left;
    rc->bottom = box->cblock_rect.bottom + box->top;

    return true;
}

bool foil_rdrbox_form_containing_block(const foil_rdrbox *box, foil_rect *rc)
{
    (void)box;
    (void)rc;

    return false;
}

const foil_rdrbox *
foil_rdrbox_find_container_for_relative(foil_layout_ctxt *ctxt,
        const foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;

    return ctxt->initial_cblock;
}

const foil_rdrbox *
foil_rdrbox_find_container_for_absolute(foil_layout_ctxt *ctxt,
        const foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;

    return NULL;
}

