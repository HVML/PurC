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

#include "node.h"
#include "utils.h"
#include "hl_pcdom_element_t.h"
#include "csseng/csseng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/ghash.h>


HLNodeType hl_pcdom_element_t_get_type(void *n)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    switch (node->type) {
    case PCDOM_NODE_TYPE_UNDEF:
        return DOM_UNDEF;
    case PCDOM_NODE_TYPE_ELEMENT:
        return DOM_ELEMENT_NODE;
    case PCDOM_NODE_TYPE_ATTRIBUTE:
        return DOM_ATTRIBUTE_NODE;
    case PCDOM_NODE_TYPE_TEXT:
        return DOM_TEXT_NODE;
    case PCDOM_NODE_TYPE_CDATA_SECTION:
        return DOM_CDATA_SECTION_NODE;
    case PCDOM_NODE_TYPE_ENTITY_REFERENCE:
        return DOM_ENTITY_REFERENCE_NODE;
    case PCDOM_NODE_TYPE_ENTITY:
        return DOM_ENTITY_NODE;
    case PCDOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        return DOM_PROCESSING_INSTRUCTION_NODE;
    case PCDOM_NODE_TYPE_COMMENT:
        return DOM_COMMENT_NODE;
    case PCDOM_NODE_TYPE_DOCUMENT:
        return DOM_DOCUMENT_NODE;
    case PCDOM_NODE_TYPE_DOCUMENT_TYPE:
        return DOM_DOCUMENT_TYPE_NODE;
    case PCDOM_NODE_TYPE_DOCUMENT_FRAGMENT:
        return DOM_DOCUMENT_FRAGMENT_NODE;
    case PCDOM_NODE_TYPE_NOTATION:
        return DOM_NOTATION_NODE;
    default:
        break;
    }
    return DOM_UNDEF;
}

const char *hl_pcdom_element_t_get_name(void *n)
{
    pcdom_element_t *elem = (pcdom_element_t *)n;
    const char *name = NULL;

    /* VM: for node not element, return NULL */
    if (elem->node.type == PCDOM_NODE_TYPE_TEXT) {
        name = "TEXT";
    }
    else if (elem->node.type == PCDOM_NODE_TYPE_ELEMENT) {
        name = (const char *)pcdom_element_tag_name(elem, NULL);
    }
    else {
        name = "NOT-INTEND-TO-LAYOUT";
    }

    return name;
}

const char *hl_pcdom_element_t_get_id(void *n)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    if (node->type == PCDOM_NODE_TYPE_ELEMENT) {
        pcdom_element_t *elem = (pcdom_element_t *)n;
        if (elem->attr_id) {
            return (const char *)pcdom_attr_value(elem->attr_id, NULL);
        }
    }

    return NULL;

#if 0
    const char *id = pcdom_element_get_attribute(elem, ATTR_ID, strlen(ATTR_ID), NULL);
    return id;
#endif
}

#define WHITESPACE      " "
int hl_pcdom_element_t_get_classes(void *n, char ***classes)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    if (node->type != PCDOM_NODE_TYPE_ELEMENT) {
        return 0;
    }

    pcdom_element_t *elem = (pcdom_element_t *)n;

    *classes = NULL;
#if 0
    const char *cls =  pcdom_element_get_attribute(elem, ATTR_CLASS,
            strlen(ATTR_CLASS), NULL);
    if (cls == NULL) {
        return 0;
    }
#else
    const char *cls;
    if (elem->attr_class) {
        cls = (const char *)pcdom_attr_value(elem->attr_class, NULL);
    }
    else
        return 0;
#endif
    int nr_classes = 0;
    char *value = strdup(cls);
    char *c = strtok(value, WHITESPACE);
    while (c != NULL) {
        nr_classes++;
        char **space = (char **)realloc(*classes, sizeof(char *) * nr_classes);
        if (space == NULL) {
            return nr_classes - 1;
        }
        *classes = space;
        (*classes)[nr_classes - 1] = strdup(c);
        c = strtok(NULL, WHITESPACE);
    }
    free(value);
    return nr_classes;
}

const char *hl_pcdom_element_t_get_attr(void *n, const char *name)
{
    pcdom_element_t *elem = (pcdom_element_t *)n;
    return (const char *)pcdom_element_get_attribute(elem,
            (const unsigned char *)name, strlen(name), NULL);
}

void hl_pcdom_element_t_set_parent(void *n, void *parent)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    node->parent = (pcdom_node_t *)parent;
}

void *hl_pcdom_element_t_get_parent(void *n)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    pcdom_node_t *parent = node->parent;
    if (parent && parent->type == PCDOM_NODE_TYPE_DOCUMENT) {
        return NULL;
    }
    return parent;
}

void *hl_pcdom_element_t_first_child(void *n)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    return node->first_child;
}

void *hl_pcdom_element_t_next(void *n)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    pcdom_node_t *next = pcdom_node_next(node);
    if (next && next->type == PCDOM_NODE_TYPE_UNDEF) {
        return NULL;
    }
    return next;
}

void *hl_pcdom_element_t_previous(void *n)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    return node->prev;
}

bool hl_pcdom_element_t_is_root(void *n)
{
    pcdom_node_t *node = (pcdom_node_t *)n;
    if (node->parent == NULL || node->parent->type == PCDOM_NODE_TYPE_DOCUMENT) {
        return true;
    }
    return false;
}

DOMRulerNodeOp hl_pcdom_element_t_op = {
    .get_type = hl_pcdom_element_t_get_type,
    .get_name = hl_pcdom_element_t_get_name,
    .get_id = hl_pcdom_element_t_get_id,
    .get_classes = hl_pcdom_element_t_get_classes,
    .get_attr = hl_pcdom_element_t_get_attr,
    .set_parent = hl_pcdom_element_t_set_parent,
    .get_parent = hl_pcdom_element_t_get_parent,
    .first_child = hl_pcdom_element_t_first_child,
    .next = hl_pcdom_element_t_next,
    .previous = hl_pcdom_element_t_previous,
    .is_root = hl_pcdom_element_t_is_root
};

DOMRulerNodeOp *hl_pcdom_element_t_get_op()
{
    return &hl_pcdom_element_t_op;
}

