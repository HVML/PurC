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

/* the position of a box. */
enum {
    FOIL_RDRBOX_POSITION_STATIC = 0,
    FOIL_RDRBOX_POSITION_RELATIVE,
    FOIL_RDRBOX_POSITION_ABSOLUTE,
    FOIL_RDRBOX_POSITION_FIXED,
    FOIL_RDRBOX_POSITION_STICKY,
};

/* the float of a box. */
enum {
    FOIL_RDRBOX_FLOAT_NONE = 0,
    FOIL_RDRBOX_FLOAT_LEFT,
    FOIL_RDRBOX_FLOAT_RIGHT,
};

/* the direction of a box. */
enum {
    FOIL_RDRBOX_DIR_LTR = 0,
    FOIL_RDRBOX_DIR_RTL,
};

/* the Unicode bidi of a box. */
enum {
    FOIL_RDRBOX_BIDI_NORMAL = 0,
    FOIL_RDRBOX_BIDI_EMBED,
    FOIL_RDRBOX_BIDI_ISOLATE,
    FOIL_RDRBOX_BIDI_BIDI_OVERRIDE,
    FOIL_RDRBOX_BIDI_ISOLATE_OVERRIDE,
    FOIL_RDRBOX_BIDI_PLAINTEXT,
};

/* the text transforms of a box. */
enum {
    FOIL_RDRBOX_TEXT_TRANS_NONE = 0,
    FOIL_RDRBOX_TEXT_TRANS_CAPITALIZE,
    FOIL_RDRBOX_TEXT_TRANS_UPPERCASE,
    FOIL_RDRBOX_TEXT_TRANS_LOWERCASE,
};

/* the white space of a box. */
enum {
    FOIL_RDRBOX_WHITE_SPACE_NORMAL = 0,
    FOIL_RDRBOX_WHITE_SPACE_PRE,
    FOIL_RDRBOX_WHITE_SPACE_NOWRAP,
    FOIL_RDRBOX_WHITE_SPACE_PRE_WRAP,
    FOIL_RDRBOX_WHITE_SPACE_PRE_LINE,
};

/* the overflow of a box. */
enum {
    FOIL_RDRBOX_OVERFLOW_VISIBLE = 0,
    FOIL_RDRBOX_OVERFLOW_HIDDEN,
    FOIL_RDRBOX_OVERFLOW_SCROLL,
    FOIL_RDRBOX_OVERFLOW_AUTO,
};

/* the visibility of a box. */
enum {
    FOIL_RDRBOX_VISIBILITY_VISIBLE = 0,
    FOIL_RDRBOX_VISIBILITY_HIDDEN,
    FOIL_RDRBOX_VISIBILITY_COLLAPSE,
};

enum {
    FOIL_RDRBOX_TYPE_INLINE = 0,
    FOIL_RDRBOX_TYPE_BLOCK,
    FOIL_RDRBOX_TYPE_LIST_ITEM,
    FOIL_RDRBOX_TYPE_MARKER,
    FOIL_RDRBOX_TYPE_INLINE_BLOCK,
    FOIL_RDRBOX_TYPE_TABLE,
    FOIL_RDRBOX_TYPE_INLINE_TABLE,
    FOIL_RDRBOX_TYPE_TABLE_ROW_GROUP,
    FOIL_RDRBOX_TYPE_TABLE_HEADER_GROUP,
    FOIL_RDRBOX_TYPE_TABLE_FOOTER_GROUP,
    FOIL_RDRBOX_TYPE_TABLE_ROW,
    FOIL_RDRBOX_TYPE_TABLE_COLUMN_GROUP,
    FOIL_RDRBOX_TYPE_TABLE_COLUMN,
    FOIL_RDRBOX_TYPE_TABLE_CELL,
    FOIL_RDRBOX_TYPE_TABLE_CAPTION,
};

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

    /* number of child boxes */
    unsigned nr_children;

    /* the element creating this box;
       for initial containing block, it has type of `PCDOC_NODE_VOID`. */
    pcdoc_element_t owner;

    /* Indicates that a box is a block container box:

       In CSS 2.2, a block-level box is also a block container box
       unless it is a table box or the principal box of a replaced element.

     */
    uint8_t is_container:1;

    /* Indicates that this box is anonymous box. */
    uint8_t is_anonymous:1;

    /* Indicates that this box is principal box. */
    uint8_t is_principal:1;

    /* Indicates that the element generating this box is a replaced one. */
    uint8_t is_replaced:1;

    /* Indicates that this box is the intial containing block. */
    uint8_t is_initial:1;

    /* used values of properties for all elements */
    unsigned type:4;
    unsigned position:3;
    unsigned float_type:2;
    unsigned direction:1;
    unsigned unicode_bidi:3;
    unsigned text_transform:2;
    unsigned text_deco_underline:1;
    unsigned text_deco_overline:1;
    unsigned text_deco_line_through:1;
    unsigned text_deco_blink:1;
    unsigned white_space:3;
    unsigned overflow_x:2;
    unsigned overflow_y:2;
    unsigned visibility:2;

    int width, height;      // content width and height
    int left, top;          // position
    int mt, ml, mr, mb;     // margins
    int bt, bl, br, bb;     // borders
    int pt, pl, pr, pb;     // paddings

    int letter_spacing;
    int word_spacing;

    uint32_t fgc;   // ARGB
    uint32_t bgc;   // ARGB

    /* the containing block */
    foil_rect cblock_rect;

    /* the creator of the current containing block */
    const foil_rdrbox *cblock_creator;

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

enum {
    FOIL_RDRBOX_POSSCHEMA_BLOCK_FORMAT = 0,
    FOIL_RDRBOX_POSSCHEMA_INLINE_FORMAT,
    FOIL_RDRBOX_POSSCHEMA_RELATIVE,
    FOIL_RDRBOX_POSSCHEMA_FLOATS,
    FOIL_RDRBOX_POSSCHEMA_ABSOLUTE,
};

typedef struct foil_rendering_ctxt {
    purc_document_t doc;

    pcmcth_udom *udom;

    /* the initial containing block  */
    const struct foil_rdrbox *initial_cblock;

    /* the box for the parent element */
    struct foil_rdrbox *parent_box;

    /* the current element */
    pcdoc_element_t elem;

    /* the current styles */
    css_select_results *computed;

    /* the tag name of the current element */
    char *tag_name;

    unsigned pos_schema:3;
    unsigned in_normal_flow:1;

} foil_rendering_ctxt;

#ifdef __cplusplus
extern "C" {
#endif

int foil_rdrbox_module_init(pcmcth_renderer *rdr);
void foil_rdrbox_module_cleanup(pcmcth_renderer *rdr);

foil_rdrbox *foil_rdrbox_new(uint8_t type);

void foil_rdrbox_append_child(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_prepend_child(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_insert_before(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_insert_after(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_remove_from_tree(foil_rdrbox *box);

void foil_rdrbox_delete(foil_rdrbox *box);
void foil_rdrbox_delete_deep(foil_rdrbox *root);

foil_rdrbox *foil_rdrbox_create(foil_rendering_ctxt *ctxt);

bool foil_rdrbox_content_box(const foil_rdrbox *box, foil_rect *rc);
bool foil_rdrbox_padding_box(const foil_rdrbox *box, foil_rect *rc);
bool foil_rdrbox_border_box(const foil_rdrbox *box, foil_rect *rc);
bool foil_rdrbox_margin_box(const foil_rdrbox *box, foil_rect *rc);

bool foil_rdrbox_form_containing_block(const foil_rdrbox *box, foil_rect *rc);

const foil_rdrbox *
foil_rdrbox_find_container_for_relative(foil_rendering_ctxt *ctxt,
        const foil_rdrbox *box);
const foil_rdrbox *
foil_rdrbox_find_container_for_absolute(foil_rendering_ctxt *ctxt,
        const foil_rdrbox *box);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_rdrbox_h */

