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

#define PURC_ENVV_VCM_LOG_ENABLE    "PURC_VCM_LOG_ENABLE"

static const char *typenames[] = {
    PCVCM_NODE_TYPE_NAME_UNDEFINED,
    PCVCM_NODE_TYPE_NAME_OBJECT,
    PCVCM_NODE_TYPE_NAME_ARRAY,
    PCVCM_NODE_TYPE_NAME_STRING,
    PCVCM_NODE_TYPE_NAME_NULL,
    PCVCM_NODE_TYPE_NAME_BOOLEAN,
    PCVCM_NODE_TYPE_NAME_NUMBER,
    PCVCM_NODE_TYPE_NAME_LONG_INT,
    PCVCM_NODE_TYPE_NAME_ULONG_INT,
    PCVCM_NODE_TYPE_NAME_LONG_DOUBLE,
    PCVCM_NODE_TYPE_NAME_BYTE_SEQUENCE,
    PCVCM_NODE_TYPE_NAME_CONCAT_STRING,
    PCVCM_NODE_TYPE_NAME_GET_VARIABLE,
    PCVCM_NODE_TYPE_NAME_GET_ELEMENT,
    PCVCM_NODE_TYPE_NAME_CALL_GETTER,
    PCVCM_NODE_TYPE_NAME_CALL_SETTER,
    PCVCM_NODE_TYPE_NAME_CJSONEE,
    PCVCM_NODE_TYPE_NAME_CJSONEE_OP_AND,
    PCVCM_NODE_TYPE_NAME_CJSONEE_OP_OR,
    PCVCM_NODE_TYPE_NAME_CJSONEE_OP_SEMICOLON,
};

#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(types, PCA_TABLESIZE(typenames) == PCVCM_NODE_TYPE_NR);
#undef _COMPILE_TIME_ASSERT

typedef
void (*pcvcm_node_handle)(purc_rwstream_t rws, struct pcvcm_node *node,
        bool ignore_string_quoted);

static
void pcvcm_node_write_to_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
        bool ignore_string_quoted);

static
void pcvcm_node_serialize_to_rwstream(purc_rwstream_t rws,
        struct pcvcm_node *node, bool ignore_string_quoted);

struct pcvcm_node_op {
    cb_find_var find_var;
    void *find_var_ctxt;
};

// expression variable
struct pcvcm_ev {
    struct pcvcm_node *vcm;
    purc_variant_t const_value;
    purc_variant_t last_value;
    bool release_vcm;
};

static bool _print_vcm_log = false;

const char *pcvcm_node_typename(enum pcvcm_node_type type)
{
    assert(type >= 0 && type < PCVCM_NODE_TYPE_NR);
    return typenames[type];
}

static struct pcvcm_node *pcvcm_node_new(enum pcvcm_node_type type, bool closed)
{
    struct pcvcm_node *node = (struct pcvcm_node*)calloc(1,
            sizeof(struct pcvcm_node));
    if (!node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    node->type = type;
    node->is_closed = closed;
    return node;
}


struct pcvcm_node *pcvcm_node_new_undefined()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_UNDEFINED, true);
}

struct pcvcm_node *pcvcm_node_new_object(size_t nr_nodes,
        struct pcvcm_node **nodes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_OBJECT, false);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        struct pcvcm_node *v = nodes[i];
        APPEND_CHILD(n, v);
    }

    return n;
}

struct pcvcm_node *pcvcm_node_new_array(size_t nr_nodes,
        struct pcvcm_node **nodes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_ARRAY, false);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        struct pcvcm_node *v = nodes[i];
        APPEND_CHILD(n, v);
    }

    return n;
}

struct pcvcm_node *pcvcm_node_new_string(const char *str_utf8)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_STRING, true);
    if (!n) {
        return NULL;
    }

    size_t nr_bytes = strlen(str_utf8);

    uint8_t *buf = (uint8_t*)malloc(nr_bytes + 1);
    memcpy(buf, str_utf8, nr_bytes);
    buf[nr_bytes] = 0;

    n->sz_ptr[0] = nr_bytes;
    n->sz_ptr[1] = (uintptr_t)buf;

    return n;
}

struct pcvcm_node *pcvcm_node_new_null()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_NULL, true);
}

struct pcvcm_node *pcvcm_node_new_boolean(bool b)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_BOOLEAN, true);
    if (!n) {
        return NULL;
    }

    n->b = b;
    return n;
}

struct pcvcm_node *pcvcm_node_new_number(double d)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_NUMBER, true);
    if (!n) {
        return NULL;
    }

    n->d = d;
    return n;
}

struct pcvcm_node *pcvcm_node_new_longint(int64_t i64)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_LONG_INT, true);
    if (!n) {
        return NULL;
    }

    n->i64 = i64;
    return n;
}

struct pcvcm_node *pcvcm_node_new_ulongint(uint64_t u64)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_ULONG_INT, true);
    if (!n) {
        return NULL;
    }

    n->u64 = u64;
    return n;
}

struct pcvcm_node *pcvcm_node_new_longdouble(long double ld)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_LONG_DOUBLE, true);
    if (!n) {
        return NULL;
    }

    n->ld = ld;
    return n;
}

struct pcvcm_node *pcvcm_node_new_byte_sequence(const void *bytes,
        size_t nr_bytes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_BYTE_SEQUENCE, true);
    if (!n) {
        return NULL;
    }

    if (nr_bytes == 0) {
        n->sz_ptr[0] = 0;
        n->sz_ptr[1] = 0;
        return n;
    }

    uint8_t *buf = (uint8_t*)malloc(nr_bytes + 1);
    memcpy(buf, bytes, nr_bytes);
    buf[nr_bytes] = 0;

    n->sz_ptr[0] = nr_bytes;
    n->sz_ptr[1] = (uintptr_t)buf;
    return n;
}


static void hex_to_bytes(const uint8_t *hex, size_t sz_hex, uint8_t *result)
{
    uint8_t h = 0;
    uint8_t l = 0;
    for (size_t i = 0; i < sz_hex/2; i++) {
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

struct pcvcm_node *pcvcm_node_new_byte_sequence_from_bx(const void *bytes,
        size_t nr_bytes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_BYTE_SEQUENCE, true);
    if (!n) {
        return NULL;
    }

    if (nr_bytes == 0) {
        n->sz_ptr[0] = 0;
        n->sz_ptr[1] = 0;
        return n;
    }

    const uint8_t *p = bytes;
    size_t sz = nr_bytes;
    if (sz % 2 != 0) {
        pcinst_set_error(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        return NULL;
    }
    size_t sz_buf = sz / 2;
    uint8_t *buf = (uint8_t*)calloc(sz_buf + 1, 1);
    hex_to_bytes(p, sz, buf);

    n->sz_ptr[0] = sz_buf;
    n->sz_ptr[1] = (uintptr_t)buf;
    return n;
}

struct pcvcm_node *pcvcm_node_new_byte_sequence_from_bb(const void *bytes,
        size_t nr_bytes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_BYTE_SEQUENCE, true);
    if (!n) {
        return NULL;
    }

    if (nr_bytes == 0) {
        n->sz_ptr[0] = 0;
        n->sz_ptr[1] = 0;
        return n;
    }

    const uint8_t *p = bytes;
    size_t sz = nr_bytes;
    if (sz % 8 != 0) {
        pcinst_set_error(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        return NULL;
    }

    size_t sz_buf = sz / 8;
    uint8_t *buf = (uint8_t*)calloc(sz_buf + 1, 1);
    for (size_t i = 0; i < sz_buf; i++) {
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
    n->sz_ptr[1] = (uintptr_t)buf;
    return n;
}

struct pcvcm_node *pcvcm_node_new_byte_sequence_from_b64 (const void *bytes,
        size_t nr_bytes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_BYTE_SEQUENCE, true);
    if (!n) {
        return NULL;
    }

    if (nr_bytes == 0) {
        n->sz_ptr[0] = 0;
        n->sz_ptr[1] = 0;
        return n;
    }

    const uint8_t *p = bytes;
    size_t sz_buf = nr_bytes;
    uint8_t *buf = (uint8_t*)calloc(sz_buf, 1);

    ssize_t ret = pcutils_b64_decode(p, buf, sz_buf);
    if (ret == -1) {
        free(buf);
        pcinst_set_error(PCHVML_ERROR_UNEXPECTED_CHARACTER);
        return NULL;
    }

    n->sz_ptr[0] = ret;
    n->sz_ptr[1] = (uintptr_t)buf;
    return n;
}

struct pcvcm_node *pcvcm_node_new_concat_string(size_t nr_nodes,
        struct pcvcm_node *nodes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_CONCAT_STRING,
            false);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        APPEND_CHILD(n, nodes  + i);
    }

    return n;
}

struct pcvcm_node *pcvcm_node_new_get_variable(struct pcvcm_node *node)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_GET_VARIABLE, 
            false);
    if (!n) {
        return NULL;
    }

    if (node) {
        APPEND_CHILD(n, node);
    }

    return n;
}

struct pcvcm_node *pcvcm_node_new_get_element(struct pcvcm_node *variable,
        struct pcvcm_node *identifier)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_GET_ELEMENT,
            false);
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

struct pcvcm_node *pcvcm_node_new_call_getter(struct pcvcm_node *variable,
        size_t nr_params, struct pcvcm_node *params)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_CALL_GETTER,
            false);
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

struct pcvcm_node *pcvcm_node_new_call_setter(struct pcvcm_node *variable,
        size_t nr_params, struct pcvcm_node *params)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_CALL_SETTER,
            false);
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

struct pcvcm_node *pcvcm_node_new_cjsonee()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_CJSONEE, false);
}

struct pcvcm_node *pcvcm_node_new_cjsonee_op_and()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_CJSONEE_OP_AND, true);
}

struct pcvcm_node *pcvcm_node_new_cjsonee_op_or()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_CJSONEE_OP_OR, true);
}

struct pcvcm_node *pcvcm_node_new_cjsonee_op_semicolon()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON, true);
}

static
void write_child_node_rwstream_ex(purc_rwstream_t rws,
        struct pcvcm_node *node, bool print_comma, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = FIRST_CHILD(node);
    while (child) {
        handle(rws, child, false);
        child = NEXT_CHILD(child);
        if (child && print_comma) {
            purc_rwstream_write(rws, ",", 1);
        }
    }
}

static
void write_child_node_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
         pcvcm_node_handle handle)
{
    write_child_node_rwstream_ex(rws, node, true, handle);
}

static
void write_object_serialize_to_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
         pcvcm_node_handle handle)
{
    struct pcvcm_node *child = FIRST_CHILD(node);
    int i = 0;
    while (child) {
        handle(rws, child, false);
        child = NEXT_CHILD(child);
        if (child) {
            if (i % 2 == 0) {
                purc_rwstream_write(rws, ":", 1);
            }
            else {
                purc_rwstream_write(rws, ", ", 2);
            }
        }
        i++;
    }
}

static
void write_concat_string_node_serialize_rwstream(purc_rwstream_t rws,
        struct pcvcm_node *node, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = FIRST_CHILD(node);
    while (child) {
        handle(rws, child, true);
        child = NEXT_CHILD(child);
    }
}

static
void write_sibling_node_rwstream(purc_rwstream_t rws,
        struct pcvcm_node *node, bool print_comma, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = NEXT_CHILD(node);
    while (child) {
        handle(rws, child, false);
        child = NEXT_CHILD(child);
        if (child && print_comma) {
            purc_rwstream_write(rws, ", ", 2);
        }
    }
}


static
void write_variant_to_rwstream(purc_rwstream_t rws, purc_variant_t v)
{
    size_t len_expected = 0;
    purc_variant_serialize(v, rws, 0,
            PCVARIANT_SERIALIZE_OPT_REAL_EJSON |
            PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BASE64 |
            PCVARIANT_SERIALIZE_OPT_PLAIN,
            &len_expected);
}

void pcvcm_node_write_to_rwstream(purc_rwstream_t rws,
        struct pcvcm_node *node, bool ignore_string_quoted)
{
    UNUSED_PARAM(ignore_string_quoted);
    pcvcm_node_handle handle = pcvcm_node_write_to_rwstream;
    switch(node->type)
    {
    case PCVCM_NODE_TYPE_UNDEFINED:
        purc_rwstream_write(rws, "undefined", 9);
        break;

    case PCVCM_NODE_TYPE_OBJECT:
        purc_rwstream_write(rws, "make_object(", 12);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_ARRAY:
        purc_rwstream_write(rws, "make_array(", 11);
        write_child_node_rwstream(rws, node, handle);
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
        purc_variant_t v = purc_variant_make_boolean(node->b);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_NUMBER:
    {
        purc_variant_t v = purc_variant_make_number(node->d);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_INT:
    {
        purc_variant_t v = purc_variant_make_longint(node->i64);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_ULONG_INT:
    {
        purc_variant_t v = purc_variant_make_ulongint(node->u64);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_DOUBLE:
    {
        purc_variant_t v = purc_variant_make_longdouble(node->ld);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
    {
        purc_variant_t v;
        if (node->sz_ptr[0]) {
            v = purc_variant_make_byte_sequence(
                (void*)node->sz_ptr[1], node->sz_ptr[0]);
        }
        else {
            v = purc_variant_make_byte_sequence_empty();
        }
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CONCAT_STRING:
        purc_rwstream_write(rws, "concat_string(", 14);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_VARIABLE:
        purc_rwstream_write(rws, "get_variable(", 13);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_ELEMENT:
        purc_rwstream_write(rws, "get_element(", 12);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_CALL_GETTER:
        purc_rwstream_write(rws, "call_getter(", 12);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_CALL_SETTER:
        purc_rwstream_write(rws, "call_setter(", 12);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;
    case PCVCM_NODE_TYPE_CJSONEE:
        purc_rwstream_write(rws, "{{ ", 3);
        write_child_node_rwstream_ex(rws, node, false, handle);
        purc_rwstream_write(rws, " }}", 3);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
        purc_rwstream_write(rws, " && ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
        purc_rwstream_write(rws, " || ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        purc_rwstream_write(rws, " ; ", 3);
        break;
    }
}

void pcvcm_node_serialize_to_rwstream(purc_rwstream_t rws,
        struct pcvcm_node *node, bool ignore_string_quoted)
{
    pcvcm_node_handle handle = pcvcm_node_serialize_to_rwstream;
    switch(node->type)
    {
    case PCVCM_NODE_TYPE_UNDEFINED:
        purc_rwstream_write(rws, "undefined", 9);
        break;

    case PCVCM_NODE_TYPE_OBJECT:
        purc_rwstream_write(rws, "{ ", 2);
        write_object_serialize_to_rwstream(rws, node, handle);
        purc_rwstream_write(rws, " }", 2);
        break;

    case PCVCM_NODE_TYPE_ARRAY:
        purc_rwstream_write(rws, "[ ", 2);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, " ]", 2);
        break;

    case PCVCM_NODE_TYPE_STRING:
    {
        char *buf = (char*)node->sz_ptr[1];
        size_t nr_buf = node->sz_ptr[0];
        char c[4] = {0};
        c[0] = '"';
        if (strchr(buf, '"')) {
            c[0] = '\'';
        }
        if (strchr(buf, '\n')) {
            c[0] = '"';
            c[1] = '"';
            c[2] = '"';
        }
        if (!ignore_string_quoted) {
            purc_rwstream_write(rws, &c, strlen(c));
        }
        for (size_t i = 0; i < nr_buf; i++) {
            if (buf[i] == '\\') {
                purc_rwstream_write(rws, "\\", 1);
            }
            purc_rwstream_write(rws, buf + i, 1);
        }
        if (!ignore_string_quoted) {
            purc_rwstream_write(rws, &c, strlen(c));
        }
        break;
    }

    case PCVCM_NODE_TYPE_NULL:
        purc_rwstream_write(rws, "null", 4);
        break;

    case PCVCM_NODE_TYPE_BOOLEAN:
    {
        purc_variant_t v = purc_variant_make_boolean(node->b);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_NUMBER:
    {
        purc_variant_t v = purc_variant_make_number(node->d);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_INT:
    {
        purc_variant_t v = purc_variant_make_longint(node->i64);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_ULONG_INT:
    {
        purc_variant_t v = purc_variant_make_ulongint(node->u64);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_DOUBLE:
    {
        purc_variant_t v = purc_variant_make_longdouble(node->ld);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
    {
        purc_variant_t v;
        if (node->sz_ptr[0]) {
            v = purc_variant_make_byte_sequence(
                (void*)node->sz_ptr[1], node->sz_ptr[0]);
        }
        else {
            v = purc_variant_make_byte_sequence_empty();
        }
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CONCAT_STRING:
        purc_rwstream_write(rws, "\"", 1);
        write_concat_string_node_serialize_rwstream(rws, node, handle);
        purc_rwstream_write(rws, "\"", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_VARIABLE:
   {
        purc_rwstream_write(rws, "$", 1);
        struct pcvcm_node *child = FIRST_CHILD(node);
        handle(rws, child, true);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_GET_ELEMENT:
   {
        struct pcvcm_node *child = FIRST_CHILD(node);
        handle(rws, child, true);

        child = NEXT_CHILD(child);
        if (child->type == PCVCM_NODE_TYPE_STRING) {
            purc_rwstream_write(rws, ".", 1);
            handle(rws, child, true);
        }
        else {
            purc_rwstream_write(rws, "[", 1);
            handle(rws, child, true);
            purc_rwstream_write(rws, "]", 1);
        }
        break;
   }

    case PCVCM_NODE_TYPE_FUNC_CALL_GETTER:
    {
        struct pcvcm_node *child = FIRST_CHILD(node);
        handle(rws, child, true);
        purc_rwstream_write(rws, "( ", 2);
        write_sibling_node_rwstream(rws, child, true, handle);
        purc_rwstream_write(rws, " )", 2);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CALL_SETTER:
    {
        struct pcvcm_node *child = FIRST_CHILD(node);
        handle(rws, child, true);
        purc_rwstream_write(rws, "(! ", 2);
        write_sibling_node_rwstream(rws, child, true, handle);
        purc_rwstream_write(rws, " )", 2);
        break;
    }

    case PCVCM_NODE_TYPE_CJSONEE:
    {
        purc_rwstream_write(rws, "{{ ", 3);
        write_child_node_rwstream_ex(rws, node, false, handle);
        purc_rwstream_write(rws, " }}", 3);
        break;
    }

    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
        purc_rwstream_write(rws, " && ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
        purc_rwstream_write(rws, " || ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        purc_rwstream_write(rws, " ; ", 3);
        break;
    }
}

static char *
pcvcm_node_dump(struct pcvcm_node *node, size_t *nr_bytes,
        pcvcm_node_handle handle)
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

    handle(rws, node, false);

    purc_rwstream_write(rws, "", 1); // writing null-terminator

    size_t sz_content = 0;
    char *buf = (char*)purc_rwstream_get_mem_buffer_ex(rws, &sz_content,
        NULL, true);
    if (nr_bytes) {
        *nr_bytes = sz_content - 1;
    }

    purc_rwstream_destroy(rws);
    return buf;
}


char *pcvcm_node_to_string(struct pcvcm_node *node, size_t *nr_bytes)
{
    return pcvcm_node_dump(node, nr_bytes, pcvcm_node_write_to_rwstream);
}

char *pcvcm_node_serialize(struct pcvcm_node *node, size_t *nr_bytes)
{
    return pcvcm_node_dump(node, nr_bytes, pcvcm_node_serialize_to_rwstream);
}

static void pcvcm_node_destroy_callback(struct pctree_node *n,  void *data)
{
    UNUSED_PARAM(data);
    struct pcvcm_node *node = VCM_NODE(n);
    if ((node->type == PCVCM_NODE_TYPE_STRING
                || node->type == PCVCM_NODE_TYPE_BYTE_SEQUENCE
        ) && node->sz_ptr[1]) {
        free((void*)node->sz_ptr[1]);
    }
    free(node);
}

void pcvcm_node_destroy(struct pcvcm_node *root)
{
    if (root) {
        pctree_node_post_order_traversal(TREE_NODE(root),
                pcvcm_node_destroy_callback, NULL);
    }
}

struct pcvcm_stack {
    struct pcutils_stack *stack;
};

struct pcvcm_stack *pcvcm_stack_new()
{
    struct pcvcm_stack *stack = (struct pcvcm_stack*)calloc(
            1, sizeof(struct pcvcm_stack));
    if (stack) {
        stack->stack = pcutils_stack_new(0);
        if (!stack->stack) {
            free(stack);
            stack = NULL;
        }
    }

    return stack;
}

bool pcvcm_stack_is_empty(struct pcvcm_stack *stack)
{
    return pcutils_stack_is_empty(stack->stack);
}

void pcvcm_stack_push(struct pcvcm_stack *stack, struct pcvcm_node *e)
{
    pcutils_stack_push(stack->stack, (uintptr_t)e);
}

struct pcvcm_node *pcvcm_stack_pop(struct pcvcm_stack *stack)
{
    return (struct pcvcm_node*)pcutils_stack_pop(stack->stack);
}

struct pcvcm_node *pcvcm_stack_bottommost(struct pcvcm_stack *stack)
{
    return (struct pcvcm_node*)pcutils_stack_top(stack->stack);
}

void pcvcm_stack_destroy(struct pcvcm_stack *stack)
{
    pcutils_stack_destroy(stack->stack);
    free(stack);
}

static
purc_variant_t pcvcm_node_to_variant(struct pcvcm_node *node,
        struct pcvcm_node_op *ops, bool silently);

static
purc_variant_t pcvcm_node_object_to_variant(struct pcvcm_node *node,
        struct pcvcm_node_op *ops, bool silently)
{
    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (object == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t key;
    purc_variant_t value;
    struct pcvcm_node *k_node = FIRST_CHILD(node);
    struct pcvcm_node *v_node = NEXT_CHILD(k_node);
    while (k_node && v_node) {
        key = pcvcm_node_to_variant(k_node, ops, silently);
        if (key == PURC_VARIANT_INVALID) {
            goto out_unref_object;
        }

        value = pcvcm_node_to_variant(v_node, ops, silently);
        if (value == PURC_VARIANT_INVALID) {
            goto out_unref_key;
        }

        if (!purc_variant_object_set(object, key, value)) {
            goto out_unref_value;
        }

        purc_variant_unref(key);
        purc_variant_unref(value);

        k_node = NEXT_CHILD(v_node);
        v_node = NEXT_CHILD(k_node);
    }

    return object;

out_unref_value:
    purc_variant_unref(value);
out_unref_key:
    purc_variant_unref(key);
out_unref_object:
    purc_variant_unref(object);
    return PURC_VARIANT_INVALID;
}

purc_variant_t pcvcm_node_array_to_variant(struct pcvcm_node *node,
       struct pcvcm_node_op *ops, bool silently)
{
    purc_variant_t array = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (array == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;
    struct pcvcm_node *array_node = FIRST_CHILD(node);
    while (array_node) {
        v = pcvcm_node_to_variant(array_node, ops, silently);
        if (v == PURC_VARIANT_INVALID) {
            goto out_unref_array;
        }

        if(!purc_variant_array_append(array, v)) {
            goto out_unref_v;
        }
        purc_variant_unref(v);

        array_node = NEXT_CHILD(array_node);
    }
    return array;

out_unref_v:
    purc_variant_unref(v);
out_unref_array:
    purc_variant_unref(array);
    return PURC_VARIANT_INVALID;
}

purc_variant_t pcvcm_node_concat_string_to_variant(struct pcvcm_node *node,
       struct pcvcm_node_op *ops, bool silently)
{
    purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUF_SIZE, MAX_BUF_SIZE);
    if (!rws) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvcm_node *child = FIRST_CHILD(node);
    while (child) {
        purc_variant_t v = pcvcm_node_to_variant(child, ops, silently);
        if (v == PURC_VARIANT_INVALID) {
            goto out_destroy_rws;
        }

        // FIXME: stringify or serialize
        char *buf = NULL;
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
    char *rw_string = purc_rwstream_get_mem_buffer_ex(rws,
            &content_size, &rw_size, true);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        PC_ASSERT(content_size <= rw_size);
        size_t len = strnlen(rw_string, rw_size);
        PC_ASSERT(len <= content_size);
        PC_ASSERT(len+1 == content_size);
        PC_ASSERT(rw_string[len] == '\0');
        ret_var = purc_variant_make_string_reuse_buff(rw_string,
                content_size, false);
        if (ret_var == PURC_VARIANT_INVALID) {
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

out_destroy_rws:
    purc_rwstream_destroy(rws);
    return ret_var;
}

static
purc_variant_t pcvcm_node_get_variable_to_variant(struct pcvcm_node *node,
       struct pcvcm_node_op *ops, bool silently)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    if (!ops) {
        goto out;
    }

    struct pcvcm_node *name_node = FIRST_CHILD(node);
    if (!name_node) {
        goto out;
    }

    purc_variant_t name_var = pcvcm_node_to_variant(name_node, ops,
            silently);
    if (name_var == PURC_VARIANT_INVALID) {
        goto out;
    }

    if (!purc_variant_is_string(name_var)) {
        goto out_unref_name_var;
    }

    const char *name = purc_variant_get_string_const(name_var);
    size_t nr_name = strlen(name);
    if (!name || nr_name == 0) {
        goto out_unref_name_var;
    }

    if(!ops->find_var) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_FOUND);
        goto out_unref_name_var;
    }

    ret = ops->find_var(ops->find_var_ctxt, name);
    if (ret) {
        purc_variant_ref(ret);
    }

out_unref_name_var:
    purc_variant_unref(name_var);

out:
    return ret;
}

static bool is_action_node(struct pcvcm_node *node)
{
    return (node && (
                node->type == PCVCM_NODE_TYPE_FUNC_GET_ELEMENT ||
                node->type == PCVCM_NODE_TYPE_FUNC_CALL_GETTER ||
                node->type == PCVCM_NODE_TYPE_FUNC_CALL_SETTER
                )
            );
}

static bool is_handle_as_getter(struct pcvcm_node *node)
{
    struct pcvcm_node *parent_node = PARENT_NODE(node);
    if (is_action_node(parent_node) && FIRST_CHILD(parent_node) == node) {
        return false;
    }
    return true;
}

enum method_type {
    GETTER_METHOD,
    SETTER_METHOD
};

static
purc_variant_t call_dvariant_method(purc_variant_t root, purc_variant_t var,
        size_t nr_args, purc_variant_t *argv, enum method_type type,
        bool silently)
{
    purc_dvariant_method func = (type == GETTER_METHOD) ?
         purc_variant_dynamic_get_getter(var) :
         purc_variant_dynamic_get_setter(var);
    if (func) {
        return func(root, nr_args, argv,
                silently ? PCVRT_CALL_FLAG_SILENTLY : 0);
    }
    return PURC_VARIANT_INVALID;
}

static
purc_variant_t call_nvariant_method(purc_variant_t var,
        const char *key_name, size_t nr_args, purc_variant_t *argv,
        enum method_type type, bool silently)
{
    struct purc_native_ops *ops = purc_variant_native_get_ops(var);
    if (ops) {
        purc_nvariant_method native_func = (type == GETTER_METHOD) ?
            ops->property_getter(key_name) :
            ops->property_setter(key_name);
        if (native_func) {
            return  native_func(purc_variant_native_get_entity(var),
                    nr_args, argv, silently ? PCVRT_CALL_FLAG_SILENTLY : 0);
        }
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t get_attach_variant(struct pcvcm_node *node)
{
    return node ? (purc_variant_t)node->attach : PURC_VARIANT_INVALID;
}

#define KEY_INNER_HANDLER           "__vcm_native_wrapper"
#define KEY_CALLER_NODE             "__vcm_caller_node"
#define KEY_PARAM_NODE              "__vcm_param_node"

static purc_variant_t
inner_native_wrapper_create(purc_variant_t caller_node, purc_variant_t param)
{
    purc_variant_t b = purc_variant_make_boolean(true);
    if (b == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (object == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_object_set_by_static_ckey(object, KEY_INNER_HANDLER, b);
    purc_variant_object_set_by_static_ckey(object, KEY_CALLER_NODE, caller_node);
    purc_variant_object_set_by_static_ckey(object, KEY_PARAM_NODE, param);
    purc_variant_unref(b);
    return object;
}

static bool
is_inner_native_wrapper(purc_variant_t val)
{
    if (!val || !purc_variant_is_object(val)) {
        return false;
    }

    // FIXME: keep last error
    int err = purc_get_last_error();
    if (purc_variant_object_get_by_ckey(val, KEY_INNER_HANDLER)) {
        return true;
    }
    purc_set_error(err);
    return false;
}

static purc_variant_t
inner_native_wrapper_get_caller(purc_variant_t val)
{
    return purc_variant_object_get_by_ckey(val, KEY_CALLER_NODE);
}

static purc_variant_t
inner_native_wrapper_get_param(purc_variant_t val)
{
    return purc_variant_object_get_by_ckey(val, KEY_PARAM_NODE);
}

static
purc_variant_t pcvcm_node_get_element_to_variant(struct pcvcm_node *node,
       struct pcvcm_node_op *ops, bool silently)
{
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvcm_node *caller_node = FIRST_CHILD(node);
    if (!caller_node) {
        goto out;
    }

    purc_variant_t caller_var = pcvcm_node_to_variant(caller_node, ops,
            silently);
    if (caller_var == PURC_VARIANT_INVALID) {
        goto out;
    }

    struct pcvcm_node *param_node  = NEXT_CHILD(caller_node);
    purc_variant_t param_var = pcvcm_node_to_variant(param_node, ops,
            silently);
    if (param_var == PURC_VARIANT_INVALID) {
        goto out_unref_caller_var;
    }

    bool has_index = true;
    int64_t index = -1;
    if (param_node->type == PCVCM_NODE_TYPE_STRING) {
        if (pcutils_parse_int64((const char*)param_node->sz_ptr[1],
                    param_node->sz_ptr[0], &index) != 0) {
            has_index = false;
        }
    }
    else if (!purc_variant_cast_to_longint(param_var, &index, true)) {
        has_index = false;
    }

    // FIXME: {{ $SESSION.myobj.bcPipe.status[0] }}
    if (is_inner_native_wrapper(caller_var)) {
        purc_variant_t inner_caller = inner_native_wrapper_get_caller(caller_var);
        purc_variant_t inner_param = inner_native_wrapper_get_param(caller_var);
        purc_variant_t inner_ret = call_nvariant_method(inner_caller,
                purc_variant_get_string_const(inner_param), 0, NULL,
                GETTER_METHOD, silently);
        if (inner_ret) {
            purc_variant_unref(caller_var);
            caller_var = inner_ret;
        }
    }

    if (purc_variant_is_object(caller_var)) {
        purc_variant_t val = purc_variant_object_get(caller_var, param_var);
        if (val == PURC_VARIANT_INVALID) {
            goto out_unref_param_var;
        }

        purc_variant_ref(val);
        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            goto out_unref_param_var;
        }

        if (!is_handle_as_getter(node)) {
            ret_var = val;
            goto out_unref_param_var;
        }

        ret_var = call_dvariant_method(caller_var, val, 0, NULL, GETTER_METHOD,
                silently);
        purc_variant_unref(val);
    }
    else if (purc_variant_is_array(caller_var)) {
        if (!has_index) {
            goto out_unref_param_var;
        }
        if (index < 0) {
            size_t len = purc_variant_array_get_size(caller_var);
            index += len;
        }
        if (index < 0) {
            goto out_unref_param_var;
        }

        purc_variant_t val = purc_variant_array_get(caller_var, index);
        if (val == PURC_VARIANT_INVALID) {
            goto out_unref_param_var;
        }

        purc_variant_ref(val);
        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            goto out_unref_param_var;
        }

        if (!is_handle_as_getter(node)) {
            ret_var = val;
            goto out_unref_param_var;
        }
        ret_var = call_dvariant_method(caller_var, val, 0, NULL, GETTER_METHOD,
                silently);
        purc_variant_unref(val);
    }
    else if (purc_variant_is_set(caller_var)) {
        if (!has_index) {
            goto out_unref_param_var;
        }
        if (index < 0) {
            size_t len = purc_variant_set_get_size(caller_var);
            index += len;
        }
        if (index < 0) {
            goto out_unref_param_var;
        }

        purc_variant_t val = purc_variant_set_get_by_index(caller_var, index);
        if (val == PURC_VARIANT_INVALID) {
            goto out_unref_param_var;
        }

        purc_variant_ref(val);
        if (!purc_variant_is_dynamic(val)) {
            ret_var = val;
            goto out_unref_param_var;
        }

        if (!is_handle_as_getter(node)) {
            ret_var = val;
            goto out_unref_param_var;
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
        goto out_unref_param_var;
    }
    else if (purc_variant_is_native(caller_var)) {
        if (!is_handle_as_getter(node)) {
            ret_var = inner_native_wrapper_create(caller_var, param_var);
            goto out_unref_param_var;
        }
        ret_var = call_nvariant_method(caller_var,
                purc_variant_get_string_const(param_var), 0, NULL,
                GETTER_METHOD, silently);
        goto out_unref_param_var;
    }

out_unref_param_var:
    purc_variant_unref(param_var);
out_unref_caller_var:
    purc_variant_unref(caller_var);
out:
    return ret_var;
}

purc_variant_t pcvcm_node_call_method_to_variant(struct pcvcm_node *node,
       struct pcvcm_node_op *ops, enum method_type type, bool silently)
{
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcvcm_node *caller_node = FIRST_CHILD(node);
    if (!caller_node) {
        goto out;
    }

    purc_variant_t caller_var = pcvcm_node_to_variant(caller_node, ops, silently);
    if (caller_var == PURC_VARIANT_INVALID) {
        goto out;
    }

    if (!purc_variant_is_dynamic(caller_var)
            && !is_inner_native_wrapper(caller_var)) {
        goto out_unref_caller_var;
    }

    purc_variant_t *params = NULL;
    size_t nr_params = CHILDREN_NUMBER(node) - 1;
    if (nr_params > 0) {
        params = (purc_variant_t*)calloc(nr_params, sizeof(purc_variant_t));
        if (!params) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out_unref_caller_var;
        }

        int i = 0;
        struct pcvcm_node *param_node = NEXT_CHILD(caller_node);
        while (param_node) {
            purc_variant_t vt = pcvcm_node_to_variant(param_node, ops, silently);
            if (vt == PURC_VARIANT_INVALID) {
                goto out_unref_params;
            }

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
    else if (is_inner_native_wrapper(caller_var)) {
        purc_variant_t nv = inner_native_wrapper_get_caller(caller_var);
        if (purc_variant_is_native(nv)) {
            purc_variant_t name = inner_native_wrapper_get_param(caller_var);
            if (name) {
                ret_var = call_nvariant_method(nv,
                        purc_variant_get_string_const(name), nr_params,
                        params, type, silently);
            }
        }
    }

out_unref_params:
    for (size_t i = 0; i < nr_params; i++) {
        if (params[i]) {
            purc_variant_unref(params[i]);
        }
    }
    free(params);

out_unref_caller_var:
    purc_variant_unref(caller_var);
out:
    return ret_var;
}

bool is_cjsonee_op(struct pcvcm_node *node)
{
    if (!node) {
        return false;
    }
    switch (node->type) {
    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        return true;
    default:
        return false;
    }
    return false;
}

static
purc_variant_t pcvcm_node_cjsonee_to_variant(struct pcvcm_node *node,
       struct pcvcm_node_op *ops, bool silently)
{
    UNUSED_PARAM(node);
    UNUSED_PARAM(ops);
    UNUSED_PARAM(silently);
    purc_variant_t curr_val = PURC_VARIANT_INVALID;
    struct pcvcm_node *curr_node = FIRST_CHILD(node);
    struct pcvcm_node *op_node = NULL;
    while (curr_node) {
        if (is_cjsonee_op(curr_node)) {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            goto failed;
        }

        curr_val = pcvcm_node_to_variant(curr_node, ops, silently);
        if (curr_val == PURC_VARIANT_INVALID) {
            goto failed;
        }

next_op:
        op_node = NEXT_CHILD(curr_node);
        if (op_node == NULL) {
            break;
        }

        if (!is_cjsonee_op(op_node)) {
            pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
            goto failed;
        }

        curr_node = NEXT_CHILD(op_node);
        switch (op_node->type) {
        case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
            {
                if (!curr_node) {
                    goto out;
                }
            }
            break;

        case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
            {
                if (!curr_node) {
                    pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
                    goto failed;
                }
                if (!purc_variant_booleanize(curr_val)) {
                    goto next_op;
                }
            }
            break;

        case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
            {
                if (!curr_node) {
                    pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
                    goto failed;
                }
                if (purc_variant_booleanize(curr_val)) {
                    goto next_op;
                }
            }
            break;

        default:
            pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
        purc_variant_unref(curr_val);
    }

out:
    return curr_val;

failed:
    if (curr_val) {
        purc_variant_unref(curr_val);
    }
    return PURC_VARIANT_INVALID;
}

static bool has_fatal_error()
{
    int err = purc_get_last_error();
    return (err == PURC_ERROR_OUT_OF_MEMORY);
}

purc_variant_t pcvcm_node_to_variant(struct pcvcm_node *node,
        struct pcvcm_node_op *ops, bool silently)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    switch(node->type)
    {
        case PCVCM_NODE_TYPE_UNDEFINED:
            ret = purc_variant_make_undefined();
            break;

        case PCVCM_NODE_TYPE_OBJECT:
            ret = pcvcm_node_object_to_variant(node, ops, silently);
            break;

        case PCVCM_NODE_TYPE_ARRAY:
            ret = pcvcm_node_array_to_variant(node, ops, silently);
            break;

        case PCVCM_NODE_TYPE_STRING:
            return purc_variant_make_string((char*)node->sz_ptr[1],
                    false);

        case PCVCM_NODE_TYPE_NULL:
            ret = purc_variant_make_null();
            break;

        case PCVCM_NODE_TYPE_BOOLEAN:
            ret = purc_variant_make_boolean(node->b);
            break;

        case PCVCM_NODE_TYPE_NUMBER:
            ret = purc_variant_make_number(node->d);
            break;

        case PCVCM_NODE_TYPE_LONG_INT:
            ret = purc_variant_make_longint(node->i64);
            break;

        case PCVCM_NODE_TYPE_ULONG_INT:
            ret = purc_variant_make_ulongint(node->u64);
            break;

        case PCVCM_NODE_TYPE_LONG_DOUBLE:
            ret = purc_variant_make_longdouble(node->ld);
            break;

        case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
            return (node->sz_ptr[0] > 0) ? purc_variant_make_byte_sequence(
                    (void*)node->sz_ptr[1], node->sz_ptr[0])
                    : purc_variant_make_byte_sequence_empty();

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

        case PCVCM_NODE_TYPE_CJSONEE:
            ret = pcvcm_node_cjsonee_to_variant(node, ops, silently);
            break;

        default:
            ret = purc_variant_make_null();
            break;
    }

    if (ret == PURC_VARIANT_INVALID
            && silently && !has_fatal_error()) {
        ret = purc_variant_make_undefined();
    }

    node->attach = (uintptr_t)ret;

    if (_print_vcm_log) {
        PRINT_VCM_NODE(node);
        PRINT_VARIANT(ret);
    }
    return ret;
}

static inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static
purc_variant_t find_stack_var(void *ctxt, const char *name)
{
    struct pcintr_stack *stack = (struct pcintr_stack*)ctxt;
    size_t nr_name = strlen(name);
    char last = name[nr_name - 1];

    if (is_digit(name[0])) {
        unsigned int number = atoi(name);

        PC_ASSERT(is_digit(last) == 0);
        return pcintr_get_symbolized_var(stack, number, last);
    }

    if (nr_name == 1 && purc_ispunct(last)) {
        return pcintr_get_symbolized_var(stack, 1, last);
    }

    // # + anchor + symbol
    if (name[0] == '#') {
        char* anchor = strndup(name + 1, nr_name - 2);
        if (!anchor) {
            return PURC_VARIANT_INVALID;
        }
        purc_variant_t var = pcintr_find_anchor_symbolized_var(stack, anchor,
                last);
        free(anchor);
        return var;
    }

    return pcintr_find_named_var(ctxt, name);
}

purc_variant_t pcvcm_eval(struct pcvcm_node *tree, struct pcintr_stack *stack,
        bool silently)
{
    if (stack) {
        return pcvcm_eval_ex(tree, NULL, find_stack_var, stack, silently);
    }
    return pcvcm_eval_ex(tree, NULL, NULL, NULL, silently);
}

purc_variant_t pcvcm_eval_again(struct pcvcm_node *tree,
        struct pcintr_stack *stack, bool silently, bool timeout)
{
    if (stack) {
        return pcvcm_eval_again_ex(tree, NULL, find_stack_var, stack,
                silently, timeout);
    }
    return pcvcm_eval_again_ex(tree, NULL, NULL, NULL, silently, timeout);
}

purc_variant_t pcvcm_eval_ex(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt **eval_ctxt,
        cb_find_var find_var, void *ctxt,
        bool silently)
{
    UNUSED_PARAM(eval_ctxt);
    const char *env_value;
    if ((env_value = getenv(PURC_ENVV_VCM_LOG_ENABLE))) {
        _print_vcm_log = (*env_value == '1' ||
                pcutils_strcasecmp(env_value, "true") == 0);
    }

    if (_print_vcm_log) {
        PC_DEBUG("pcvcm_eval_ex|begin|silently=%d\n", silently);
    }

    purc_variant_t ret = PURC_VARIANT_INVALID;

    struct pcvcm_node_op ops = {
        .find_var = find_var,
        .find_var_ctxt = ctxt,
    };

    if (tree) {
        ret = pcvcm_node_to_variant(tree, &ops, silently);
    }
    else if (silently) {
        ret = purc_variant_make_undefined();
    }

    if (_print_vcm_log) {
        PRINT_VARIANT(ret);
        PC_DEBUG("pcvcm_eval_ex|end|silently=%d\n", silently);
    }
    return ret;
}

purc_variant_t pcvcm_eval_again_ex(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *eval_ctxt,
        cb_find_var find_var, void *ctxt,
        bool silently, bool timeout)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(eval_ctxt);
    UNUSED_PARAM(find_var);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(silently);
    UNUSED_PARAM(timeout);

    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
eval_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    struct pcintr_stack *stack = pcintr_get_stack();
    if (!stack) {
        return PURC_VARIANT_INVALID;
    }
    return pcvcm_eval(vcm_ev->vcm, stack,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
}

static purc_variant_t
eval_const_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    if (vcm_ev->const_value) {
        return vcm_ev->const_value;
    }

    vcm_ev->const_value = eval_getter(native_entity, nr_args, argv,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
    return vcm_ev->const_value;
}

static purc_variant_t
vcm_ev_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return purc_variant_make_boolean(true);
}


static purc_variant_t
last_value_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    return vcm_ev->last_value;
}

static purc_variant_t
last_value_setter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    if (nr_args == 0) {
        return PURC_VARIANT_INVALID;
    }

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    if (vcm_ev->last_value) {
        purc_variant_unref(vcm_ev->last_value);
    }
    vcm_ev->last_value = argv[0];
    if (vcm_ev->last_value) {
        purc_variant_ref(vcm_ev->last_value);
    }
    return vcm_ev->last_value;
}

static inline
purc_nvariant_method property_getter(const char* key_name)
{
    if (strcmp(key_name, PCVCM_EV_PROPERTY_EVAL) == 0) {
        return eval_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_EVAL_CONST) == 0) {
        return eval_const_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_VCM_EV) == 0) {
        return vcm_ev_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_LAST_VALUE) == 0) {
        return last_value_getter;
    }

    return NULL;
}

static inline
purc_nvariant_method property_setter(const char* key_name)
{
    if (strcmp(key_name, PCVCM_EV_PROPERTY_LAST_VALUE) == 0) {
        return last_value_setter;
    }

    return NULL;
}

bool on_observe(void *native_entity, const char *event_name,
        const char *event_subname)
{
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    struct pcintr_stack *stack = pcintr_get_stack();
    if (!stack) {
        return false;
    }
    vcm_ev->last_value = pcvcm_eval(vcm_ev->vcm, stack, false);
    return (vcm_ev->last_value) ? true : false;
}

static void
on_release(void *native_entity)
{
    struct pcvcm_ev *vcm_variant = (struct pcvcm_ev*)native_entity;
    if (vcm_variant->release_vcm) {
        free(vcm_variant->vcm);
    }
    if (vcm_variant->const_value) {
        purc_variant_unref(vcm_variant->const_value);
    }
    if (vcm_variant->last_value) {
        purc_variant_unref(vcm_variant->last_value);
    }
    free(vcm_variant);
}

purc_variant_t
pcvcm_to_expression_variable(struct pcvcm_node *vcm, bool release_vcm)
{
    static struct purc_native_ops ops = {
        .property_getter        = property_getter,
        .property_setter        = property_setter,
        .property_eraser        = NULL,
        .property_cleaner       = NULL,

        .updater                = NULL,
        .cleaner                = NULL,
        .eraser                 = NULL,

        .on_observe            = on_observe,
        .on_release            = on_release,
    };

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)calloc(1,
            sizeof(struct pcvcm_ev));
    if (!vcm_ev) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = purc_variant_make_native(vcm_ev, &ops);
    if (v == PURC_VARIANT_INVALID) {
        free(vcm_ev);
        return PURC_VARIANT_INVALID;
    }

    vcm_ev->vcm = vcm;
    vcm_ev->release_vcm = release_vcm;

    return v;
}

