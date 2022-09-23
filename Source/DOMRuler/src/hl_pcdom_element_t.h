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

void hl_pcdom_element_t_set_attach(void *node, void *data,
        cb_free_attach_data cb_free);
void *hl_pcdom_element_t_get_attach(void *node, cb_free_attach_data *cb_free);
HLNodeType hl_pcdom_element_t_get_type(void *node);
const char *hl_pcdom_element_t_get_name(void *node);
const char *hl_pcdom_element_t_get_id(void *node);
int hl_pcdom_element_t_get_classes(void *node, char ***classes);
const char *hl_pcdom_element_t_get_attr(void *node, const char *attr);
void hl_pcdom_element_t_set_parent(void *node, void *parent);
void *hl_pcdom_element_t_get_parent(void *node);
void *hl_pcdom_element_t_first_child(void *node);
void *hl_pcdom_element_t_next(void *node);
void *hl_pcdom_element_t_previous(void *node);
bool hl_pcdom_element_t_is_root(void *node);

DOMRulerNodeOp *hl_pcdom_element_t_get_op();

#ifdef __cplusplus
}
#endif

#endif // _HL_PCDOM_ELEMENT_T_H
