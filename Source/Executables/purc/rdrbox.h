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

typedef enum {
    PCTH_RDR_BOX_TYPE_INLINE,
    PCTH_RDR_BOX_TYPE_BLOCK,
    PCTH_RDR_BOX_TYPE_INLINE_BLOCK,
    PCTH_RDR_BOX_TYPE_MARKER,
} pcth_rdrbox_type_k;

struct _inline_box_data;
struct _block_box_data;
struct _inline_block_data;
struct _marker_box_data;

struct purcth_rdrbox {
    struct purcth_rdrbox* parent;
    struct purcth_rdrbox* first;
    struct purcth_rdrbox* last;

    struct purcth_rdrbox* prev;
    struct purcth_rdrbox* next;

    /* type of box */
    pcth_rdrbox_type_k type;

    /* number of child boxes */
    unsigned nr_children;

    /* the rectangle of this box */
    foil_rect   rect;

    /* the extra data of this box */
    union {
        void *data;     // aliases
        struct _inline_box_data     *inline_data;
        struct _block_box_data      *block_data;
        struct _inline_block_data   *inline_block_data;
        struct _marker_box_data     *marker_data;
    };
};

struct purcth_rendering_ctxt {
    purc_document_t doc;

    purcth_udom *udom;

    /* the current containing block */
    struct purcth_rdrbox *current_cblock;
};

#ifdef __cplusplus
extern "C" {
#endif

int foil_rdrbox_module_init(void);
void foil_rdrbox_module_cleanup(void);

purcth_rdrbox *foil_rdrbox_new_block(void);

void foil_rdrbox_append_child(purcth_rdrbox *to, purcth_rdrbox *node);
void foil_rdrbox_prepend_child(purcth_rdrbox *to, purcth_rdrbox *node);
void foil_rdrbox_insert_before(purcth_rdrbox *to, purcth_rdrbox *node);
void foil_rdrbox_insert_after(purcth_rdrbox *to, purcth_rdrbox *node);
void foil_rdrbox_remove_from_tree(purcth_rdrbox *node);

void foil_rdrbox_delete(purcth_rdrbox *box);
void foil_rdrbox_delete_deep(purcth_rdrbox *root);

purcth_rdrbox *foil_create_rdrbox(struct purcth_rendering_ctxt *ctxt,
        pcdoc_element_t element, css_select_results *result);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_rdrbox_h */

