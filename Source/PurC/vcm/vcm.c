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
#include <ctype.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "purc-rwstream.h"
#include "private/errors.h"
#include "private/vcm.h"
#include "private/stack.h"
#include "private/interpreter.h"
#include "private/utils.h"

#define TREE_NODE(node)              ((struct pctree_node*)(node))
#define VCM_NODE(node)               ((struct pcvcm_node*)(node))
#define FIRST_CHILD(node)            \
    (VCM_NODE(pctree_node_child(TREE_NODE(node))))
#define NEXT_CHILD(node)             \
    ((node) ? VCM_NODE(pctree_node_next(TREE_NODE(node))) : NULL)
#define PARENT_NODE(node)            \
    (VCM_NODE(pctree_node_parent(TREE_NODE(node))))
#define CHILDREN_NUMBER(node)        \
    (pctree_node_children_number(TREE_NODE(node)))
#define APPEND_CHILD(parent, child)  \
    pctree_node_append_child(TREE_NODE(parent), TREE_NODE(child))


#define MIN_BUF_SIZE         32
#define MAX_BUF_SIZE         SIZE_MAX

#define PCVCM_CHECK_FAIL_RET(node, variant, silently)                       \
    if (node->type != PCVCM_NODE_TYPE_UNDEFINED) {                          \
        if (purc_variant_is_undefined(variant)) {                           \
            return variant;                                                 \
        }                                                                   \
        else if (variant == PURC_VARIANT_INVALID) {                         \
            return silently ? purc_variant_make_undefined() : variant;      \
        }                                                                   \
    }                                                                       \
    else if (variant == PURC_VARIANT_INVALID) {                             \
        return variant;                                                     \
    }

#define PCVCM_CHECK_FAIL_GOTO(node, variant, silently, label)               \
    if (node->type != PCVCM_NODE_TYPE_UNDEFINED) {                          \
        if (purc_variant_is_undefined(variant)) {                           \
            goto label;                                                     \
        }                                                                   \
        else if (variant == PURC_VARIANT_INVALID) {                         \
            goto label;                                                     \
        }                                                                   \
    }                                                                       \
    else if (variant == PURC_VARIANT_INVALID) {                             \
        goto label;                                                         \
    }

struct pcvcm_node_op {
    cb_find_var find_var;
    void* find_var_ctxt;
};

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


struct pcvcm_node* pcvcm_node_new_undefined ()
{
    return pcvcm_node_new (PCVCM_NODE_TYPE_UNDEFINED);
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
        APPEND_CHILD(n, v);
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
        APPEND_CHILD(n, v);
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
        APPEND_CHILD(n, nodes  + i);
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
        APPEND_CHILD(n, node);
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
        APPEND_CHILD(n, variable);
    }

    if (identifier) {
        APPEND_CHILD(n, identifier);
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
        APPEND_CHILD(n, variable);
    }

    for (size_t i = 0; i < nr_params; i++) {
        APPEND_CHILD(n, params + i);
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
        APPEND_CHILD(n, variable);
    }

    for (size_t i = 0; i < nr_params; i++) {
        APPEND_CHILD(n, params + i);
    }

    return n;
}

#define WRITE_CHILD_NODE()                                                  \
    do {                                                                    \
        struct pcvcm_node* child = FIRST_CHILD(node);                       \
        while (child) {                                                     \
            pcvcm_node_write_to_rwstream(rws, child);                       \
            child = NEXT_CHILD(child);                                      \
            if (child) {                                                    \
                purc_rwstream_write(rws, ",", 1);                           \
            }                                                               \
        }                                                                   \
    } while (false)

#define WRITE_VARIANT()                                                     \
    do {                                                                    \
        size_t len_expected = 0;                                            \
        purc_variant_serialize(v, rws, 0,                                   \
                PCVARIANT_SERIALIZE_OPT_REAL_EJSON |                        \
                PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BASE64 |                  \
                PCVARIANT_SERIALIZE_OPT_PLAIN,                              \
                &len_expected);                                             \
    } while (false)

static
void pcvcm_node_write_to_rwstream(purc_rwstream_t rws, struct pcvcm_node* node)
{
    switch (node->type)
    {
    case PCVCM_NODE_TYPE_UNDEFINED:
        purc_rwstream_write(rws, "undefined", 9);
        break;

    case PCVCM_NODE_TYPE_OBJECT:
        purc_rwstream_write(rws, "make_object(", 12);
        WRITE_CHILD_NODE();
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_ARRAY:
        purc_rwstream_write(rws, "make_array(", 11);
        WRITE_CHILD_NODE();
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_STRING:
        purc_rwstream_write(rws, "\"", 1);
        purc_rwstream_write(rws, (char*)node->sz_ptr[1],
                node->sz_ptr[0]);
        purc_rwstream_write(rws, "\"", 1);
        break;

    case PCVCM_NODE_TYPE_NULL:
        purc_rwstream_write(rws, "null", 4);
        break;

    case PCVCM_NODE_TYPE_BOOLEAN:
    {
        purc_variant_t v = purc_variant_make_boolean (node->b);
        WRITE_VARIANT();
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_NUMBER:
    {
        purc_variant_t v = purc_variant_make_number (node->d);
        WRITE_VARIANT();
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_INT:
    {
        purc_variant_t v = purc_variant_make_longint (node->i64);
        WRITE_VARIANT();
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_ULONG_INT:
    {
        purc_variant_t v = purc_variant_make_ulongint (node->u64);
        WRITE_VARIANT();
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_DOUBLE:
    {
        purc_variant_t v = purc_variant_make_longdouble (node->ld);
        WRITE_VARIANT();
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
    {
        purc_variant_t v = purc_variant_make_byte_sequence(
                (void*)node->sz_ptr[1], node->sz_ptr[0]);
        WRITE_VARIANT();
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CONCAT_STRING:
        purc_rwstream_write(rws, "concat_string(", 14);
        WRITE_CHILD_NODE();
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_VARIABLE:
        purc_rwstream_write(rws, "get_variable(", 13);
        WRITE_CHILD_NODE();
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_ELEMENT:
        purc_rwstream_write(rws, "get_element(", 12);
        WRITE_CHILD_NODE();
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_CALL_GETTER:
        purc_rwstream_write(rws, "call_getter(", 12);
        WRITE_CHILD_NODE();
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_CALL_SETTER:
        purc_rwstream_write(rws, "call_setter(", 12);
        WRITE_CHILD_NODE();
        purc_rwstream_write(rws, ")", 1);
        break;
    }
}

char* pcvcm_node_to_string (struct pcvcm_node* node, size_t* nr_bytes)
{
    if (!node) {
        if (nr_bytes) {
            *nr_bytes = 0;
        }
        return NULL;
    }

    purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUF_SIZE, MAX_BUF_SIZE);
    if (!rws) {
        if (nr_bytes) {
            *nr_bytes = 0;
        }
        return NULL;
    }

    pcvcm_node_write_to_rwstream(rws, node);

    purc_rwstream_write(rws, "", 1); // writing null-terminator

    size_t sz_content = 0;
    char* buf = (char*) purc_rwstream_get_mem_buffer_ex(rws, &sz_content,
        NULL, true);
    if (nr_bytes) {
        *nr_bytes = sz_content - 1;
    }

    purc_rwstream_destroy(rws);
    return buf;
}

static void pcvcm_node_destroy_callback (struct pctree_node* n,  void* data)
{
    UNUSED_PARAM(data);
    struct pcvcm_node* node = VCM_NODE(n);
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
        pctree_node_post_order_traversal(TREE_NODE(root),
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

purc_variant_t pcvcm_node_to_variant (struct pcvcm_node* node,
        struct pcvcm_node_op* ops, bool silently);

purc_variant_t pcvcm_node_object_to_variant (struct pcvcm_node* node,
        struct pcvcm_node_op* ops, bool silently)
{
    purc_variant_t object = purc_variant_make_object (0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (!object) {
        return PURC_VARIANT_INVALID;
    }

    struct pcvcm_node* k_node = FIRST_CHILD(node);
    struct pcvcm_node* v_node = NEXT_CHILD(k_node);
    while (k_node && v_node) {
        purc_variant_t key = pcvcm_node_to_variant (k_node, ops, silently);
        PCVCM_CHECK_FAIL_RET(k_node, key, silently);

        purc_variant_t value = pcvcm_node_to_variant(v_node, ops, silently);
        PCVCM_CHECK_FAIL_RET(v_node, value, silently);

        purc_variant_object_set (object, key, value);

        purc_variant_unref (key);
        purc_variant_unref (value);

        k_node = NEXT_CHILD(v_node);
        v_node = NEXT_CHILD(k_node);
    }

    return object;
}

purc_variant_t pcvcm_node_array_to_variant (struct pcvcm_node* node,
       struct pcvcm_node_op* ops, bool silently)
{
    purc_variant_t array = purc_variant_make_array (0, PURC_VARIANT_INVALID);
    if (!array) {
        return PURC_VARIANT_INVALID;
    }

    struct pcvcm_node* array_node = FIRST_CHILD(node);
    while (array_node) {
        purc_variant_t vt = pcvcm_node_to_variant (array_node, ops, silently);
        PCVCM_CHECK_FAIL_GOTO(array_node, vt, silently, err);
        purc_variant_array_append (array, vt);
        purc_variant_unref (vt);

        array_node = NEXT_CHILD(array_node);
    }
    return array;

err:
    if (array) {
        purc_variant_unref(array);
    }
    return PURC_VARIANT_INVALID;
}

purc_variant_t pcvcm_node_concat_string_to_variant (struct pcvcm_node* node,
       struct pcvcm_node_op* ops, bool silently)
{
    UNUSED_PARAM(node);
    UNUSED_PARAM(ops);

    purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUF_SIZE, MAX_BUF_SIZE);
    if (!rws) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvcm_node* child = FIRST_CHILD(node);
    while (child) {
        purc_variant_t v = pcvcm_node_to_variant(child, ops, silently);
        PCVCM_CHECK_FAIL_GOTO(child, v, silently, err);

        char* buf = NULL;
        int total = purc_variant_stringify_alloc(&buf, v);
        if (total) {
            purc_rwstream_write(rws, buf, total);
        }
        free(buf);
        purc_variant_unref(v);

        child = NEXT_CHILD(child);
    }

    // do not forget tailing-null-terminator
    purc_rwstream_write(rws, "", 1);

    size_t rw_size = 0;
    size_t content_size = 0;
    char *rw_string = purc_rwstream_get_mem_buffer_ex (rws,
            &content_size, &rw_size, true);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        PC_ASSERT(content_size <= rw_size);
        size_t len = strnlen(rw_string, rw_size);
        PC_ASSERT(len <= content_size);
        PC_ASSERT(len+1 == content_size);
        PC_ASSERT(rw_string[len] == '\0');
        ret_var = purc_variant_make_string_reuse_buff (rw_string,
                content_size, false);
        if(ret_var == PURC_VARIANT_INVALID) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

err:
    purc_rwstream_destroy(rws);
    return ret_var;
}

purc_variant_t pcvcm_node_get_variable_to_variant (struct pcvcm_node* node,
       struct pcvcm_node_op* ops, bool silently)
{
    UNUSED_PARAM(silently);
    if (!ops) {
        return PURC_VARIANT_INVALID;
    }

    struct pcvcm_node* name_node = FIRST_CHILD(node);
    if (!name_node) {
        return PURC_VARIANT_INVALID;
    }

    size_t nr_name = (size_t)name_node->sz_ptr[0];
    char* name = (char*)name_node->sz_ptr[1];
    if (!name || nr_name == 0) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret =  ops->find_var ?  ops->find_var(ops->find_var_ctxt,
            name) : PURC_VARIANT_INVALID;
    if (ret) {
        purc_variant_ref(ret);
    }
    return ret;
}

static bool is_action_node(struct pcvcm_node* node)
{
    return (node && (
                node->type == PCVCM_NODE_TYPE_FUNC_GET_ELEMENT ||
                node->type == PCVCM_NODE_TYPE_FUNC_CALL_GETTER ||
                node->type == PCVCM_NODE_TYPE_FUNC_CALL_SETTER
                )
            );
}

static bool is_handle_as_getter(struct pcvcm_node* node)
{
    struct pcvcm_node* parent_node = PARENT_NODE(node);
    if (is_action_node(parent_node) && FIRST_CHILD(parent_node) == node) {
        return false;
    }
    return true;
}

enum method_type {
    GETTER_METHOD,
    SETTER_METHOD
};

purc_variant_t call_dvariant_method(purc_variant_t root, purc_variant_t var,
        size_t nr_args, purc_variant_t* argv, enum method_type type,
        bool silently)
{
    purc_dvariant_method func = (type == GETTER_METHOD) ?
         purc_variant_dynamic_get_getter (var) :
         purc_variant_dynamic_get_setter (var);
    if (func) {
        return func (root, nr_args, argv, silently);
    }
    return PURC_VARIANT_INVALID;
}

purc_variant_t call_nvariant_method(purc_variant_t var,
        const char* key_name, size_t nr_args, purc_variant_t* argv,
        enum method_type type, bool silently)
{
    struct purc_native_ops *ops = purc_variant_native_get_ops (var);
    if (ops) {
        purc_nvariant_method native_func = (type == GETTER_METHOD) ?
            ops->property_getter(key_name) :
            ops->property_setter(key_name);
        if (native_func) {
            return  native_func (purc_variant_native_get_entity (var),
                    nr_args, argv, silently);
        }
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t get_attach_variant(struct pcvcm_node* node)
{
    return node ? (purc_variant_t)node->attach : PURC_VARIANT_INVALID;
}

purc_variant_t pcvcm_node_get_element_to_variant (struct pcvcm_node* node,
       struct pcvcm_node_op* ops, bool silently)
{
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvcm_node* caller_node = FIRST_CHILD(node);
    if (!caller_node) {
        goto err;
    }

    purc_variant_t caller_var = pcvcm_node_to_variant(caller_node, ops, silently);
    PCVCM_CHECK_FAIL_GOTO(caller_node, caller_var, silently, err);

    struct pcvcm_node* param_node  = NEXT_CHILD(caller_node);
    purc_variant_t param_var = pcvcm_node_to_variant(param_node, ops, silently);
    PCVCM_CHECK_FAIL_GOTO(param_node, param_var, silently, clean_caller_var);

    bool has_index = true;
    int64_t index = -1;
    if (param_node->type == PCVCM_NODE_TYPE_STRING) {
        if (pcutils_parse_int64((const char*)param_node->sz_ptr[1], param_node->sz_ptr[0],
                    &index) != 0) {
            has_index = false;
        }
    }
    else if (!purc_variant_cast_to_longint(param_var, &index, false)) {
        has_index = false;
    }

    if (purc_variant_is_object(caller_var)) {
        purc_variant_t val = purc_variant_object_get(caller_var, param_var,
                silently);
        if (!val) {
            goto clear_param_var;
        }

        purc_variant_ref(val);
        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            goto clear_param_var;
        }

        if (!is_handle_as_getter(node)) {
            ret_var = val;
            goto clear_param_var;
        }

        ret_var = call_dvariant_method(caller_var, val, 0, NULL, GETTER_METHOD,
                silently);
        purc_variant_unref(val);
    }
    else if (purc_variant_is_array(caller_var)) {
        if (!has_index) {
            goto clear_param_var;
        }
        if (index < 0) {
            size_t len = purc_variant_array_get_size(caller_var);
            index += len;
        }
        if (index < 0) {
            goto clear_param_var;
        }

        purc_variant_t val = purc_variant_array_get(caller_var, index);
        if (!val) {
            goto clear_param_var;
        }

        purc_variant_ref(val);
        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            goto clear_param_var;
        }

        if (!is_handle_as_getter(node)) {
            ret_var = val;
            goto clear_param_var;
        }
        ret_var = call_dvariant_method(caller_var, val, 0, NULL, GETTER_METHOD,
                silently);
        purc_variant_unref(val);
    }
    else if (purc_variant_is_set(caller_var)) {
        if (!has_index) {
            goto clear_param_var;
        }
        if (index < 0) {
            size_t len = purc_variant_set_get_size(caller_var);
            index += len;
        }
        if (index < 0) {
            goto clear_param_var;
        }

        purc_variant_t val = purc_variant_set_get_by_index(caller_var, index);
        if (!val) {
            goto clear_param_var;
        }

        purc_variant_ref(val);
        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            goto clear_param_var;
        }

        if (!is_handle_as_getter(node)) {
            ret_var = val;
            goto clear_param_var;
        }
        ret_var = call_dvariant_method(caller_var, val, 0, NULL, GETTER_METHOD,
                silently);
        purc_variant_unref(val);
    }
    else if (purc_variant_is_dynamic(caller_var)) {
        ret_var = call_dvariant_method(
                get_attach_variant(FIRST_CHILD(caller_node)),
                caller_var, 1, &param_var, GETTER_METHOD,
                silently);
        goto clear_param_var;
    }
    else if (purc_variant_is_native(caller_var)) {
        if (!is_handle_as_getter(node)) {
            ret_var = purc_variant_make_array(2, caller_var, param_var);
            goto clear_param_var;
        }
        ret_var = call_nvariant_method(caller_var,
                purc_variant_get_string_const(param_var), 0, NULL,
                GETTER_METHOD, silently);
        goto clear_param_var;
    }

clear_param_var:
    purc_variant_unref(param_var);
clean_caller_var:
    purc_variant_unref(caller_var);
err:
    return ret_var;
}

purc_variant_t pcvcm_node_call_method_to_variant (struct pcvcm_node* node,
       struct pcvcm_node_op* ops, enum method_type type, bool silently)
{
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvcm_node* caller_node = FIRST_CHILD(node);
    if (!caller_node) {
        goto err;
    }

    purc_variant_t caller_var = pcvcm_node_to_variant(caller_node, ops, silently);
    PCVCM_CHECK_FAIL_GOTO(caller_node, caller_var, silently, err);

    if (!purc_variant_is_dynamic(caller_var)
            && !purc_variant_is_array(caller_var)) {
        goto clean_caller_var;
    }

    purc_variant_t* params = NULL;
    size_t nr_params = CHILDREN_NUMBER(node) - 1;
    if (nr_params > 0) {
        params = (purc_variant_t*) calloc(nr_params, sizeof(purc_variant_t));

        int i = 0;
        struct pcvcm_node* param_node = NEXT_CHILD(caller_node);
        while (param_node) {
            purc_variant_t vt = pcvcm_node_to_variant (param_node, ops, silently);
            PCVCM_CHECK_FAIL_GOTO(param_node, vt, silently, clean_params);

            params[i] = vt;
            i++;
            param_node = NEXT_CHILD(param_node);
        }
    }

    if (purc_variant_is_dynamic(caller_var)) {
        ret_var = call_dvariant_method(
                get_attach_variant(FIRST_CHILD(caller_node)),
                caller_var, nr_params, params, type, silently);
    }
    else if (purc_variant_is_array(caller_var)) {
        purc_variant_t nv = purc_variant_array_get(caller_var, 0);
        if (purc_variant_is_native(nv)) {
            purc_variant_t name = purc_variant_array_get(caller_var, 1);
            if (name) {
                ret_var = call_nvariant_method(nv,
                        purc_variant_get_string_const(name), nr_params,
                        params, type, silently);
            }
        }
    }

clean_params:
    for (size_t i = 0; i < nr_params; i++) {
        if (params[i]) {
            purc_variant_unref(params[i]);
        }
    }
    free(params);

clean_caller_var:
    purc_variant_unref(caller_var);
err:
    return ret_var;
}

purc_variant_t pcvcm_node_to_variant (struct pcvcm_node* node,
        struct pcvcm_node_op* ops, bool silently)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    switch (node->type)
    {
        case PCVCM_NODE_TYPE_UNDEFINED:
            ret = purc_variant_make_undefined();
            break;

        case PCVCM_NODE_TYPE_OBJECT:
            ret = pcvcm_node_object_to_variant (node, ops, silently);
            break;

        case PCVCM_NODE_TYPE_ARRAY:
            ret = pcvcm_node_array_to_variant (node, ops, silently);
            break;

        case PCVCM_NODE_TYPE_STRING:
            return purc_variant_make_string ((char*)node->sz_ptr[1],
                    false);

        case PCVCM_NODE_TYPE_NULL:
            ret = purc_variant_make_null ();
            break;

        case PCVCM_NODE_TYPE_BOOLEAN:
            ret = purc_variant_make_boolean (node->b);
            break;

        case PCVCM_NODE_TYPE_NUMBER:
            ret = purc_variant_make_number (node->d);
            break;

        case PCVCM_NODE_TYPE_LONG_INT:
            ret = purc_variant_make_longint (node->i64);
            break;

        case PCVCM_NODE_TYPE_ULONG_INT:
            ret = purc_variant_make_ulongint (node->u64);
            break;

        case PCVCM_NODE_TYPE_LONG_DOUBLE:
            ret = purc_variant_make_longdouble (node->ld);
            break;

        case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
            return purc_variant_make_byte_sequence(
                    (void*)node->sz_ptr[1], node->sz_ptr[0]);

        case PCVCM_NODE_TYPE_FUNC_CONCAT_STRING:
            ret = pcvcm_node_concat_string_to_variant(node, ops, silently);
            break;

        case PCVCM_NODE_TYPE_FUNC_GET_VARIABLE:
            ret = pcvcm_node_get_variable_to_variant(node, ops, silently);
            break;

        case PCVCM_NODE_TYPE_FUNC_GET_ELEMENT:
            ret = pcvcm_node_get_element_to_variant(node, ops, silently);
            break;

        case PCVCM_NODE_TYPE_FUNC_CALL_GETTER:
            ret = pcvcm_node_call_method_to_variant(node, ops, GETTER_METHOD,
                    silently);
            break;

        case PCVCM_NODE_TYPE_FUNC_CALL_SETTER:
            ret = pcvcm_node_call_method_to_variant(node, ops, SETTER_METHOD,
                    silently);
            break;

        default:
            ret = purc_variant_make_null();
            break;
    }
    node->attach = (uintptr_t) ret;
#ifndef NDEBUG
    PRINT_VCM_NODE(node);
    PRINT_VARIANT(ret);
#endif
    return ret;
}

PCA_INLINE UNUSED_FUNCTION bool is_digit (char c)
{
    return c >= '0' && c <= '9';
}

static
purc_variant_t find_stack_var (void* ctxt, const char* name)
{
    struct pcintr_stack* stack = (struct pcintr_stack*) ctxt;
    size_t nr_name = strlen(name);
    char last = name[nr_name - 1];

    if(is_digit(name[0])) {
        unsigned int number = atoi(name);

        PC_ASSERT(is_digit(last) == 0);
        return pcintr_get_symbolized_var(stack, number, last);
    }

    if (nr_name == 1 && purc_ispunct(last)) {
        return pcintr_get_symbolized_var(stack, 1, last);
    }

    return pcintr_find_named_var(ctxt, name);
}

purc_variant_t pcvcm_eval (struct pcvcm_node* tree, struct pcintr_stack* stack,
        bool silently)
{
    if (stack) {
        return pcvcm_eval_ex(tree, find_stack_var, stack, silently);
    }
    return pcvcm_eval_ex(tree, NULL, NULL, silently);
}

purc_variant_t pcvcm_eval_ex (struct pcvcm_node* tree,
        cb_find_var find_var, void* ctxt, bool silently)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;

    struct pcvcm_node_op ops = {
        .find_var = find_var,
        .find_var_ctxt = ctxt,
    };

    if (tree) {
        ret = pcvcm_node_to_variant (tree, &ops, silently);
    }

    if (ret == PURC_VARIANT_INVALID && silently) {
        ret = purc_variant_make_undefined();
    }
    return ret;
}

