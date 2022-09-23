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
*  This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General License for more details.
**
** You should have received a copy of the GNU Lesser General License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _HL_DOM_ELEMENT_NODE_H
#define _HL_DOM_ELEMENT_NODE_H

#include "domruler.h"
#include "node.h"
#include "csseng/csseng.h"

#include <glib.h>
#include <glib/ghash.h>

#define HL_INNER_CSS_SELECT_ATTACH "hl_inner_css_select_attach"
#define HL_INNER_LAYOUT_ATTACH "hl_inner_layout_attach"

typedef struct HiLayoutNode HiLayoutNode;

typedef struct HLDomElement_ {
    struct HLDomElement_* parent;  /**< Parent node */
    struct HLDomElement_* first_child; /**< First child node */
    struct HLDomElement_* last_child;  /**< Last child node */
    struct HLDomElement_* previous;    /**< Previous sibling */
    struct HLDomElement_* next;        /**< Next sibling */
    uint32_t n_children;        // child count;

    char* tag;

    GHashTable* common_attrs;  // common attrs key(uint64_t) -> value(string)

    GHashTable* general_attrs;     // user attrs key(string) -> value(string)
    GHashTable* user_data;     // user data key(string) -> value(HLAttachData)

    GHashTable* inner_attrs;    // inner attrs key(string) -> value(string)
    GHashTable* inner_data;     // inner data key(string) -> value(HLAttachData)

    HLAttachData* attach_data; // attach data


    // class name
    GList* class_list;

    HLNodeType inner_dom_type;

} HLDomElement;

#ifdef __cplusplus
extern "C" {
#endif

int hl_element_node_set_inner_attr(HLDomElement* node, const char* attr_name, const char* attr_value);
const char* hl_element_node_get_inner_attr(HLDomElement* node, const char* attr_name);
int hl_element_node_set_inner_data(HLDomElement* node, const char* key, void* data, HlDestroyCallback destroy_callback);
void* hl_element_node_get_inner_data(HLDomElement* node, const char* key);

// HLDomElelementNode op
void hl_dom_element_node_set_attach(void *node, void *data,
        cb_free_attach_data cb_free);
void *hl_dom_element_node_get_attach(void *node, cb_free_attach_data *cb_free);
HLNodeType hl_dom_element_node_get_type(void *node);
const char *hl_dom_element_node_get_name(void *node);
const char *hl_dom_element_node_get_id(void *node);
int hl_dom_element_node_get_classes(void *node, char ***classes);
const char *hl_dom_element_node_get_attr(void *node, const char *attr);
void hl_dom_element_node_set_parent(void *node, void *parent);
void *hl_dom_element_node_get_parent(void *node);
void *hl_dom_element_node_first_child(void *node);
void *hl_dom_element_node_next(void *node);
void *hl_dom_element_node_previous(void *node);
bool hl_dom_element_node_is_root(void *node);

DOMRulerNodeOp *hl_dom_element_node_get_op();
// End HLDomElelementNode op

#ifdef __cplusplus
}
#endif

#endif // _HL_DOM_ELEMENT_NODE_H
