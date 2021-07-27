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


#ifndef PCEDOM_NODE_H
#define PCEDOM_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "private/edom/interface.h"
#include "private/edom/event_target.h"


typedef pchtml_action_t
(*pcedom_node_simple_walker_f)(pcedom_node_t *node, void *ctx);


typedef enum {
    PCEDOM_NODE_TYPE_UNDEF                  = 0x00,
    PCEDOM_NODE_TYPE_ELEMENT                = 0x01,
    PCEDOM_NODE_TYPE_ATTRIBUTE              = 0x02,
    PCEDOM_NODE_TYPE_TEXT                   = 0x03,
    PCEDOM_NODE_TYPE_CDATA_SECTION          = 0x04,
    PCEDOM_NODE_TYPE_ENTITY_REFERENCE       = 0x05, // historical
    PCEDOM_NODE_TYPE_ENTITY                 = 0x06, // historical
    PCEDOM_NODE_TYPE_PROCESSING_INSTRUCTION = 0x07,
    PCEDOM_NODE_TYPE_COMMENT                = 0x08,
    PCEDOM_NODE_TYPE_DOCUMENT               = 0x09,
    PCEDOM_NODE_TYPE_DOCUMENT_TYPE          = 0x0A,
    PCEDOM_NODE_TYPE_DOCUMENT_FRAGMENT      = 0x0B,
    PCEDOM_NODE_TYPE_NOTATION               = 0x0C, // historical
    PCEDOM_NODE_TYPE_LAST_ENTRY             = 0x0D
}
pcedom_node_type_t;

struct pcedom_node {
    pcedom_event_target_t event_target;

    /* For example: <LalAla:DiV Fix:Me="value"> */

    uintptr_t              local_name; /* , lowercase, without prefix: div */
    uintptr_t              prefix;     /* lowercase: lalala */
    uintptr_t              ns;         /* namespace */

    pcedom_document_t     *owner_document;

    pcedom_node_t         *next;
    pcedom_node_t         *prev;
    pcedom_node_t         *parent;
    pcedom_node_t         *first_child;
    pcedom_node_t         *last_child;
    void                   *user;

    pcedom_node_type_t    type;

#ifdef PCEDOM_NODE_USER_VARIABLES
    PCEDOM_NODE_USER_VARIABLES
#endif /* PCEDOM_NODE_USER_VARIABLES */
};


pcedom_node_t *
pcedom_node_interface_create(pcedom_document_t *document) WTF_INTERNAL;

pcedom_node_t *
pcedom_node_interface_destroy(pcedom_node_t *node) WTF_INTERNAL;

pcedom_node_t *
pcedom_node_destroy(pcedom_node_t *node) WTF_INTERNAL;

pcedom_node_t *
pcedom_node_destroy_deep(pcedom_node_t *root) WTF_INTERNAL;

const unsigned char *
pcedom_node_name(pcedom_node_t *node, 
                size_t *len) WTF_INTERNAL;

void
pcedom_node_insert_child(pcedom_node_t *to, 
                pcedom_node_t *node) WTF_INTERNAL;

void
pcedom_node_insert_before(pcedom_node_t *to, 
                pcedom_node_t *node) WTF_INTERNAL;

void
pcedom_node_insert_after(pcedom_node_t *to, 
                pcedom_node_t *node) WTF_INTERNAL;

void
pcedom_node_remove(pcedom_node_t *node) WTF_INTERNAL;

unsigned int
pcedom_node_replace_all(pcedom_node_t *parent, 
                pcedom_node_t *node) WTF_INTERNAL;

void
pcedom_node_simple_walk(pcedom_node_t *root,
                pcedom_node_simple_walker_f walker_cb, 
                void *ctx) WTF_INTERNAL;

/*
 * Memory of returns value will be freed in document destroy moment.
 * If you need to release returned resource after use, then call the
 * pcedom_document_destroy_text(node->owner_document, text) function.
 */
unsigned char *
pcedom_node_text_content(pcedom_node_t *node, size_t *len) WTF_INTERNAL;

unsigned int
pcedom_node_text_content_set(pcedom_node_t *node,
                const unsigned char *content, size_t len) WTF_INTERNAL;

/*
 * Inline functions
 */
static inline pchtml_tag_id_t
pcedom_node_tag_id(pcedom_node_t *node)
{
    return node->local_name;
}

static inline pcedom_node_t *
pcedom_node_next(pcedom_node_t *node)
{
    return node->next;
}

static inline pcedom_node_t *
pcedom_node_prev(pcedom_node_t *node)
{
    return node->prev;
}

static inline pcedom_node_t *
pcedom_node_parent(pcedom_node_t *node)
{
    return node->parent;
}

static inline pcedom_node_t *
pcedom_node_first_child(pcedom_node_t *node)
{
    return node->first_child;
}

static inline pcedom_node_t *
pcedom_node_last_child(pcedom_node_t *node)
{
    return node->last_child;
}

/*
 * No inline functions for ABI.
 */
pchtml_tag_id_t
pcedom_node_tag_id_noi(pcedom_node_t *node);

pcedom_node_t *
pcedom_node_next_noi(pcedom_node_t *node);

pcedom_node_t *
pcedom_node_prev_noi(pcedom_node_t *node);

pcedom_node_t *
pcedom_node_parent_noi(pcedom_node_t *node);

pcedom_node_t *
pcedom_node_first_child_noi(pcedom_node_t *node);

pcedom_node_t *
pcedom_node_last_child_noi(pcedom_node_t *node);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_NODE_H */
