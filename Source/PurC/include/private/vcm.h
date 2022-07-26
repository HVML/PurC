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

#include "private/debug.h"
#include "private/tree.h"
#include "purc-variant.h"


enum pcvcm_node_type {
    PCVCM_NODE_TYPE_UNDEFINED,
    PCVCM_NODE_TYPE_OBJECT,
    PCVCM_NODE_TYPE_ARRAY,
    PCVCM_NODE_TYPE_STRING,
    PCVCM_NODE_TYPE_NULL,
    PCVCM_NODE_TYPE_BOOLEAN,
    PCVCM_NODE_TYPE_NUMBER,
    PCVCM_NODE_TYPE_LONG_INT,
    PCVCM_NODE_TYPE_ULONG_INT,
    PCVCM_NODE_TYPE_LONG_DOUBLE,
    PCVCM_NODE_TYPE_BYTE_SEQUENCE,
    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING,
    PCVCM_NODE_TYPE_FUNC_GET_VARIABLE,
    PCVCM_NODE_TYPE_FUNC_GET_ELEMENT,
    PCVCM_NODE_TYPE_FUNC_CALL_GETTER,
    PCVCM_NODE_TYPE_FUNC_CALL_SETTER,
    PCVCM_NODE_TYPE_CJSONEE,
    PCVCM_NODE_TYPE_CJSONEE_OP_AND,
    PCVCM_NODE_TYPE_CJSONEE_OP_OR,
    PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON,
};

#define EXTRA_NULL              0x0000
#define EXTRA_PROTECT_FLAG      0x0001
#define EXTRA_SUGAR_FLAG        0x0002

#define PCVCM_EV_PROPERTY_EVAL            "eval"
#define PCVCM_EV_PROPERTY_EVAL_CONST      "eval_const"
#define PCVCM_EV_PROPERTY_VCM_EV          "vcm_ev"
#define PCVCM_EV_PROPERTY_LAST_VALUE      "last_value"

struct pcvcm_node {
    struct pctree_node tree_node;
    enum pcvcm_node_type type;
    uint32_t extra;
    uintptr_t attach;
    bool is_closed;
    union {
        bool        b;
        double      d;
        int64_t     i64;
        uint64_t    u64;
        long double ld;
        uintptr_t   sz_ptr[2];
    };
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pcvcm_node *pcvcm_node_new_undefined();

struct pcvcm_node *pcvcm_node_new_object(size_t nr_nodes,
        struct pcvcm_node **nodes);

struct pcvcm_node *pcvcm_node_new_array(size_t nr_nodes,
        struct pcvcm_node **nodes);

struct pcvcm_node *pcvcm_node_new_string(const char *str_utf8);

struct pcvcm_node *pcvcm_node_new_null();

struct pcvcm_node *pcvcm_node_new_boolean(bool b);

struct pcvcm_node *pcvcm_node_new_number(double d);

struct pcvcm_node *pcvcm_node_new_longint(int64_t i64);

struct pcvcm_node *pcvcm_node_new_ulongint(uint64_t u64);

struct pcvcm_node *pcvcm_node_new_longdouble(long double ld);

struct pcvcm_node *pcvcm_node_new_byte_sequence(const void *bytes,
        size_t nr_bytes);

struct pcvcm_node *pcvcm_node_new_byte_sequence_from_bx(const void *bytes,
        size_t nr_bytes);

struct pcvcm_node *pcvcm_node_new_byte_sequence_from_bb(const void *bytes,
        size_t nr_bytes);

struct pcvcm_node *pcvcm_node_new_byte_sequence_from_b64 (const void *bytes,
        size_t nr_bytes);

struct pcvcm_node *pcvcm_node_new_concat_string(size_t nr_nodes,
        struct pcvcm_node *nodes);

struct pcvcm_node *pcvcm_node_new_get_variable(struct pcvcm_node *node);

struct pcvcm_node *pcvcm_node_new_get_element(struct pcvcm_node *variable,
        struct pcvcm_node *identifier);

struct pcvcm_node *pcvcm_node_new_call_getter(struct pcvcm_node *variable,
        size_t nr_params, struct pcvcm_node *params);

struct pcvcm_node *pcvcm_node_new_call_setter(struct pcvcm_node *variable,
        size_t nr_params, struct pcvcm_node *params);

struct pcvcm_node *pcvcm_node_new_cjsonee();

struct pcvcm_node *pcvcm_node_new_cjsonee_op_and();

struct pcvcm_node *pcvcm_node_new_cjsonee_op_or();

struct pcvcm_node *pcvcm_node_new_cjsonee_op_semicolon();

static inline bool
pcvcm_node_is_closed(struct pcvcm_node *node) {
    return node && node->is_closed;
}

static inline void
pcvcm_node_set_closed(struct pcvcm_node *node, bool closed)
{
    if (node) {
        node->is_closed = closed;
    }
}

char *pcvcm_node_to_string(struct pcvcm_node *node, size_t *nr_bytes);

char *pcvcm_node_serialize(struct pcvcm_node *node, size_t *nr_bytes);

/*
 * Removes root and its children from the tree, freeing any memory allocated.
 */
void pcvcm_node_destroy(struct pcvcm_node *root);

struct pcvcm_stack;
struct pcvcm_stack *pcvcm_stack_new();

bool pcvcm_stack_is_empty(struct pcvcm_stack *stack);

void pcvcm_stack_push(struct pcvcm_stack *stack, struct pcvcm_node *e);

struct pcvcm_node *pcvcm_stack_pop(struct pcvcm_stack *stack);

struct pcvcm_node *pcvcm_stack_bottommost(struct pcvcm_stack *stack);

void pcvcm_stack_destroy(struct pcvcm_stack *stack);

typedef purc_variant_t(*cb_find_var) (void *ctxt, const char *name);

purc_variant_t pcvcm_eval_ex(struct pcvcm_node *tree, cb_find_var find_var,
        void *ctxt, bool silently);

struct pcintr_stack;
purc_variant_t pcvcm_eval(struct pcvcm_node *tree, struct pcintr_stack *stack,
        bool silently);

purc_variant_t
pcvcm_to_expression_variable(struct pcvcm_node *vcm, bool release_vcm);

#define PRINT_VCM_NODE(_node) do {                                        \
    size_t len;                                                           \
    char *s = pcvcm_node_to_string(_node, &len);                          \
    PC_DEBUG("%s[%d]:%s(): %s=%.*s\n",                                    \
            pcutils_basename((char*)__FILE__), __LINE__, __func__,        \
            #_node, (int)len, s);                                         \
    free(s);                                                              \
} while (0)

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_VCM_H */

