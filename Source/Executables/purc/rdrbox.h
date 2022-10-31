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
    FOIL_RDRBOX_DIRECTION_LTR = 0,
    FOIL_RDRBOX_DIRECTION_RTL,
};

/* the Unicode bidi of a box. */
enum {
    FOIL_RDRBOX_UNICODE_BIDI_NORMAL = 0,
    FOIL_RDRBOX_UNICODE_BIDI_EMBED,
    FOIL_RDRBOX_UNICODE_BIDI_ISOLATE,
    FOIL_RDRBOX_UNICODE_BIDI_BIDI_OVERRIDE,
    FOIL_RDRBOX_UNICODE_BIDI_ISOLATE_OVERRIDE,
    FOIL_RDRBOX_UNICODE_BIDI_PLAINTEXT,
};

/* the text transforms of a box. */
enum {
    FOIL_RDRBOX_TEXT_TRANSFORM_NONE = 0,
    FOIL_RDRBOX_TEXT_TRANSFORM_CAPITALIZE = 0x01,
    FOIL_RDRBOX_TEXT_TRANSFORM_UPPERCASE = 0x02,
    FOIL_RDRBOX_TEXT_TRANSFORM_LOWERCASE = 0x03,
    FOIL_RDRBOX_TEXT_TRANSFORM_FULL_WIDTH = 0x10,
    FOIL_RDRBOX_TEXT_TRANSFORM_FULL_SIZE_KANA = 0x20,
};

/* the white space of a box. */
enum {
    FOIL_RDRBOX_WHITE_SPACE_NORMAL = 0,
    FOIL_RDRBOX_WHITE_SPACE_PRE,
    FOIL_RDRBOX_WHITE_SPACE_NOWRAP,
    FOIL_RDRBOX_WHITE_SPACE_PRE_WRAP,
    FOIL_RDRBOX_WHITE_SPACE_BREAK_SPACES,
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

/* the text-align of a block container. */
enum {
    FOIL_RDRBOX_TEXT_ALIGN_LEFT = 0,
    FOIL_RDRBOX_TEXT_ALIGN_RIGHT,
    FOIL_RDRBOX_TEXT_ALIGN_CENTER,
    FOIL_RDRBOX_TEXT_ALIGN_JUSTIFY,
};

/* the text-overflow of a block container. */
enum {
    FOIL_RDRBOX_TEXT_OVERFLOW_CLIP = 0,
    FOIL_RDRBOX_TEXT_OVERFLOW_ELLIPSIS,
};

/* the word-break of a box. */
enum {
    FOIL_RDRBOX_WORD_BREAK_NORMAL = 0,
    FOIL_RDRBOX_WORD_BREAK_KEEP_ALL,
    FOIL_RDRBOX_WORD_BREAK_BREAK_ALL,
};

/* the line-break of a box. */
enum {
    FOIL_RDRBOX_LINE_BREAK_AUTO = 0,
    FOIL_RDRBOX_LINE_BREAK_LOOSE,
    FOIL_RDRBOX_LINE_BREAK_NORMAL,
    FOIL_RDRBOX_LINE_BREAK_STRICT,
    FOIL_RDRBOX_LINE_BREAK_ANYWHERE,
};

/* the word-wrap of a box. */
enum {
    FOIL_RDRBOX_WORD_WRAP_NORMAL = 0,
    FOIL_RDRBOX_WORD_WRAP_BREAK_WORD,
    FOIL_RDRBOX_WORD_WRAP_ANYWHERE,
};

/* the list-style-type of a list-item box. */
enum {
    FOIL_RDRBOX_LIST_STYLE_TYPE_DISC = 0,
    FOIL_RDRBOX_LIST_STYLE_TYPE_CIRCLE,
    FOIL_RDRBOX_LIST_STYLE_TYPE_SQUARE,
    FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL,
    FOIL_RDRBOX_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO,
    FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ROMAN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ROMAN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_GREEK,
    FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_LATIN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_LATIN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_ARMENIAN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_NONE,
};

/* the list-style-position of a list-item box. */
enum {
    FOIL_RDRBOX_LIST_STYLE_POSITION_OUTSIDE = 0,
    FOIL_RDRBOX_LIST_STYLE_POSITION_INSIDE,
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
struct _list_item_data;
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

    uint8_t is_block_level:1;
    uint8_t is_inline_level:1;

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
    unsigned is_block_container:1;
    unsigned position:3;
    unsigned floating:2;
    unsigned direction:1;
    unsigned visibility:2;
    unsigned overflow_x:2;
    unsigned overflow_y:2;
    unsigned unicode_bidi:3;
    unsigned text_transform:2;
    unsigned text_deco_underline:1;
    unsigned text_deco_overline:1;
    unsigned text_deco_line_through:1;
    unsigned text_deco_blink:1;
    unsigned white_space:3;
    unsigned text_align:2;
    unsigned text_overflow:1;
    unsigned word_break:2;
    unsigned line_break:3;
    unsigned word_wrap:2;

    unsigned list_style_type:4;
    unsigned list_style_position:1;

    int letter_spacing;
    int word_spacing;
    int text_indent;

    uint32_t fgc;   // ARGB
    uint32_t bgc;   // ARGB

    /* layout flags */
    unsigned height_pending:1;

    int width, height;      // content width and height
    int left, top;          // position
    int mt, ml, mr, mb;     // margins
    int bt, bl, br, bb;     // borders
    int pt, pl, pr, pb;     // paddings

    /* the bounding rectangle of the containing block */
    foil_rect cblock_rect;

    /* the creator of the current containing block */
    const foil_rdrbox *cblock_creator;

    /* the number of child list items */
    unsigned nr_child_list_items;

    /* the number of inline level child boxes */
    unsigned nr_child_inlines;

    /* the extra data of this box */
    union {
        void *data;     // aliases
        struct _inline_box_data     *inline_data;
        struct _block_box_data      *block_data;
        struct _inline_block_data   *inline_block_data;
        struct _list_item_data      *list_item_data;
        struct _marker_box_data     *marker_data;
        /* TODO: for other box types */
    };

    /* the callback to cleanup the extra data */
    void (*cb_data_cleanup)(void *data);
};

enum {
    FOIL_RDRBOX_POSSCHEMA_BLOCK_FORMAT = 0,
    FOIL_RDRBOX_POSSCHEMA_INLINE_FORMAT,
    FOIL_RDRBOX_POSSCHEMA_RELATIVE,
    FOIL_RDRBOX_POSSCHEMA_FLOATS,
    FOIL_RDRBOX_POSSCHEMA_ABSOLUTE,
};

typedef struct foil_create_ctxt {
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
} foil_create_ctxt;

typedef struct foil_layout_ctxt {
    purc_document_t doc;
    pcmcth_udom *udom;
} foil_layout_ctxt;

typedef struct foil_render_ctxt {
    purc_document_t doc;
    pcmcth_udom *udom;
    pcmcth_page *page;
    unsigned level;
} foil_render_ctxt;

#ifdef __cplusplus
extern "C" {
#endif

int foil_rdrbox_module_init(pcmcth_renderer *rdr);
void foil_rdrbox_module_cleanup(pcmcth_renderer *rdr);

foil_rdrbox *foil_rdrbox_new(uint8_t type);
void foil_rdrbox_delete(foil_rdrbox *box);
void foil_rdrbox_delete_deep(foil_rdrbox *root);

void foil_rdrbox_dump(const foil_rdrbox *box,
        purc_document_t doc, unsigned level);

void foil_rdrbox_render_before(const foil_rdrbox *box, unsigned level);
void foil_rdrbox_render_content(const foil_rdrbox *box, unsigned level);
void foil_rdrbox_render_after(const foil_rdrbox *box, unsigned level);

void foil_rdrbox_append_child(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_prepend_child(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_insert_before(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_insert_after(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_remove_from_tree(foil_rdrbox *box);

/* create the principal box and the subsidiary box (e.g. marker) */
foil_rdrbox *foil_rdrbox_create_principal(foil_create_ctxt *ctxt);

/* create an anonymous block box */
foil_rdrbox *foil_rdrbox_create_anonymous_block(foil_create_ctxt *ctxt,
        foil_rdrbox *parent);

/* create an anonymous inline box */
foil_rdrbox *foil_rdrbox_create_anonymous_inline(foil_create_ctxt *ctxt,
        foil_rdrbox *parent);

/* initialize type-specific data for a rendering box */
bool foil_rdrbox_init_data(foil_create_ctxt *ctxt, foil_rdrbox *box);

/* initialize the data of an inline box */
bool foil_rdrbox_init_inline_data(foil_create_ctxt *ctxt, foil_rdrbox *box,
        const char *text, size_t len);

/* initialize the data of a marker box */
bool foil_rdrbox_init_marker_data(foil_create_ctxt *ctxt,
        foil_rdrbox *marker, const foil_rdrbox *list_item);

bool foil_rdrbox_content_box(const foil_rdrbox *box, foil_rect *rc);
bool foil_rdrbox_padding_box(const foil_rdrbox *box, foil_rect *rc);
bool foil_rdrbox_border_box(const foil_rdrbox *box, foil_rect *rc);
bool foil_rdrbox_margin_box(const foil_rdrbox *box, foil_rect *rc);

bool foil_rdrbox_form_containing_block(const foil_rdrbox *box, foil_rect *rc);

const foil_rdrbox *
foil_rdrbox_find_container_for_relative(foil_create_ctxt *ctxt,
        const foil_rdrbox *box);
const foil_rdrbox *
foil_rdrbox_find_container_for_absolute(foil_create_ctxt *ctxt,
        const foil_rdrbox *box);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_rdrbox_h */

