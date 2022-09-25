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

#ifndef _HL_PCDOM_ELEMENT_T_H
#define _HL_PCDOM_ELEMENT_T_H

#include "domruler.h"
#include "node.h"
#include "csseng/csseng.h"

#include <glib.h>
#include <glib/ghash.h>

void pcdom_node_set_attach(void *node, void *data,
        cb_free_attach_data cb_free);
void *pcdom_node_get_attach(void *node, cb_free_attach_data *cb_free);

DOMRulerNodeOp *pcdom_node_get_op();

#ifdef __cplusplus
}
#endif

#endif // _HL_PCDOM_ELEMENT_T_H
