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
#include "private/tkz-helper.h"

#include "eval.h"

static const char *typenames[] = {
    PCVCM_NODE_TYPE_NAME_UNDEFINED,
    PCVCM_NODE_TYPE_NAME_OBJECT,
    PCVCM_NODE_TYPE_NAME_ARRAY,
    PCVCM_NODE_TYPE_NAME_TUPLE,
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
    PCVCM_NODE_TYPE_NAME_CONSTANT,
};

#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(types, PCA_TABLESIZE(typenames) == PCVCM_NODE_TYPE_NR);
#undef _COMPILE_TIME_ASSERT

const char *
pcvcm_node_typename(enum pcvcm_node_type type)
{
    assert(type >= 0 && type < PCVCM_NODE_TYPE_NR);
    return typenames[type];
}

static struct pcvcm_node *
pcvcm_node_new(enum pcvcm_node_type type, bool closed)
{
    struct pcvcm_node *node = (struct pcvcm_node*)calloc(1,
            sizeof(struct pcvcm_node));
    if (!node) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    node->type = type;
    node->is_closed = closed;
    node->position = -1;
    node->idx = -1;
    node->nr_nodes = -1;
    return node;
}


struct pcvcm_node *
pcvcm_node_new_undefined()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_UNDEFINED, true);
}


struct pcvcm_node *
pcvcm_node_new_object(size_t nr_nodes, struct pcvcm_node **nodes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_OBJECT, false);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        struct pcvcm_node *v = nodes[i];
        pcvcm_node_append_child(n, v);
    }

    return n;
}

struct pcvcm_node *
pcvcm_node_new_array(size_t nr_nodes, struct pcvcm_node **nodes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_ARRAY, false);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        struct pcvcm_node *v = nodes[i];
        pcvcm_node_append_child(n, v);
    }

    return n;
}

struct pcvcm_node *
pcvcm_node_new_string(const char *str_utf8)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_STRING, true);
    if (!n) {
        return NULL;
    }

    size_t nr_bytes = strlen(str_utf8);

    uint8_t *buf = (uint8_t*)malloc(nr_bytes + 1);
    memcpy(buf, str_utf8, nr_bytes);
    buf[nr_bytes] = 0;

    n->quoted_type = PCVCM_NODE_QUOTED_TYPE_DOUBLE;
    n->sz_ptr[0] = nr_bytes;
    n->sz_ptr[1] = (uintptr_t)buf;

    return n;
}

struct pcvcm_node *
pcvcm_node_new_null()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_NULL, true);
}

struct pcvcm_node *
pcvcm_node_new_boolean(bool b)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_BOOLEAN, true);
    if (!n) {
        return NULL;
    }

    n->b = b;
    return n;
}

struct pcvcm_node *
pcvcm_node_new_number(double d)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_NUMBER, true);
    if (!n) {
        return NULL;
    }

    n->d = d;
    return n;
}

struct pcvcm_node *
pcvcm_node_new_longint(int64_t i64)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_LONG_INT, true);
    if (!n) {
        return NULL;
    }

    n->i64 = i64;
    return n;
}

struct pcvcm_node *
pcvcm_node_new_ulongint(uint64_t u64)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_ULONG_INT, true);
    if (!n) {
        return NULL;
    }

    n->u64 = u64;
    return n;
}

struct pcvcm_node *
pcvcm_node_new_longdouble(long double ld)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_LONG_DOUBLE, true);
    if (!n) {
        return NULL;
    }

    n->ld = ld;
    return n;
}

struct pcvcm_node *
pcvcm_node_new_byte_sequence(const void *bytes, size_t nr_bytes)
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


static void
hex_to_bytes(const uint8_t *hex, size_t sz_hex, uint8_t *result)
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

struct pcvcm_node *
pcvcm_node_new_byte_sequence_from_bx(const void *bytes, size_t nr_bytes)
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

struct pcvcm_node *
pcvcm_node_new_byte_sequence_from_bb(const void *bytes, size_t nr_bytes)
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

struct pcvcm_node *
pcvcm_node_new_byte_sequence_from_b64(const void *bytes, size_t nr_bytes)
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

struct pcvcm_node *
pcvcm_node_new_concat_string(size_t nr_nodes, struct pcvcm_node *nodes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_CONCAT_STRING,
            false);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        pcvcm_node_append_child(n, nodes  + i);
    }

    return n;
}

struct pcvcm_node *
pcvcm_node_new_get_variable(struct pcvcm_node *node)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_GET_VARIABLE,
            false);
    if (!n) {
        return NULL;
    }

    if (node) {
        pcvcm_node_append_child(n, node);
    }

    return n;
}

struct pcvcm_node *
pcvcm_node_new_get_element(struct pcvcm_node *variable,
        struct pcvcm_node *identifier)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_GET_ELEMENT,
            false);
    if (!n) {
        return NULL;
    }

    if (variable) {
        pcvcm_node_append_child(n, variable);
    }

    if (identifier) {
        pcvcm_node_append_child(n, identifier);
    }

    return n;
}

struct pcvcm_node *
pcvcm_node_new_call_getter(struct pcvcm_node *variable, size_t nr_params,
        struct pcvcm_node *params)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_CALL_GETTER,
            false);
    if (!n) {
        return NULL;
    }

    if (variable) {
        pcvcm_node_append_child(n, variable);
    }

    for (size_t i = 0; i < nr_params; i++) {
        pcvcm_node_append_child(n, params + i);
    }

    return n;
}

struct pcvcm_node *
pcvcm_node_new_call_setter(struct pcvcm_node *variable, size_t nr_params,
        struct pcvcm_node *params)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_FUNC_CALL_SETTER,
            false);
    if (!n) {
        return NULL;
    }

    if (variable) {
        pcvcm_node_append_child(n, variable);
    }

    for (size_t i = 0; i < nr_params; i++) {
        pcvcm_node_append_child(n, params + i);
    }

    return n;
}

struct pcvcm_node *
pcvcm_node_new_cjsonee()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_CJSONEE, false);
}

struct pcvcm_node *
pcvcm_node_new_cjsonee_op_and()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_CJSONEE_OP_AND, true);
}

struct pcvcm_node *
pcvcm_node_new_cjsonee_op_or()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_CJSONEE_OP_OR, true);
}

struct pcvcm_node *
pcvcm_node_new_cjsonee_op_semicolon()
{
    return pcvcm_node_new(PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON, true);
}

static void
pcvcm_node_destroy_callback(struct pctree_node *n,  void *data)
{
    UNUSED_PARAM(data);
    struct pcvcm_node *node = (struct pcvcm_node*)n;
    if ((node->type == PCVCM_NODE_TYPE_STRING
                || node->type == PCVCM_NODE_TYPE_BYTE_SEQUENCE
        ) && node->sz_ptr[1]) {
        free((void*)node->sz_ptr[1]);
    }
    if (node->ucs) {
        tkz_ucs_destroy(node->ucs);
    }
    free(node);
}

void pcvcm_node_destroy(struct pcvcm_node *root)
{
    if (root) {
        pctree_node_post_order_traversal((struct pctree_node*)root,
                pcvcm_node_destroy_callback, NULL);
    }
}

static inline bool
is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static purc_variant_t
find_stack_var(void *ctxt, const char *name)
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

purc_variant_t
pcvcm_eval(struct pcvcm_node *tree, struct pcintr_stack *stack, bool silently)
{
    if (stack) {
        /* FIXME */
        if (stack->vcm_ctxt) {
            pcvcm_eval_ctxt_destroy(stack->vcm_ctxt);
            stack->vcm_ctxt = NULL;
        }
        purc_variant_t ret = pcvcm_eval_ex(tree, &stack->vcm_ctxt,
                find_stack_var, stack, silently);
        return ret;
    }
    return pcvcm_eval_ex(tree, NULL, NULL, NULL, silently);
}

purc_variant_t
pcvcm_eval_again(struct pcvcm_node *tree, struct pcintr_stack *stack,
        bool silently, bool timeout)
{
    if (stack) {
        purc_variant_t ret = pcvcm_eval_again_ex(tree, stack->vcm_ctxt,
                find_stack_var, stack, silently, timeout);
        return ret;
    }
    return pcvcm_eval_again_ex(tree, NULL, NULL, NULL, silently, timeout);
}

purc_variant_t pcvcm_eval_sub_expr(struct pcvcm_node *tree,
        struct pcintr_stack *stack, purc_variant_t args, bool silently)
{
    if (stack->vcm_ctxt) {
        return pcvcm_eval_sub_expr_full(tree, stack->vcm_ctxt, args, silently);
    }
    return pcvcm_eval_full(tree, &stack->vcm_ctxt, args,
                find_stack_var, stack, silently);
}

purc_variant_t
pcvcm_eval_ex(struct pcvcm_node *tree, struct pcvcm_eval_ctxt **ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently)
{
    return pcvcm_eval_full(tree, ctxt, PURC_VARIANT_INVALID,
            find_var, find_var_ctxt, silently);
}

purc_variant_t
pcvcm_eval_again_ex(struct pcvcm_node *tree, struct pcvcm_eval_ctxt *ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently, bool timeout)
{
    return pcvcm_eval_again_full(tree, ctxt, find_var, find_var_ctxt,
        silently, timeout);
}

struct pcvcm_node *
pcvcm_node_new_tuple(size_t nr_nodes, struct pcvcm_node **nodes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_TUPLE, false);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        struct pcvcm_node *v = nodes[i];
        pcvcm_node_append_child(n, v);
    }

    return n;
}

struct pcvcm_node *
pcvcm_node_new_constant(size_t nr_nodes, struct pcvcm_node **nodes)
{
    struct pcvcm_node *n = pcvcm_node_new(PCVCM_NODE_TYPE_CONSTANT, false);
    if (!n) {
        return NULL;
    }

    for (size_t i = 0; i < nr_nodes; i++) {
        struct pcvcm_node *v = nodes[i];
        pcvcm_node_append_child(n, v);
    }

    return n;
}
