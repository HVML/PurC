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

#include <assert.h>

struct _text_segment {
    struct list_head ln;

    unsigned i; // the index of first character
    unsigned n; // number of characters in this segment

    /* position of this segment in the containing block box */
    int x, y;

    /* rows taken by this segment (always be 1). */
    unsigned height;
    /* columns taken by this segment. */
    unsigned width;
};

struct _inline_box_data {
    /* the code points of text in Unicode (should be in visual order) */
    uint32_t *ucs;

    int letter_spacing;
    int word_spacing;

    /* text color */
    int color;

    /* the text segments */
    struct list_head segs;
};

struct _block_box_data {
    int text_indent;

    uint8_t text_align:3;
};

struct _inline_block_data {
    int foo, bar;

    uint8_t text_align:3;
};

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

void foil_rdrbox_delete(foil_rdrbox *box)
{
    foil_rdrbox_remove_from_tree(box);
    free(box->data);
    free(box);
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
    "INLINE",
    "BLOCK",
    "LIST_ITEM",
    "MARKER",
    "INLINE_BLOCK",
    "TABLE",
    "INLINE_TABLE",
    "TABLE_ROW_GROUP",
    "TABLE_HEADER_GROUP",
    "TABLE_FOOTER_GROUP",
    "TABLE_ROW",
    "TABLE_COLUMN_GROUP",
    "TABLE_COLUMN",
    "TABLE_CELL",
    "TABLE_CAPTION",
};

static const char *literal_values_position[] = {
    "STATIC",
    "RELATIVE",
    "ABSOLUTE",
    "FIXED",
    "STICKY",
};
#endif

#define INVALID_USED_VALUE_UINT8     0xFF

static uint8_t
used_value_display(foil_rendering_ctxt *ctxt, uint8_t computed)
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

static uint8_t used_value_position(foil_rendering_ctxt *ctxt,
        uint8_t computed)
{
    switch (computed) {
        case CSS_POSITION_STATIC:
            return FOIL_RDRBOX_POSITION_STATIC;

        case CSS_POSITION_RELATIVE:
            return FOIL_RDRBOX_POSITION_RELATIVE;

        case CSS_POSITION_ABSOLUTE:
            return FOIL_RDRBOX_POSITION_ABSOLUTE;

        case CSS_POSITION_FIXED:
            return FOIL_RDRBOX_POSITION_FIXED;

        default:
            break;
    }

    return ctxt->parent_box->position;
}

/* calculate widths and margins */
static void
calc_widths_margins(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
}

/* calculate heights and margins */
static void
calc_heights_margins(foil_rendering_ctxt *ctxt, foil_rdrbox *box)
{
    (void)ctxt;
    (void)box;
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

foil_rdrbox *foil_rdrbox_create(foil_rendering_ctxt *ctxt)
{
    pcdoc_node node = { PCDOC_NODE_ELEMENT, { ctxt->elem } };
    const char *name;
    size_t len;
    foil_rdrbox *box = NULL;

    pcdoc_element_get_tag_name(ctxt->doc, ctxt->elem, &name, &len,
            NULL, NULL, NULL, NULL);
    assert(name != NULL && len > 0);
    ctxt->tag_name = strndup(name, len);

    LOG_DEBUG("Styles of element (%s):\n", ctxt->tag_name);

    /* determine the box type */
    uint8_t display = css_computed_display(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            pcdoc_node_get_parent(ctxt->doc, node) == NULL);

    // return INVALID_USED_VALUE_UINT8 for 'display:none;'
    uint8_t type = used_value_display(ctxt, display);
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

    uint8_t position = css_computed_position(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE]);
    box->position = used_value_position(ctxt, position);
    LOG_DEBUG("\tposition: %s\n", literal_values_position[box->position]);

    /* determine the containing block */
    if (purc_document_root(ctxt->doc) == ctxt->elem) {
        box->containing_block.left = 0;
        box->containing_block.top = 0;
        box->containing_block.right = ctxt->initial_cblock->width;
        box->containing_block.bottom = ctxt->initial_cblock->height;
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
        foil_rdrbox_content_box(container, &box->containing_block);
    }
    else if (box->position == FOIL_RDRBOX_POSITION_FIXED) {
        box->containing_block.left = 0;
        box->containing_block.top = 0;
        box->containing_block.right = ctxt->initial_cblock->width;
        box->containing_block.bottom = ctxt->initial_cblock->height;
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
                        &box->containing_block);
            else
                foil_rdrbox_padding_box(container, &box->containing_block);
        }
        else {
            box->containing_block.left = 0;
            box->containing_block.top = 0;
            box->containing_block.right = ctxt->initial_cblock->width;
            box->containing_block.bottom = ctxt->initial_cblock->height;
        }
    }

    /* calculate widths and margins */
    calc_widths_margins(ctxt, box);

    /* calculate heights and margins */
    calc_heights_margins(ctxt, box);

    /* adjust position according to 'vertical-align' */
    adjust_position_vertically(ctxt, box);

    /* determine foreground color */
    css_color color_argb;
    uint8_t color_type = css_computed_color(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &color_argb);
    if (color_type == CSS_COLOR_INHERIT)
        box->fgc = ctxt->parent_box->fgc;
    else
        box->fgc = color_argb;

    LOG_DEBUG("\tcolor: 0x%08x\n", box->fgc);

    /* determine background color */
    color_type = css_computed_background_color(
            ctxt->computed->styles[CSS_PSEUDO_ELEMENT_NONE],
            &color_argb);
    if (color_type == CSS_COLOR_INHERIT)
        box->bgc = ctxt->parent_box->bgc;
    else
        box->bgc = color_argb;

    LOG_DEBUG("\tbackground color: 0x%08x\n", box->bgc);

    if (ctxt->tag_name)
        free(ctxt->tag_name);

    foil_rdrbox_append_child(ctxt->parent_box, box);

    /* TODO
    if (type == FOIL_RDRBOX_TYPE_LIST_ITEM) {
        // allocate the marker box
        box = foil_rdrbox_new(FOIL_RDRBOX_TYPE_MARKER);
        if (box == NULL)
            goto failed;
        box->owner = ctxt->elem;
        box->is_anonymous = 1;
    } */

    return box;

failed:
    if (ctxt->tag_name)
        free(ctxt->tag_name);

    if (box)
        foil_rdrbox_delete(box);

    return NULL;
}

bool foil_rdrbox_content_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->containing_block.left   + box->left +
        box->ml + box->bl + box->pl;
    rc->top    = box->containing_block.top    + box->top  +
        box->mt + box->bt + box->pt;
    rc->right  = box->containing_block.right  + box->left +
        box->mr + box->br + box->pr;
    rc->bottom = box->containing_block.bottom + box->top  +
        box->mb + box->bb + box->pb;
    return true;
}

bool foil_rdrbox_padding_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->containing_block.left   + box->left + box->ml + box->bl;
    rc->top    = box->containing_block.top    + box->top  + box->mt + box->bt;
    rc->right  = box->containing_block.right  + box->left + box->mr + box->br;
    rc->bottom = box->containing_block.bottom + box->top  + box->mb + box->bb;

    return true;
}

bool foil_rdrbox_border_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->containing_block.left   + box->left + box->ml;
    rc->top    = box->containing_block.top    + box->top  + box->mt;
    rc->right  = box->containing_block.right  + box->left + box->mr;
    rc->bottom = box->containing_block.bottom + box->top  + box->mb;

    return true;
}

bool foil_rdrbox_margin_box(const foil_rdrbox *box, foil_rect *rc)
{
    if (box->type == FOIL_RDRBOX_TYPE_INLINE)
        return false;

    rc->left   = box->containing_block.left   + box->left;
    rc->top    = box->containing_block.top    + box->top;
    rc->right  = box->containing_block.right  + box->left;
    rc->bottom = box->containing_block.bottom + box->top;

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

