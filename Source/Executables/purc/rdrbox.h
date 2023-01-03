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
#include "region/rect.h"

#include <glib.h>
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
    FOIL_RDRBOX_OVERFLOW_VISIBLE_PROPAGATED,
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
    FOIL_RDRBOX_LIST_STYLE_TYPE_LOWER_ARMENIAN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_UPPER_ARMENIAN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_GEORGIAN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_CJK_DECIMAL,
    FOIL_RDRBOX_LIST_STYLE_TYPE_TIBETAN,
    FOIL_RDRBOX_LIST_STYLE_TYPE_NONE,
};

/* the list-style-position of a list-item box. */
enum {
    FOIL_RDRBOX_LIST_STYLE_POSITION_OUTSIDE = 0,
    FOIL_RDRBOX_LIST_STYLE_POSITION_INSIDE,
};

/* the border width (light lines or heavy lines). */
enum {
    FOIL_RDRBOX_BORDER_WIDTH_ZERO = 0,
    FOIL_RDRBOX_BORDER_WIDTH_THIN = CSS_BORDER_WIDTH_THIN,
    FOIL_RDRBOX_BORDER_WIDTH_MEDIUM = CSS_BORDER_WIDTH_MEDIUM,
    FOIL_RDRBOX_BORDER_WIDTH_THICK = CSS_BORDER_WIDTH_THICK,
};

/* the border styles (dotted, dashed, solid, and double). */
enum {
    FOIL_RDRBOX_BORDER_STYLE_NONE = 0,
    FOIL_RDRBOX_BORDER_STYLE_HIDDEN,
    FOIL_RDRBOX_BORDER_STYLE_DOTTED,
    FOIL_RDRBOX_BORDER_STYLE_DASHED,
    FOIL_RDRBOX_BORDER_STYLE_SOLID,
    FOIL_RDRBOX_BORDER_STYLE_DOUBLE,
};

/* the values for clear property. */
enum {
    FOIL_RDRBOX_CLEAR_NONE = 0,
    FOIL_RDRBOX_CLEAR_LEFT,
    FOIL_RDRBOX_CLEAR_RIGHT,
    FOIL_RDRBOX_CLEAR_BOTH,
};

/* the values for vertical-align property. */
enum {
    FOIL_RDRBOX_VALIGN_BOTTOM = 0,
    FOIL_RDRBOX_VALIGN_TOP,
    FOIL_RDRBOX_VALIGN_MIDDLE,
};

/* use which property when calculating width */
enum {
    FOIL_RDRBOX_USE_WIDTH = 0,
    FOIL_RDRBOX_USE_MAX_WIDTH,
    FOIL_RDRBOX_USE_MIN_WIDTH,
};

/* use which property when calculating height */
enum {
    FOIL_RDRBOX_USE_HEIGHT = 0,
    FOIL_RDRBOX_USE_MAX_HEIGHT,
    FOIL_RDRBOX_USE_MIN_HEIGHT,
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

typedef struct foil_quotes {
    /* reference count */
    unsigned refc;

    /* the number of quotation mark strings contained in strings */
    unsigned nr_strings;

    /* the list of pairs of quotation marks */
    lwc_string **strings;
} foil_quotes;

typedef struct foil_named_counter {
    lwc_string *name;
    intptr_t    value;
} foil_named_counter;

typedef struct foil_counters {
    /* reference count */
    unsigned refc;

    /* the number of named counters */
    unsigned nr_counters;

    /* the list of named counters */
    foil_named_counter *counters;
} foil_counters;

struct foil_rdrbox {
    struct foil_rdrbox* parent;
    struct foil_rdrbox* first;
    struct foil_rdrbox* last;

    struct foil_rdrbox* prev;
    struct foil_rdrbox* next;

    /* the element creating this box;
       for initial containing block, it has type of `PCDOC_NODE_VOID`. */
    pcdoc_element_t owner;

    /* the pricipal box if this box is created for a pseudo element */
    struct foil_rdrbox *principal;

    /* the computed CSS style */
    css_computed_style *computed_style;

    // Indicates that this box is an anonymous box.
    uint32_t is_anonymous:1;
    // Indicates that this box is a principal box.
    uint32_t is_principal:1;
    // Indicates that this box is created for an pseudo element.
    uint32_t is_pseudo:1;
    // Indicates that the element generating this box is a replaced one.
    uint32_t is_replaced:1;
    // Indicates that the element generating this box is a control.
    uint32_t is_control:1;
    // Indicates that this box is the initial containing block.
    uint32_t is_initial:1;
    // Indicates that this box is created by the root element.
    uint32_t is_root:1;
    // Indicates that this box is created by the `body` element.
    uint32_t is_body:1;
    // Generated by a non-replaced element with a display value of inline.
    uint32_t is_inline_box:1;
    // Generated by a display values of inline, inline-table, and inline-block.
    uint32_t is_inline_level:1;
    // Generated by a display values of block, list-item, and table.
    uint32_t is_block_level:1;
    // Indicates that this box is a block container.
    uint32_t is_block_container:1;
    // Indicates that this box is absolutely positioned.
    uint32_t is_abs_positioned:1;
    // Indicates that this box is in flow.
    uint32_t is_in_flow:1;
    // Indicates that this box is in normal flow.
    uint32_t is_in_normal_flow:1;
    // Indicates which property to use when calculating width and margins.
    uint32_t prop_for_width:2;
    // Indicates which property to use when calculating height and margins.
    uint32_t prop_for_height:2;
    // Indicates that the widths and margins of the box is resolved.
    uint32_t is_width_resolved:1;
    // Indicates that the heights and margsin of the box is resolved.
    uint32_t is_height_resolved:1;

    /* Used values of non-inherited properties */
    uint32_t type:4;
    uint32_t position:3;
    uint32_t floating:2;
    uint32_t clear:2;
    uint32_t overflow_x:3;
    uint32_t overflow_y:3;
    uint32_t unicode_bidi:3;
    uint32_t text_deco_underline:1;
    uint32_t text_deco_overline:1;
    uint32_t text_deco_line_through:1;
    uint32_t text_deco_blink:1;
    uint32_t text_overflow:1;
    uint32_t word_break:2;
    uint32_t line_break:3;
    uint32_t word_wrap:2;
    uint32_t lang_code:8;
    uint32_t vertical_align:2;
    uint32_t border_top_width:2;
    uint32_t border_right_width:2;
    uint32_t border_bottom_width:2;
    uint32_t border_left_width:2;
    uint32_t border_top_style:4;
    uint32_t border_right_style:4;
    uint32_t border_bottom_style:4;
    uint32_t border_left_style:4;

    uint32_t border_top_color:4;
    uint32_t border_right_color:4;
    uint32_t border_bottom_color:4;
    uint32_t border_left_color:4;

    uint32_t background_color:4;
    int32_t  z_index;

    int32_t width, height;              // content width and height
    int32_t left, right, top, bottom;   // position
    int32_t mt, ml, mr, mb;             // margins
    int32_t bt, bl, br, bb;             // borders (actual values)
    int32_t pt, pl, pr, pb;             // paddings

    foil_counters *counter_reset;   // NULL when `counter-reset` is `none`
    foil_counters *counter_incrm;   // NULL when `counter-increment` is `none`

    /* the following non-inherited properties have non-zero intial values */
    int32_t min_height, max_height;    // initial value: 0, -1 (none)
    int32_t min_width,  max_width;     // initial value: 0, -1 (none)

    /* End of non-inherited properties */

    /* Used values of inheritable properties */
    uint32_t __copy_start;
    uint32_t border_collapse:1;
    uint32_t direction:1;
    uint32_t caption_side:1;
    uint32_t empty_cells:1;
    uint32_t list_style_type:4;
    uint32_t list_style_position:1;
    uint32_t text_align:2;
    uint32_t text_transform:2;
    uint32_t visibility:2;
    uint32_t white_space:3;
    uint32_t color:4;

    uint32_t border_spacing_x;
    uint32_t border_spacing_y;
    uint32_t line_height;
    int32_t  letter_spacing;
    int32_t  text_indent;
    int32_t  word_spacing;
    uint32_t __copy_finish;

    foil_quotes *quotes;    // NULL when `quotes` is `none`
    /* End of inheritable properties */

    /* Layout fields */
    unsigned nr_block_level_children;
    unsigned nr_inline_level_children;
    unsigned nr_floating_children;
    unsigned nr_abspos_children;

    const foil_rdrbox *cblock_creator;  // the container of this box
    foil_rect ctnt_rect;                // the content rectangle of the box
                                        // in the container of this box

    /* Internal fields */
    GHashTable *counters_table; // The counters table for the current element
                                // and its descendants and its following
                                // siblings with their descendants
    unsigned nr_child_list_items;   // the number of child list items

    union {
        /* the extra data for different box types */
        void *data;                                 // aliases
        struct _inline_box_data     *inline_data;
        struct _block_box_data      *block_data;
        struct _inline_block_data   *inline_block_data;
        struct _list_item_data      *list_item_data;
        struct _marker_box_data     *marker_data;
        /* TODO: for other box types */
    };

    /* the callback to cleanup the extra data */
    void (*cb_data_cleanup)(void *data);

    /* box formatting context */
    struct _block_fmt_ctxt  *block_fmt_ctxt;

    /* the stacking context created by the box if it is a positioned box,
       and z-index is not auto. */
    struct foil_stacking_context *stacking_ctxt;
};

typedef void (*foil_data_cleanup_cb)(void *data);

typedef struct foil_create_ctxt {
    pcmcth_udom *udom;

    /* the initial containing block  */
    struct foil_rdrbox *initial_cblock;

    /* the box for the root element */
    struct foil_rdrbox *root_box;

    /* the box for the parent element */
    struct foil_rdrbox *parent_box;

    /* the root element */
    pcdoc_element_t root;

    /* the body element */
    pcdoc_element_t body;

    /* the current element */
    pcdoc_element_t elem;

    /* the current styles */
    css_select_results *computed;
    css_computed_style *style;

    /* the tag name of the current element */
    const char *tag_name;
} foil_create_ctxt;

typedef struct foil_layout_ctxt {
    pcmcth_udom *udom;
    const struct foil_rdrbox *initial_cblock;
} foil_layout_ctxt;

typedef struct foil_render_ctxt {
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

void foil_rdrbox_render_before(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level);
void foil_rdrbox_render_content(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level);
void foil_rdrbox_render_after(foil_render_ctxt *ctxt,
        const foil_rdrbox *box, unsigned level);

void foil_rdrbox_append_child(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_prepend_child(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_insert_before(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_insert_after(foil_rdrbox *to, foil_rdrbox *box);
void foil_rdrbox_remove_from_tree(foil_rdrbox *box);

/* Gets the box name; call free() to release the name after done. */
char *foil_rdrbox_get_name(purc_document_t doc, const foil_rdrbox *box);

/* Creates the principal box and the subsidiary box (e.g. marker) */
foil_rdrbox *foil_rdrbox_create_principal(foil_create_ctxt *ctxt);

/* Creates the box for :before pseudo element */
foil_rdrbox *foil_rdrbox_create_before(foil_create_ctxt *ctxt,
        foil_rdrbox *principal);

/* Creates the box for :after pseudo element */
foil_rdrbox *foil_rdrbox_create_after(foil_create_ctxt *ctxt,
        foil_rdrbox *principal);

/* Creates an anonymous block box */
foil_rdrbox *foil_rdrbox_create_anonymous_block(foil_create_ctxt *ctxt,
        foil_rdrbox *parent, foil_rdrbox *before, foil_rdrbox *after);

/* Creates an anonymous inline box */
foil_rdrbox *foil_rdrbox_create_anonymous_inline(foil_create_ctxt *ctxt,
        foil_rdrbox *parent);

/* Initializes type-specific data for a rendering box */
bool foil_rdrbox_init_data(foil_create_ctxt *ctxt, foil_rdrbox *box);

/* Initializes the data of an inline box */
bool foil_rdrbox_init_inline_data(foil_create_ctxt *ctxt, foil_rdrbox *box,
        const char *text, size_t len);

/* Gets the list number according to the list-item-type.
   The caller will take the ownership of the returned string,
   should call free() to release it after done. */
char *foil_rdrbox_list_number(const int max,
        const int number, uint8_t type, const char *tail);

/* Initializes the data of a marker box. */
bool foil_rdrbox_init_marker_data(foil_create_ctxt *ctxt,
        foil_rdrbox *marker, const foil_rdrbox *list_item);

/* Creates a new quotes */
foil_quotes *foil_quotes_new(unsigned nr_strings, const char **strings);
foil_quotes *foil_quotes_new_lwc(unsigned nr_strings, lwc_string **strings);

/* Deletes a quotes object */
void foil_quotes_delete(foil_quotes *quotes);

/* References a quotes object */
static inline foil_quotes *
foil_quotes_ref(foil_quotes *quotes)
{
    quotes->refc++;
    return quotes;
}

/* Un-references a quotes object */
static inline void
foil_quotes_unref(foil_quotes *quotes)
{
    assert(quotes->refc > 0);
    quotes->refc--;
    if (quotes->refc == 0)
        foil_quotes_delete(quotes);
}

/* Gets the initial quotes for specific language code */
foil_quotes *foil_quotes_get_initial(uint8_t lang_code);

/* Methods to operate foil_counter */
foil_counters *foil_counters_new(const css_computed_counter *counters);
void foil_counters_delete(foil_counters *counters);

static inline foil_counters *
foil_counters_ref(foil_counters *counters)
{
    counters->refc++;
    return counters;
}

static inline void
foil_counters_unref(foil_counters *counters)
{
    assert(counters->refc > 0);
    counters->refc--;
    if (counters->refc == 0)
        foil_counters_delete(counters);
}

void foil_rdrbox_pre_layout(foil_layout_ctxt *ctxt, foil_rdrbox *box);
void foil_rdrbox_resolve_width(foil_layout_ctxt *ctxt, foil_rdrbox *box);
void foil_rdrbox_resolve_height(foil_layout_ctxt *ctxt, foil_rdrbox *box);
void foil_rdrbox_lay_block_inlines(foil_layout_ctxt *ctxt, foil_rdrbox *box);

void foil_rdrbox_containing_block(const foil_rdrbox *box, foil_rect *rc);
void foil_rdrbox_containing_block_from_inlines(const foil_rdrbox *box,
        foil_rect *rc);
void foil_rdrbox_content_box(const foil_rdrbox *box, foil_rect *rc);
void foil_rdrbox_padding_box(const foil_rdrbox *box, foil_rect *rc);
void foil_rdrbox_border_box(const foil_rdrbox *box, foil_rect *rc);
void foil_rdrbox_margin_box(const foil_rdrbox *box, foil_rect *rc);

const foil_rdrbox *
foil_rdrbox_find_container_for_relative(foil_layout_ctxt *ctxt,
        const foil_rdrbox *box);
const foil_rdrbox *
foil_rdrbox_find_container_for_absolute(foil_layout_ctxt *ctxt,
        const foil_rdrbox *box);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_rdrbox_h */

