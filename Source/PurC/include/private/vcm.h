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

#define EXTRA_NULL                                  0x0000
#define EXTRA_PROTECT_FLAG                          0x0001
#define EXTRA_SUGAR_FLAG                            0x0002

#define PCVCM_EV_PROPERTY_EVAL                      "eval"
#define PCVCM_EV_PROPERTY_EVAL_CONST                "eval_const"
#define PCVCM_EV_PROPERTY_VCM_EV                    "vcm_ev"
#define PCVCM_EV_PROPERTY_LAST_VALUE                "last_value"


enum pcvcm_node_type {
    PCVCM_NODE_TYPE_FIRST = 0,

#define PCVCM_NODE_TYPE_NAME_UNDEFINED              "undefined"
    PCVCM_NODE_TYPE_UNDEFINED = PCVCM_NODE_TYPE_FIRST,
#define PCVCM_NODE_TYPE_NAME_OBJECT                 "object"
    PCVCM_NODE_TYPE_OBJECT,
#define PCVCM_NODE_TYPE_NAME_ARRAY                  "array"
    PCVCM_NODE_TYPE_ARRAY,
#define PCVCM_NODE_TYPE_NAME_STRING                 "string"
    PCVCM_NODE_TYPE_STRING,
#define PCVCM_NODE_TYPE_NAME_NULL                   "null"
    PCVCM_NODE_TYPE_NULL,
#define PCVCM_NODE_TYPE_NAME_BOOLEAN                "boolean"
    PCVCM_NODE_TYPE_BOOLEAN,
#define PCVCM_NODE_TYPE_NAME_NUMBER                 "number"
    PCVCM_NODE_TYPE_NUMBER,
#define PCVCM_NODE_TYPE_NAME_LONG_INT               "long_int"
    PCVCM_NODE_TYPE_LONG_INT,
#define PCVCM_NODE_TYPE_NAME_ULONG_INT              "ulong_int"
    PCVCM_NODE_TYPE_ULONG_INT,
#define PCVCM_NODE_TYPE_NAME_LONG_DOUBLE            "long_double"
    PCVCM_NODE_TYPE_LONG_DOUBLE,
#define PCVCM_NODE_TYPE_NAME_BYTE_SEQUENCE          "byte_sequence"
    PCVCM_NODE_TYPE_BYTE_SEQUENCE,
#define PCVCM_NODE_TYPE_NAME_CONCAT_STRING          "concatString"
    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING,
#define PCVCM_NODE_TYPE_NAME_GET_VARIABLE           "getVariable"
    PCVCM_NODE_TYPE_FUNC_GET_VARIABLE,
#define PCVCM_NODE_TYPE_NAME_GET_ELEMENT            "getElement"
    PCVCM_NODE_TYPE_FUNC_GET_ELEMENT,
#define PCVCM_NODE_TYPE_NAME_CALL_GETTER            "callGetter"
    PCVCM_NODE_TYPE_FUNC_CALL_GETTER,
#define PCVCM_NODE_TYPE_NAME_CALL_SETTER            "callSetter"
    PCVCM_NODE_TYPE_FUNC_CALL_SETTER,
#define PCVCM_NODE_TYPE_NAME_CJSONEE                "cjsonee"
    PCVCM_NODE_TYPE_CJSONEE,
#define PCVCM_NODE_TYPE_NAME_CJSONEE_OP_AND         "cjsonee_op_and"
    PCVCM_NODE_TYPE_CJSONEE_OP_AND,
#define PCVCM_NODE_TYPE_NAME_CJSONEE_OP_OR          "cjsonee_op_or"
    PCVCM_NODE_TYPE_CJSONEE_OP_OR,
#define PCVCM_NODE_TYPE_NAME_CJSONEE_OP_SEMICOLON   "cjsonee_op_semicolon"
    PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON,

    PCVCM_NODE_TYPE_LAST = PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON,
};

#define PCVCM_NODE_TYPE_NR \
    (PCVCM_NODE_TYPE_LAST - PCVCM_NODE_TYPE_FIRST + 1)

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

static inline enum pcvcm_node_type
pcvcm_node_get_type(struct pcvcm_node *node) {
    return node->type;
}
const char *
pcvcm_node_typename(enum pcvcm_node_type type);

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

static inline size_t
pcvcm_node_children_count(struct pcvcm_node *node)
{
    if (node) {
        return pctree_node_children_number(&node->tree_node);
    }
    return 0;
}

static inline struct pcvcm_node *
pcvcm_node_first_child(struct pcvcm_node *node)
{
    if (node) {
        return (struct pcvcm_node *)pctree_node_child(&node->tree_node);
    }
    return NULL;
}

static inline struct pcvcm_node *
pcvcm_node_last_child(struct pcvcm_node *node)
{
    if (node) {
        return (struct pcvcm_node *)pctree_node_last_child(&node->tree_node);
    }
    return NULL;
}

static inline void
pcvcm_node_remove_child(struct pcvcm_node *parent, struct pcvcm_node *child)
{
    UNUSED_PARAM(parent);
    if (child) {
        pctree_node_remove(&child->tree_node);
    }
}

char *pcvcm_node_to_string(struct pcvcm_node *node, size_t *nr_bytes);

char *pcvcm_node_serialize(struct pcvcm_node *node, size_t *nr_bytes);

/*
 * Removes root and its children from the tree, freeing any memory allocated.
 */
void pcvcm_node_destroy(struct pcvcm_node *root);


typedef purc_variant_t(*find_var_fn) (void *ctxt, const char *name);

struct pcvcm_eval_ctxt;
purc_variant_t pcvcm_eval_ex(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt **ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently);

purc_variant_t pcvcm_eval_again_ex(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently, bool timeout);

struct pcintr_stack;
purc_variant_t pcvcm_eval(struct pcvcm_node *tree, struct pcintr_stack *stack,
        bool silently);

purc_variant_t pcvcm_eval_again(struct pcvcm_node *tree,
        struct pcintr_stack *stack, bool silently, bool timeout);

void
pcvcm_eval_ctxt_destroy(struct pcvcm_eval_ctxt *ctxt);

int
pcvcm_eval_ctxt_error_code(struct pcvcm_eval_ctxt *ctxt);

int
pcvcm_dump_stack(struct pcvcm_eval_ctxt *ctxt, purc_rwstream_t rws,
        int indent, bool ignore_prefix);

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

