/*
 * @file vcm.c
 * @author XueShuming
 * @date 2021/07/28
 * @brief The API for vcm.
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/errors.h"
#include "private/vcm.h"

static struct pcvcm_node* pcvcm_node_new (enum pcvcm_node_type type)
{
    struct pcvcm_node* node = (struct pcvcm_node*) calloc (
            sizeof(struct pcvcm_node), 1);
    if (!node) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    struct pctree_node* tree_node = pctree_node_new (node);
    node->tree_node = tree_node;
    node->type = type;
    return node;
}


struct pcvcm_node* pcvcm_node_new_object (size_t nr_nodes,
        struct pcvcm_node* nodes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_OBJECT);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(nodes + i));
    }

    return n;
}

struct pcvcm_node* pcvcm_node_new_array (size_t nr_nodes,
        struct pcvcm_node* nodes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_ARRAY);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(nodes + i));
    }

    return n;
}

struct pcvcm_node* pcvcm_node_new_string (const char* str_utf8)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_STRING);
    if (!n) {
        return NULL;
    }

    size_t nr_bytes = strlen (str_utf8);

    uint8_t* buf = (uint8_t*) malloc (nr_bytes + 1);
    memcpy(buf, str_utf8, nr_bytes);
    buf[nr_bytes] = 0;

    n->data.sz_ptr[0] = nr_bytes;
    n->data.sz_ptr[1] = (uintptr_t) buf;

    return n;
}

struct pcvcm_node* pcvcm_node_new_null ()
{
    return pcvcm_node_new (PCVCM_NODE_TYPE_NULL);
}

struct pcvcm_node* pcvcm_node_new_boolean (bool b)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_BOOLEAN);
    if (!n) {
        return NULL;
    }

    n->data.b = b;
    return n;
}

struct pcvcm_node* pcvcm_node_new_number (double d)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_NUMBER);
    if (!n) {
        return NULL;
    }

    n->data.d = d;
    return n;
}

struct pcvcm_node* pcvcm_node_new_longint (int64_t i64)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_LONG_INT);
    if (!n) {
        return NULL;
    }

    n->data.i64 = i64;
    return n;
}

struct pcvcm_node* pcvcm_node_new_ulongint (uint64_t u64)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_ULONG_INT);
    if (!n) {
        return NULL;
    }

    n->data.u64 = u64;
    return n;
}

struct pcvcm_node* pcvcm_node_new_longdouble (long double ld)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_LONG_DOUBLE);
    if (!n) {
        return NULL;
    }

    n->data.ld = ld;
    return n;
}

struct pcvcm_node* pcvcm_node_new_byte_sequence (const void* bytes,
        size_t nr_bytes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_BYTE_SEQUENCE);
    if (!n) {
        return NULL;
    }

    uint8_t* buf = (uint8_t*) malloc (nr_bytes + 1);
    memcpy(buf, bytes, nr_bytes);
    buf[nr_bytes] = 0;

    n->data.sz_ptr[0] = nr_bytes;
    n->data.sz_ptr[1] = (uintptr_t) buf;
    return n;
}

struct pcvcm_node* pcvcm_node_new_concat_string (size_t nr_nodes,
        struct pcvcm_node* nodes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_FUNC_CONCAT_STRING);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(nodes  + i));
    }

    return n;
}

struct pcvcm_node* pcvcm_node_new_get_variable (struct pcvcm_node* node)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_FUNC_GET_VARIABLE);
    if (!n) {
        return NULL;
    }

    if (node) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(node));
    }

    return n;
}

struct pcvcm_node* pcvcm_node_new_get_element (struct pcvcm_node* variable,
        struct pcvcm_node* identifier)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_FUNC_GET_ELEMENT);
    if (!n) {
        return NULL;
    }

    if (variable) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(variable));
    }

    if (identifier) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(identifier));
    }

    return n;
}

struct pcvcm_node* pcvcm_node_new_call_getter (struct pcvcm_node* variable,
        size_t nr_params, struct pcvcm_node* params)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_FUNC_CALL_GETTER);
    if (!n) {
        return NULL;
    }

    if (variable) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(variable));
    }

    for (size_t i = 0; i < nr_params; i++) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(params + i));
    }

    return n;
}

struct pcvcm_node* pcvcm_node_new_call_setter (struct pcvcm_node* variable,
        size_t nr_params, struct pcvcm_node* params)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_FUNC_CALL_SETTER);
    if (!n) {
        return NULL;
    }

    if (variable) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(variable));
    }

    for (size_t i = 0; i < nr_params; i++) {
        pctree_node_append_child (pcvcm_node_to_pctree_node(n),
                pcvcm_node_to_pctree_node(params + i));
    }

    return n;
}

purc_variant_t pcvcm_node_to_variant (struct pcvcm_node* node);

purc_variant_t pcvcm_node_object_to_variant (struct pcvcm_node* node)
{
    struct pctree_node* tree_node = pcvcm_node_to_pctree_node (node);
    purc_variant_t object = purc_variant_make_object (0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    struct pctree_node* k_node = tree_node->first_child;
    struct pctree_node* v_node = k_node ? k_node->next : NULL;
    while (k_node && v_node) {
        purc_variant_t key = pcvcm_node_to_variant (
                pcvcm_node_from_pctree_node(k_node));
        purc_variant_t value = pcvcm_node_to_variant (
                pcvcm_node_from_pctree_node(v_node));

        purc_variant_object_set (object, key, value);

        purc_variant_unref (key);
        purc_variant_unref (value);

        k_node = v_node->next;
        v_node = k_node ? k_node->next : NULL;
    }

    return object;
}

purc_variant_t pcvcm_node_array_to_variant (struct pcvcm_node* node)
{
    struct pctree_node* tree_node = pcvcm_node_to_pctree_node (node);
    purc_variant_t array = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    struct pctree_node* array_node = tree_node->first_child;
    while (array_node) {
        purc_variant_t vt = pcvcm_node_to_variant (
                pcvcm_node_from_pctree_node(array_node));
        purc_variant_array_append (array, vt);
        purc_variant_unref (vt);

        array_node = array_node->next;
    }
    return array;
}

purc_variant_t pcvcm_node_to_variant (struct pcvcm_node* node)
{
    switch (node->type)
    {
        case PCVCM_NODE_TYPE_OBJECT:
            return pcvcm_node_object_to_variant (node);

        case PCVCM_NODE_TYPE_ARRAY:
            return pcvcm_node_array_to_variant (node);

        case PCVCM_NODE_TYPE_STRING:
            return purc_variant_make_string ((char*)node->data.sz_ptr[1],
                    false);

        case PCVCM_NODE_TYPE_NULL:
            return purc_variant_make_null ();

        case PCVCM_NODE_TYPE_BOOLEAN:
            return purc_variant_make_boolean (node->data.b);

        case PCVCM_NODE_TYPE_NUMBER:
            return purc_variant_make_number (node->data.d);

        case PCVCM_NODE_TYPE_LONG_INT:
            return purc_variant_make_longint (node->data.i64);

        case PCVCM_NODE_TYPE_ULONG_INT:
            return purc_variant_make_ulongint (node->data.u64);

        case PCVCM_NODE_TYPE_LONG_DOUBLE:
            return purc_variant_make_longdouble (node->data.ld);

        case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
            return purc_variant_make_byte_sequence(
                    (void*)node->data.sz_ptr[1], node->data.sz_ptr[0]);
        default:  //TODO
            return purc_variant_make_null();
    }
    return purc_variant_make_null();
}

purc_variant_t pcvcm_eval (struct pcvcm_node* tree, struct pcvdom_element* elem)
{
    UNUSED_PARAM(elem);
    if (!tree) {
        return purc_variant_make_null();
    }
    return pcvcm_node_to_variant (tree);
}
