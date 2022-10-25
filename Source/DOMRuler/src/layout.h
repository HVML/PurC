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

#ifndef _HL_LAYOUT_H_
#define _HL_LAYOUT_H_

#include "node.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

int hl_computed_z_index(HLLayoutNode *node);

int hl_layout_do_layout(struct DOMRulerCtxt* ctx, HLLayoutNode *root);
int hl_layout_child_node_grid(struct DOMRulerCtxt* ctx, HLLayoutNode *node,
        int level);

#ifdef __cplusplus
}
#endif

#endif // _HL_LAYOUT_H_
