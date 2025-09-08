/*
 * @file vcm.h
 * @author XueShuming
 * @date 2021/07/28
 * @brief The interfaces for vcm.
 *
 * Copyright (C) 2021, 2025 FMSoft <https://www.fmsoft.cn>
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
#define EXTRA_ASSIGN_FLAG                           0x0004
#define EXTRA_STATIC_FLAG                           0x0010

#define PCVCM_EV_DEFAULT_METHOD_NAME                "eval"
#define PCVCM_EV_CONST_SUFFIX                       "_const"

#define PCVCM_EV_PROPERTY_METHOD_NAME               "method_name"
#define PCVCM_EV_PROPERTY_CONST_METHOD_NAME         "const_method_name"
#define PCVCM_EV_PROPERTY_EVAL                      "eval"
#define PCVCM_EV_PROPERTY_EVAL_CONST                "eval_const"
#define PCVCM_EV_PROPERTY_VCM_EV                    "vcm_ev"
#define PCVCM_EV_PROPERTY_LAST_VALUE                "last_value"
#define PCVCM_EV_PROPERTY_CONSTANTLY                "constantly"


enum pcvcm_node_type {
    PCVCM_NODE_TYPE_FIRST = 0,

#define PCVCM_NODE_TYPE_NAME_UNDEFINED              "undefined"
    PCVCM_NODE_TYPE_UNDEFINED = PCVCM_NODE_TYPE_FIRST,
#define PCVCM_NODE_TYPE_NAME_OBJECT                 "object"
    PCVCM_NODE_TYPE_OBJECT,
#define PCVCM_NODE_TYPE_NAME_ARRAY                  "array"
    PCVCM_NODE_TYPE_ARRAY,
#define PCVCM_NODE_TYPE_NAME_TUPLE                  "tuple"
    PCVCM_NODE_TYPE_TUPLE,
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
#define PCVCM_NODE_TYPE_NAME_BIG_INT                "big_int"
    PCVCM_NODE_TYPE_BIG_INT,
#define PCVCM_NODE_TYPE_NAME_LONG_DOUBLE            "long_double"
    PCVCM_NODE_TYPE_LONG_DOUBLE,
#define PCVCM_NODE_TYPE_NAME_BYTE_SEQUENCE          "byte_sequence"
    PCVCM_NODE_TYPE_BYTE_SEQUENCE,
#define PCVCM_NODE_TYPE_NAME_CONCAT_STRING          "concatString"
    PCVCM_NODE_TYPE_FUNC_CONCAT_STRING,
#define PCVCM_NODE_TYPE_NAME_GET_VARIABLE           "getVariable"
    PCVCM_NODE_TYPE_FUNC_GET_VARIABLE,
#define PCVCM_NODE_TYPE_NAME_GET_MEMBER             "getMember"
    PCVCM_NODE_TYPE_FUNC_GET_MEMBER,
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
#define PCVCM_NODE_TYPE_NAME_CONSTANT               "constant"
    PCVCM_NODE_TYPE_CONSTANT,

    PCVCM_NODE_TYPE_OP_FIRST,

    // Arithmetic operators
#define PCVCM_NODE_TYPE_NAME_OP_ADD                 "op_add"
    PCVCM_NODE_TYPE_OP_ADD = PCVCM_NODE_TYPE_OP_FIRST, // +
#define PCVCM_NODE_TYPE_NAME_OP_SUB                 "op_sub"
    PCVCM_NODE_TYPE_OP_SUB,                     // -
#define PCVCM_NODE_TYPE_NAME_OP_MULTIPLY            "op_multiply"
    PCVCM_NODE_TYPE_OP_MULTIPLY,                // *
#define PCVCM_NODE_TYPE_NAME_OP_DIVIDE              "op_divide"
    PCVCM_NODE_TYPE_OP_DIVIDE,                  // /
#define PCVCM_NODE_TYPE_NAME_OP_MODULO              "op_modulo"
    PCVCM_NODE_TYPE_OP_MODULO,                  // %
#define PCVCM_NODE_TYPE_NAME_OP_FLOOR_DIVIDE        "op_floor_divide"
    PCVCM_NODE_TYPE_OP_FLOOR_DIVIDE,            // //
#define PCVCM_NODE_TYPE_NAME_OP_POWER               "op_power"
    PCVCM_NODE_TYPE_OP_POWER,                   // **

    // Unary operators
#define PCVCM_NODE_TYPE_NAME_OP_UNARY_PLUS          "op_unary_plus"
    PCVCM_NODE_TYPE_OP_UNARY_PLUS,              // +x
#define PCVCM_NODE_TYPE_NAME_OP_UNARY_MINUS         "op_unary_minus"
    PCVCM_NODE_TYPE_OP_UNARY_MINUS,             // -x

    // Comparison operators
#define PCVCM_NODE_TYPE_NAME_OP_EQUAL               "op_equal"
    PCVCM_NODE_TYPE_OP_EQUAL,                   // ==
#define PCVCM_NODE_TYPE_NAME_OP_NOT_EQUAL           "op_not_equal"
    PCVCM_NODE_TYPE_OP_NOT_EQUAL,               // !=
#define PCVCM_NODE_TYPE_NAME_OP_GREATER             "op_greater"
    PCVCM_NODE_TYPE_OP_GREATER,                 // >
#define PCVCM_NODE_TYPE_NAME_OP_GREATER_EQUAL       "op_greater_equal"
    PCVCM_NODE_TYPE_OP_GREATER_EQUAL,           // >=
#define PCVCM_NODE_TYPE_NAME_OP_LESS                "op_less"
    PCVCM_NODE_TYPE_OP_LESS,                    // <
#define PCVCM_NODE_TYPE_NAME_OP_LESS_EQUAL          "op_less_equal"
    PCVCM_NODE_TYPE_OP_LESS_EQUAL,              // <=

    // Logical operators
#define PCVCM_NODE_TYPE_NAME_OP_LOGICAL_NOT         "op_logical_not"
    PCVCM_NODE_TYPE_OP_LOGICAL_NOT,             // not
#define PCVCM_NODE_TYPE_NAME_OP_LOGICAL_AND         "op_logical_and"
    PCVCM_NODE_TYPE_OP_LOGICAL_AND,             // and
#define PCVCM_NODE_TYPE_NAME_OP_LOGICAL_OR          "op_logical_or"
    PCVCM_NODE_TYPE_OP_LOGICAL_OR,              // or

    // Membership operators
#define PCVCM_NODE_TYPE_NAME_OP_IN                  "op_in"
    PCVCM_NODE_TYPE_OP_IN,                      // in
#define PCVCM_NODE_TYPE_NAME_OP_NOT_IN              "op_not_in"
    PCVCM_NODE_TYPE_OP_NOT_IN,                  // not in

    // Bitwise operators
#define PCVCM_NODE_TYPE_NAME_OP_BITWISE_AND         "op_bitwise_and"
    PCVCM_NODE_TYPE_OP_BITWISE_AND,             // &
#define PCVCM_NODE_TYPE_NAME_OP_BITWISE_OR          "op_bitwise_or"
    PCVCM_NODE_TYPE_OP_BITWISE_OR,              // |
#define PCVCM_NODE_TYPE_NAME_OP_BITWISE_INVERT      "op_bitwise_invert"
    PCVCM_NODE_TYPE_OP_BITWISE_INVERT,          // ~x
#define PCVCM_NODE_TYPE_NAME_OP_BITWISE_XOR         "op_bitwise_xor"
    PCVCM_NODE_TYPE_OP_BITWISE_XOR,             // ^
#define PCVCM_NODE_TYPE_NAME_OP_LEFT_SHIFT          "op_left_shift"
    PCVCM_NODE_TYPE_OP_LEFT_SHIFT,              // <<
#define PCVCM_NODE_TYPE_NAME_OP_RIGHT_SHIFT         "op_right_shift"
    PCVCM_NODE_TYPE_OP_RIGHT_SHIFT,             // >>

    // Conditional operator
#define PCVCM_NODE_TYPE_NAME_OP_CONDITIONAL         "op_conditional"
    PCVCM_NODE_TYPE_OP_CONDITIONAL,             // ? :

    // Comma operator
#define PCVCM_NODE_TYPE_NAME_OP_COMMA               "op_comma"
    PCVCM_NODE_TYPE_OP_COMMA,                   // ,

    // Assignment operators
#define PCVCM_NODE_TYPE_NAME_OP_ASSIGN              "op_assign"
    PCVCM_NODE_TYPE_OP_ASSIGN,                  // =
#define PCVCM_NODE_TYPE_NAME_OP_PLUS_ASSIGN         "op_plus_assign"
    PCVCM_NODE_TYPE_OP_PLUS_ASSIGN,             // +=
#define PCVCM_NODE_TYPE_NAME_OP_MINUS_ASSIGN        "op_minus_assign"
    PCVCM_NODE_TYPE_OP_MINUS_ASSIGN,            // -=
#define PCVCM_NODE_TYPE_NAME_OP_MULTIPLY_ASSIGN     "op_multiply_assign"
    PCVCM_NODE_TYPE_OP_MULTIPLY_ASSIGN,         // *=
#define PCVCM_NODE_TYPE_NAME_OP_DIVIDE_ASSIGN       "op_divide_assign"
    PCVCM_NODE_TYPE_OP_DIVIDE_ASSIGN,           // /=
#define PCVCM_NODE_TYPE_NAME_OP_MODULO_ASSIGN       "op_modulo_assign"
    PCVCM_NODE_TYPE_OP_MODULO_ASSIGN,           // %=
#define PCVCM_NODE_TYPE_NAME_OP_FLOOR_DIV_ASSIGN    "op_floor_div_assign"
    PCVCM_NODE_TYPE_OP_FLOOR_DIV_ASSIGN,        // //=
#define PCVCM_NODE_TYPE_NAME_OP_POWER_ASSIGN        "op_power_assign"
    PCVCM_NODE_TYPE_OP_POWER_ASSIGN,            // **=
#define PCVCM_NODE_TYPE_NAME_OP_BITWISE_AND_ASSIGN  "op_bitwise_and_assign"
    PCVCM_NODE_TYPE_OP_BITWISE_AND_ASSIGN,      // &=
#define PCVCM_NODE_TYPE_NAME_OP_BITWISE_OR_ASSIGN   "op_bitwise_or_assign"
    PCVCM_NODE_TYPE_OP_BITWISE_OR_ASSIGN,       // |=
#define PCVCM_NODE_TYPE_NAME_OP_BITWISE_INVERT_ASSIGN   "op_bitwise_invert_assign"
    PCVCM_NODE_TYPE_OP_BITWISE_INVERT_ASSIGN,       // ~=
#define PCVCM_NODE_TYPE_NAME_OP_BITWISE_XOR_ASSIGN  "op_bitwise_xor_assign"
    PCVCM_NODE_TYPE_OP_BITWISE_XOR_ASSIGN,      // ^=
#define PCVCM_NODE_TYPE_NAME_OP_LEFT_SHIFT_ASSIGN   "op_left_shift_assign"
    PCVCM_NODE_TYPE_OP_LEFT_SHIFT_ASSIGN,       // <<=
#define PCVCM_NODE_TYPE_NAME_OP_RIGHT_SHIFT_ASSIGN  "op_right_shift_assign"
    PCVCM_NODE_TYPE_OP_RIGHT_SHIFT_ASSIGN,      // >>=
#define PCVCM_NODE_TYPE_NAME_OP_INCREMENT           "op_increment"
    PCVCM_NODE_TYPE_OP_INCREMENT,               // ++
#define PCVCM_NODE_TYPE_NAME_OP_DECREMENT           "op_decrement"
    PCVCM_NODE_TYPE_OP_DECREMENT,               // --

    /* ( )*/
#define PCVCM_NODE_TYPE_NAME_OP_LP              "op_lp"
    PCVCM_NODE_TYPE_OP_LP,                      // (
#define PCVCM_NODE_TYPE_NAME_OP_RP              "op_rp"
    PCVCM_NODE_TYPE_OP_RP,                      // )

    PCVCM_NODE_TYPE_OP_LAST = PCVCM_NODE_TYPE_OP_RP,

    // Operator expression container
#define PCVCM_NODE_TYPE_NAME_OPERATOR_EXPRESSION    "operator_expression"
    PCVCM_NODE_TYPE_OPERATOR_EXPRESSION,        // (...)

#define PCVCM_NODE_TYPE_NAME_SUB_EXPR               "sub_expr"
    PCVCM_NODE_TYPE_SUB_EXPR,        // (...)

    PCVCM_NODE_TYPE_LAST = PCVCM_NODE_TYPE_SUB_EXPR,
};

#define PCVCM_NODE_TYPE_NR \
    (PCVCM_NODE_TYPE_LAST - PCVCM_NODE_TYPE_FIRST + 1)

enum pcvcm_node_quoted_type {
    PCVCM_NODE_QUOTED_TYPE_NONE,
    PCVCM_NODE_QUOTED_TYPE_SINGLE,
    PCVCM_NODE_QUOTED_TYPE_DOUBLE,
    PCVCM_NODE_QUOTED_TYPE_BACKQUOTE,
};

// Forward declaration
struct pcvcm_node;

// Function pointer type for cleaning up private data
typedef void (*pcvcm_cleanup_priv_data_fn)(struct pcvcm_node *node, void *private_data);

struct pcvcm_node {
    struct pctree_node          tree_node;
    enum pcvcm_node_type        type;
    enum pcvcm_node_quoted_type quoted_type;
    struct tkz_ucs             *ucs;
    uintptr_t                   attach;
    uint32_t                    extra;
    int32_t                     position;
    int32_t                     idx;
    int32_t                     nr_nodes; /* nr_nodes of the tree */
    int                         int_base;
    bool                        is_closed;
    /* Private data for different node types */
    void                       *priv_data;
    /*  Function to cleanup private data */
    pcvcm_cleanup_priv_data_fn  cleanup_priv_data_fn;
    union {
        bool                    b;
        double                  d;
        int64_t                 i64;
        uint64_t                u64;
        long double             ld;
        uintptr_t               sz_ptr[2];
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

struct pcvcm_node *pcvcm_node_new_bigint(const char *str_utf8, int base);

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

struct pcvcm_node *pcvcm_node_new_tuple(size_t nr_nodes,
        struct pcvcm_node **nodes);

struct pcvcm_node *pcvcm_node_new_constant(size_t nr_nodes,
        struct pcvcm_node **nodes);

// Unary operators (1 operand)
struct pcvcm_node *pcvcm_node_new_op_unary_plus(void);
struct pcvcm_node *pcvcm_node_new_op_unary_minus(void);
struct pcvcm_node *pcvcm_node_new_op_logical_not(void);
struct pcvcm_node *pcvcm_node_new_op_increment(void);
struct pcvcm_node *pcvcm_node_new_op_decrement(void);

// Binary operators (2 operands)
// Arithmetic operators
struct pcvcm_node *pcvcm_node_new_op_add(void);
struct pcvcm_node *pcvcm_node_new_op_sub(void);
struct pcvcm_node *pcvcm_node_new_op_mul(void);
struct pcvcm_node *pcvcm_node_new_op_true_div(void);
struct pcvcm_node *pcvcm_node_new_op_modulo(void);
struct pcvcm_node *pcvcm_node_new_op_floor_div(void);
struct pcvcm_node *pcvcm_node_new_op_power(void);

// Comparison operators
struct pcvcm_node *pcvcm_node_new_op_equal(void);
struct pcvcm_node *pcvcm_node_new_op_not_equal(void);
struct pcvcm_node *pcvcm_node_new_op_greater(void);
struct pcvcm_node *pcvcm_node_new_op_greater_equal(void);
struct pcvcm_node *pcvcm_node_new_op_less(void);
struct pcvcm_node *pcvcm_node_new_op_less_equal(void);

// Logical operators
struct pcvcm_node *pcvcm_node_new_op_logical_and(void);
struct pcvcm_node *pcvcm_node_new_op_logical_or(void);

// Membership operators
struct pcvcm_node *pcvcm_node_new_op_in(void);
struct pcvcm_node *pcvcm_node_new_op_not_in(void);

// Bitwise operators
struct pcvcm_node *pcvcm_node_new_op_bitwise_and(void);
struct pcvcm_node *pcvcm_node_new_op_bitwise_or(void);
struct pcvcm_node *pcvcm_node_new_op_bitwise_invert(void);
struct pcvcm_node *pcvcm_node_new_op_bitwise_xor(void);
struct pcvcm_node *pcvcm_node_new_op_left_shift(void);
struct pcvcm_node *pcvcm_node_new_op_right_shift(void);

// Assignment operators
struct pcvcm_node *pcvcm_node_new_op_assign(void);
struct pcvcm_node *pcvcm_node_new_op_plus_assign(void);
struct pcvcm_node *pcvcm_node_new_op_minus_assign(void);
struct pcvcm_node *pcvcm_node_new_op_multiply_assign(void);
struct pcvcm_node *pcvcm_node_new_op_divide_assign(void);
struct pcvcm_node *pcvcm_node_new_op_modulo_assign(void);
struct pcvcm_node *pcvcm_node_new_op_floor_div_assign(void);
struct pcvcm_node *pcvcm_node_new_op_power_assign(void);
struct pcvcm_node *pcvcm_node_new_op_bitwise_and_assign(void);
struct pcvcm_node *pcvcm_node_new_op_bitwise_or_assign(void);
struct pcvcm_node *pcvcm_node_new_op_bitwise_invert_assign(void);
struct pcvcm_node *pcvcm_node_new_op_bitwise_xor_assign(void);
struct pcvcm_node *pcvcm_node_new_op_left_shift_assign(void);
struct pcvcm_node *pcvcm_node_new_op_right_shift_assign(void);

struct pcvcm_node *pcvcm_node_new_op_lp(void);
struct pcvcm_node *pcvcm_node_new_op_rp(void);

// Comma operator
struct pcvcm_node *pcvcm_node_new_op_comma(void);

// Ternary operators (3 operands)
struct pcvcm_node *pcvcm_node_new_op_conditional(void);

// Special node types (variable operands)
struct pcvcm_node *pcvcm_node_new_operator_expression(size_t nr_nodes, struct pcvcm_node **nodes);

// sub expr
struct pcvcm_node *pcvcm_node_new_subexpr(void);

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

static inline struct pcvcm_node *
pcvcm_node_parent(struct pcvcm_node *node)
{
    if (node) {
        return (struct pcvcm_node *)pctree_node_parent(&node->tree_node);
    }
    return NULL;
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

static inline struct pcvcm_node *
pcvcm_node_next_child(struct pcvcm_node *node)
{
    if (node) {
        return (struct pcvcm_node *)pctree_node_next(&node->tree_node);
    }
    return NULL;
}

static inline struct pcvcm_node *
pcvcm_node_prev_child(struct pcvcm_node *node)
{
    if (node) {
        return (struct pcvcm_node *)pctree_node_prev(&node->tree_node);
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

static inline bool
pcvcm_node_append_child(struct pcvcm_node *parent, struct pcvcm_node *child)
{
    if (!child) {
        return false;
    }
    return pctree_node_append_child(&parent->tree_node, &child->tree_node);
}

char *pcvcm_node_to_string_ex(struct pcvcm_node *node, size_t *nr_bytes,
        struct pcvcm_node *err_node, char **err_msg, size_t *nr_err_msg);

static inline char *pcvcm_node_to_string(struct pcvcm_node *node,
        size_t *nr_bytes)
{
    return pcvcm_node_to_string_ex(node, nr_bytes, NULL, NULL, NULL);
}


char *pcvcm_node_serialize_ex(struct pcvcm_node *node, size_t *nr_bytes,
        struct pcvcm_node *err_node, char **err_msg, size_t *nr_err_msg);

static inline char *pcvcm_node_serialize(struct pcvcm_node *node,
        size_t *nr_bytes)
{
    return pcvcm_node_serialize_ex(node, nr_bytes, NULL, NULL, NULL);
}

int pcvcm_node_min_position(struct pcvcm_node *node);

/*
 * Removes root and its children from the tree, freeing any memory allocated.
 */
void pcvcm_node_destroy(struct pcvcm_node *root);

// Functions to set and get private data
static inline void pcvcm_node_set_private_data(struct pcvcm_node *node,
     void *private_data, pcvcm_cleanup_priv_data_fn cleanup_fn)
{
    if (!node) {
        return;
    }

    // Clean up existing private data if cleanup function exists
    if (node->cleanup_priv_data_fn && node->priv_data) {
        node->cleanup_priv_data_fn(node, node->priv_data);
    }

    node->priv_data = private_data;
    node->cleanup_priv_data_fn = cleanup_fn;
}

typedef purc_variant_t (*find_var_fn) (void *ctxt, const char *name);
typedef int (*bind_var_fn)(void *ctxt, const char *name, purc_variant_t val,
                           bool temporarily);

struct pcvcm_eval_ctxt;
purc_variant_t pcvcm_eval_ex(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt **ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bind_var_fn bind_var, void *bind_var_ctxt,
        bool silently);

purc_variant_t pcvcm_eval_again_ex(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bind_var_fn bind_var, void *bind_var_ctxt,
        bool silently, bool timeout);

struct pcintr_stack;
purc_variant_t pcvcm_eval(struct pcvcm_node *tree, struct pcintr_stack *stack,
        bool silently);

purc_variant_t pcvcm_eval_again(struct pcvcm_node *tree,
        struct pcintr_stack *stack, bool silently, bool timeout);

/* substitue expression  */
purc_variant_t pcvcm_eval_sub_expr(struct pcvcm_node *tree,
        struct pcintr_stack *stack, purc_variant_t args, bool silently);

void
pcvcm_eval_ctxt_destroy(struct pcvcm_eval_ctxt *ctxt);

int
pcvcm_eval_ctxt_error_code(struct pcvcm_eval_ctxt *ctxt);

int
pcvcm_dump_stack(struct pcvcm_eval_ctxt *ctxt, purc_rwstream_t rws,
        int indent, bool ignore_prefix, bool print_exception);

purc_variant_t
pcvcm_to_expression_variable(struct pcvcm_node *vcm, const char *method_name,
        bool constantly, bool release_vcm);

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

