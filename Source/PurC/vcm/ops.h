/*
 * @file ops.h
 * @author XueShuming
 * @date 2021/09/02
 * @brief The interfaces for vcm eval frame ops.
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

#ifndef _VCM_OPS_H
#define _VCM_OPS_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "private/debug.h"
#include "private/tree.h"
#include "private/list.h"
#include "private/vcm.h"
#include "purc-variant.h"

#include "eval.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


struct pcvcm_eval_stack_frame_ops *
pcvcm_eval_get_ops_by_node(struct pcvcm_node *node);

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_undefind_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_undefined_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_object_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_array_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_tuple_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_string_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_null_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_boolean_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_number_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_long_int_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_ulong_int_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_big_int_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_long_double_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_byte_sequence_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_concat_string_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_get_variable_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_get_element_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_call_getter_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_call_setter_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_cjsonee_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_cjsonee_op_and_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_cjsonee_op_or_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_cjsonee_op_semicolon_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_constant_ops();

// Arithmetic operators
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_add_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_minus_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_multiply_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_divide_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_modulo_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_floor_divide_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_power_ops();

// Unary operators
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_unary_plus_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_unary_minus_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_bitwise_not_ops();

// Comparison operators
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_equal_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_not_equal_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_greater_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_greater_equal_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_less_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_less_equal_ops();

// Logical operators
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_logical_not_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_logical_and_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_logical_or_ops();

// Membership operators
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_in_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_not_in_ops();

// Bitwise operators
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_bitwise_and_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_bitwise_or_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_bitwise_xor_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_left_shift_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_right_shift_ops();

// Conditional operator
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_conditional_ops();

// Comma operator
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_comma_ops();

// Assignment operators
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_plus_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_minus_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_multiply_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_divide_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_modulo_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_floor_div_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_power_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_bitwise_and_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_bitwise_or_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_bitwise_xor_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_left_shift_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_right_shift_assign_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_increment_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_op_decrement_ops();

// Special node types
struct pcvcm_eval_stack_frame_ops *
pcvcm_get_operator_expression_ops();

struct pcvcm_eval_stack_frame_ops *
pcvcm_get_context_var_alias_ops();

struct pcvcm_eval_node *
select_param_default(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame, size_t pos);

void
pcvcm_set_frame_result(struct pcvcm_eval_ctxt *ctxt, int32_t frame_idx,
        size_t pos, purc_variant_t v, const char *name);

purc_variant_t
pcvcm_get_frame_result(struct pcvcm_eval_ctxt *ctxt,
        int32_t frame_idx, size_t pos, const char **name);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined _VCM_OPS_H */

