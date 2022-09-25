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

#ifndef _HL_INTERNAL_H
#define _HL_INTERNAL_H

#include "domruler.h"
#include <csseng/csseng.h>

#include <glib.h>
#include <glib/ghash.h>

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

struct HLLayoutNode;

struct DOMRulerCtxt {
    // media
    uint32_t width;
    uint32_t height;
    uint32_t dpi;
    uint32_t density;

    // css
    HLCSS *css;
    css_fixed hl_css_media_dpi;
    css_fixed hl_css_baseline_pixel_density;

    int vw;
    int vh;
    const css_computed_style *root_style;

    struct HLLayoutNode *root;

    // Origin Root Node
    void *origin_root;
    DOMRulerNodeOp *origin_op;

    GHashTable *node_map; // key(origin node pointer) -> value(HLLayoutNode *)
};

typedef void (*cb_free_attach_data) (void *data);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif // _HL_INTERNAL_H

