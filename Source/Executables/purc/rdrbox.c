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
used_value_display(struct foil_rendering_ctxt *ctxt, uint8_t computed)
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

static uint8_t used_value_position(struct foil_rendering_ctxt *ctxt,
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

foil_rdrbox *foil_rdrbox_create(struct foil_rendering_ctxt *ctxt,
        pcdoc_element_t elem, css_select_results *result)
{
    pcdoc_node node = { PCDOC_NODE_ELEMENT, { elem } };
    const char *name;
    size_t len;
    char *tag_name = NULL;
    foil_rdrbox *box = NULL;

    pcdoc_element_get_tag_name(ctxt->doc, elem, &name, &len,
            NULL, NULL, NULL, NULL);
    assert(name != NULL && len > 0);
    tag_name = strndup(name, len);

    LOG_DEBUG("Styles of element (%s):\n", tag_name);

    /* determine the box type */
    uint8_t display = css_computed_display(
            result->styles[CSS_PSEUDO_ELEMENT_NONE],
            pcdoc_node_get_parent(ctxt->doc, node) == NULL);

    // return INVALID_USED_VALUE_UINT8 for 'display:none;'
    uint8_t type = used_value_display(ctxt, display);
    if (type == INVALID_USED_VALUE_UINT8) {
        LOG_DEBUG("\tdisplay: %s\n", "none");
        goto failed;
    }

    LOG_DEBUG("\ttype: %s\n", literal_values_boxtype[type]);

    /* allocate a new rdrbox */
    box = foil_rdrbox_new(type);
    if (box == NULL)
        goto failed;

    uint8_t position = css_computed_position(
            result->styles[CSS_PSEUDO_ELEMENT_NONE]);
    box->position = used_value_position(ctxt, position);
    LOG_DEBUG("\tposition: %s\n", literal_values_position[box->position]);

    /* determine the containing block */
    if (purc_document_root(ctxt->doc) == elem) {
        box->containing_block.left = 0;
        box->containing_block.top = 0;
        box->containing_block.right = ctxt->initial_cblock->width;
        box->containing_block.bottom = ctxt->initial_cblock->height;
    }
    else {
    }

    uint8_t color_type;
    css_color color_argb;
    color_type = css_computed_color(
            result->styles[CSS_PSEUDO_ELEMENT_NONE],
            &color_argb);
    if (color_type == CSS_COLOR_INHERIT)
        purc_log_info("\tcolor: 'inherit'\n");
    else
        purc_log_info("\tcolor: 0x%08x\n", color_argb);

    if (tag_name)
        free(tag_name);

    foil_rdrbox_append_child(ctxt->parent_box, box);
    return box;

failed:
    if (tag_name)
        free(tag_name);

    if (box)
        foil_rdrbox_delete(box);

    return NULL;
}


