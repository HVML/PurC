/*
 * @file rdrbox.h
 * @author Vincent Wei
 * @date 2022/10/10
 * @brief The header for rendering box.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef purc_foil_rdrbox_h
#define purc_foil_rdrbox_h

#include "foil.h"

#include <csseng/csseng.h>

/* The rendered box */
struct foil_rdrbox;
typedef struct foil_rdrbox foil_rdrbox;

/* Indicates that a box is a block container box:

   In CSS 2.2, a block-level box is also a block container box
   unless it is a table box or the principal box of a replaced element.

   Values of the 'display' property which make a non-replaced element
   generate a block container include 'block', 'list-item' and 'inline-block'.

   In Foil, because there is no replaced element, values of the 'display'
   property that make a block container box include: 'block', 'inlin-block',
   and 'list-item'.
 */
#define FOIL_RDRBOX_FLAG_CONTAINER      0x0100

/* Indicates that a box is anonymous box. */
#define FOIL_RDRBOX_FLAG_ANONYMOUS      0x0200

/* Indicates that a box is principal box. */
#define FOIL_RDRBOX_FLAG_PRINCIPAL      0x0400

typedef enum {
    FOIL_RDRBOX_TYPE_FIRST = 0x0000,

    FOIL_RDRBOX_TYPE_INLINE = FOIL_RDRBOX_TYPE_FIRST,
    FOIL_RDRBOX_TYPE_BLOCK,
    FOIL_RDRBOX_TYPE_INLINE_BLOCK,
    FOIL_RDRBOX_TYPE_MARKER,
    FOIL_RDRBOX_TYPE_TABLE,
    FOIL_RDRBOX_TYPE_INLINE_TABLE,
    FOIL_RDRBOX_TYPE_TABLE_ROW_GROUP,
    FOIL_RDRBOX_TYPE_TABLE_COLUMN,
    FOIL_RDRBOX_TYPE_TABLE_COLUMN_GROUP,
    FOIL_RDRBOX_TYPE_TABLE_HEADER_GROUP,
    FOIL_RDRBOX_TYPE_TABLE_FOOTER_GROUP,
    FOIL_RDRBOX_TYPE_TABLE_ROW,
    FOIL_RDRBOX_TYPE_TABLE_CELL,
    FOIL_RDRBOX_TYPE_TABLE_CAPTION,
} foil_rdrbox_type_k;

struct _inline_box_data;
struct _block_box_data;
struct _inline_block_data;
struct _marker_box_data;

struct foil_rdrbox {
    struct foil_rdrbox* parent;
    struct foil_rdrbox* first;
    struct foil_rdrbox* last;

    struct foil_rdrbox* prev;
    struct foil_rdrbox* next;

    /* type and flags of this box */
    unsigned type_flags;

    /* number of child boxes */
    unsigned nr_children;

    /* the node creates this box */
    pcdoc_node node;

    /* the rectangle of this box */
    foil_rect   rect;

    /* the extra data of this box */
    union {
        void *data;     // aliases
        struct _inline_box_data     *inline_data;
        struct _block_box_data      *block_data;
        struct _inline_block_data   *inline_block_data;
        struct _marker_box_data     *marker_data;
        /* TODO: for other box types */
    };
};

struct foil_rendering_ctxt {
    purc_document_t doc;

    pcmcth_udom *udom;

    /* the current containing block */
    struct foil_rdrbox *current_cblock;
};

#ifdef __cplusplus
extern "C" {
#endif

int foil_rdrbox_module_init(pcmcth_renderer *rdr);
void foil_rdrbox_module_cleanup(pcmcth_renderer *rdr);

foil_rdrbox *foil_rdrbox_new_block(void);

void foil_rdrbox_append_child(foil_rdrbox *to, foil_rdrbox *node);
void foil_rdrbox_prepend_child(foil_rdrbox *to, foil_rdrbox *node);
void foil_rdrbox_insert_before(foil_rdrbox *to, foil_rdrbox *node);
void foil_rdrbox_insert_after(foil_rdrbox *to, foil_rdrbox *node);
void foil_rdrbox_remove_from_tree(foil_rdrbox *node);

void foil_rdrbox_delete(foil_rdrbox *box);
void foil_rdrbox_delete_deep(foil_rdrbox *root);

foil_rdrbox *foil_rdrbox_create(struct foil_rendering_ctxt *ctxt,
        pcdoc_element_t element, css_select_results *result);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_rdrbox_h */

