/*
** This file is part of DOM Ruler. DOM Ruler is a library to
** maintain a DOM tree, lay out and stylize the DOM nodes by
** using CSS (Cascaded Style Sheets).
**
** Copyright (C) 2021~2022 Beijing FMSoft Technologies Co., Ltd.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General License for more details.
**
** You should have received a copy of the GNU Lesser General License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _DOMRULER_H_
#define _DOMRULER_H_

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "csseng/csseng.h"
#include "domruler-version.h"

#include "purc/purc.h"

// log begin
#if defined(_DEBUG)
#   define HL_LOGD(fmt, ...)                                                               \
    do {                                                                                   \
        fprintf (stderr, "D|%s:%d:%s|"fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__);   \
    } while (0)
# else
#   define HL_LOGD(fmt, ...) do { } while (0)
#endif

#define HL_LOGE(fmt, ...)                           \
    do {                                            \
        fprintf (stderr, "E|"fmt, ##__VA_ARGS__);   \
    } while (0)

#define HL_LOGW(fmt, ...)                           \
    do {                                            \
        fprintf (stderr, "W|"fmt, ##__VA_ARGS__);   \
    } while (0)

// log end

#define HL_AUTO         INT_MIN
#define HL_UNKNOWN   INT_MAX

// error code begin
#define DOMRULER_OK                 0
#define DOMRULER_NOMEM              1
#define DOMRULER_BADPARM            2
#define DOMRULER_INVALID            3
#define DOMRULER_FILENOTFOUND       4
#define DOMRULER_NEEDDATA           5
#define DOMRULER_BADCHARSET         6
#define DOMRULER_EOF                7
#define DOMRULER_IMPORTS_PENDING    8
#define DOMRULER_PROPERTY_NOT_SET   9
#define DOMRULER_NOT_SUPPORT        10
#define DOMRULER_SELECT_STYLE_ERR   11

// error code end

// common attribute

typedef enum HLCommonAttribute_ {
    HL_COMMON_ATTR_ID           = 0,
    HL_COMMON_ATTR_CLASS_NAME   = 1,
    HL_COMMON_ATTR_STYLE        = 2,
    HL_COMMON_ATTR_NAME         = 3,

    HL_COMMON_ATTR_COUNT
} HLCommonAttribute;

typedef enum {
    LAYOUT_INVALID,
    LAYOUT_BLOCK,
    LAYOUT_INLINE_CONTAINER,
    LAYOUT_INLINE,
    LAYOUT_TABLE,
    LAYOUT_TABLE_ROW,
    LAYOUT_TABLE_CELL,
    LAYOUT_TABLE_ROW_GROUP,
    LAYOUT_FLOAT_LEFT,
    LAYOUT_FLOAT_RIGHT,
    LAYOUT_INLINE_BLOCK,
    LAYOUT_BR,
    LAYOUT_TEXT,
    LAYOUT_INLINE_END,
    LAYOUT_GRID,
    LAYOUT_INLINE_GRID,
    LAYOUT_NONE
} LayoutType;

typedef enum {
    DOM_UNDEF               = 0,
    DOM_ELEMENT_NODE        = 1,
    DOM_ATTRIBUTE_NODE      = 2,
    DOM_TEXT_NODE           = 3,
    DOM_CDATA_SECTION_NODE      = 4,
    DOM_ENTITY_REFERENCE_NODE   = 5,
    DOM_ENTITY_NODE         = 6,
    DOM_PROCESSING_INSTRUCTION_NODE = 7,
    DOM_COMMENT_NODE        = 8,
    DOM_DOCUMENT_NODE       = 9,
    DOM_DOCUMENT_TYPE_NODE      = 10,
    DOM_DOCUMENT_FRAGMENT_NODE  = 11,
    DOM_NOTATION_NODE       = 12,

    /* And a count of the number of node types */
    DOM_NODE_TYPE_COUNT
} HLNodeType;


typedef void (*cb_free_attach_data) (void *data);

typedef struct DOMRulerNodeOp {
    HLNodeType (*get_type)(void *node);
    const char *(*get_name)(void *node);
    const char *(*get_id)(void *node);
    int (*get_classes)(void *node, char ***classes);
    const char *(*get_attr)(void *node, const char *attr);
    void (*set_parent)(void *node, void *parent);
    void *(*get_parent)(void *node);
    void *(*first_child)(void *node);
    void *(*next)(void *node);
    void *(*previous)(void *node);
    bool (*is_root)(void *node);
} DOMRulerNodeOp;

// property

#define  HL_PROP_CATEGORY_BOX                  (1 << 0)
#define  HL_PROP_CATEGORY_BACKGROUND           (1 << 1)
#define  HL_PROP_CATEGORY_TEXT                 (1 << 2)
#define  HL_PROP_CATEGORY_SVG                  (1 << 3)

#define  HL_PROP_CATEGORY_ALL                  (HL_PROP_CATEGORY_BOX | HL_PROP_CATEGORY_BACKGROUND | HL_PROP_CATEGORY_TEXT | HL_PROP_CATEGORY_SVG)

#define HL_MAKE_PROP_ID(gid, i)             (((uint32_t)gid << 16) | (uint32_t)i)

// box group begin
// width
#define HL_PROP_ID_WIDTH                        HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 0)
// height
#define HL_PROP_ID_HEIGHT                       HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 1)
// margin-top
#define HL_PROP_ID_MARGIN_TOP                   HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 2)
// margin-right
#define HL_PROP_ID_MARGIN_RIGHT                 HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 3)
// margin-bottom
#define HL_PROP_ID_MARGIN_BOTTOM                HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 4)
// margin-left
#define HL_PROP_ID_MARGIN_LEFT                  HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 5)
// padding-top
#define HL_PROP_ID_PADDING_TOP                  HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 6)
// padding-right
#define HL_PROP_ID_PADDING_RIGHT                HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 7)
// padding-bottom
#define HL_PROP_ID_PADDING_BOTTOM               HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 8)
// padding-left
#define HL_PROP_ID_PADDING_LEFT                 HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 9)
// border-top-width
#define HL_PROP_ID_BORDER_TOP_WIDTH             HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 10)
// border-right-width
#define HL_PROP_ID_BORDER_RIGHT_WIDTH           HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 11)
// border-bottom-width
#define HL_PROP_ID_BORDER_BOTTOM_WIDTH          HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 12)
// border-left-width
#define HL_PROP_ID_BORDER_LEFT_WIDTH            HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 13)
// border-top-left-radius
#define HL_PROP_ID_BORDER_TOP_LEFT_RADIUS       HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 14)
// border-top-right-radius
#define HL_PROP_ID_BORDER_TOP_RIGHT_RADIUS      HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 15)
// border-bottom-left-radius
#define HL_PROP_ID_BORDER_BOTTOM_LEFT_RADIUS    HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 16)
// border-bottom-right-radius
#define HL_PROP_ID_BORDER_BOTTOM_RIGHT_RADIUS   HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BOX, 17)

// box group end

// background begin
// background-color
#define HL_PROP_ID_BACKGROUND_COLOR             HL_MAKE_PROP_ID(HL_PROP_CATEGORY_BACKGROUND, 0)

// background end

// text begin
// color
#define HL_PROP_ID_COLOR                        HL_MAKE_PROP_ID(HL_PROP_CATEGORY_TEXT, 0)
// font-family
#define HL_PROP_ID_FONT_FAMILY                  HL_MAKE_PROP_ID(HL_PROP_CATEGORY_TEXT, 1)
// font-size
#define HL_PROP_ID_FONT_SIZE                    HL_MAKE_PROP_ID(HL_PROP_CATEGORY_TEXT, 2)
// font-weight
#define HL_PROP_ID_FONT_WEIGHT                  HL_MAKE_PROP_ID(HL_PROP_CATEGORY_TEXT, 3)

// text end

// svg begin
// svg end

typedef enum HLDisplayEnum_ {
    HL_DISPLAY_BLOCK            = 0x02,
    HL_DISPLAY_INLINE_BLOCK     = 0x05,
    HL_DISPLAY_NONE             = 0x10,
    HL_DISPLAY_FLEX             = 0x11,
    HL_DISPLAY_INLINE_FLEX      = 0x12,
    HL_DISPLAY_GRID             = 0x13,
    HL_DISPLAY_INLINE_GRID      = 0x14
} HLDisplayEnum;

typedef enum HLPositionEnum_ {
    HL_POSITION_STATIC          = 0x1,
    HL_POSITION_RELATIVE        = 0x2,
    HL_POSITION_ABSOLUTE        = 0x3,
    HL_POSITION_FIXED           = 0x4
} HLPositionEnum;

typedef enum HLVisibilityEnum_ {
    HL_VISIBILITY_INHERIT           = 0x0,
    HL_VISIBILITY_VISIBLE           = 0x1,
    HL_VISIBILITY_HIDDEN            = 0x2,
    HL_VISIBILITY_COLLAPSE          = 0x3
} HLVisibilityEnum;

typedef struct HLDomElement_ HLDomElement;
typedef struct HLCSS_ HLCSS;
typedef float hl_real_t;

typedef struct HLBox_ {
    hl_real_t x;
    hl_real_t y;
    hl_real_t w;
    hl_real_t h;

    hl_real_t margin_top;
    hl_real_t margin_right;
    hl_real_t margin_bottom;
    hl_real_t margin_left;

    hl_real_t padding_top;
    hl_real_t padding_right;
    hl_real_t padding_bottom;
    hl_real_t padding_left;

    hl_real_t border_top;
    hl_real_t border_right;
    hl_real_t border_bottom;
    hl_real_t border_left;

    hl_real_t border_top_left_radius;
    hl_real_t border_top_right_radius;
    hl_real_t border_bottom_left_radius;
    hl_real_t border_bottom_right_radius;

    int z_index;

    HLDisplayEnum display;
    HLPositionEnum position;
    HLVisibilityEnum visibility;
    hl_real_t opacity;
} HLBox;

typedef struct HLUsedBackgroundValues_ {
    uint32_t color;
} HLUsedBackgroundValues;

typedef enum {
    HLFONT_WEIGHT_THIN,           // 100
    HLFONT_WEIGHT_EXTRA_LIGHT,    // 200
    HLFONT_WEIGHT_LIGHT,          // 300
    HLFONT_WEIGHT_NORMAL,         // 400
    HLFONT_WEIGHT_MEDIUM,         // 500
    HLFONT_WEIGHT_DEMIBOLD,       // 600
    HLFONT_WEIGHT_BOLD,           // 700
    HLFONT_WEIGHT_EXTRA_BOLD,     // 800
    HLFONT_WEIGHT_BLACK           // 900
} HLFontWeight;

typedef enum {
    HLTEXT_ALIGN_LEFT,
    HLTEXT_ALIGN_RIGHT,
    HLTEXT_ALIGN_CENTER,
    HLTEXT_ALIGN_JUSTIFY
} HLTextAlign;

typedef enum {
    HLTEXT_ALIGN_LAST_AUTO,
    HLTEXT_ALIGN_LAST_LEFT,
    HLTEXT_ALIGN_LAST_RIGHT,
    HLTEXT_ALIGN_LAST_CENTER,
    HLTEXT_ALIGN_LAST_JUSTIFY,
    HLTEXT_ALIGN_LAST_START,
    HLTEXT_ALIGN_LAST_END
} HLTextAlignLast;

typedef enum {
    HLTEXT_JUSTIFY_AUTO,
    HLTEXT_JUSTIFY_NONE,
    HLTEXT_JUSTIFY_INTER_WORD,
    HLTEXT_JUSTIFY_INTER_IDEOGRAPH,
    HLTEXT_JUSTIFY_INTER_CLUSTER,
    HLTEXT_JUSTIFY_DISTRIBUTE,
    HLTEXT_JUSTIFY_KASHIDA
} HLTextJustify;


typedef enum {
    HLTEXT_OVERFLOW_CLIP,
    HLTEXT_OVERFLOW_ELLIPSIS,
    HLTEXT_OVERFLOW_STRING
} HLTextOverflow;

typedef enum {
    HLTEXT_TRANSFORM_NONE,
    HLTEXT_TRANSFORM_CAPITALIZE,
    HLTEXT_TRANSFORM_UPPERCASE,
    HLTEXT_TRANSFORM_LOWERCASE
} HLTextTransform;

typedef enum {
    HLWORD_BREAK_NORMAL,
    HLWORD_BREAK_BREAK_ALL,
    HLWORD_BREAK_KEEP_ALL
} HLWordBreak;

typedef enum {
    HLWORD_WRAP_NORMAL,
    HLWORD_WRAP_BREAK_WORD
} HLWordWrap;

typedef enum {
    HLWORDWRAP_HORIZONTAL_TB,
    HLWORDWRAP_VERTICAL_RL,
    HLWORDWRAP_VERTICAL_LR
} HLWritingMode;

typedef struct HLUsedTextValues_ {
    uint32_t color;

    char *font_family;
    hl_real_t font_size;
    HLFontWeight font_weight;

    HLTextAlign text_align;
    HLTextAlignLast text_align_last;
    hl_real_t text_indent;
    HLTextJustify text_justify;
    HLTextOverflow text_overflow;
    char *text_overflow_string;
    hl_real_t text_shadow_h;
    hl_real_t text_shadow_v;
    hl_real_t text_shadow_blur;
    uint32_t text_shadow_color;
    HLTextTransform text_transform;

    HLWordBreak word_break;
    hl_real_t word_spacing;
    HLWordWrap word_wrap;

    HLWritingMode writing_mode;
} HLUsedTextValues;

typedef enum css_baseline_shift_e HLBaseLineShiftEnum;

typedef enum css_fill_e HLFillEnum;

typedef enum css_fill_rule_e HLFillRuleEnum;

typedef enum css_fill_opacity_e HLFillOpacityEnum;

typedef enum css_flood_color_e HLFloodColorEnum;

typedef enum css_clip_rule_e HLClipRuleEnum;

typedef enum css_stroke_e HLStrokeEnum;

typedef enum css_color_e HLColorEnum;

typedef enum css_direction_e HLDirectionEnum;

typedef enum css_enable_background_e HLEnableBackgroundEnum;

typedef enum css_opacity_e HLOpacityEnum;

typedef enum css_flood_opacity_e HLFloodOpacityEnum;

typedef enum css_comp_op_e HLCompOpEnum;

typedef enum css_overflow_e HLOverflowEnum;

typedef enum css_stop_color_e HLStopColorEnum;

typedef enum css_stop_opacity_e HLStopOpacityEnum;

typedef enum css_unicode_bidi_e HLUnicodeBidiEnum;

typedef enum css_writing_mode_e HLWritingModeEnum;

typedef enum css_text_anchor_e HLTextAnchorEnum;

typedef enum css_text_decoration_e HLTextDecorationEnum;

typedef enum css_shape_rendering_e HLShapeRenderingEnum;

typedef enum css_text_rendering_e HLTextRenderingEnum;

typedef enum css_stroke_miterlimit_e HLStrokeMiterlimitEnum;

typedef enum css_stroke_opacity_e HLStrokeOpacityEnum;

typedef enum css_stroke_linejoin_e HLStrokeLinejoinEnum;

typedef enum css_stroke_linecap_e HLStrokeLinecapEnum;

typedef enum css_font_stretch_e HLFontStretchEnum;

typedef enum css_font_weight_e HLFontWeightEnum;

typedef enum css_font_style_e HLFontStyleEnum;

typedef enum css_font_variant_e HLFontVariantEnum;

typedef enum css_stroke_width_e HLStrokeWidthEnum;

typedef enum css_font_size_e HLFontSizeEnum;

typedef enum css_font_family_e HLFontFamilyEnum;

typedef enum css_letter_spacing_e HLLetterSpacingEnum;

typedef enum css_stroke_dashoffset_e HLStrokeDashoffsetEnum;

typedef enum css_stroke_dasharray_e HLStrokeDasharrayEnum;

typedef struct HLUsedSvgValues_ {
    HLBaseLineShiftEnum baseline_shift;
    char *clip_path;
    HLClipRuleEnum clip_rule;

    HLColorEnum color_type;
    uint32_t color;

    HLDisplayEnum display;
    HLEnableBackgroundEnum enable_background;
    HLCompOpEnum comp_op;

    HLDirectionEnum direction;


    // fill
    HLFillEnum fill_type;
    char *fill_string;
    uint32_t fill_color;

    HLFillOpacityEnum fill_opacity_type;
    hl_real_t fill_opacity;

    HLFillRuleEnum fill_rule;

    char *filter;

    HLFloodColorEnum flood_color_type;
    uint32_t flood_color;

    HLFloodOpacityEnum flood_opacity_type;
    hl_real_t flood_opacity;

    HLFontFamilyEnum font_family_type;
    char *font_family;

    HLFontSizeEnum font_size_type;
    css_unit font_size_unit;
    hl_real_t font_size;

    HLFontStretchEnum font_stretch;
    HLFontStyleEnum font_style;
    HLFontVariantEnum font_variant;
    HLFontWeightEnum font_weight;

    char *marker_end;
    char *mask;
    char *marker_mid;
    char *marker_start;

    HLOpacityEnum opacity_type;
    hl_real_t opacity;

    HLOverflowEnum overflow;
    HLShapeRenderingEnum shape_rendering;
    HLTextRenderingEnum text_rendering;
    HLStopColorEnum stop_color_type;
    uint32_t stop_color;

    HLStopOpacityEnum stop_opacity_type;
    hl_real_t stop_opacity;

    // stroke
    HLStrokeEnum stroke_type;
    char *stroke_string;
    uint32_t stroke_color;

    HLStrokeDasharrayEnum stroke_dasharray_type;
    size_t stroke_dasharray_count;
    hl_real_t *stroke_dasharray;

    HLStrokeDashoffsetEnum stroke_dashoffset_type;
    css_unit stroke_dashoffset_unit;
    hl_real_t stroke_dashoffset;

    HLStrokeLinecapEnum stroke_linecap;
    HLStrokeLinejoinEnum stroke_linejoin;
    HLStrokeMiterlimitEnum stroke_miterlimit_type;
    hl_real_t stroke_miterlimit;

    HLStrokeOpacityEnum stroke_opacity_type;
    hl_real_t stroke_opacity;

    HLStrokeWidthEnum stroke_width_type;
    css_unit stroke_width_unit;
    hl_real_t stroke_width;

    HLTextAnchorEnum text_anchor;
    HLTextDecorationEnum text_decoration;

    HLUnicodeBidiEnum unicode_bidi;

    HLLetterSpacingEnum letter_spacing_type;
    css_unit letter_spacing_unit;
    hl_real_t letter_spacing;

    HLVisibilityEnum visibility;

    HLWritingModeEnum writing_mode;
} HLUsedSvgValues;

typedef struct HLMedia_ {
    unsigned int width;
    unsigned int height;
    unsigned int dpi;
    unsigned int density;
} HLMedia;


struct DOMRulerCtxt;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup DOM Ruler API
 * @{
 */

/**
 *  create DOMRulerCtxt
 *
 * @param width: media width
 * @param height: media height
 * @param dpi: media dpi
 * @param density: media density
 *
 * Returns: DOMRulerCtxt pointer if success; NULL otherwise.
 *
 * Since: 1.2
 */
struct DOMRulerCtxt *domruler_create(uint32_t width, uint32_t height,
        uint32_t dpi, uint32_t density);

/**
 * Add css data into DOMRulerCtxt.
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param css: the css data
 * @param nr_css: The length of css data , in bytes
 *
 * Returns: 0 if success; an error code (!=0) otherwise.
 *
 * Since: 1.2
 */
int domruler_append_css(struct DOMRulerCtxt *ctxt, const char *css,
        size_t nr_css);

/**
 * layout dom tree
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param root_node: the pointer to the root node
 * @param op: the opinter to DOMRulerNodeOp
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.2
 */
int domruler_layout(struct DOMRulerCtxt *ctxt, void *root_node,
        DOMRulerNodeOp *op);

/**
 * Get HLBox of the node
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param node: the pointer to the node
 *
 * Returns: HLBox pointer if success; NULL otherwise.
 *
 * Since: 1.2
 */
const HLBox *domruler_get_node_bounding_box(struct DOMRulerCtxt *ctxt,
        void *node);

/**
 * Reset the elements cached by the DOMRulerCtxt.
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 *
 * Since: 1.2.1
 */
void domruler_reset_nodes(struct DOMRulerCtxt *ctxt);

/**
 * Destroy DOMRulerCtxt
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 *
 * Since: 1.2
 */
void domruler_destroy(struct DOMRulerCtxt *ctxt);


/**
 * layout HLDomElement
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param root_node: the pointer to the root node HLDomElement
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.2
 */
int domruler_layout_hldom_elements(struct DOMRulerCtxt *ctxt,
        HLDomElement *root_node);

/**
 * layout pcdom_element_t
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param root_node: the pointer to the root node pcdom_element_t
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.2
 */
int domruler_layout_pcdom_elements(struct DOMRulerCtxt *ctxt,
        pcdom_element_t *root_node);

/**
 * Create a HLCSS object
 *
 * Returns: A valid HLCSS object if success, NULL otherwise.
 *
 * Since: 1.0
 */
HLCSS *domruler_css_create();

/**
 * Add css data into HLcss object.
 *
 * @param css: the pointer to the HLCSS
 * @param data: the css data
 * @param len: The length of data , in bytes
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_css_append_data(HLCSS *css, const char *data, size_t len);

/**
 * Free a HLCSS.
 *
 * @param css: the pointer to the HLCSS.
 *
 * Frees the space used by the HLCSS, including the HLCSS itself.
 *
 * Since: 1.0
 */
int domruler_css_destroy(HLCSS *css);

/**
 * Create a HLDomElement object
 *
 * @param tag_name: the tag name of the element node
 *
 * Returns: A valid HLDomElement object if success, NULL otherwise.
 *
 * Since: 1.0
 */
HLDomElement *domruler_element_node_create(const char *tag_name);

/**
 * Get the tag name of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the tag name of the element node
 *
 * Since: 1.0
 */
const char *domruler_element_node_get_tag_name(HLDomElement *node);

/**
 * Set the common attribute for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param attr_id: attribute id
 * @param value: attribute value
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_element_node_set_common_attr(HLDomElement *node,
        HLCommonAttribute attr_id, const char *value);

/**
 * Get the common attribute of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param attr_id: attribute id
 *
 * Returns: the value of the attribute
 *
 * Since: 1.0
 */
const char *domruler_element_node_get_common_attr(const HLDomElement *node,
        HLCommonAttribute attr_id);

/**
 * Set id attribute for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param id: id of the element node
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
static inline int
domruler_element_node_set_id(HLDomElement *node, const char *id)
{
    return domruler_element_node_set_common_attr(node, HL_COMMON_ATTR_ID, id);
}

/**
 * Get the id of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the id of the element node 
 *
 * Since: 1.0
 */
static inline const char *
domruler_element_node_get_id(const HLDomElement *node)
{
    return domruler_element_node_get_common_attr(node, HL_COMMON_ATTR_ID);
}

/**
 * Set class attribute for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param class_name: class of the element node
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
static inline int
domruler_element_node_set_class(HLDomElement *node, const char *class_name)
{
    return domruler_element_node_set_common_attr(node,
            HL_COMMON_ATTR_CLASS_NAME, class_name);
}

/**
 * Get the class of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the class of the element node 
 *
 * Since: 1.0
 */
static inline const char *
domruler_element_node_get_class(const HLDomElement *node)
{
    return domruler_element_node_get_common_attr(node,
            HL_COMMON_ATTR_CLASS_NAME);
}

/**
 * Checks whether the class exists in the element node.
 *
 * @param node: the pointer to the node.
 * @param class_name: the class name
 *
 * Returns: zero if class name exists; (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_element_node_has_class(HLDomElement *node,
        const char *class_name);

/**
 * add class into the element node.
 *
 * @param node: the pointer to the node.
 * @param class_name: the class name
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_element_node_include_class(HLDomElement *node,
        const char *class_name);

/**
 * remove class from the element node.
 *
 * @param node: the pointer to the node.
 * @param class_name: the class name
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_element_node_exclude_class(HLDomElement *node,
        const char *class_name);

/**
 * Set style attribute for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param style: style of the element node
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
static inline int
domruler_element_node_set_style (HLDomElement *node, const char *style)
{
    return domruler_element_node_set_common_attr (node, HL_COMMON_ATTR_STYLE,
            style);
}

/**
 * Get the style of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the style of the element node 
 *
 * Since: 1.0
 */
static inline const char *
domruler_element_node_get_style(const HLDomElement *node)
{
    return domruler_element_node_get_common_attr(node, HL_COMMON_ATTR_STYLE);
}

/**
 * Set name attribute for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param name: name of the element node
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
static inline int
domruler_element_node_set_name(HLDomElement *node, const char *name)
{
    return domruler_element_node_set_common_attr(node, HL_COMMON_ATTR_NAME, name);
}

/**
 * Get the name of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the name of the element node 
 *
 * Since: 1.0
 */
static inline const char *
domruler_element_node_get_name(const HLDomElement *node)
{
    return domruler_element_node_get_common_attr(node, HL_COMMON_ATTR_NAME);
}

/**
 * Set general attribute for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param attr_name: attribute name
 * @param attr_value: attribute value
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_element_node_set_general_attr(HLDomElement *node,
        const char *attr_name, const char *attr_value);

/**
 * get general attribute for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param attr_name: attribute name
 *
 * Returns: attribute value
 *
 * Since: 1.0
 */
const char *
domruler_element_node_get_general_attr(const HLDomElement *node,
        const char *attr_name);

/**
 * Specifies the type of function which is called when the element node is
 * destroyed.
 *
 * @param data: the user data
 *
 * Since: 1.0
 */
typedef void  (*HlDestroyCallback)(void *data);

/**
 * Set user data for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param key: user data key
 * @param data: user data
 * @param destroy_callback: the function to be called to free user data
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_element_node_set_user_data(HLDomElement *node,
        const char *key, void *data, HlDestroyCallback destroy_callback);

/**
 * Get user data of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param key: user data key
 *
 * Returns: user data pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
void *domruler_element_node_get_user_data(const HLDomElement *node,
        const char *key);

/**
 * Set user attach data for the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param index: index of the attach data, limit  MAX_ATTACH_DATA_SIZE
 * @param data: user data
 * @param destroy_callback: the function to be called to free user data
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_element_node_set_attach_data(HLDomElement *node,
        uint32_t index, void *data, HlDestroyCallback destroy_callback);

/**
 * Get user attach data of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 * @param index: user attach data index
 *
 * Returns: user data pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
void *domruler_element_node_get_attach_data(const HLDomElement *node,
        uint32_t index);

/**
 * Free a HLDomElement.
 *
 * @param node: the pointer to the HLDomElement
 *
 * Frees the space used by the HLDomElement, including the
 * HLDomElement itself.
 *
 * Since: 1.0
 */
void domruler_element_node_destroy(HLDomElement *node);

/**
 * @deprecated since 1.2
 * Get HLBox of the HLDomElement
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param node: the pointer to the HLDomElement
 *
 * Returns: HLBox pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
const HLBox *
domruler_element_node_get_used_box_value(struct DOMRulerCtxt *ctxt,
        HLDomElement *node);

/**
 * @deprecated since 1.2
 * Get HLUsedBackgroundValues of the HLDomElement
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param node: the pointer to the HLDomElement
 *
 * Returns: HLUsedBackgroundValues pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
const HLUsedBackgroundValues *
domruler_element_node_get_used_background_value(struct DOMRulerCtxt *ctxt,
        HLDomElement *node);

/**
 * @deprecated since 1.2
 * Get HLUsedTextValues of the HLDomElement
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param node: the pointer to the HLDomElement
 *
 * Returns: HLUsedTextValues pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
const HLUsedTextValues *
domruler_element_node_get_used_text_value(struct DOMRulerCtxt *ctxt,
        HLDomElement *node);

/**
 * @deprecated since 1.2
 * Get HLUsedSvgValues of the HLDomElement
 *
 * @param ctxt: the pointer to the DOMRulerCtxt
 * @param node: the pointer to the HLDomElement
 *
 * Returns: HLUsedSvgValues pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
HLUsedSvgValues *
domruler_element_node_get_used_svg_value(struct DOMRulerCtxt *ctxt,
        HLDomElement *node);

/**
 * add child node for the HLDomElement
 *
 * @param node: the pointer to the child HLDomElement
 * @param parent: the pointer to the parent HLDomElement
 *
 * Returns: zero if success; an error code (!=0) otherwise.
 *
 * Since: 1.0
 */
int domruler_element_node_append_as_last_child(HLDomElement *node,
        HLDomElement *parent);

/**
 * get parent node of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: parent node pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
HLDomElement *domruler_element_node_get_parent(HLDomElement *node);

/**
 * get the first child node of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the first child node pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
HLDomElement *domruler_element_node_get_first_child(HLDomElement *node);

/**
 * get the last child node of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the last child node pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
HLDomElement *domruler_element_node_get_last_child(HLDomElement *node);

/**
 * get the previous child node of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the previous child node pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
HLDomElement *domruler_element_node_get_prev(HLDomElement *node);

/**
 * get the next child node of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the next child node pointer if success; NULL otherwise.
 *
 * Since: 1.0
 */
HLDomElement *domruler_element_node_get_next(HLDomElement *node);

/**
 * get the children count node of the HLDomElement
 *
 * @param node: the pointer to the HLDomElement
 *
 * Returns: the children count.
 *
 * Since: 1.0
 */
uint32_t domruler_element_node_get_children_count(HLDomElement *node);

/**
 * Specifies the type of function which is called when travel the dom tree.
 *
 * @param node: the pointer to the HLDomElement
 * @param user_data: the pointer to the user data 
 *
 * Since: 1.0
 */
typedef void (*NodeCallback)(HLDomElement *node, void *user_data);

/**
 * travel the children of the element node
 *
 * @param node: the pointer to the HLDomElement
 * @param callback: the function to be called for each child
 * @param user_data: the pointer to the user data 
 *
 * Since: 1.0
 */
void domruler_element_node_for_each_child(HLDomElement *node,
        NodeCallback callback, void *user_data);

/**
 * travel the children of the dom tree
 *
 * @param node: the pointer to the HLDomElement
 * @param callback: the function to be called for each child
 * @param user_data: the pointer to the user data 
 *
 * Since: 1.0
 */
void domruler_element_node_depth_first_search_tree(HLDomElement *node,
        NodeCallback callback, void *user_data);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif // _DOMRULER_H_
