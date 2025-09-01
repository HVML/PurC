/*
 * @file operator_expression.c
 * @author XueShuming
 * @date 2024/01/01
 * @brief The impl of ops for operator expression vcm node.
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
#include "private/stack.h"
#include "private/interpreter.h"
#include "private/vcm.h"
#include "private/utils.h"
#include <math.h>

#include "../eval.h"
#include "../ops.h"
#include "purc-variant.h"

// Operator precedence definitions
#define PRECEDENCE_PARENTHESES      17  // () [] {}
#define PRECEDENCE_POSTFIX         16  // x++ x--
#define PRECEDENCE_UNARY           15  // + - ~
#define PRECEDENCE_POWER           14  // **
#define PRECEDENCE_MULTIPLICATIVE  13  // * / % //
#define PRECEDENCE_ADDITIVE        12  // + -
#define PRECEDENCE_SHIFT           11  // << >>
#define PRECEDENCE_BITWISE_AND     10  // &
#define PRECEDENCE_BITWISE_OR       9  // |
#define PRECEDENCE_BITWISE_XOR      8  // ^
#define PRECEDENCE_COMPARISON       7  // < <= > >= == !=
#define PRECEDENCE_MEMBERSHIP      PRECEDENCE_COMPARISON  // in not_in
#define PRECEDENCE_LOGICAL_NOT      6  // not
#define PRECEDENCE_LOGICAL_AND      5  // and
#define PRECEDENCE_LOGICAL_OR       4  // or
#define PRECEDENCE_CONDITIONAL      3  // ? :
#define PRECEDENCE_ASSIGNMENT       2  // = += -= etc.
#define PRECEDENCE_COMMA            1  // ,

static int
after_pushed(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame)
{
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(frame);
    return 0;
}

typedef enum {
    ASSOC_LEFT,
    ASSOC_RIGHT
} associativity_t;

typedef struct {
    int precedence;
    associativity_t associativity;
} operator_info_t;



static operator_info_t get_operator_info(enum pcvcm_node_type type) {
    switch (type) {
        // Parentheses (highest precedence)
        case PCVCM_NODE_TYPE_OP_LP:
        case PCVCM_NODE_TYPE_OP_RP:
            return (operator_info_t){PRECEDENCE_PARENTHESES, ASSOC_LEFT};

        // Power (exponentiation)
        case PCVCM_NODE_TYPE_OP_POWER:
            return (operator_info_t){PRECEDENCE_POWER, ASSOC_RIGHT};

        // Postfix operators (x++, x--)
        case PCVCM_NODE_TYPE_OP_INCREMENT:
        case PCVCM_NODE_TYPE_OP_DECREMENT:
            return (operator_info_t){PRECEDENCE_POSTFIX, ASSOC_LEFT};

        // Unary operators (+x, -x, ~x)
        case PCVCM_NODE_TYPE_OP_UNARY_PLUS:
        case PCVCM_NODE_TYPE_OP_UNARY_MINUS:
        case PCVCM_NODE_TYPE_OP_BITWISE_INVERT:
            return (operator_info_t){PRECEDENCE_UNARY, ASSOC_RIGHT};

        // Multiplicative (*, /, //, %)
        case PCVCM_NODE_TYPE_OP_MULTIPLY:
        case PCVCM_NODE_TYPE_OP_DIVIDE:
        case PCVCM_NODE_TYPE_OP_FLOOR_DIVIDE:
        case PCVCM_NODE_TYPE_OP_MODULO:
            return (operator_info_t){PRECEDENCE_MULTIPLICATIVE, ASSOC_LEFT};

        // Additive (+, -)
        case PCVCM_NODE_TYPE_OP_ADD:
        case PCVCM_NODE_TYPE_OP_SUB:
            return (operator_info_t){PRECEDENCE_ADDITIVE, ASSOC_LEFT};

        // Shift (<<, >>)
        case PCVCM_NODE_TYPE_OP_LEFT_SHIFT:
        case PCVCM_NODE_TYPE_OP_RIGHT_SHIFT:
            return (operator_info_t){PRECEDENCE_SHIFT, ASSOC_LEFT};

        // Bitwise AND (&)
        case PCVCM_NODE_TYPE_OP_BITWISE_AND:
            return (operator_info_t){PRECEDENCE_BITWISE_AND, ASSOC_LEFT};

        // Bitwise XOR (^)
        case PCVCM_NODE_TYPE_OP_BITWISE_XOR:
            return (operator_info_t){PRECEDENCE_BITWISE_XOR, ASSOC_LEFT};

        // Bitwise OR (|)
        case PCVCM_NODE_TYPE_OP_BITWISE_OR:
            return (operator_info_t){PRECEDENCE_BITWISE_OR, ASSOC_LEFT};

        // Comparison and membership (==, !=, >, >=, <, <=, in, not in)
        case PCVCM_NODE_TYPE_OP_EQUAL:
        case PCVCM_NODE_TYPE_OP_NOT_EQUAL:
        case PCVCM_NODE_TYPE_OP_GREATER:
        case PCVCM_NODE_TYPE_OP_GREATER_EQUAL:
        case PCVCM_NODE_TYPE_OP_LESS:
        case PCVCM_NODE_TYPE_OP_LESS_EQUAL:
            return (operator_info_t){PRECEDENCE_COMPARISON, ASSOC_LEFT};
        case PCVCM_NODE_TYPE_OP_IN:
        case PCVCM_NODE_TYPE_OP_NOT_IN:
            return (operator_info_t){PRECEDENCE_MEMBERSHIP, ASSOC_LEFT};

        // Logical NOT (not) - higher precedence than logical AND
        case PCVCM_NODE_TYPE_OP_LOGICAL_NOT:
            return (operator_info_t){PRECEDENCE_LOGICAL_NOT, ASSOC_RIGHT};

        // Logical AND (and)
        case PCVCM_NODE_TYPE_OP_LOGICAL_AND:
            return (operator_info_t){PRECEDENCE_LOGICAL_AND, ASSOC_LEFT};

        // Logical OR (or)
        case PCVCM_NODE_TYPE_OP_LOGICAL_OR:
            return (operator_info_t){PRECEDENCE_LOGICAL_OR, ASSOC_LEFT};

        // Ternary conditional (?:)
        case PCVCM_NODE_TYPE_OP_CONDITIONAL:
            return (operator_info_t){PRECEDENCE_CONDITIONAL, ASSOC_RIGHT};

        // Assignment operators (=, +=, -=, etc.)
        case PCVCM_NODE_TYPE_OP_ASSIGN:
        case PCVCM_NODE_TYPE_OP_PLUS_ASSIGN:
        case PCVCM_NODE_TYPE_OP_MINUS_ASSIGN:
        case PCVCM_NODE_TYPE_OP_MULTIPLY_ASSIGN:
        case PCVCM_NODE_TYPE_OP_DIVIDE_ASSIGN:
        case PCVCM_NODE_TYPE_OP_MODULO_ASSIGN:
        case PCVCM_NODE_TYPE_OP_FLOOR_DIV_ASSIGN:
        case PCVCM_NODE_TYPE_OP_POWER_ASSIGN:
        case PCVCM_NODE_TYPE_OP_BITWISE_AND_ASSIGN:
        case PCVCM_NODE_TYPE_OP_BITWISE_OR_ASSIGN:
        case PCVCM_NODE_TYPE_OP_BITWISE_XOR_ASSIGN:
        case PCVCM_NODE_TYPE_OP_LEFT_SHIFT_ASSIGN:
        case PCVCM_NODE_TYPE_OP_RIGHT_SHIFT_ASSIGN:
            return (operator_info_t){PRECEDENCE_ASSIGNMENT, ASSOC_RIGHT};

        // Comma operator (,)
        case PCVCM_NODE_TYPE_OP_COMMA:
            return (operator_info_t){PRECEDENCE_COMMA, ASSOC_LEFT};

        default:
            return (operator_info_t){0, ASSOC_LEFT}; // Unknown operator
    }
}

static bool is_operator(enum pcvcm_node_type type) {
    return (type >= PCVCM_NODE_TYPE_OP_FIRST && type <= PCVCM_NODE_TYPE_OP_LAST);
}

static bool is_left_paren(enum pcvcm_node_type type) {
    return type == PCVCM_NODE_TYPE_OP_LP;
}

static bool is_right_paren(enum pcvcm_node_type type) {
    return type == PCVCM_NODE_TYPE_OP_RP;
}

// Arithmetic operations
static purc_variant_t evaluate_add(purc_variant_t left, purc_variant_t right)
{
    /* Implement addition operation (+) */
    if (left == PURC_VARIANT_INVALID || right == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    enum purc_variant_type ltype = purc_variant_get_type(left);
    enum purc_variant_type rtype = purc_variant_get_type(right);

    if ((ltype == PURC_VARIANT_TYPE_STRING ||
         ltype == PURC_VARIANT_TYPE_BSEQUENCE) &&
        (rtype == PURC_VARIANT_TYPE_STRING ||
         rtype == PURC_VARIANT_TYPE_BSEQUENCE)) {
        return purc_variant_operator_concat(left, right);
    }

    if ((ltype == PURC_VARIANT_TYPE_ARRAY || ltype == PURC_VARIANT_TYPE_TUPLE) &&
        (rtype == PURC_VARIANT_TYPE_ARRAY || rtype == PURC_VARIANT_TYPE_TUPLE ||
         rtype == PURC_VARIANT_TYPE_SET)) {
        return purc_variant_operator_concat(left, right);
    }

    if (ltype == PURC_VARIANT_TYPE_OBJECT && rtype == PURC_VARIANT_TYPE_OBJECT) {
        purc_variant_t ret = purc_variant_make_object_0();
        if (ret == PURC_VARIANT_INVALID) {
            return PURC_VARIANT_INVALID;
        }
        purc_variant_object_unite(ret, left, PCVRNT_CR_METHOD_OVERWRITE);
        purc_variant_object_unite(ret, right, PCVRNT_CR_METHOD_OVERWRITE);
        return ret;
    }

    if ((ltype == PURC_VARIANT_TYPE_SET) && (rtype == PURC_VARIANT_TYPE_ARRAY ||
        rtype == PURC_VARIANT_TYPE_SET || rtype == PURC_VARIANT_TYPE_TUPLE)) {
        const char *unique_key = NULL;
        purc_variant_set_unique_keys(left, &unique_key);

        purc_variant_t ret =
            purc_variant_make_set_by_ckey(0, unique_key, PURC_VARIANT_INVALID);
        if (ret == PURC_VARIANT_INVALID) {
            return PURC_VARIANT_INVALID;
        }

        purc_variant_set_unite(ret, left, PCVRNT_CR_METHOD_OVERWRITE);
        purc_variant_set_unite(ret, right, PCVRNT_CR_METHOD_OVERWRITE);
        return ret;
    }
    return purc_variant_operator_add(left, right);
}

static purc_variant_t evaluate_subtract(purc_variant_t left,
                                        purc_variant_t right)
{
    /* Implement subtraction operation (-) */
    return purc_variant_operator_sub(left, right);
}

static purc_variant_t evaluate_multiply(purc_variant_t left,
                                        purc_variant_t right)
{
    /* Implement multiplication operation (*) */
    return purc_variant_operator_mul(left, right);
}

static purc_variant_t evaluate_divide(purc_variant_t left, purc_variant_t right)
{
    /* Implement division operation (/) */
    return purc_variant_operator_truediv(left, right);
}

static purc_variant_t evaluate_modulo(purc_variant_t left, purc_variant_t right)
{
    /* Implement modulo operation (%) */
    return purc_variant_operator_mod(left, right);
}

static purc_variant_t evaluate_floor_divide(purc_variant_t left,
                                            purc_variant_t right)
{
    /* Implement floor division operation (//) */
    return purc_variant_operator_floordiv(left, right);
}

static purc_variant_t evaluate_power(purc_variant_t left, purc_variant_t right)
{
    /* Implement power operation (**) */
    return purc_variant_operator_pow(left, right);
}

// Comparison operations
static purc_variant_t evaluate_equal(purc_variant_t left, purc_variant_t right)
{
    /* Implement equal comparison operation (==) */
    bool ret = purc_variant_operator_eq(left, right);
    return purc_variant_make_boolean(ret);
}

static purc_variant_t evaluate_not_equal(purc_variant_t left,
                                         purc_variant_t right)
{
    /* Implement not equal comparison operation (!=) */
    bool result = purc_variant_operator_ne(left, right);
    return purc_variant_make_boolean(result);
}

static purc_variant_t evaluate_less(purc_variant_t left, purc_variant_t right)
{
    /* Implement less than comparison operation (<) */
    bool ret = purc_variant_operator_lt(left, right);
    return purc_variant_make_boolean(ret);
}

static purc_variant_t evaluate_less_equal(purc_variant_t left,
                                          purc_variant_t right)
{
    /* Implement less than or equal comparison operation (<=) */
    bool ret = purc_variant_operator_le(left, right);
    return purc_variant_make_boolean(ret);
}

static purc_variant_t evaluate_greater(purc_variant_t left,
                                       purc_variant_t right)
{
    /* Implement greater than comparison operation (>) */
    bool ret = purc_variant_operator_gt(left, right);
    return purc_variant_make_boolean(ret);
}

static purc_variant_t evaluate_greater_equal(purc_variant_t left,
                                             purc_variant_t right)
{
    /* Implement greater than or equal comparison operation (>=) */
    bool ret = purc_variant_operator_ge(left, right);;
    return purc_variant_make_boolean(ret);
}

// Logical operations
static purc_variant_t evaluate_logical_and(purc_variant_t left,
                                           purc_variant_t right)
{
    /* Implement logical and operation (and) */
    bool ret =
        purc_variant_operator_truth(left) && purc_variant_operator_truth(right);
    return purc_variant_make_boolean(ret);
}

static purc_variant_t evaluate_logical_or(purc_variant_t left,
                                          purc_variant_t right)
{
    /* Implement logical or operation (or) */
    bool ret =
        purc_variant_operator_truth(left) || purc_variant_operator_truth(right);

    return purc_variant_make_boolean(ret);
}

static purc_variant_t evaluate_logical_not(purc_variant_t operand)
{
    /* Implement logical not operation (not) */
    bool ret = purc_variant_operator_not(operand);
    return purc_variant_make_boolean(ret);
}

// Membership operations
static purc_variant_t evaluate_in(purc_variant_t left, purc_variant_t right)
{
    /* Implement in membership operation (in) */

    /* Perform the contains operation (@b in @a) for sequences and
     * return a boolean result.
     * purc_variant_operator_contains(purc_variant_t a, purc_variant_t b);
     */
    return purc_variant_operator_contains(right, left);
}

static purc_variant_t evaluate_not_in(purc_variant_t left, purc_variant_t right)
{
    /* Implement not in membership operation (not_in) */
    purc_variant_t v = purc_variant_operator_contains(right, left);
    if (v) {
        purc_variant_t ret =
            purc_variant_make_boolean(!purc_variant_operator_truth(v));
        purc_variant_unref(v);
        return ret;
    }
    return v;
}

// Comma operation
static purc_variant_t evaluate_comma(purc_variant_t value)
{
    /* Implement comma operation (,) */
    return value ? purc_variant_ref(value) : PURC_VARIANT_INVALID;
}

// Bitwise operations
static purc_variant_t evaluate_bitwise_and(purc_variant_t left,
                                           purc_variant_t right)
{
    /* Implement bitwise and operation (&) */
    return purc_variant_operator_and(left, right);
}

static purc_variant_t evaluate_bitwise_or(purc_variant_t left,
                                          purc_variant_t right)
{
    /* Implement bitwise or operation (|) */
    return purc_variant_operator_or(left, right);
}

static purc_variant_t evaluate_bitwise_xor(purc_variant_t left,
                                           purc_variant_t right)
{
    /* Implement bitwise xor operation (^) */
    return purc_variant_operator_xor(left, right);
}

static purc_variant_t evaluate_bitwise_invert(purc_variant_t operand)
{
    /* Implement bitwise invert operation (~) */
    return purc_variant_operator_invert(operand);
}

static purc_variant_t evaluate_left_shift(purc_variant_t left,
                                          purc_variant_t right)
{
    /* Implement left shift operation (<<) */
    return purc_variant_operator_lshift(left, right);
}

static purc_variant_t evaluate_right_shift(purc_variant_t left,
                                           purc_variant_t right)
{
    /* Implement right shift operation (>>) */
    return purc_variant_operator_rshift(left, right);
}

// Ternary conditional operator
static purc_variant_t evaluate_ternary_conditional(purc_variant_t value)
{
    /* Implement ternary conditional operation (? :) */
    return value ? purc_variant_ref(value) : PURC_VARIANT_INVALID;
}

// Unary operations
static purc_variant_t evaluate_unary_plus(purc_variant_t operand)
{
    /* Implement unary plus operation (+) */
    return purc_variant_operator_pos(operand);
}

static purc_variant_t evaluate_unary_minus(purc_variant_t operand)
{
    /* Implement unary minus operation (-) */
    return purc_variant_operator_neg(operand);
}

// Assignment operators
static purc_variant_t evaluate_assign(struct pcvcm_eval_ctxt *ctxt,
    purc_variant_t left, purc_variant_t right,
    struct pcvcm_node *left_node, struct pcvcm_node *right_node)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    UNUSED_PARAM(left_node);
    UNUSED_PARAM(right_node);

    /* Implement assignment operation (=) */

    assert(left_node->type == PCVCM_NODE_TYPE_FUNC_GET_VARIABLE);

    const char *name = NULL;
    pcutils_map_entry *entry = pcutils_map_find(ctxt->node_var_name_map, left_node);
    if (entry) {
        name = (const char *) entry->val;
    }

    if (!name) {
        return PURC_VARIANT_INVALID;
    }

    if (ctxt->bind_var) {
        ctxt->bind_var(ctxt, name, right, true);
    }

    return right ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_add_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement add assignment operation (+=) */

    if (left == PURC_VARIANT_INVALID || right == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    enum purc_variant_type ltype = purc_variant_get_type(left);
    enum purc_variant_type rtype = purc_variant_get_type(right);

    if ((ltype == PURC_VARIANT_TYPE_STRING ||
         ltype == PURC_VARIANT_TYPE_BSEQUENCE) &&
        (rtype == PURC_VARIANT_TYPE_STRING ||
         rtype == PURC_VARIANT_TYPE_BSEQUENCE)) {
        purc_variant_operator_iconcat(left, right);
        goto out;
    }

    if ((ltype == PURC_VARIANT_TYPE_ARRAY || ltype == PURC_VARIANT_TYPE_TUPLE) &&
        (rtype == PURC_VARIANT_TYPE_ARRAY || rtype == PURC_VARIANT_TYPE_TUPLE ||
         rtype == PURC_VARIANT_TYPE_SET)) {
        purc_variant_operator_iconcat(left, right);
        goto out;
    }

    if (ltype == PURC_VARIANT_TYPE_OBJECT && rtype == PURC_VARIANT_TYPE_OBJECT) {
        purc_variant_object_unite(left,right,PCVRNT_CR_METHOD_OVERWRITE);
        goto out;
    }

    if ((ltype == PURC_VARIANT_TYPE_SET) && (rtype == PURC_VARIANT_TYPE_ARRAY ||
        rtype == PURC_VARIANT_TYPE_SET || rtype == PURC_VARIANT_TYPE_TUPLE)) {
        purc_variant_set_unite(left, right, PCVRNT_CR_METHOD_OVERWRITE);
        goto out;
    }

    int ret = purc_variant_operator_iadd(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;

out:
    return purc_variant_ref(right);
}

static purc_variant_t evaluate_sub_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement subtract assignment operation (-=) */
    int ret = purc_variant_operator_isub(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_mul_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement multiply assignment operation (*=) */
    int ret = purc_variant_operator_imul(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_div_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement divide assignment operation (/=) */
    int ret = purc_variant_operator_itruediv(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_mod_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement modulo assignment operation (%=) */
    int ret = purc_variant_operator_imod(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_floor_div_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement floor division assignment operation (//=) */
    int ret = purc_variant_operator_ifloordiv(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_power_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement power assignment operation (**=) */
    int ret = purc_variant_operator_ipow(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_bitwise_and_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement bitwise AND assignment operation (&=) */
    int ret = purc_variant_operator_iand(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_bitwise_or_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement bitwise OR assignment operation (|=) */
    int ret = purc_variant_operator_ior(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_bitwise_xor_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement bitwise XOR assignment operation (^=) */
    int ret = purc_variant_operator_ixor(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_left_shift_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement left shift assignment operation (<<=) */
    int ret = purc_variant_operator_ilshift(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_right_shift_assign(purc_variant_t left, purc_variant_t right)
{
    /* Implement right shift assignment operation (>>=) */
    int ret = purc_variant_operator_irshift(left, right);
    return (ret == 0 && right) ? purc_variant_ref(right) : PURC_VARIANT_INVALID;
}

// Increment/Decrement operators
static purc_variant_t evaluate_increment(purc_variant_t operand)
{
    /* Implement increment operation (++) */
    purc_variant_t v = purc_variant_make_longint(1);

    int ret = purc_variant_operator_iadd(operand, v);

    purc_variant_unref(v);

    return (ret == 0) ? purc_variant_ref(operand) : PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_decrement(purc_variant_t operand)
{
    /* Implement decrement operation (--) */
    purc_variant_t v = purc_variant_make_longint(1);

    int ret = purc_variant_operator_isub(operand, v);

    purc_variant_unref(v);

    return (ret == 0) ? purc_variant_ref(operand) : PURC_VARIANT_INVALID;
}

// Binary operator dispatcher
static purc_variant_t evaluate_binary_operator(struct pcvcm_eval_ctxt *ctxt,
    enum pcvcm_node_type op_type, purc_variant_t left, purc_variant_t right,
    struct pcvcm_node *left_node, struct pcvcm_node *right_node)
{
    switch (op_type) {
    // Arithmetic operators
    case PCVCM_NODE_TYPE_OP_ADD:
        return evaluate_add(left, right);
    case PCVCM_NODE_TYPE_OP_SUB:
        return evaluate_subtract(left, right);
    case PCVCM_NODE_TYPE_OP_MULTIPLY:
        return evaluate_multiply(left, right);
    case PCVCM_NODE_TYPE_OP_DIVIDE:
        return evaluate_divide(left, right);
    case PCVCM_NODE_TYPE_OP_MODULO:
        return evaluate_modulo(left, right);
    case PCVCM_NODE_TYPE_OP_FLOOR_DIVIDE:
        return evaluate_floor_divide(left, right);
    case PCVCM_NODE_TYPE_OP_POWER:
        return evaluate_power(left, right);
    // Comparison operators
    case PCVCM_NODE_TYPE_OP_EQUAL:
        return evaluate_equal(left, right);
    case PCVCM_NODE_TYPE_OP_NOT_EQUAL:
        return evaluate_not_equal(left, right);
    case PCVCM_NODE_TYPE_OP_LESS:
        return evaluate_less(left, right);
    case PCVCM_NODE_TYPE_OP_LESS_EQUAL:
        return evaluate_less_equal(left, right);
    case PCVCM_NODE_TYPE_OP_GREATER:
        return evaluate_greater(left, right);
    case PCVCM_NODE_TYPE_OP_GREATER_EQUAL:
        return evaluate_greater_equal(left, right);
    // Logical operators
    case PCVCM_NODE_TYPE_OP_LOGICAL_AND:
        return evaluate_logical_and(left, right);
    case PCVCM_NODE_TYPE_OP_LOGICAL_OR:
        return evaluate_logical_or(left, right);
    // Membership operators
    case PCVCM_NODE_TYPE_OP_IN:
        return evaluate_in(left, right);
    case PCVCM_NODE_TYPE_OP_NOT_IN:
        return evaluate_not_in(left, right);
    // Bitwise operators
    case PCVCM_NODE_TYPE_OP_BITWISE_AND:
        return evaluate_bitwise_and(left, right);
    case PCVCM_NODE_TYPE_OP_BITWISE_OR:
        return evaluate_bitwise_or(left, right);
    case PCVCM_NODE_TYPE_OP_BITWISE_XOR:
        return evaluate_bitwise_xor(left, right);
    case PCVCM_NODE_TYPE_OP_LEFT_SHIFT:
        return evaluate_left_shift(left, right);
    case PCVCM_NODE_TYPE_OP_RIGHT_SHIFT:
        return evaluate_right_shift(left, right);
    // Assignment operators
    case PCVCM_NODE_TYPE_OP_ASSIGN:
        return evaluate_assign(ctxt, left, right, left_node, right_node);
    case PCVCM_NODE_TYPE_OP_PLUS_ASSIGN:
        return evaluate_add_assign(left, right);
    case PCVCM_NODE_TYPE_OP_MINUS_ASSIGN:
        return evaluate_sub_assign(left, right);
    case PCVCM_NODE_TYPE_OP_MULTIPLY_ASSIGN:
        return evaluate_mul_assign(left, right);
    case PCVCM_NODE_TYPE_OP_DIVIDE_ASSIGN:
        return evaluate_div_assign(left, right);
    case PCVCM_NODE_TYPE_OP_MODULO_ASSIGN:
        return evaluate_mod_assign(left, right);
    case PCVCM_NODE_TYPE_OP_FLOOR_DIV_ASSIGN:
        return evaluate_floor_div_assign(left, right);
    case PCVCM_NODE_TYPE_OP_POWER_ASSIGN:
        return evaluate_power_assign(left, right);
    case PCVCM_NODE_TYPE_OP_BITWISE_AND_ASSIGN:
        return evaluate_bitwise_and_assign(left, right);
    case PCVCM_NODE_TYPE_OP_BITWISE_OR_ASSIGN:
        return evaluate_bitwise_or_assign(left, right);
    case PCVCM_NODE_TYPE_OP_BITWISE_XOR_ASSIGN:
        return evaluate_bitwise_xor_assign(left, right);
    case PCVCM_NODE_TYPE_OP_LEFT_SHIFT_ASSIGN:
        return evaluate_left_shift_assign(left, right);
    case PCVCM_NODE_TYPE_OP_RIGHT_SHIFT_ASSIGN:
        return evaluate_right_shift_assign(left, right);
    default:
        return PURC_VARIANT_INVALID;
    }
}

// Unary operator dispatcher
static purc_variant_t evaluate_unary_operator(enum pcvcm_node_type op_type,
     purc_variant_t operand)
{
    switch (op_type) {
    case PCVCM_NODE_TYPE_OP_UNARY_PLUS:
        return evaluate_unary_plus(operand);
    case PCVCM_NODE_TYPE_OP_UNARY_MINUS:
        return evaluate_unary_minus(operand);
    case PCVCM_NODE_TYPE_OP_LOGICAL_NOT:
        return evaluate_logical_not(operand);
    case PCVCM_NODE_TYPE_OP_BITWISE_INVERT:
        return evaluate_bitwise_invert(operand);
    default:
        return PURC_VARIANT_INVALID;
    }
}

static struct pcutils_stack *
infix_to_postfix(struct pcvcm_eval_ctxt *ctxt,
                 struct pcvcm_eval_stack_frame *frame)
{
    struct pcutils_stack *output_stack =
        pcutils_stack_new(sizeof(struct pcvcm_eval_node *));
    struct pcutils_stack *operator_stack =
        pcutils_stack_new(sizeof(struct pcvcm_eval_node *));

    if (!output_stack || !operator_stack) {
        if (output_stack) {
            pcutils_stack_destroy(output_stack);
        }
        if (operator_stack) {
            pcutils_stack_destroy(operator_stack);
        }
        return NULL;
    }

    // Get all child nodes (operands and operators)
    size_t nr_children = pcvcm_node_children_count(frame->node);

    for (size_t i = 0; i < nr_children; i++) {
        struct pcvcm_eval_node *eval_node =
            ctxt->eval_nodes + frame->eval_node_idx;
        struct pcvcm_eval_node *child =
            ctxt->eval_nodes + eval_node->first_child_idx + i;

        if (is_operator(child->node->type)) {
            if (is_left_paren(child->node->type)) {
                // Left parenthesis: push to operator stack
                pcutils_stack_push(operator_stack, (uintptr_t)child);
            } else if (is_right_paren(child->node->type)) {
                // Right parenthesis: pop operators until left parenthesis
                while (!pcutils_stack_is_empty(operator_stack)) {
                    struct pcvcm_eval_node *op =
                        (struct pcvcm_eval_node *)pcutils_stack_top(
                            operator_stack);
                    pcutils_stack_pop(operator_stack);

                    if (is_left_paren(op->node->type)) {
                        break;
                    }
                    pcutils_stack_push(output_stack, (uintptr_t)op);
                }
            } else {
                // Regular operator
                operator_info_t current_op =
                    get_operator_info(child->node->type);

                // Pop operators with higher or equal precedence (considering
                // associativity)
                while (!pcutils_stack_is_empty(operator_stack)) {
                    struct pcvcm_eval_node *top_op =
                        (struct pcvcm_eval_node *)pcutils_stack_top(
                            operator_stack);

                    if (is_left_paren(top_op->node->type)) {
                        break;
                    }

                    operator_info_t top_op_info =
                        get_operator_info(top_op->node->type);

                    bool should_pop = false;
                    if (current_op.associativity == ASSOC_LEFT) {
                        should_pop =
                            (top_op_info.precedence >= current_op.precedence);
                    } else {
                        should_pop =
                            (top_op_info.precedence > current_op.precedence);
                    }

                    if (!should_pop) {
                        break;
                    }

                    pcutils_stack_pop(operator_stack);
                    pcutils_stack_push(output_stack, (uintptr_t)top_op);
                }

                // Push current operator to operator stack
                pcutils_stack_push(operator_stack, (uintptr_t)child);
            }
        } else {
            // Operand: add to output
            pcutils_stack_push(output_stack, (uintptr_t)child);
        }
    }

    // Pop remaining operators
    while (!pcutils_stack_is_empty(operator_stack)) {
        struct pcvcm_eval_node *op =
            (struct pcvcm_eval_node *)pcutils_stack_top(operator_stack);
        pcutils_stack_pop(operator_stack);
        pcutils_stack_push(output_stack, (uintptr_t)op);
    }

    pcutils_stack_destroy(operator_stack);
    return output_stack;
}

static purc_variant_t evaluate_postfix(struct pcvcm_eval_ctxt *ctxt,
    struct pcutils_stack *postfix_stack, struct pcvcm_eval_stack_frame *frame)
{
    UNUSED_PARAM(frame);
    struct pcutils_stack *eval_stack =
        pcutils_stack_new(sizeof(purc_variant_t));
    if (!eval_stack) {
        return PURC_VARIANT_INVALID;
    }

    // Process each element in postfix expression
    for (size_t i = 0; i < pcutils_stack_size(postfix_stack); i++) {
        struct pcvcm_eval_node *eval_node =
            (struct pcvcm_eval_node *)pcutils_stack_get(postfix_stack, i);
        if (!eval_node) {
            continue;
        }

        if (is_operator(eval_node->node->type)) {
            if (eval_node->node->type == PCVCM_NODE_TYPE_OP_CONDITIONAL) {
                // Ternary operator: condition ? true_val : false_val
                purc_variant_t value = eval_node->result;
                purc_variant_t result = evaluate_ternary_conditional(value);
                pcutils_stack_push(eval_stack, (uintptr_t)result);
            } else if (eval_node->node->type == PCVCM_NODE_TYPE_OP_COMMA) {
                // Comma operator
                purc_variant_t value = eval_node->result;
                purc_variant_t result = evaluate_comma(value);
                pcutils_stack_push(eval_stack, (uintptr_t)result);
            } else {
                // Check if it's a unary operator
                bool is_unary =
                    (eval_node->node->type == PCVCM_NODE_TYPE_OP_UNARY_PLUS ||
                     eval_node->node->type == PCVCM_NODE_TYPE_OP_UNARY_MINUS ||
                     eval_node->node->type == PCVCM_NODE_TYPE_OP_LOGICAL_NOT ||
                     eval_node->node->type ==
                         PCVCM_NODE_TYPE_OP_BITWISE_INVERT);

                // Check if it's a postfix operator (treated as unary but handled differently)
                bool is_postfix =
                    (eval_node->node->type == PCVCM_NODE_TYPE_OP_INCREMENT ||
                     eval_node->node->type == PCVCM_NODE_TYPE_OP_DECREMENT);

                if (is_unary) {
                    // Unary operator
                    if (pcutils_stack_size(eval_stack) < 1) {
                        pcutils_stack_destroy(eval_stack);
                        return PURC_VARIANT_INVALID;
                    }

                    purc_variant_t operand =
                        (purc_variant_t)pcutils_stack_top(eval_stack);
                    pcutils_stack_pop(eval_stack);

                    purc_variant_t result =
                        evaluate_unary_operator(eval_node->node->type, operand);
                    pcutils_stack_push(eval_stack, (uintptr_t)result);

                    PURC_VARIANT_SAFE_CLEAR(operand);
                } else if (is_postfix) {
                    // Postfix operator (x++, x--)
                    if (pcutils_stack_size(eval_stack) < 1) {
                        pcutils_stack_destroy(eval_stack);
                        return PURC_VARIANT_INVALID;
                    }

                    purc_variant_t operand =
                        (purc_variant_t)pcutils_stack_top(eval_stack);
                    pcutils_stack_pop(eval_stack);

                    purc_variant_t result = PURC_VARIANT_INVALID;
                    if (eval_node->node->type == PCVCM_NODE_TYPE_OP_INCREMENT) {
                        result = evaluate_increment(operand);
                    } else if (eval_node->node->type == PCVCM_NODE_TYPE_OP_DECREMENT) {
                        result = evaluate_decrement(operand);
                    }
                    pcutils_stack_push(eval_stack, (uintptr_t)result);

                    PURC_VARIANT_SAFE_CLEAR(operand);
                } else {
                    // Binary operator
                    if (pcutils_stack_size(eval_stack) < 2) {
                        pcutils_stack_destroy(eval_stack);
                        return PURC_VARIANT_INVALID;
                    }

                    purc_variant_t right =
                        (purc_variant_t)pcutils_stack_top(eval_stack);
                    pcutils_stack_pop(eval_stack);
                    purc_variant_t left =
                        (purc_variant_t)pcutils_stack_top(eval_stack);
                    pcutils_stack_pop(eval_stack);

                    struct pcvcm_node *left_node = NULL;
                    struct pcvcm_node *right_node = NULL;
                    assert(i > 1);
                    {
                        struct pcvcm_eval_node *left =
                            (struct pcvcm_eval_node *)pcutils_stack_get(
                                postfix_stack, i - 2);
                        left_node = left->node;

                        struct pcvcm_eval_node *right =
                            (struct pcvcm_eval_node *)pcutils_stack_get(
                                postfix_stack, i - 1);
                        right_node = right->node;
                    }

                    purc_variant_t result = evaluate_binary_operator(ctxt,
                        eval_node->node->type, left, right, left_node, right_node);
                    pcutils_stack_push(eval_stack, (uintptr_t)result);

                    PURC_VARIANT_SAFE_CLEAR(left);
                    PURC_VARIANT_SAFE_CLEAR(right);
                }
            }
        } else {
            // Operand: get its value
            purc_variant_t value = eval_node->result;
            if (value) {
                purc_variant_ref(value);
            }
            pcutils_stack_push(eval_stack, (uintptr_t)value);
        }
    }

    // The final result should be the only element left on the stack
    purc_variant_t result = PURC_VARIANT_INVALID;
    if (pcutils_stack_size(eval_stack) == 1) {
        result = (purc_variant_t)pcutils_stack_top(eval_stack);
    }

    pcutils_stack_destroy(eval_stack);
    return result;
}

// Cleanup function for postfix_stack stored in priv_data
static void cleanup_postfix_stack(struct pcvcm_node *node, void *private_data)
{
    UNUSED_PARAM(node);
    struct pcutils_stack *postfix_stack = (struct pcutils_stack *)private_data;
    if (postfix_stack) {
        pcutils_stack_destroy(postfix_stack);
    }
}

static purc_variant_t
eval(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame, const char **name)
{
    UNUSED_PARAM(name);

    struct pcutils_stack *postfix_stack = NULL;

    // Check if postfix_stack is already cached in priv_data
    if (frame->node->priv_data) {
        postfix_stack = (struct pcutils_stack *)frame->node->priv_data;
    } else {
        // Convert infix expression to postfix using Shunting Yard algorithm
        postfix_stack = infix_to_postfix(ctxt, frame);
        if (!postfix_stack) {
            return PURC_VARIANT_INVALID;
        }

        // Cache the postfix_stack in priv_data for reuse
        pcvcm_node_set_private_data(frame->node, postfix_stack, cleanup_postfix_stack);
    }

    // Evaluate postfix expression
    purc_variant_t result = evaluate_postfix(ctxt, postfix_stack, frame);

    return result;
}

static struct pcvcm_eval_stack_frame_ops ops = {
    .after_pushed = after_pushed,
    .select_param = select_param_default,
    .eval = eval
};

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_operator_expression_ops() {
    return &ops;
}
