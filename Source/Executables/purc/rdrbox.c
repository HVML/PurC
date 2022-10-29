/*
** @file rdrbox.c
** @author Vincent Wei
** @date 2022/10/10
** @brief The implementation of renderring box.
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

#include <assert.h>

int foil_rdrbox_module_init(pcmcth_renderer *rdr)
{
    (void)rdr;
    return 0;
}

void foil_rdrbox_module_cleanup(pcmcth_renderer *rdr)
{
    (void)rdr;
}

foil_rdrbox *foil_rdrbox_new(uint8_t type)
{
    foil_rdrbox *box = calloc(1, sizeof(*box));
    if (box == NULL)
        goto failed;

    box->type = type;
    switch (type) {
    case FOIL_RDRBOX_TYPE_BLOCK:
        box->block_data = calloc(1, sizeof(*box->block_data));
        if (box->block_data == NULL) {
            goto failed;
        }
        break;

    case FOIL_RDRBOX_TYPE_INLINE:
        box->inline_data = calloc(1, sizeof(*box->inline_data));
        if (box->inline_data == NULL) {
            goto failed;
        }
        break;

    case FOIL_RDRBOX_TYPE_LIST_ITEM:
        box->list_item_data = calloc(1, sizeof(*box->list_item_data));
        if (box->list_item_data == NULL) {
            goto failed;
        }
        break;

    case FOIL_RDRBOX_TYPE_MARKER:
        box->marker_data = calloc(1, sizeof(*box->marker_data));
        if (box->marker_data == NULL) {
            goto failed;
        }
        break;

    case FOIL_RDRBOX_TYPE_INLINE_BLOCK:
        box->inline_block_data = calloc(1, sizeof(*box->inline_block_data));
        if (box->inline_block_data == NULL) {
            goto failed;
        }
        break;

    default:
        // TODO:
        LOG_WARN("Not supported box type: %d\n", type);
        goto failed;
    }

    return box;

failed:
    if (box)
        free(box);
    return NULL;
}

void foil_rdrbox_delete(foil_rdrbox *box)
{
    foil_rdrbox_remove_from_tree(box);

    if (box->data) {
        if (box->cb_data_cleanup) {
            box->cb_data_cleanup(box->data);
        }

        free(box->data);
    }

    free(box);
}

void foil_rdrbox_append_child(foil_rdrbox *to, foil_rdrbox *box)
{
    if (to->last != NULL) {
        to->last->next = box;
    }
    else {
        to->first = box;
    }

    box->parent = to;
    box->next = NULL;
    box->prev = to->last;

    to->last = box;
}

void foil_rdrbox_prepend_child(foil_rdrbox *to, foil_rdrbox *box)
{
    if (to->first != NULL) {
        to->first->prev = box;
    }
    else {
        to->last = box;
    }

    box->parent = to;
    box->next = to->first;
    box->prev = NULL;

    to->first = box;
}

void foil_rdrbox_insert_before(foil_rdrbox *to, foil_rdrbox *box)
{
    if (to->prev != NULL) {
        to->prev->next = box;
    }
    else {
        if (to->parent != NULL) {
            to->parent->first = box;
        }
    }

    box->parent = to->parent;
    box->next = to;
    box->prev = to->prev;

    to->prev = box;
}

void foil_rdrbox_insert_after(foil_rdrbox *to, foil_rdrbox *box)
{
    if (to->next != NULL) {
        to->next->prev = box;
    }
    else {
        if (to->parent != NULL) {
            to->parent->last = box;
        }
    }

    box->parent = to->parent;
    box->next = to->next;
    box->prev = to;
    to->next = box;
}

void foil_rdrbox_remove_from_tree(foil_rdrbox *box)
{
    if (box->parent != NULL) {
        if (box->parent->first == box) {
            box->parent->first = box->next;
        }

        if (box->parent->last == box) {
            box->parent->last = box->prev;
        }
    }

    if (box->next != NULL) {
        box->next->prev = box->prev;
    }

    if (box->prev != NULL) {
        box->prev->next = box->next;
    }

    box->parent = NULL;
    box->next = NULL;
    box->prev = NULL;
}

void foil_rdrbox_delete_deep(foil_rdrbox *root)
{
    foil_rdrbox *tmp;
    foil_rdrbox *box = root;

    while (box) {
        if (box->first) {
            box = box->first;
        }
        else {
            while (box != root && box->next == NULL) {
                tmp = box->parent;
                foil_rdrbox_delete(box);
                box = tmp;
            }

            if (box == root) {
                foil_rdrbox_delete(box);
                break;
            }

            tmp = box->next;
            foil_rdrbox_delete(box);
            box = tmp;
        }
    }
}

#ifndef NDEBUG
static const char *literal_values_boxtype[] = {
    "inline",
    "block",
    "list-item",
    "marker",
    "inline-block",
    "table",
    "inline-table",
    "table-row_group",
    "table-header-group",
    "table-footer-group",
    "table-row",
    "table-column-group",
    "table-column",
    "table-cell",
    "table-caption",
};

static const char *literal_values_position[] = {
    "static",
    "relative",
    "absolute",
    "fixed",
    "sticky",
};

static const char *literal_values_float[] = {
    "none",
    "left",
    "right",
};

static const char *literal_values_direction[] = {
    "ltr",
    "rtl",
};

static const char *literal_values_visibility[] = {
    "visible",
    "hidden",
    "collapse",
};

static const char *literal_values_overflow[] = {
    "visible",
    "hidden",
    "scroll",
    "auto",
};

static const char *literal_values_unicode_bidi[] = {
    "normal",
    "embed",
    "isolate",
    "bidi_override",
    "isolate_override",
    "plaintext",
};

static const char *literal_values_text_transform[] = {
    "none",
    "capitalize",
    "uppercase",
    "lowercase",
};

static const char *literal_values_white_space[] = {
    "normal",
    "pre",
    "nowrap",
    "pre-wrap",
    "pre-line",
};

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

static const char *literal_values_word_break[] = {
    "normal",
    "keep-all",
    "break-all",
    "break-word",
};

static const char *literal_values_line_break[] = {
    "auto",
    "loose",
    "normal",
    "strict",
    "anywhere",
};

static const char *literal_values_word_wrap[] = {
    "normal",
    "break-word",
    "anywhere",
};

static const char *literal_values_list_style_type[] = {
    "disc",
    "circle",
    "square",
    "decimal",
    "decimal-leading-zero",
    "lower-roman",
    "upper-roman",
    "lower-greek",
    "lower-latin",
    "upper-latin",
    "armenian",
    "georgian",
    "none",
};

static const char *literal_values_list_style_position[] = {
    "outside",
    "inside",
};

#endif /* not defined NDEBUG */

#define INVALID_USED_VALUE_UINT8     0xFF

static uint8_t
display_to_type(foil_rendering_ctxt *ctxt, uint8_t computed)
{
    assert(ctxt->parent_box);

    if (computed == CSS_DISPLAY_INHERIT) {
        goto inherit;
    }
    else {
        static const struct uint8_values_map {
            uint8_t from;
            uint8_t to;
        } display_value_map[] = {
            { CSS_DISPLAY_INLINE, FOIL_RDRBOX_TYPE_INLINE },
            { CSS_DISPLAY_BLOCK, FOIL_RDRBOX_TYPE_BLOCK },
            { CSS_DISPLAY_LIST_ITEM, FOIL_RDRBOX_TYPE_LIST_ITEM },
            { CSS_DISPLAY_RUN_IN, FOIL_RDRBOX_TYPE_INLINE_BLOCK },
            { CSS_DISPLAY_INLINE_BLOCK, FOIL_RDRBOX_TYPE_INLINE_BLOCK },
            { CSS_DISPLAY_TABLE, FOIL_RDRBOX_TYPE_TABLE },
            { CSS_DISPLAY_INLINE_TABLE, FOIL_RDRBOX_TYPE_INLINE_TABLE },
            { CSS_DISPLAY_TABLE_ROW_GROUP,  FOIL_RDRBOX_TYPE_TABLE_ROW_GROUP },
            { CSS_DISPLAY_TABLE_HEADER_GROUP, FOIL_RDRBOX_TYPE_TABLE_HEADER_GROUP },
            { CSS_DISPLAY_TABLE_FOOTER_GROUP, FOIL_RDRBOX_TYPE_TABLE_FOOTER_GROUP },
            { CSS_DISPLAY_TABLE_ROW, FOIL_RDRBOX_TYPE_TABLE_ROW },
            { CSS_DISPLAY_TABLE_COLUMN_GROUP, FOIL_RDRBOX_TYPE_TABLE_COLUMN_GROUP },
            { CSS_DISPLAY_TABLE_COLUMN, FOIL_RDRBOX_TYPE_TABLE_COLUMN },
            { CSS_DISPLAY_TABLE_CELL, FOIL_RDRBOX_TYPE_TABLE_CELL },
            { CSS_DISPLAY_TABLE_CAPTION, FOIL_RDRBOX_TYPE_TABLE_CAPTION },
            { CSS_DISPLAY_NONE, INVALID_USED_VALUE_UINT8 },

            // TODO
            { CSS_DISPLAY_FLEX, FOIL_RDRBOX_TYPE_BLOCK },
            { CSS_DISPLAY_INLINE_FLEX, FOIL_RDRBOX_TYPE_INLINE_BLOCK },
            { CSS_DISPLAY_GRID, FOIL_RDRBOX_TYPE_BLOCK },
            { CSS_DISPLAY_INLINE_GRID, FOIL_RDRBOX_TYPE_INLINE_BLOCK },
        };

        int lower = 0;
        int upper = PCA_TABLESIZE(display_value_map) - 1;

        while (lower <= upper) {
            int mid = (lower + upper) >> 1;

            if (computed < display_value_map[mid].from)
                upper = mid - 1;
            else if (computed > display_value_map[mid].from)
                lower = mid + 1;
            else
                return display_value_map[mid].to;
        }
    }

inherit:
    return ctxt->parent_box->type;
}

static bool is_table_box(foil_rdrbox *box)
{
    return box->type >= FOIL_RDRBOX_TYPE_TABLE &&
        box->type <= FOIL_RDRBOX_TYPE_TABLE_CAPTION;
}

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

/* display, positionn, and float must be determined
   before calling this function */
static void dtrm_used_values_common_properties(foil_rendering_ctxt *ctxt,
        foil_rdrbox *box)
{
    uint8_t v;

    LOG_DEBUG("Common style properties of element (%s):\n", ctxt->tag_name);

    /* determine direction */
    v = css_computed_direction(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_DIRECTION_INHERIT) {
        box->direction = ctxt->parent_box->direction;
    }
    else if (v == CSS_DIRECTION_RTL) {
        box->direction = FOIL_RDRBOX_DIRECTION_RTL;
    }
    else {
        box->direction = FOIL_RDRBOX_DIRECTION_LTR;
    }

    LOG_DEBUG("\tdirection: %s\n", literal_values_direction[box->direction]);

    /* determine visibility */
    v = css_computed_visibility(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_VISIBILITY_INHERIT) {
        box->visibility = ctxt->parent_box->visibility;
    }
    else {
        switch (v) {
        case CSS_VISIBILITY_HIDDEN:
            box->visibility = FOIL_RDRBOX_VISIBILITY_HIDDEN;
            break;
        case CSS_VISIBILITY_COLLAPSE:
            box->visibility = FOIL_RDRBOX_VISIBILITY_COLLAPSE;
            break;
        case CSS_VISIBILITY_VISIBLE:
        default:
            box->visibility = FOIL_RDRBOX_VISIBILITY_VISIBLE;
            break;
        }
    }

    LOG_DEBUG("\tvisibility: %s\n",
            literal_values_visibility[box->visibility]);

    /* determine overflow_x */
    v = css_computed_overflow_x(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_OVERFLOW_INHERIT) {
        box->overflow_x = ctxt->parent_box->overflow_x;
    }
    else {
        switch (v) {
        case CSS_OVERFLOW_HIDDEN:
            box->overflow_x = FOIL_RDRBOX_OVERFLOW_HIDDEN;
            break;
        case CSS_OVERFLOW_SCROLL:
            box->overflow_x = FOIL_RDRBOX_OVERFLOW_SCROLL;
            break;
        case CSS_OVERFLOW_AUTO:
            if (is_table_box(box))
                box->overflow_x = FOIL_RDRBOX_OVERFLOW_AUTO;
            else
                box->overflow_x = FOIL_RDRBOX_OVERFLOW_AUTO;
            break;
        default:
            box->overflow_x = FOIL_RDRBOX_OVERFLOW_VISIBLE;
            break;
        }
    }

    LOG_DEBUG("\toverflow-x: %s\n", literal_values_overflow[box->overflow_x]);

    /* determine overflow_y */
    v = css_computed_overflow_y(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_OVERFLOW_INHERIT) {
        box->overflow_y = ctxt->parent_box->overflow_y;
    }
    else {
        switch (v) {
        case CSS_OVERFLOW_HIDDEN:
            box->overflow_y = FOIL_RDRBOX_OVERFLOW_HIDDEN;
            break;
        case CSS_OVERFLOW_SCROLL:
            box->overflow_y = FOIL_RDRBOX_OVERFLOW_SCROLL;
            break;
        case CSS_OVERFLOW_AUTO:
            if (is_table_box(box))
                box->overflow_x = FOIL_RDRBOX_OVERFLOW_AUTO;
            else
                box->overflow_x = FOIL_RDRBOX_OVERFLOW_AUTO;
            break;
        default:
            box->overflow_x = FOIL_RDRBOX_OVERFLOW_VISIBLE;
            break;
        }
    }

    LOG_DEBUG("\toverflow-y: %s\n", literal_values_overflow[box->overflow_y]);

    /* determine unicode_bidi */
    v = css_computed_unicode_bidi(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_UNICODE_BIDI_INHERIT) {
        box->unicode_bidi = ctxt->parent_box->unicode_bidi;
    }
    else {
        switch (v) {
        case CSS_UNICODE_BIDI_EMBED:
            box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_EMBED;
            break;
        case CSS_UNICODE_BIDI_ISOLATE:
            box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_ISOLATE;
            break;
        case CSS_UNICODE_BIDI_BIDI_OVERRIDE:
            box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_BIDI_OVERRIDE;
            break;
        case CSS_UNICODE_BIDI_ISOLATE_OVERRIDE:
            box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_ISOLATE_OVERRIDE;
            break;
        case CSS_UNICODE_BIDI_PLAINTEXT:
            box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_PLAINTEXT;
            break;
        case CSS_UNICODE_BIDI_NORMAL:
        default:
            box->unicode_bidi = FOIL_RDRBOX_UNICODE_BIDI_NORMAL;
            break;
        }
    }

    LOG_DEBUG("\tunicode-bidi: %s\n",
            literal_values_unicode_bidi[box->unicode_bidi]);

    /* determine text_transform */
    v = css_computed_text_transform(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_TEXT_TRANSFORM_INHERIT) {
        box->text_transform = ctxt->parent_box->text_transform;
    }
    else {
        switch (v) {
        case CSS_TEXT_TRANSFORM_CAPITALIZE:
            box->text_transform = FOIL_RDRBOX_TEXT_TRANSFORM_CAPITALIZE;
            break;
        case CSS_TEXT_TRANSFORM_UPPERCASE:
            box->text_transform = FOIL_RDRBOX_TEXT_TRANSFORM_UPPERCASE;
            break;
        case CSS_TEXT_TRANSFORM_LOWERCASE:
            box->text_transform = FOIL_RDRBOX_TEXT_TRANSFORM_LOWERCASE;
            break;
        case CSS_TEXT_TRANSFORM_NONE:
        default:
            box->text_transform = FOIL_RDRBOX_TEXT_TRANSFORM_NONE;
            break;
        }
    }

    LOG_DEBUG("\ttext-transform: %s\n",
            literal_values_text_transform[box->text_transform]);

    /* determine white-space */
    v = css_computed_white_space(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_WHITE_SPACE_INHERIT) {
        box->white_space = ctxt->parent_box->white_space;
    }
    else {
        switch (v) {
        case CSS_WHITE_SPACE_PRE:
            box->white_space = FOIL_RDRBOX_WHITE_SPACE_PRE;
            break;
        case CSS_WHITE_SPACE_NOWRAP:
            box->white_space = FOIL_RDRBOX_WHITE_SPACE_NOWRAP;
            break;
        case CSS_WHITE_SPACE_PRE_WRAP:
            box->white_space = FOIL_RDRBOX_WHITE_SPACE_PRE_WRAP;
            break;
        case CSS_WHITE_SPACE_PRE_LINE:
            box->white_space = FOIL_RDRBOX_WHITE_SPACE_PRE_LINE;
            break;
        case CSS_WHITE_SPACE_BREAK_SPACES:
            box->white_space = FOIL_RDRBOX_WHITE_SPACE_BREAK_SPACES;
            break;
        case CSS_WHITE_SPACE_NORMAL:
        default:
            box->white_space = FOIL_RDRBOX_WHITE_SPACE_NORMAL;
            break;
        }
    }

    LOG_DEBUG("\twhite-space: %s\n",
            literal_values_white_space[box->white_space]);

    /* determine text-decoration */
    v = css_computed_text_decoration(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_TEXT_DECORATION_INHERIT) {
        box->text_deco_underline = ctxt->parent_box->text_deco_underline;
        box->text_deco_overline = ctxt->parent_box->text_deco_overline;
        box->text_deco_line_through = ctxt->parent_box->text_deco_line_through;
        box->text_deco_blink = ctxt->parent_box->text_deco_blink;
    }
    else if (v != CSS_TEXT_DECORATION_NONE) {
        if (v & CSS_TEXT_DECORATION_BLINK)
            box->text_deco_blink = 1;
        if (v & CSS_TEXT_DECORATION_LINE_THROUGH)
            box->text_deco_line_through = 1;
        if (v & CSS_TEXT_DECORATION_OVERLINE)
            box->text_deco_overline = 1;
        if (v & CSS_TEXT_DECORATION_UNDERLINE)
            box->text_deco_underline = 1;
    }

    LOG_DEBUG("\ttext-decoration: blink/%s, line-through/%s, "
            "overline/%s, underline/%s\n",
            box->text_deco_blink ? "yes" : "no",
            box->text_deco_line_through ? "yes" : "no",
            box->text_deco_overline ? "yes" : "no",
            box->text_deco_underline ? "yes" : "no");

    css_fixed length;
    css_unit unit;

    /* determine letter-spacing */
    v = css_computed_letter_spacing(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &length, &unit);
    if (v == CSS_LETTER_SPACING_INHERIT) {
        box->letter_spacing = ctxt->parent_box->letter_spacing;
    }
    else if (v == CSS_LETTER_SPACING_SET) {
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
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &length, &unit);
    if (v == CSS_WORD_SPACING_INHERIT) {
        box->word_spacing = ctxt->parent_box->word_spacing;
    }
    else if (v == CSS_WORD_SPACING_SET) {
        box->word_spacing = calc_used_value_widths(box, unit, length);
    }
    /* CSS_WORD_SPACING_NORMAL */

    if (box->word_spacing < 0) {
        box->word_spacing = 0;
    }

    LOG_DEBUG("\tword-spacing: %d\n", box->word_spacing);

    if (box->is_block_container) {
        /* determine text-indent */
        v = css_computed_text_indent(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &length, &unit);
        if (v == CSS_TEXT_INDENT_INHERIT) {
            box->text_indent = ctxt->parent_box->text_indent;
        }
        else {
            box->text_indent = calc_used_value_widths(box, unit, length);
        }

        if (box->text_indent < 0)
            box->text_indent = 0;

        LOG_DEBUG("\ttext-indent: %d\n", box->text_indent);

        /* determine text-align */
        v = css_computed_text_align(
                ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
        if (v == CSS_TEXT_ALIGN_INHERIT) {
            box->text_align = ctxt->parent_box->text_align;
        }
        else {
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
        }

        LOG_DEBUG("\ttext-align: %s\n",
                literal_values_text_align[box->text_align]);

        /* determine text-overflow.
           Note that CSSEng has a wrong interface for text-overflow */
        lwc_string *string;
        v = css_computed_text_overflow(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE], &string);
        if (v == CSS_TEXT_OVERFLOW_ELLIPSIS)
            box->text_overflow = FOIL_RDRBOX_TEXT_OVERFLOW_ELLIPSIS;
        else
            box->text_overflow = FOIL_RDRBOX_TEXT_OVERFLOW_CLIP;

        LOG_DEBUG("\ttext-overflow: %s\n",
                literal_values_text_overflow[box->text_overflow]);
    }

    /* determine word-break */
    v = css_computed_word_break(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_WORD_BREAK_INHERIT)
        box->word_break = ctxt->parent_box->word_break;
    else {
        switch (v) {
        case CSS_WORD_BREAK_BREAK_WORD:
            /* this is a legacy keyword */
            box->word_break = FOIL_RDRBOX_WORD_BREAK_NORMAL;
            break;

        case CSS_WORD_BREAK_BREAK_ALL:
            box->word_break = FOIL_RDRBOX_WORD_BREAK_BREAK_ALL;
            break;

        case CSS_WORD_BREAK_KEEP_ALL:
            box->word_break = FOIL_RDRBOX_WORD_BREAK_KEEP_ALL;
            break;

        case CSS_WORD_BREAK_NORMAL:
        default:
            box->word_break = FOIL_RDRBOX_WORD_BREAK_NORMAL;
            break;
        }
    }

    LOG_DEBUG("\tword-break: %s\n",
            literal_values_word_break[box->word_break]);

    /* determine line-break */
    v = css_computed_line_break(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_LINE_BREAK_INHERIT)
        box->line_break = ctxt->parent_box->line_break;
    else {
        switch (v) {
        case CSS_LINE_BREAK_LOOSE:
            box->line_break = FOIL_RDRBOX_LINE_BREAK_LOOSE;
            break;

        case CSS_LINE_BREAK_NORMAL:
            box->line_break = FOIL_RDRBOX_LINE_BREAK_NORMAL;
            break;

        case CSS_LINE_BREAK_STRICT:
            box->line_break = FOIL_RDRBOX_LINE_BREAK_STRICT;
            break;

        case CSS_LINE_BREAK_ANYWHERE:
            box->line_break = FOIL_RDRBOX_LINE_BREAK_ANYWHERE;
            break;

        case CSS_LINE_BREAK_AUTO:
        default:
            box->line_break = FOIL_RDRBOX_LINE_BREAK_AUTO;
            break;
        }
    }

    LOG_DEBUG("\tline-break: %s\n",
            literal_values_line_break[box->line_break]);

    /* determine word-wrap */
    v = css_computed_word_wrap(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_WORD_WRAP_INHERIT)
        box->word_wrap = ctxt->parent_box->word_wrap;
    else {
        switch (v) {
        case CSS_WORD_WRAP_BREAK_WORD:
            box->word_wrap = FOIL_RDRBOX_WORD_WRAP_BREAK_WORD;
            break;

        case CSS_WORD_WRAP_ANYWHERE:
            box->word_wrap = FOIL_RDRBOX_WORD_WRAP_ANYWHERE;
            break;

        case CSS_WORD_WRAP_NORMAL:
        default:
            box->word_wrap = FOIL_RDRBOX_WORD_WRAP_NORMAL;
            break;
        }
    }

    LOG_DEBUG("\tword-wrap: %s\n",
            literal_values_word_wrap[box->word_wrap]);

    /* determine list-style-type
       (Foil always assumes list-style-image is `none`) */
    v = css_computed_list_style_type(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_LIST_STYLE_TYPE_INHERIT)
        box->list_style_type = ctxt->parent_box->list_style_type;
    else {
        switch (v) {
        default:
        case CSS_LIST_STYLE_TYPE_DISC:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_DISC;
            break;

        case CSS_LIST_STYLE_TYPE_CIRCLE:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_CIRCLE;
            break;

        case CSS_LIST_STYLE_TYPE_SQUARE:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_SQUARE;
            break;

        case CSS_LIST_STYLE_TYPE_DECIMAL:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL;
            break;

        case CSS_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO;
            break;

        case CSS_LIST_STYLE_TYPE_LOWER_ROMAN:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ROMAN;
            break;

        case CSS_LIST_STYLE_TYPE_UPPER_ROMAN:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ROMAN;
            break;

        case CSS_LIST_STYLE_TYPE_LOWER_GREEK:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_GREEK;
            break;

        case CSS_LIST_STYLE_TYPE_LOWER_ALPHA:
        case CSS_LIST_STYLE_TYPE_LOWER_LATIN:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_LATIN;
            break;

        case CSS_LIST_STYLE_TYPE_UPPER_ALPHA:
        case CSS_LIST_STYLE_TYPE_UPPER_LATIN:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_LATIN;
            break;

        case CSS_LIST_STYLE_TYPE_ARMENIAN:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_ARMENIAN;
            break;

        case CSS_LIST_STYLE_TYPE_GEORGIAN:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN;
            break;

        case CSS_LIST_STYLE_TYPE_NONE:
            box->list_style_type = FOIL_RDRBOX_LIST_STYLE_TYPE_NONE;
            break;
        }
    }

    LOG_DEBUG("\tlist-style-type: %s\n",
            literal_values_list_style_type[box->list_style_type]);

    /* determine word-wrap */
    v = css_computed_list_style_position(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_LIST_STYLE_POSITION_INHERIT)
        box->list_style_position = ctxt->parent_box->list_style_position;
    else {
        switch (v) {
        default:
        case CSS_LIST_STYLE_POSITION_OUTSIDE:
            box->list_style_position = FOIL_RDRBOX_LIST_STYLE_POSITION_OUTSIDE;
            break;

        case CSS_LIST_STYLE_POSITION_INSIDE:
            box->list_style_position = FOIL_RDRBOX_LIST_STYLE_POSITION_INSIDE;
            break;
        }
    }

    LOG_DEBUG("\tlist-style-position: %s\n",
            literal_values_list_style_position[box->list_style_position]);

    /* determine foreground color */
    css_color color_argb;
    v = css_computed_color(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &color_argb);
    if (v == CSS_COLOR_INHERIT)
        box->fgc = ctxt->parent_box->fgc;
    else
        box->fgc = color_argb;

    LOG_DEBUG("\tcolor: 0x%08x\n", box->fgc);

    /* determine background color */
    v = css_computed_background_color(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &color_argb);
    if (v == CSS_COLOR_INHERIT)
        box->bgc = ctxt->parent_box->bgc;
    else
        box->bgc = color_argb;

    LOG_DEBUG("\tbackground-color: 0x%08x\n", box->bgc);

}

static void
dtrm_margin_left_right(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t value;
    css_fixed length;
    css_unit unit;
    value = css_computed_margin_left(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &length, &unit);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->ml = 0;
            break;
        case CSS_MARGIN_INHERIT:
            box->ml = ctxt->parent_box->ml;
            break;
        case CSS_MARGIN_SET:
            box->ml = calc_used_value_widths(box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }

    value = css_computed_margin_right(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &length, &unit);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mr = 0;
            break;
        case CSS_MARGIN_INHERIT:
            box->mr = ctxt->parent_box->mr;
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
dtrm_margin_top_bottom(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t value;
    css_fixed length;
    css_unit unit;
    value = css_computed_margin_top(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &length, &unit);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mt = 0;
            break;
        case CSS_MARGIN_INHERIT:
            box->mt = ctxt->parent_box->mt;
            break;
        case CSS_MARGIN_SET:
            box->mr = calc_used_value_heights(box, unit, length);
            break;
        default:
            assert(0);  // must be a bug
            break;
    }

    value = css_computed_margin_bottom(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &length, &unit);
    switch (value) {
        case CSS_MARGIN_AUTO:
            box->mb = 0;
            break;
        case CSS_MARGIN_INHERIT:
            box->mb = ctxt->parent_box->mb;
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
get_intrinsic_width(foil_rendering_ctxt *ctxt)
{
    int l = 0;
    const char *value;
    size_t len;

    if (pcdoc_element_get_attribute(ctxt->doc, ctxt->elem,
            "width", &value, &len) == 0) {

        char *v = strndup(value, len);
        l = strtol(v, NULL, 10);
        free(v);
    }

    return (int)l;
}

static int
get_intrinsic_height(foil_rendering_ctxt *ctxt)
{
    int l = 0;
    const char *value;
    size_t len;

    if (pcdoc_element_get_attribute(ctxt->doc, ctxt->elem,
            "height", &value, &len) == 0) {

        char *v = strndup(value, len);
        l = strtol(v, NULL, 10);
        free(v);
    }

    return (int)l;
}

static int
get_intrinsic_ratio(foil_rendering_ctxt *ctxt)
{
    (void)ctxt;

    return 2.0f; // always assume the instrinsic ratio is 2:1
}

static uint8_t
dtrm_width_replaced(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v, height_v;
    css_fixed width_l, height_l;
    css_unit width_u, height_u;

    assert(box->is_replaced);

    width_v = css_computed_width(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &width_l, &width_u);

    if (width_v != CSS_WIDTH_AUTO) {
        if (width_v == CSS_HEIGHT_INHERIT)
            box->width = ctxt->parent_box->width;
        box->width = calc_used_value_widths(box, width_u, width_l);

        return width_v;
    }

    height_v = css_computed_height(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &height_l, &height_u);

    int intrinsic_width = get_intrinsic_width(ctxt);
    int intrinsic_height = get_intrinsic_height(ctxt);
    float intrinsic_ratio = get_intrinsic_ratio(ctxt);

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
        if (height_v == CSS_HEIGHT_INHERIT)
            height = ctxt->parent_box->height;
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
dtrm_height_replaced(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v, height_v;
    css_fixed width_l, height_l;
    css_unit width_u, height_u;

    assert(box->is_replaced);

    height_v = css_computed_height(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &height_l, &height_u);
    if (height_v != CSS_WIDTH_AUTO) {
        if (height_v == CSS_HEIGHT_INHERIT)
            box->height = ctxt->parent_box->height;
        box->height = calc_used_value_heights(box, height_u, height_l);

        return height_v;
    }

    width_v = css_computed_width(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &width_l, &width_u);

    int intrinsic_height = get_intrinsic_height(ctxt);
    float intrinsic_ratio = get_intrinsic_ratio(ctxt);

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
dtrm_width_shrink_to_fit(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    uint8_t width_v;
    css_fixed width_l;
    css_unit width_u;

    width_v = css_computed_width(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &width_l, &width_u);

    if (width_v != CSS_WIDTH_AUTO) {
        if (width_v == CSS_HEIGHT_INHERIT)
            box->width = ctxt->parent_box->width;
        box->width = calc_used_value_widths(box, width_u, width_l);

        return width_v;
    }

    /* TODO */
    LOG_WARN("Not implemented\n");
    box->width = FOIL_PX_GRID_CELL_W * 10;
    return CSS_WIDTH_SET;
}

static void
dtrm_margin_left_right_block_normal(foil_rendering_ctxt *ctxt,
        foil_rdrbox *box, uint8_t width_v)
{
    int nr_autos = 0;
    int cblock_width = foil_rect_width(&box->cblock_rect);

    if (width_v == CSS_WIDTH_AUTO)
        nr_autos++;

    uint8_t padding_left_v;
    css_fixed padding_left_l;
    css_unit padding_left_u;
    padding_left_v = css_computed_padding_left(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &padding_left_l, &padding_left_u);
    if (padding_left_v == CSS_PADDING_INHERIT)
        box->pl = ctxt->parent_box->pl;
    else
        box->pl = calc_used_value_widths(box, padding_left_u, padding_left_l);

    uint8_t padding_right_v;
    css_fixed padding_right_l;
    css_unit padding_right_u;
    padding_right_v = css_computed_padding_right(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &padding_right_l, &padding_right_u);
    if (padding_right_v == CSS_PADDING_INHERIT)
        box->pr = ctxt->parent_box->pr;
    else
        box->pr = calc_used_value_widths(box, padding_right_u, padding_right_l);

    uint8_t margin_left_v;
    css_fixed margin_left_l;
    css_unit margin_left_u;
    margin_left_v = css_computed_margin_left(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &margin_left_l, &margin_left_u);
    if (margin_left_v == CSS_MARGIN_AUTO) {
        nr_autos++;
    }
    else if (margin_left_v == CSS_MARGIN_INHERIT) {
        box->ml = ctxt->parent_box->ml;
    }
    else {
        box->ml = calc_used_value_widths(box, margin_left_u, margin_left_l);
    }

    uint8_t margin_right_v;
    css_fixed margin_right_l;
    css_unit margin_right_u;
    margin_right_v = css_computed_margin_right(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &margin_right_l, &margin_right_u);
    if (margin_right_v == CSS_MARGIN_AUTO) {
        nr_autos++;
    }
    else if (margin_right_v == CSS_MARGIN_INHERIT) {
        box->mr = ctxt->parent_box->mr;
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
calc_widths_margins(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
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
            width_v = css_computed_width(
                    ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
                    &width_l, &width_u);
            if (width_v == CSS_WIDTH_AUTO) {
                box->width = 0;
            }
            else if (width_v == CSS_WIDTH_INHERIT) {
                box->width = ctxt->parent_box->width;
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
calc_heights_margins(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
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
        uint8_t height_v = css_computed_height(
                ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
                &height_l, &height_u);

        if (height_v != CSS_WIDTH_AUTO) {
            if (height_v == CSS_HEIGHT_INHERIT)
                box->height = ctxt->parent_box->height;
            box->height = calc_used_value_heights(box, height_u, height_l);
        }
        else {
            if (box->overflow_y == CSS_OVERFLOW_VISIBLE) {
                dtrm_margin_top_bottom(ctxt, box);

                // set height is pending.
                box->height_pending = 1;
            }
        }
    }
}

/* adjust position according to 'vertical-align' */
static void
adjust_position_vertically(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
}

/* TODO: check whether an element is replaced or non-replaced */
static int
is_replaced_element(pcdoc_element_t elem, const char *tag_name)
{
    (void)elem;
    (void)tag_name;
    return 0;
}

foil_rdrbox *foil_rdrbox_create_principal(foil_rendering_ctxt *ctxt)
{
    pcdoc_node node = { PCDOC_NODE_ELEMENT, { ctxt->elem } };
    const char *name;
    size_t len;
    foil_rdrbox *box = NULL;

    pcdoc_element_get_tag_name(ctxt->doc, ctxt->elem, &name, &len,
            NULL, NULL, NULL, NULL);
    assert(name != NULL && len > 0);
    ctxt->tag_name = strndup(name, len);

    LOG_DEBUG("Layout style properties of element (%s):\n", ctxt->tag_name);

    /* determine the box type */
    uint8_t display = css_computed_display(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            pcdoc_node_get_parent(ctxt->doc, node) == NULL);

    // return INVALID_USED_VALUE_UINT8 for 'display:none;'
    uint8_t type = display_to_type(ctxt, display);
    if (type == INVALID_USED_VALUE_UINT8) {
        LOG_DEBUG("\tdisplay: %s\n", "none");
        goto failed;
    }

    LOG_DEBUG("\ttype: %s\n", literal_values_boxtype[type]);

    /* allocate the principal box */
    box = foil_rdrbox_new(type);
    if (box == NULL)
        goto failed;

    box->owner = ctxt->elem;
    box->is_principal = 1;
    box->is_replaced = is_replaced_element(ctxt->elem, ctxt->tag_name);

    uint8_t v;
    v = css_computed_position(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_POSITION_INHERIT) {
        box->position = ctxt->parent_box->position;
    }
    else {
        switch (v) {
        case CSS_POSITION_RELATIVE:
            box->position = FOIL_RDRBOX_POSITION_RELATIVE;
            break;

        case CSS_POSITION_ABSOLUTE:
            box->position = FOIL_RDRBOX_POSITION_ABSOLUTE;
            break;

        case CSS_POSITION_FIXED:
            box->position = FOIL_RDRBOX_POSITION_FIXED;
            break;

        /* CSSEng does not support position: sticky so far
        case CSS_POSITION_STICKY:
            box->position = FOIL_RDRBOX_POSITION_STICKY;
            break; */

        case CSS_POSITION_STATIC:
        default:
            box->position = FOIL_RDRBOX_POSITION_STATIC;
            break;
        }
    }

    LOG_DEBUG("\tposition: %s\n", literal_values_position[box->position]);

    /* determine float */
    v = css_computed_float(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    if (v == CSS_FLOAT_INHERIT) {
        box->floating = ctxt->parent_box->floating;
    }
    else {
        switch (v) {
        case CSS_FLOAT_LEFT:
            box->floating = FOIL_RDRBOX_FLOAT_LEFT;
            break;

        case CSS_FLOAT_RIGHT:
            box->floating = FOIL_RDRBOX_FLOAT_RIGHT;
            break;

        case CSS_FLOAT_NONE:
        default:
            box->floating = FOIL_RDRBOX_FLOAT_NONE;
            break;
        }
    }

    LOG_DEBUG("\tfloat: %s\n", literal_values_float[box->floating]);

    /* TODO: determine the positioning schema */

    /* determine the containing block */
    if (purc_document_root(ctxt->doc) == ctxt->elem) {
        box->cblock_rect.left = 0;
        box->cblock_rect.top = 0;
        box->cblock_rect.right = ctxt->initial_cblock->width;
        box->cblock_rect.bottom = ctxt->initial_cblock->height;
        box->cblock_creator = ctxt->initial_cblock;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_STATIC ||
            box->position == FOIL_RDRBOX_POSITION_RELATIVE) {
        /* the containing block is formed by the content edge of
           the nearest ancestor box that is a block container or
           which establishes a formatting context. */

        const foil_rdrbox *container;
        container = foil_rdrbox_find_container_for_relative(ctxt,
                ctxt->parent_box);
        assert(container);
        foil_rdrbox_content_box(container, &box->cblock_rect);
        box->cblock_creator = container;
    }
    else if (box->position == FOIL_RDRBOX_POSITION_FIXED) {
        box->cblock_rect.left = 0;
        box->cblock_rect.top = 0;
        box->cblock_rect.right = ctxt->initial_cblock->width;
        box->cblock_rect.bottom = ctxt->initial_cblock->height;
        box->cblock_creator = ctxt->initial_cblock;
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
        container = foil_rdrbox_find_container_for_absolute(ctxt,
                ctxt->parent_box);
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
            box->cblock_rect.right = ctxt->initial_cblock->width;
            box->cblock_rect.bottom = ctxt->initial_cblock->height;
        }
        box->cblock_creator = ctxt->initial_cblock;
    }

    /* determine the used values for common properties */
    dtrm_used_values_common_properties(ctxt, box);

    /* calculate widths and margins */
    calc_widths_margins(ctxt, box);

    /* calculate heights and margins */
    calc_heights_margins(ctxt, box);

    /* adjust position according to 'vertical-align' */
    adjust_position_vertically(ctxt, box);

    if (ctxt->tag_name)
        free(ctxt->tag_name);

    foil_rdrbox_append_child(ctxt->parent_box, box);

    if (type == FOIL_RDRBOX_TYPE_LIST_ITEM) {
        box->list_item_data->index = ctxt->parent_box->nr_child_list_items;
        ctxt->parent_box->nr_child_list_items++;

        if (box->list_style_type != FOIL_RDRBOX_LIST_STYLE_TYPE_NONE) {
            // allocate the marker box
            foil_rdrbox *marker_box = foil_rdrbox_new(FOIL_RDRBOX_TYPE_MARKER);
            if (marker_box == NULL)
                goto failed;

            foil_rdrbox_insert_before(box, marker_box);
            box->list_item_data->marker_box = marker_box;
        }
    }

    return box;

failed:
    if (ctxt->tag_name)
        free(ctxt->tag_name);

    if (box)
        foil_rdrbox_delete(box);

    return NULL;
}

foil_rdrbox *foil_rdrbox_create_anonymous_block(foil_rendering_ctxt *ctxt,
        foil_rdrbox *parent)
{
    foil_rdrbox *box;

    box = foil_rdrbox_new(FOIL_RDRBOX_TYPE_BLOCK);
    if (box == NULL)
        goto failed;

    box->owner = ctxt->elem;
    box->is_anonymous = 1;

    foil_rdrbox_append_child(parent, box);
    return box;

failed:
    return NULL;
}

/* create an anonymous inline box */
foil_rdrbox *foil_rdrbox_create_anonymous_inline(foil_rendering_ctxt *ctxt,
        foil_rdrbox *parent)
{
    foil_rdrbox *box;

    box = foil_rdrbox_new(FOIL_RDRBOX_TYPE_INLINE);
    if (box == NULL)
        goto failed;

    box->owner = ctxt->elem;
    box->is_anonymous = 1;

    foil_rdrbox_append_child(parent, box);
    return box;

failed:
    return NULL;
}

bool foil_rdrbox_init_data(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    if (box->type == FOIL_RDRBOX_TYPE_LIST_ITEM &&
            box->list_item_data->marker_box) {
        if (!foil_rdrbox_init_marker_data(ctxt,
                    box->list_item_data->marker_box, box)) {
            LOG_ERROR("Failed to initialize marker box\n");
            goto failed;
        }
    }

    return true;

failed:
    return false;
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
foil_rdrbox_find_container_for_relative(foil_rendering_ctxt *ctxt,
        const foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;

    return ctxt->initial_cblock;
}

const foil_rdrbox *
foil_rdrbox_find_container_for_absolute(foil_rendering_ctxt *ctxt,
        const foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;

    return NULL;
}

