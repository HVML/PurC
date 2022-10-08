/*
** @file udom.c
** @author Vincent Wei
** @date 2022/10/06
** @brief The implementation of uDOM (the rendering tree).
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

#include <assert.h>

#include "foil.h"
#include "udom.h"
#include "util/sorted-array.h"
#include "util/list.h"

typedef struct foil_rect {
    int left, top;
    int right, bottom;
} foil_rect;

typedef enum {
    PCTH_RDR_BOX_TYPE_INLINE,
    PCTH_RDR_BOX_TYPE_BLOCK,
    PCTH_RDR_BOX_TYPE_INLINE_BLOCK,
    PCTH_RDR_BOX_TYPE_MARKER,
} pcth_rdr_box_t;

typedef enum {
    PCTH_RDR_ALIGN_LEFT,
    PCTH_RDR_ALIGN_RIGHT,
    PCTH_RDR_ALIGN_CENTER,
    PCTH_RDR_ALIGN_JUSTIFY,
} pcth_rdr_align_t;

typedef enum {
    PCTH_RDR_DECORATION_NONE,
    PCTH_RDR_DECORATION_UNDERLINE,
    PCTH_RDR_DECORATION_OVERLINE,
    PCTH_RDR_DECORATION_LINE_THROUGH,
    PCTH_RDR_DECORATION_BLINK,
} pcth_rdr_decoration_t;

typedef enum {
    PCTH_RDR_WHITE_SPACE_NORMAL,
    PCTH_RDR_WHITE_SPACE_PRE,
    PCTH_RDR_WHITE_SPACE_NOWRAP,
    PCTH_RDR_WHITE_SPACE_PRE_WRAP,
    PCTH_RDR_WHITE_SPACE_PRE_LINE,
} pcth_rdr_white_space_t;

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
    /* text decoration */
    pcth_rdr_decoration_t deco;

    /* the text segments */
    struct list_head segs;
};

struct _block_box_data {
    // margins
    int ml, mt, mr, mb;
    // paddings
    int pl, pt, pr, pb;

    int              text_indent;
    pcth_rdr_align_t text_align;

    /* the code points of text in Unicode (should be in visual order) */
    uint32_t *ucs;

    /* text color and attributes */
    int color;

    /* the text segments */
    struct list_head segs;
};

struct purcth_rdrbox {
    struct purcth_rdrbox* parent;
    struct purcth_rdrbox* first;
    struct purcth_rdrbox* last;

    struct purcth_rdrbox* prev;
    struct purcth_rdrbox* next;

    /* type of box */
    pcth_rdr_box_t type;

    /* number of child boxes */
    unsigned nr_children;

    /* the visual region (rectangles) of the box */
    unsigned nr_rects;
    struct foil_rect *rects;

    /* the extra data if the box type is INLINE */
    union {
        void *data;     // aliases
        struct _inline_box_data *inline_data;
        struct _block_box_data *block_data;
    };
};

struct purcth_udom {
    /* the sorted array of eDOM element and the corresponding rendering box. */
    struct sorted_array *elem2rdrbox;

    /* position of viewport */
    int left, top;

    /* size of whole page */
    unsigned width, height;

    /* size of viewport */
    unsigned cols, rows;
};

purcth_udom *foil_udom_new(purcth_page *page)
{
    purcth_udom* udom = calloc(1, sizeof(purcth_udom));

    udom->elem2rdrbox = sorted_array_create(SAFLAG_DEFAULT, 8, NULL, NULL);
    if (udom->elem2rdrbox == NULL) {
        goto failed;
    }

    /* TODO */
    (void)page;
    return udom;

failed:

    if (udom->elem2rdrbox)
        sorted_array_destroy(udom->elem2rdrbox);

    free(udom);
    return NULL;
}

static void foil_udom_cleanup(purcth_udom *udom)
{
    assert(udom->elem2rdrbox);
    sorted_array_cleanup(udom->elem2rdrbox);
}

void foil_udom_delete(purcth_udom *udom)
{
    foil_udom_cleanup(udom);
    free(udom);
}

purcth_rdrbox *foil_udom_find_rdrbox(purcth_udom *udom,
        uint64_t element_handle)
{
    void *data;

    if (!sorted_array_find(udom->elem2rdrbox, element_handle, &data)) {
        return NULL;
    }

    return data;
}

struct rendering_ctxt {
    purcth_udom *udom;
};

static pchtml_action_t
rendering_walker(pcdom_node_t *node, void *ctxt)
{
    switch (node->type) {
    case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
        return PCHTML_ACTION_NEXT;

    case PCDOM_NODE_TYPE_TEXT:
    case PCDOM_NODE_TYPE_COMMENT:
    case PCDOM_NODE_TYPE_CDATA_SECTION:
        return PCHTML_ACTION_NEXT;

    case PCDOM_NODE_TYPE_ELEMENT: {
        // struct rendering_ctxt *my_ctxt = ctxt;

        /* walk to the siblings. */
        return PCHTML_ACTION_NEXT;
    }

    default:
        /* ignore any unknown node types */
        break;
    }

    return PCHTML_ACTION_NEXT;
}

purcth_rdrbox *
foil_udom_load_edom(purcth_udom *udom, pcdom_document_t *edom_doc, int *retv)
{
    size_t len;
    const unsigned char *doctype = pcdom_document_type_name(
            edom_doc->doctype, &len);

    if (len == 0 || strcasecmp(doctype, "html")) {
        *retv = PCRDR_SC_NOT_ACCEPTABLE;
        return NULL;
    }

    foil_udom_cleanup(udom);

    // TODO: parse CSS here

    pcdom_element_t *root = edom_doc->element;
    struct rendering_ctxt ctxt = { udom, };
    pcdom_node_simple_walk(pcdom_interface_node(root), rendering_walker, &ctxt);

    return NULL;
}

int foil_udom_update_rdrbox(purcth_udom *udom, purcth_rdrbox *rdrbox,
        int op, const char *property, purc_variant_t ref_info)
{
    (void)udom;
    (void)rdrbox;
    (void)op;
    (void)property;
    (void)ref_info;

    /* TODO */
    return PCRDR_SC_NOT_IMPLEMENTED;
}

purc_variant_t foil_udom_call_method(purcth_udom *udom, purcth_rdrbox *rdrbox,
        const char *method, purc_variant_t arg)
{
    (void)udom;
    (void)rdrbox;
    (void)method;
    (void)arg;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

purc_variant_t foil_udom_get_property(purcth_udom *udom, purcth_rdrbox *rdrbox,
        const char *property)
{
    (void)udom;
    (void)rdrbox;
    (void)property;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

purc_variant_t foil_udom_set_property(purcth_udom *udom, purcth_rdrbox *rdrbox,
        const char *property, purc_variant_t value)
{
    (void)udom;
    (void)rdrbox;
    (void)property;
    (void)value;

    /* TODO */
    return PURC_VARIANT_INVALID;
}

