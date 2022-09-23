/*
** This file is part of DOM Ruler. DOM Ruler is a library to
** maintain a DOM tree, lay out and stylize the DOM nodes by
** using CSS (Cascaded Style Sheets).
**
** Copyright (C) 2022 Beijing FMSoft Technologies Co., Ltd.
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

#include "layout.h"
#include "select.h"
#include "utils.h"
#include "hl_dom_element_node.h"

#include <stdio.h>
#include <stdlib.h>

typedef uint8_t (*css_len_func)(const css_computed_style *style,
        css_fixed *length, css_unit *unit);
typedef uint8_t (*css_border_style_func)(const css_computed_style *style);
typedef uint8_t (*css_border_color_func)(const css_computed_style *style,
        css_color *color);

/* Array of per-side access functions for computed style margins. */
static const css_len_func margin_funcs[4] = {
    css_computed_margin_top,
    css_computed_margin_right,
    css_computed_margin_bottom,
    css_computed_margin_left,
};

/* Array of per-side access functions for computed style paddings. */
static const css_len_func padding_funcs[4] = {
    css_computed_padding_top,
    css_computed_padding_right,
    css_computed_padding_bottom,
    css_computed_padding_left,
};

/* Array of per-side access functions for computed style border_widths. */
static const css_len_func border_width_funcs[4] = {
    css_computed_border_top_width,
    css_computed_border_right_width,
    css_computed_border_bottom_width,
    css_computed_border_left_width,
};

/* Array of per-side access functions for computed style border styles. */
static const css_border_style_func border_style_funcs[4] = {
    css_computed_border_top_style,
    css_computed_border_right_style,
    css_computed_border_bottom_style,
    css_computed_border_left_style,
};

/* Array of per-side access functions for computed style border colors.
static const css_border_color_func border_color_funcs[4] = {
    css_computed_border_top_color,
    css_computed_border_right_color,
    css_computed_border_bottom_color,
    css_computed_border_left_color,
};
*/

int hl_select_child_style(const css_media *media, css_select_ctx *select_ctx,
        HiLayoutNode *node)
{
    int ret = hl_select_node_style(media, select_ctx, node);
    if (ret != DOMRULER_OK) {
        return ret;
    }
    HiLayoutNode *child = hi_layout_node_first_child(node);
    while(child) {
        ret = hl_select_child_style(media, select_ctx, child);
        if (ret != DOMRULER_OK) {
            return ret;
        }
        child = hi_layout_node_next(child);
    }
    return DOMRULER_OK;
}


void hl_calculate_mbp_width(const struct DOMRulerCtxt *len_ctx,
            const css_computed_style *style, unsigned int side,
            bool margin, bool border, bool padding,
            int *fixed, float *frac)
{
    css_fixed value = 0;
    css_unit unit = CSS_UNIT_PX;

    assert(style);

    /* margin */
    if (margin) {
        enum css_margin_e type;

        type = margin_funcs[side](style, &value, &unit);
        if (type == CSS_MARGIN_SET) {
            if (unit == CSS_UNIT_PCT) {
                *frac += FIXTOINT(FDIV(value, F_100));
            } else {
                *fixed += FIXTOINT(hl_css_len2px(len_ctx, value, unit, style));
            }
        }
    }

    /* border */
    if (border) {
        if (border_style_funcs[side](style) != CSS_BORDER_STYLE_NONE) {
            border_width_funcs[side](style, &value, &unit);
            *fixed += FIXTOINT(hl_css_len2px(len_ctx, value, unit, style));
        }
    }

    /* padding */
    if (padding) {
        padding_funcs[side](style, &value, &unit);
        if (unit == CSS_UNIT_PCT) {
            *frac += FIXTOINT(FDIV(value, F_100));
        } else {
            *fixed += FIXTOINT(hl_css_len2px(len_ctx, value, unit, style));
        }
    }
}

void hl_handle_box_sizing(const struct DOMRulerCtxt *len_ctx, HiLayoutNode *node,
        int available_width, bool setwidth, int *dimension)
{
    enum css_box_sizing_e bs;

    assert(node && node->computed_style);

    bs = css_computed_box_sizing(node->computed_style);

    if (bs == CSS_BOX_SIZING_BORDER_BOX) {
        int orig = *dimension;
        int fixed = 0;
        float frac = 0;

        hl_calculate_mbp_width(len_ctx, node->computed_style,
                setwidth ? HL_LEFT : HL_TOP,
                false, true, true, &fixed, &frac);
        hl_calculate_mbp_width(len_ctx, node->computed_style,
                setwidth ? HL_RIGHT : HL_BOTTOM,
                false, true, true, &fixed, &frac);
        orig -= frac * available_width + fixed;
        *dimension = orig > 0 ? orig : 0;
    }
}

void hl_find_dimensions(const struct DOMRulerCtxt *len_ctx, int available_width,
               int viewport_height, HiLayoutNode *box,
               const css_computed_style *style,
               int *width, int *height,
               int *max_width, int *min_width,
               int *max_height, int *min_height
               )
{
    HiLayoutNode *containing_block = NULL;
    HiLayoutNode *parent = hi_layout_node_get_parent(box);

    if (width) {
        enum css_width_e wtype;
        css_fixed value = 0;
        css_unit unit = CSS_UNIT_PX;

        wtype = css_computed_width(style, &value, &unit);

        if (wtype == CSS_WIDTH_SET) {
            if (unit == CSS_UNIT_PCT) {
                *width = HL_FPCT_OF_INT_TOINT( value, available_width);
            } else {
                *width = FIXTOINT(hl_css_len2px(len_ctx, value, unit, style));
            }
        } else {
            *width = HL_AUTO;
        }

        if (*width != HL_AUTO) {
            hl_handle_box_sizing(len_ctx, box, available_width, true, width);
        }
    }

    if (height) {
        enum css_height_e htype;
        css_fixed value = 0;
        css_unit unit = CSS_UNIT_PX;

        htype = css_computed_height(style, &value, &unit);

        if (htype == CSS_HEIGHT_SET) {
            if (unit == CSS_UNIT_PCT) {
                enum css_height_e cbhtype;

                if (parent && parent->layout_type != LAYOUT_INLINE_CONTAINER) {
                    /* Box is a block level element */
                    containing_block = parent;
                } else if (parent && parent->layout_type ==
                        LAYOUT_INLINE_CONTAINER) {
                    /* Box is an inline block */
                    containing_block = hi_layout_node_get_parent(parent);
                    assert(containing_block);
                }

                if (containing_block) {
                    css_fixed f = 0;
                    css_unit u = CSS_UNIT_PX;

                    cbhtype = css_computed_height(
                            containing_block->computed_style, &f, &u);
                }

                if (containing_block &&
                        containing_block->box_values.h != HL_AUTO &&
                        (css_computed_position(box->computed_style) ==
                         CSS_POSITION_ABSOLUTE ||
                         cbhtype == CSS_HEIGHT_SET)) {
                    /* Box is absolutely positioned or its
                     * containing block has a valid
                     * specified height.
                     * (CSS 2.1 Section 10.5) */
                    *height = HL_FPCT_OF_INT_TOINT(value,
                            containing_block->box_values.h);
                } else if ((!parent || !hi_layout_node_get_parent(parent)) &&
                        viewport_height >= 0) {
                    /* If root element or it's child
                     * (HTML or BODY) */
                    *height = HL_FPCT_OF_INT_TOINT(value,
                            viewport_height);
                } else {
                    /* precentage height not permissible
                     * treat height as auto */
                    *height = HL_AUTO;
                }
            } else {
                *height = FIXTOINT(hl_css_len2px(len_ctx,
                            value, unit, style));
            }
        } else {
            *height = HL_AUTO;
        }

        if (*height != HL_AUTO) {
            hl_handle_box_sizing(len_ctx, box, available_width,
                    false, height);
        }
    }

    if (max_width) {
        enum css_max_width_e type;
        css_fixed value = 0;
        css_unit unit = CSS_UNIT_PX;

        type = css_computed_max_width(style, &value, &unit);

        if (type == CSS_MAX_WIDTH_SET) {
            if (unit == CSS_UNIT_PCT) {
                *max_width = HL_FPCT_OF_INT_TOINT(value,
                        available_width);
            } else {
                *max_width = FIXTOINT(hl_css_len2px(len_ctx,
                        value, unit, style));
            }
        } else {
            /* Inadmissible */
            *max_width = -1;
        }

        if (*max_width != -1) {
            hl_handle_box_sizing(len_ctx, box, available_width,
                    true, max_width);
        }
    }

    if (min_width) {
        enum css_min_width_e type;
        css_fixed value = 0;
        css_unit unit = CSS_UNIT_PX;

        type = hl_computed_min_width(style, &value, &unit);

        if (type == CSS_MIN_WIDTH_SET) {
            if (unit == CSS_UNIT_PCT) {
                *min_width = HL_FPCT_OF_INT_TOINT(value,
                        available_width);
            } else {
                *min_width = FIXTOINT(hl_css_len2px(len_ctx,
                        value, unit, style));
            }
        } else {
            /* Inadmissible */
            *min_width = 0;
        }

        if (*min_width != 0) {
            hl_handle_box_sizing(len_ctx, box, available_width,
                    true, min_width);
        }
    }

    if (max_height) {
        enum css_max_height_e type;
        css_fixed value = 0;
        css_unit unit = CSS_UNIT_PX;

        type = css_computed_max_height(style, &value, &unit);

        if (type == CSS_MAX_HEIGHT_SET) {
            if (unit == CSS_UNIT_PCT) {
                /* TODO: handle percentage */
                *max_height = -1;
            } else {
                *max_height = FIXTOINT(hl_css_len2px(len_ctx,
                        value, unit, style));
            }
        } else {
            /* Inadmissible */
            *max_height = -1;
        }
    }

    if (min_height) {
        enum css_min_height_e type;
        css_fixed value = 0;
        css_unit unit = CSS_UNIT_PX;

        type = hl_computed_min_height(style, &value, &unit);

        if (type == CSS_MIN_HEIGHT_SET) {
            if (unit == CSS_UNIT_PCT) {
                /* TODO: handle percentage */
                *min_height = 0;
            } else {
                *min_height = FIXTOINT(hl_css_len2px(len_ctx,
                        value, unit, style));
            }
        } else {
            /* Inadmissible */
            *min_height = 0;
        }
    }
}

int hl_solve_width(HiLayoutNode *box,
           int available_width,
           int width,
           int lm,
           int rm,
           int max_width,
           int min_width)
{
    bool auto_width = false;

    /* Increase specified left/right margins */
    if (box->margin[HL_LEFT] != HL_AUTO && box->margin[HL_LEFT] < lm &&
            box->margin[HL_LEFT] >= 0)
        box->margin[HL_LEFT] = lm;
    if (box->margin[HL_RIGHT] != HL_AUTO && box->margin[HL_RIGHT] < rm &&
            box->margin[HL_RIGHT] >= 0)
        box->margin[HL_RIGHT] = rm;

    /* Find width */
    if (width == HL_AUTO) {
        int margin_left = box->margin[HL_LEFT];
        int margin_right = box->margin[HL_RIGHT];

        if (margin_left == HL_AUTO) {
            margin_left = lm;
        }
        if (margin_right == HL_AUTO) {
            margin_right = rm;
        }

        width = available_width -
                (margin_left + box->border[HL_LEFT] +
                box->padding[HL_LEFT] + box->padding[HL_RIGHT] +
                box->border[HL_RIGHT] + margin_right);
        width = width < 0 ? 0 : width;
        auto_width = true;
    }

    if (max_width >= 0 && width > max_width) {
        /* max-width is admissable and width exceeds max-width */
        width = max_width;
        auto_width = false;
    }

    if (min_width > 0 && width < min_width) {
        /* min-width is admissable and width is less than max-width */
        width = min_width;
        auto_width = false;
    }

    /* Width was auto, and unconstrained by min/max width, so we're done */
    if (auto_width) {
        /* any other 'auto' become 0 or the minimum required values */
        if (box->margin[HL_LEFT] == HL_AUTO) {
            box->margin[HL_LEFT] = lm;
        }
        if (box->margin[HL_RIGHT] == HL_AUTO) {
            box->margin[HL_RIGHT] = rm;
        }
        return width;
    }

    /* Width was not auto, or was constrained by min/max width
     * Need to compute left/right margins */

    /* HTML alignment (only applies to over-constrained boxes) */
    HiLayoutNode *parent = hi_layout_node_get_parent(box);
    if (box->margin[HL_LEFT] != HL_AUTO && box->margin[HL_RIGHT] != HL_AUTO &&
            parent != NULL && parent->computed_style != NULL) {
        switch (css_computed_text_align(parent->computed_style)) {
        case CSS_TEXT_ALIGN_LIBCSS_RIGHT:
            box->margin[HL_LEFT] = HL_AUTO;
            box->margin[HL_RIGHT] = 0;
            break;
        case CSS_TEXT_ALIGN_LIBCSS_CENTER:
            box->margin[HL_LEFT] = box->margin[HL_RIGHT] = HL_AUTO;
            break;
        case CSS_TEXT_ALIGN_LIBCSS_LEFT:
            box->margin[HL_LEFT] = 0;
            box->margin[HL_RIGHT] = HL_AUTO;
            break;
        default:
            /* Leave it alone; no HTML alignment */
            break;
        }
    }

    if (box->margin[HL_LEFT] == HL_AUTO && box->margin[HL_RIGHT] == HL_AUTO) {
        /* make the margins equal, centering the element */
        box->margin[HL_LEFT] = box->margin[HL_RIGHT] =
                (available_width - lm - rm -
                (box->border[HL_LEFT] + box->padding[HL_LEFT] +
                width + box->padding[HL_RIGHT] +
                box->border[HL_RIGHT])) / 2;

        if (box->margin[HL_LEFT] < 0) {
            box->margin[HL_RIGHT] += box->margin[HL_LEFT];
            box->margin[HL_LEFT] = 0;
        }

        box->margin[HL_LEFT] += lm;

    } else if (box->margin[HL_LEFT] == HL_AUTO) {
        box->margin[HL_LEFT] = available_width - lm -
                (box->border[HL_LEFT] + box->padding[HL_LEFT] +
                width + box->padding[HL_RIGHT] +
                box->border[HL_RIGHT] + box->margin[HL_RIGHT]);
        box->margin[HL_LEFT] = box->margin[HL_LEFT] < lm
                ? lm : box->margin[HL_LEFT];
    } else {
        /* margin-right auto or "over-constrained" */
        box->margin[HL_RIGHT] = available_width - rm -
                (box->margin[HL_LEFT] + box->border[HL_LEFT] +
                 box->padding[HL_LEFT] + width +
                 box->padding[HL_RIGHT] +
                 box->border[HL_RIGHT]);
    }

    return width;
}

int hl_computed_z_index(HiLayoutNode *node)
{
    int32_t index = 0;
    int8_t val = css_computed_z_index(node->computed_style, &index);
    switch (val) {
    case CSS_Z_INDEX_INHERIT:
        {
            HiLayoutNode *parent = hi_layout_node_get_parent(node);
            if (parent) {
                index = parent->box_values.z_index;
            }
            else {
                index = 0;
            }
        }
        break;

    case CSS_Z_INDEX_AUTO:
        break;

    case CSS_Z_INDEX_SET:
        index = FIXTOINT(index);
        break;

    default:
        break;
    }
    node->box_values.z_index = index;
    return index;
}

int hl_block_find_dimensions(struct DOMRulerCtxt *ctx,
        HiLayoutNode *node,
        int container_width,
        int container_height,
        int lm,
        int rm)
{
    (void)lm;
    (void)rm;
    int width = 0;
    int max_width = 0;
    int min_width = 0;
    int height = 0;
    int max_height = 0;
    int min_height = 0;

    hl_find_dimensions(ctx,
            container_width,
            container_height,
            node,
            node->computed_style,
            &width,
            &height,
            &max_width,
            &min_width,
            &max_height,
            &min_height
            );
    int sw = hl_solve_width(node, container_width, width, 0, 0,
            max_width, min_width);
    int sh = height;
    node->box_values.w = sw;
    node->box_values.h = sh;
    return 0;
}

void hl_computed_offsets(const struct DOMRulerCtxt *len_ctx,
               HiLayoutNode *box,
               HiLayoutNode *containing_block,
               int *top,
               int *right,
               int *bottom,
               int *left
               )
{
    uint32_t type;
    css_fixed value = 0;
    css_unit unit = CSS_UNIT_PX;

#if 0
    assert(containing_block->box_values.w != HL_UNKNOWN &&
            containing_block->box_values.w != HL_AUTO &&
            containing_block->box_values.h != HL_AUTO);
#endif

    /* left */
    type = css_computed_left(box->computed_style, &value, &unit);
    if (type == CSS_LEFT_SET) {
        if (unit == CSS_UNIT_PCT) {
            *left = HL_FPCT_OF_INT_TOINT(value,
                    containing_block->box_values.w);
        } else {
            *left = FIXTOINT(hl_css_len2px(len_ctx,
                    value, unit, box->computed_style));
        }
    } else {
        *left = HL_AUTO;
    }

    /* right */
    type = css_computed_right(box->computed_style, &value, &unit);
    if (type == CSS_RIGHT_SET) {
        if (unit == CSS_UNIT_PCT) {
            *right = HL_FPCT_OF_INT_TOINT(value,
                    containing_block->box_values.w);
        } else {
            *right = FIXTOINT(hl_css_len2px(len_ctx,
                    value, unit, box->computed_style));
        }
    } else {
        *right = HL_AUTO;
    }

    /* top */
    type = css_computed_top(box->computed_style, &value, &unit);
    if (type == CSS_TOP_SET) {
        if (unit == CSS_UNIT_PCT) {
            *top = HL_FPCT_OF_INT_TOINT(value,
                    containing_block->box_values.h);
        } else {
            *top = FIXTOINT(hl_css_len2px(len_ctx,
                    value, unit, box->computed_style));
        }
    } else {
        *top = HL_AUTO;
    }

    /* bottom */
    type = css_computed_bottom(box->computed_style, &value, &unit);
    if (type == CSS_BOTTOM_SET) {
        if (unit == CSS_UNIT_PCT) {
            *bottom = HL_FPCT_OF_INT_TOINT(value,
                    containing_block->box_values.h);
        } else {
            *bottom = FIXTOINT(hl_css_len2px(len_ctx,
                    value, unit, box->computed_style));
        }
    } else {
        *bottom = HL_AUTO;
    }
}

int hl_layout_node(struct DOMRulerCtxt *ctx, HiLayoutNode *node, int x, int y,
        int container_width, int container_height, int level)
{
    if (node == NULL) {
        HL_LOGD("layout node|level=%d|node=%p(%p)|name=%s|id=%s\n", level,
                node, node->origin, hi_layout_node_get_name(node),
                hi_layout_node_get_id(node));
        return DOMRULER_OK;
    }

    node->box_values.x = x;
    node->box_values.y = y;

    hl_computed_z_index(node);
    hl_find_background(node);
    hl_find_font(ctx, node);

    // filter non element node
    if (hi_layout_node_get_type(node) != DOM_ELEMENT_NODE) {
        return DOMRULER_OK;
    }

    if (hi_layout_node_is_root(node)) {
        node->box_values.w = container_width;
        node->box_values.h = container_height;
    }
    else if (css_computed_position(node->computed_style) ==
            CSS_POSITION_FIXED) {
        HiLayoutNode *parent = hi_layout_node_get_parent(node);
        hi_layout_node_set_parent(node, ctx->root);
        hl_block_find_dimensions(ctx, node, ctx->root->box_values.w,
                ctx->root->box_values.h, 0, 0);
        hi_layout_node_set_parent(node, parent);
    }
    else {
        switch (node->layout_type) {
        case LAYOUT_BLOCK:
            hl_block_find_dimensions(ctx, node, container_width,
                    container_height, 0, 0);
            break;

        case LAYOUT_INLINE_BLOCK:
            hl_block_find_dimensions(ctx, node, container_width,
                    container_height, 0, 0);
            break;

        case LAYOUT_GRID:
            hl_block_find_dimensions(ctx, node, container_width,
                    container_height, 0, 0);
            break;

        case LAYOUT_INLINE_GRID:
            hl_block_find_dimensions(ctx, node, container_width,
                    container_height, 0, 0);
            break;

        case LAYOUT_NONE:
            return DOMRULER_OK;

        default:
            hl_block_find_dimensions(ctx, node, container_width,
                    container_height, 0, 0);
            break;
        }
    }

    HiLayoutNode *child = hi_layout_node_first_child(node);
    if (child == NULL) {
        HL_LOGD("layout node end|level=%d|node=%p(%p)|name=%s|id=%s|"
                "(x, y, w, h)=(%d, %d, %d, %d)\n", level, node, node->origin,
                hi_layout_node_get_name(node), hi_layout_node_get_id(node),
                (int)node->box_values.x, (int)node->box_values.y,
                (int)node->box_values.w, (int)node->box_values.h);
        return DOMRULER_OK;
    }

    switch (node->layout_type) {
    case LAYOUT_GRID:
    case LAYOUT_INLINE_GRID:
        return hl_layout_child_node_grid(ctx, node, level);

    default:
        break;
    }

    int cx = x;
    int cy = y;
    int cw = node->box_values.w;
    int ch = node->box_values.h;
    int cl = level + 1;

    int top = 0;
    int right = 0;
    int bottom = 0;
    int left = 0;
    int line_height = 0;
    int prev_width = 0;
    while(child) {
        // filter non element node
        if (hi_layout_node_get_type(child) != DOM_ELEMENT_NODE) {
            child = hi_layout_node_next(child);
            continue;
        }

        if (css_computed_position(child->computed_style) ==
                CSS_POSITION_FIXED) {
            int x = ctx->root->box_values.x;
            int y = ctx->root->box_values.y;
            int w = ctx->root->box_values.w;
            int h = ctx->root->box_values.h;

            hl_computed_offsets(ctx, child, ctx->root, &top, &right, &bottom,
                    &left);
            if (left == HL_AUTO)
                left = 0;

            if (top == HL_AUTO)
                top = 0;
            hl_layout_node(ctx, child, x + left, y + top, w, h, cl);
            line_height = 0;
            child = hi_layout_node_next(child);
            continue;
        }

        switch (child->layout_type) {
        case LAYOUT_BLOCK:
        case LAYOUT_GRID:
            cx = x;
            if (css_computed_position(child->computed_style) ==
                    CSS_POSITION_RELATIVE) {
                hl_computed_offsets(ctx, child, node, &top, &right,
                        &bottom, &left);
            }
            cy = cy + line_height;
            hl_layout_node(ctx, child, cx + left, cy + top, cw, ch, cl);
            line_height = 0;
            break;

        case LAYOUT_INLINE_BLOCK:
        case LAYOUT_INLINE_GRID:
            {
                if (css_computed_position(child->computed_style) ==
                        CSS_POSITION_RELATIVE) {
                    hl_computed_offsets(ctx, child, node, &top, &right, &bottom,
                            &left);
                }
                hl_block_find_dimensions(ctx, child, cw, ch, 0, 0);
                HiLayoutNode *previous = hi_layout_node_previous(child);
                if (previous != NULL
                        && (previous->layout_type == LAYOUT_BLOCK
                            || previous->layout_type == LAYOUT_GRID)
                        ) {
                    cx = x;
                    cy = cy + line_height;
                }
                else if (cx + prev_width + child->box_values.w + left > cw) {
                    cx = x;
                    cy = cy + line_height;
                }
                else {
                    cx = cx + prev_width;
                }
                hl_layout_node(ctx, child, cx + left, cy + top, cw, ch, cl);
                prev_width = child->box_values.w;
            }
            break;

        default:
            cx = x;
            if (css_computed_position(child->computed_style) ==
                    CSS_POSITION_RELATIVE) {
                hl_computed_offsets(ctx, child, node, &top, &right, &bottom,
                        &left);
            }
            cy = cy + line_height;
            hl_layout_node(ctx, child, cx + left, cy + top, cw, ch, cl);
            line_height = 0;
            break;
        }
        line_height = line_height < child->box_values.h ?
            child->box_values.h : line_height;
        child = hi_layout_node_next(child);
    }

    HL_LOGD("layout node end|level=%d|node=%p(%p)|name=%s|id=%s|"
            "(x, y, w, h)=(%d, %d, %d, %d)\n", level, node, node->origin,
            hi_layout_node_get_name(node), hi_layout_node_get_id(node),
            (int)node->box_values.x, (int)node->box_values.y,
            (int)node->box_values.w, (int)node->box_values.h);
    return DOMRULER_OK;
}

int hi_layout_do_layout(struct DOMRulerCtxt *ctxt, HiLayoutNode *root)
{
    if (ctxt == NULL || ctxt->css == NULL || ctxt->css->sheet == NULL) {
        return DOMRULER_BADPARM;
    }

    hl_set_media_dpi(ctxt, ctxt->dpi);
    hl_set_baseline_pixel_density(ctxt, ctxt->density);

    css_media m;
    m.type = CSS_MEDIA_SCREEN;
    m.width  = hl_css_pixels_physical_to_css(ctxt, INTTOFIX(ctxt->width));
    m.height = hl_css_pixels_physical_to_css(ctxt, INTTOFIX(ctxt->height));
    ctxt->vw = m.width;
    ctxt->vh = m.height;
    ctxt->root = root;

    // create css select context
    css_select_ctx *select_ctx = hl_css_select_ctx_create(ctxt->css);

    int ret = hl_select_child_style(&m, select_ctx, root);
    if (ret != DOMRULER_OK) {
        HL_LOGD("%s|select child style failed.|code=%d\n", __func__, ret);
        hl_css_select_ctx_destroy(select_ctx);
        return ret;
    }
    ctxt->root_style = root->computed_style;

    hl_layout_node(ctxt, root, 0, 0, ctxt->width, ctxt->height, 0);
    hl_css_select_ctx_destroy(select_ctx);
    return ret;
}


