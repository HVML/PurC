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
#include "private/stack.h"

static struct pcvcm_node* pcvcm_node_new (enum pcvcm_node_type type)
{
    struct pcvcm_node* node = (struct pcvcm_node*) calloc (1,
            sizeof(struct pcvcm_node));
    if (!node) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    node->type = type;
    return node;
}


struct pcvcm_node* pcvcm_node_new_object (size_t nr_nodes,
        struct pcvcm_node** nodes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_OBJECT);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        struct pcvcm_node *v = nodes[i];
        pctree_node_append_child(&n->tree_node, &v->tree_node);
    }

    return n;
}

struct pcvcm_node* pcvcm_node_new_array (size_t nr_nodes,
        struct pcvcm_node** nodes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_ARRAY);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        struct pcvcm_node *v = nodes[i];
        pctree_node_append_child(&n->tree_node, &v->tree_node);
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

    n->sz_ptr[0] = nr_bytes;
    n->sz_ptr[1] = (uintptr_t) buf;

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

    n->b = b;
    return n;
}

struct pcvcm_node* pcvcm_node_new_number (double d)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_NUMBER);
    if (!n) {
        return NULL;
    }

    n->d = d;
    return n;
}

struct pcvcm_node* pcvcm_node_new_longint (int64_t i64)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_LONG_INT);
    if (!n) {
        return NULL;
    }

    n->i64 = i64;
    return n;
}

struct pcvcm_node* pcvcm_node_new_ulongint (uint64_t u64)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_ULONG_INT);
    if (!n) {
        return NULL;
    }

    n->u64 = u64;
    return n;
}

struct pcvcm_node* pcvcm_node_new_longdouble (long double ld)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_LONG_DOUBLE);
    if (!n) {
        return NULL;
    }

    n->ld = ld;
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

    n->sz_ptr[0] = nr_bytes;
    n->sz_ptr[1] = (uintptr_t) buf;
    return n;
}


static void hex_to_bytes (const uint8_t* hex, size_t sz_hex, uint8_t* result)
{
    uint8_t h = 0;
    uint8_t l = 0;
    for(size_t i = 0; i < sz_hex/2; i++) {
        if (*hex < 58) {
            h = *hex - 48;
        }
        else if (*hex < 71) {
            h = *hex - 55;
        }
        else {
            h = *hex - 87;
        }

        hex++;
        if (*hex < 58) {
            l = *hex - 48;
        }
        else if (*hex < 71) {
            l = *hex - 55;
        }
        else {
            l = *hex - 87;
        }
        hex++;
        *result++ = h<<4|l;
    }
}

struct pcvcm_node* pcvcm_node_new_byte_sequence_from_bx (const void* bytes,
        size_t nr_bytes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_BYTE_SEQUENCE);
    if (!n) {
        return NULL;
    }

    const uint8_t* p = bytes;
    size_t sz = nr_bytes;
    if (sz % 2 != 0) {
        pcinst_set_error(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        return NULL;
    }
    size_t sz_buf = sz / 2;
    uint8_t* buf = (uint8_t*) calloc (sz_buf + 1, 1);
    hex_to_bytes (p, sz, buf);

    n->sz_ptr[0] = sz_buf;
    n->sz_ptr[1] = (uintptr_t) buf;
    return n;
}

struct pcvcm_node* pcvcm_node_new_byte_sequence_from_bb (const void* bytes,
        size_t nr_bytes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_BYTE_SEQUENCE);
    if (!n) {
        return NULL;
    }

    const uint8_t* p = bytes;
    size_t sz = nr_bytes;
    if (sz % 8 != 0) {
        pcinst_set_error(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        return NULL;
    }

    size_t sz_buf = sz / 8;
    uint8_t* buf = (uint8_t*) calloc (sz_buf + 1, 1);
    for(size_t i = 0; i < sz_buf; i++) {
        uint8_t b = 0;
        uint8_t c = 0;
        for (int j = 7; j >= 0; j--) {
            c = *p == '0' ? 0 : 1;
            b = b | c << j;
            p++;
        }
        buf[i] = b;
    }

    n->sz_ptr[0] = sz_buf;
    n->sz_ptr[1] = (uintptr_t) buf;
    return n;
}

int b64_decode(const void *src, void *dest, size_t dest_len);
struct pcvcm_node* pcvcm_node_new_byte_sequence_from_b64 (const void* bytes,
        size_t nr_bytes)
{
    struct pcvcm_node* n = pcvcm_node_new (PCVCM_NODE_TYPE_BYTE_SEQUENCE);
    if (!n) {
        return NULL;
    }

    const uint8_t* p = bytes;
    size_t sz_buf = nr_bytes;
    uint8_t* buf = (uint8_t*) calloc (sz_buf, 1);

    int ret = b64_decode (p, buf, sz_buf);
    if (ret == -1) {
        free (buf);
        pcinst_set_error(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        return NULL;
    }

    n->sz_ptr[0] = ret;
    n->sz_ptr[1] = (uintptr_t) buf;
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
        pctree_node_append_child ((struct pctree_node*)n,
                (struct pctree_node*)(nodes  + i));
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
        pctree_node_append_child ((struct pctree_node*)n,
                (struct pctree_node*)(node));
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
        pctree_node_append_child ((struct pctree_node*)n,
                (struct pctree_node*)(variable));
    }

    if (identifier) {
        pctree_node_append_child ((struct pctree_node*)n,
                (struct pctree_node*)(identifier));
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
        pctree_node_append_child ((struct pctree_node*)n,
                (struct pctree_node*)(variable));
    }

    for (size_t i = 0; i < nr_params; i++) {
        pctree_node_append_child ((struct pctree_node*)n,
                (struct pctree_node*)(params + i));
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
        pctree_node_append_child ((struct pctree_node*)n,
                (struct pctree_node*)variable);
    }

    for (size_t i = 0; i < nr_params; i++) {
        pctree_node_append_child ((struct pctree_node*)n,
                (struct pctree_node*)(params + i));
    }

    return n;
}

static void pcvcm_node_destroy_callback (struct pctree_node* n,  void* data)
{
    UNUSED_PARAM(data);
    struct pcvcm_node* node = (struct pcvcm_node*) n;
    if ((node->type == PCVCM_NODE_TYPE_STRING
                || node->type == PCVCM_NODE_TYPE_BYTE_SEQUENCE
        ) && node->sz_ptr[1]) {
        free((void*)node->sz_ptr[1]);
    }
    free(node);
}

void pcvcm_node_destroy (struct pcvcm_node* root)
{
    if (root) {
        pctree_node_post_order_traversal ((struct pctree_node*) root,
                pcvcm_node_destroy_callback, NULL);
    }
}

struct pcvcm_stack {
    struct pcutils_stack* stack;
};

struct pcvcm_stack* pcvcm_stack_new ()
{
    struct pcvcm_stack* stack = (struct pcvcm_stack*) calloc(
            1, sizeof(struct pcvcm_stack));
    if (stack) {
        stack->stack = pcutils_stack_new (0);
        if (!stack->stack) {
            free (stack);
            stack = NULL;
        }
    }

    return stack;
}

bool pcvcm_stack_is_empty (struct pcvcm_stack* stack)
{
    return pcutils_stack_is_empty (stack->stack);
}

void pcvcm_stack_push (struct pcvcm_stack* stack, struct pcvcm_node* e)
{
    pcutils_stack_push (stack->stack, (uintptr_t)e);
}

struct pcvcm_node* pcvcm_stack_pop (struct pcvcm_stack* stack)
{
    return (struct pcvcm_node*) pcutils_stack_pop (stack->stack);
}

struct pcvcm_node* pcvcm_stack_bottommost (struct pcvcm_stack* stack)
{
    return (struct pcvcm_node*) pcutils_stack_top (stack->stack);
}

void pcvcm_stack_destroy (struct pcvcm_stack* stack)
{
    pcutils_stack_destroy (stack->stack);
    free (stack);
}

purc_variant_t pcvcm_node_to_variant (struct pcvcm_node* node);

purc_variant_t pcvcm_node_object_to_variant (struct pcvcm_node* node)
{
    struct pctree_node* tree_node = (struct pctree_node*) (node);
    purc_variant_t object = purc_variant_make_object (0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    struct pctree_node* k_node = tree_node->first_child;
    struct pctree_node* v_node = k_node ? k_node->next : NULL;
    while (k_node && v_node) {
        purc_variant_t key = pcvcm_node_to_variant ((struct pcvcm_node*)k_node);
        purc_variant_t value = pcvcm_node_to_variant (
                (struct pcvcm_node*)v_node);

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
    struct pctree_node* tree_node = (struct pctree_node*) (node);
    purc_variant_t array = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    struct pctree_node* array_node = tree_node->first_child;
    while (array_node) {
        purc_variant_t vt = pcvcm_node_to_variant (
                (struct pcvcm_node*)array_node);
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
            return purc_variant_make_string ((char*)node->sz_ptr[1],
                    false);

        case PCVCM_NODE_TYPE_NULL:
            return purc_variant_make_null ();

        case PCVCM_NODE_TYPE_BOOLEAN:
            return purc_variant_make_boolean (node->b);

        case PCVCM_NODE_TYPE_NUMBER:
            return purc_variant_make_number (node->d);

        case PCVCM_NODE_TYPE_LONG_INT:
            return purc_variant_make_longint (node->i64);

        case PCVCM_NODE_TYPE_ULONG_INT:
            return purc_variant_make_ulongint (node->u64);

        case PCVCM_NODE_TYPE_LONG_DOUBLE:
            return purc_variant_make_longdouble (node->ld);

        case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
            return purc_variant_make_byte_sequence(
                    (void*)node->sz_ptr[1], node->sz_ptr[0]);
        default:  //TODO
            return purc_variant_make_null();
    }
    return purc_variant_make_null();
}

// TODO : need pcintr_stack_t
purc_variant_t pcvcm_eval (struct pcvcm_node* tree, struct pcintr_stack* stack)
{
    UNUSED_PARAM(stack);
    if (!tree) {
        return purc_variant_make_null();
    }
    return pcvcm_node_to_variant (tree);
}
