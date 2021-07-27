/**
 * @file node.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html node.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef PCHTML_DOM_NODE_H
#define PCHTML_DOM_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "private/edom/interface.h"
#include "private/edom/event_target.h"


typedef pchtml_action_t
(*pchtml_dom_node_simple_walker_f)(pchtml_dom_node_t *node, void *ctx);


typedef enum {
    PCHTML_DOM_NODE_TYPE_UNDEF                  = 0x00,
    PCHTML_DOM_NODE_TYPE_ELEMENT                = 0x01,
    PCHTML_DOM_NODE_TYPE_ATTRIBUTE              = 0x02,
    PCHTML_DOM_NODE_TYPE_TEXT                   = 0x03,
    PCHTML_DOM_NODE_TYPE_CDATA_SECTION          = 0x04,
    PCHTML_DOM_NODE_TYPE_ENTITY_REFERENCE       = 0x05, // historical
    PCHTML_DOM_NODE_TYPE_ENTITY                 = 0x06, // historical
    PCHTML_DOM_NODE_TYPE_PROCESSING_INSTRUCTION = 0x07,
    PCHTML_DOM_NODE_TYPE_COMMENT                = 0x08,
    PCHTML_DOM_NODE_TYPE_DOCUMENT               = 0x09,
    PCHTML_DOM_NODE_TYPE_DOCUMENT_TYPE          = 0x0A,
    PCHTML_DOM_NODE_TYPE_DOCUMENT_FRAGMENT      = 0x0B,
    PCHTML_DOM_NODE_TYPE_NOTATION               = 0x0C, // historical
    PCHTML_DOM_NODE_TYPE_LAST_ENTRY             = 0x0D
}
pchtml_dom_node_type_t;

struct pchtml_dom_node {
    pchtml_dom_event_target_t event_target;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    uintptr_t              local_name; /* , lowercase, without prefix: div */
    uintptr_t              prefix;     /* lowercase: lalala */
    uintptr_t              ns;         /* namespace */

    pchtml_dom_document_t     *owner_document;

    pchtml_dom_node_t         *next;
    pchtml_dom_node_t         *prev;
    pchtml_dom_node_t         *parent;
    pchtml_dom_node_t         *first_child;
    pchtml_dom_node_t         *last_child;
    void                   *user;

    pchtml_dom_node_type_t    type;

#ifdef PCHTML_DOM_NODE_USER_VARIABLES
    PCHTML_DOM_NODE_USER_VARIABLES
#endif /* PCHTML_DOM_NODE_USER_VARIABLES */
};


pchtml_dom_node_t *
pchtml_dom_node_interface_create(pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_node_t *
pchtml_dom_node_interface_destroy(pchtml_dom_node_t *node) WTF_INTERNAL;

pchtml_dom_node_t *
pchtml_dom_node_destroy(pchtml_dom_node_t *node) WTF_INTERNAL;

pchtml_dom_node_t *
pchtml_dom_node_destroy_deep(pchtml_dom_node_t *root) WTF_INTERNAL;

const unsigned char *
pchtml_dom_node_name(pchtml_dom_node_t *node, 
                size_t *len) WTF_INTERNAL;

void
pchtml_dom_node_insert_child(pchtml_dom_node_t *to, 
                pchtml_dom_node_t *node) WTF_INTERNAL;

void
pchtml_dom_node_insert_before(pchtml_dom_node_t *to, 
                pchtml_dom_node_t *node) WTF_INTERNAL;

void
pchtml_dom_node_insert_after(pchtml_dom_node_t *to, 
                pchtml_dom_node_t *node) WTF_INTERNAL;

void
pchtml_dom_node_remove(pchtml_dom_node_t *node) WTF_INTERNAL;

unsigned int
pchtml_dom_node_replace_all(pchtml_dom_node_t *parent, 
                pchtml_dom_node_t *node) WTF_INTERNAL;

void
pchtml_dom_node_simple_walk(pchtml_dom_node_t *root,
                pchtml_dom_node_simple_walker_f walker_cb, 
                void *ctx) WTF_INTERNAL;

/*
 * Memory of returns value will be freed in document destroy moment.
 * If you need to release returned resource after use, then call the
 * pchtml_dom_document_destroy_text(node->owner_document, text) function.
 */
unsigned char *
pchtml_dom_node_text_content(pchtml_dom_node_t *node, size_t *len) WTF_INTERNAL;

unsigned int
pchtml_dom_node_text_content_set(pchtml_dom_node_t *node,
                const unsigned char *content, size_t len) WTF_INTERNAL;

/*
 * Inline functions
 */
static inline pchtml_tag_id_t
pchtml_dom_node_tag_id(pchtml_dom_node_t *node)
{
    return node->local_name;
}

static inline pchtml_dom_node_t *
pchtml_dom_node_next(pchtml_dom_node_t *node)
{
    return node->next;
}

static inline pchtml_dom_node_t *
pchtml_dom_node_prev(pchtml_dom_node_t *node)
{
    return node->prev;
}

static inline pchtml_dom_node_t *
pchtml_dom_node_parent(pchtml_dom_node_t *node)
{
    return node->parent;
}

static inline pchtml_dom_node_t *
pchtml_dom_node_first_child(pchtml_dom_node_t *node)
{
    return node->first_child;
}

static inline pchtml_dom_node_t *
pchtml_dom_node_last_child(pchtml_dom_node_t *node)
{
    return node->last_child;
}

/*
 * No inline functions for ABI.
 */
pchtml_tag_id_t
pchtml_dom_node_tag_id_noi(pchtml_dom_node_t *node);

pchtml_dom_node_t *
pchtml_dom_node_next_noi(pchtml_dom_node_t *node);

pchtml_dom_node_t *
pchtml_dom_node_prev_noi(pchtml_dom_node_t *node);

pchtml_dom_node_t *
pchtml_dom_node_parent_noi(pchtml_dom_node_t *node);

pchtml_dom_node_t *
pchtml_dom_node_first_child_noi(pchtml_dom_node_t *node);

pchtml_dom_node_t *
pchtml_dom_node_last_child_noi(pchtml_dom_node_t *node);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_NODE_H */
