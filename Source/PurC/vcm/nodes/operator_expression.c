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

// Operator precedence definitions
#define PRECEDENCE_PARENTHESES      17  // () [] {}
#define PRECEDENCE_UNARY           16  // + - ~
#define PRECEDENCE_POWER           15  // **
#define PRECEDENCE_MULTIPLICATIVE  14  // * / % //
#define PRECEDENCE_ADDITIVE        13  // + -
#define PRECEDENCE_SHIFT           12  // << >>
#define PRECEDENCE_BITWISE_AND     11  // &
#define PRECEDENCE_BITWISE_OR      10  // |
#define PRECEDENCE_BITWISE_XOR      9  // ^
#define PRECEDENCE_COMPARISON       8  // < <= > >= == !=
#define PRECEDENCE_MEMBERSHIP      PRECEDENCE_COMPARISON  // in not_in
#define PRECEDENCE_LOGICAL_NOT      7  // not
#define PRECEDENCE_LOGICAL_AND      6  // and
#define PRECEDENCE_LOGICAL_OR       5  // or
#define PRECEDENCE_CONDITIONAL      4  // ? :
#define PRECEDENCE_ASSIGNMENT       3  // = += -= etc.
#define PRECEDENCE_COMMA            2  // ,

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
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement equal comparison operation (==) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_not_equal(purc_variant_t left,
                                         purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement not equal comparison operation (!=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_less(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement less than comparison operation (<) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_less_equal(purc_variant_t left,
                                          purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement less than or equal comparison operation (<=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_greater(purc_variant_t left,
                                       purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement greater than comparison operation (>) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_greater_equal(purc_variant_t left,
                                             purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement greater than or equal comparison operation (>=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Logical operations
static purc_variant_t evaluate_logical_and(purc_variant_t left,
                                           purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement logical and operation (&&) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_logical_or(purc_variant_t left,
                                          purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement logical or operation (||) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Membership operations
static purc_variant_t evaluate_in(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement in membership operation (in) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_not_in(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement not in membership operation (not_in) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Comma operation
static purc_variant_t evaluate_comma(void)
{
    /* TODO: Implement comma operation (,) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Bitwise operations
static purc_variant_t evaluate_bitwise_and(purc_variant_t left,
                                           purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement bitwise and operation (&) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_bitwise_or(purc_variant_t left,
                                          purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement bitwise or operation (|) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_bitwise_xor(purc_variant_t left,
                                           purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement bitwise xor operation (^) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_left_shift(purc_variant_t left,
                                          purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement left shift operation (<<) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_right_shift(purc_variant_t left,
                                           purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement right shift operation (>>) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Ternary conditional operator
static purc_variant_t evaluate_ternary_conditional(void)
{
    /* TODO: Implement ternary conditional operation (? :) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Unary operations
static purc_variant_t evaluate_unary_plus(purc_variant_t operand)
{
    UNUSED_PARAM(operand);
    /* TODO: Implement unary plus operation (+) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_unary_minus(purc_variant_t operand)
{
    UNUSED_PARAM(operand);
    /* TODO: Implement unary minus operation (-) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_logical_not(purc_variant_t operand)
{
    UNUSED_PARAM(operand);
    /* TODO: Implement logical not operation (!) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Assignment operators
static purc_variant_t evaluate_assign(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement assignment operation (=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_add_assign(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement add assignment operation (+=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_sub_assign(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement subtract assignment operation (-=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_mul_assign(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement multiply assignment operation (*=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_div_assign(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement divide assignment operation (/=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_mod_assign(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);
    UNUSED_PARAM(right);
    /* TODO: Implement modulo assignment operation (%=) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Increment/Decrement operators
static purc_variant_t evaluate_pre_increment(purc_variant_t operand)
{
    UNUSED_PARAM(operand);
    /* TODO: Implement pre-increment operation (++var) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_pre_decrement(purc_variant_t operand)
{
    UNUSED_PARAM(operand);
    /* TODO: Implement pre-decrement operation (--var) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t evaluate_bitwise_invert(purc_variant_t operand)
{
    UNUSED_PARAM(operand);
    /* TODO: Implement bitwise invert operation (~) */
    assert(0);
    return PURC_VARIANT_INVALID;
}

// Binary operator dispatcher
static purc_variant_t evaluate_binary_operator(enum pcvcm_node_type op_type,
                                               purc_variant_t left,
                                               purc_variant_t right)
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
        return evaluate_assign(left, right);
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
    // Increment/Decrement operators
    case PCVCM_NODE_TYPE_OP_INCREMENT:
        return evaluate_pre_increment(operand);
    case PCVCM_NODE_TYPE_OP_DECREMENT:
        return evaluate_pre_decrement(operand);
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

static purc_variant_t evaluate_postfix(struct pcutils_stack *postfix_stack,
     struct pcvcm_eval_stack_frame *frame)
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
                purc_variant_t result = evaluate_ternary_conditional();
                pcutils_stack_push(eval_stack, (uintptr_t)result);
            } else if (eval_node->node->type == PCVCM_NODE_TYPE_OP_COMMA) {
                // Comma operator
                purc_variant_t result = evaluate_comma();
                pcutils_stack_push(eval_stack, (uintptr_t)result);
            } else {
                // Check if it's a unary operator
                bool is_unary =
                    (eval_node->node->type == PCVCM_NODE_TYPE_OP_UNARY_PLUS ||
                     eval_node->node->type == PCVCM_NODE_TYPE_OP_UNARY_MINUS ||
                     eval_node->node->type == PCVCM_NODE_TYPE_OP_LOGICAL_NOT ||
                     eval_node->node->type ==
                         PCVCM_NODE_TYPE_OP_BITWISE_INVERT);

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

                    purc_variant_t result = evaluate_binary_operator(
                        eval_node->node->type, left, right);
                    pcutils_stack_push(eval_stack, (uintptr_t)result);
                }
            }
        } else {
            // Operand: get its value
            purc_variant_t value = eval_node->result;
            if (value == PURC_VARIANT_INVALID) {
                // If result is not available, this might be a leaf node
                // For now, we'll use PURC_VARIANT_INVALID as placeholder
                value = PURC_VARIANT_INVALID;
            }
            pcutils_stack_push(eval_stack, (uintptr_t)value);
        }
    }

    // The final result should be the only element left on the stack
    purc_variant_t result = PURC_VARIANT_INVALID;
    if (pcutils_stack_size(eval_stack) == 1) {
        result = (purc_variant_t)pcutils_stack_top(eval_stack);
        purc_variant_ref(result);
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
    purc_variant_t result = evaluate_postfix(postfix_stack, frame);

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
