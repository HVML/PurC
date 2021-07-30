/*
 * @file vcm.h
 * @author XueShuming
 * @date 2021/07/28
 * @brief The interfaces for vcm.
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

#ifndef PURC_PRIVATE_VCM_H
#define PURC_PRIVATE_VCM_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "purc-variant.h"
#include "private/tree.h"


enum pcvcm_node_type {
    PCVCM_NODE_TYPE_OBJECT,
    PCVCM_NODE_TYPE_ARRAY,
    PCVCM_NODE_TYPE_KEY,
    PCVCM_NODE_TYPE_STRING,
    PCVCM_NODE_TYPE_NULL,
    PCVCM_NODE_TYPE_BOOLEAN,
    PCVCM_NODE_TYPE_NUMBER,
    PCVCM_NODE_TYPE_LONG_INT,
    PCVCM_NODE_TYPE_ULONG_INT,
    PCVCM_NODE_TYPE_LONG_DOUBLE,
    PCVCM_NODE_TYPE_BYTE_SEQUENCE,
};

union pcvcm_node_data {
    bool        b;
    double      d;
    int64_t     i64;
    uint64_t    u64;
    long double ld;
    uintptr_t   sz_ptr[2];
};

struct pcvcm_node {
    struct pctree_node* tree_node;
    enum pcvcm_node_type type;
    union pcvcm_node_data data;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

static inline
struct pcvcm_node* pcvcm_node_new (enum pcvcm_node_type type,
        union pcvcm_node_data data)
{
    struct pcvcm_node* node = (struct pcvcm_node*) calloc (
            sizeof(struct pcvcm_node), 1);
    if (node) {
        struct pctree_node* tree_node = pctree_node_new (node);
        node->tree_node = tree_node;
        node->type = type;
        node->data = data;
    }
    return node;
}

static inline
void pcvcm_node_destroy (struct pcvcm_node* node)
{
    if (node) {
        if ((node->type == PCVCM_NODE_TYPE_KEY
                || node->type == PCVCM_NODE_TYPE_STRING
                || node->type == PCVCM_NODE_TYPE_BYTE_SEQUENCE
                ) && node->data.sz_ptr[1]) {
            free((void*)node->data.sz_ptr[1]);
        }
        free(node);
    }
}

static inline
void pcvcm_node_pctree_node_destory_callback (void* data)
{
    pcvcm_node_destroy ((struct pcvcm_node*) data);
}

static inline
struct pctree_node* pcvcm_node_to_pctree_node (struct pcvcm_node* node)
{
    return node->tree_node;
}

static inline
struct pcvcm_node* pcvcm_node_from_pctree_node (struct pctree_node* tree_node)
{
    return (struct pcvcm_node*)tree_node->user_data;
}

struct pcvdom_element;
purc_variant_t pcvcm_eval (struct pcvcm_node* tree, struct pcvdom_element* elem);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_VCM_H */

