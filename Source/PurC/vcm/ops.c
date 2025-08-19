/*
 * @file ops.c
 * @author XueShuming
 * @date 2021/09/02
 * @brief The impl of vcm stack frame ops.
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
#include "private/utils.h"
#include "private/vcm.h"

#include "eval.h"
#include "ops.h"

typedef struct pcvcm_eval_stack_frame_ops *(*get_ops_fn)(void);

static get_ops_fn frame_ops[] = {
    pcvcm_get_undefined_ops,
    pcvcm_get_object_ops,
    pcvcm_get_array_ops,
    pcvcm_get_tuple_ops,
    pcvcm_get_string_ops,
    pcvcm_get_null_ops,
    pcvcm_get_boolean_ops,
    pcvcm_get_number_ops,
    pcvcm_get_long_int_ops,
    pcvcm_get_ulong_int_ops,
    pcvcm_get_big_int_ops,
    pcvcm_get_long_double_ops,
    pcvcm_get_byte_sequence_ops,
    pcvcm_get_concat_string_ops,
    pcvcm_get_get_variable_ops,
    pcvcm_get_get_element_ops,
    pcvcm_get_call_getter_ops,
    pcvcm_get_call_setter_ops,
    pcvcm_get_cjsonee_ops,
    pcvcm_get_cjsonee_op_and_ops,
    pcvcm_get_cjsonee_op_or_ops,
    pcvcm_get_cjsonee_op_semicolon_ops,
    pcvcm_get_constant_ops,
};

#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(types, PCA_TABLESIZE(frame_ops) == PCVCM_NODE_TYPE_NR);
#undef _COMPILE_TIME_ASSERT

struct pcvcm_eval_node *
select_param_default(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_eval_stack_frame *frame, size_t pos)
{
    UNUSED_PARAM(ctxt);
    struct pcvcm_eval_node *eval_node = ctxt->eval_nodes + frame->eval_node_idx;
    return ctxt->eval_nodes + eval_node->first_child_idx + pos;
}

void
pcvcm_set_frame_result(struct pcvcm_eval_ctxt *ctxt, int32_t frame_idx,
        size_t pos, purc_variant_t v, const char *name)
{
    UNUSED_PARAM(name);
    struct pcvcm_eval_stack_frame *frame = ctxt->frames + frame_idx;
    struct pcvcm_eval_node *eval_node = ctxt->eval_nodes + frame->eval_node_idx;
    int32_t idx = eval_node->first_child_idx + pos;
    struct pcvcm_eval_node *child = ctxt->eval_nodes + idx;
#ifdef PCVCM_KEEP_NAME
    ctxt->names[idx] = name;
#endif

    child->result = v;
}

purc_variant_t
pcvcm_get_frame_result(struct pcvcm_eval_ctxt *ctxt,
        int32_t frame_idx, size_t pos, const char **name)
{
    UNUSED_PARAM(name);
    struct pcvcm_eval_stack_frame *frame = ctxt->frames + frame_idx;
    struct pcvcm_eval_node *eval_node = ctxt->eval_nodes + frame->eval_node_idx;
    int32_t idx = eval_node->first_child_idx + pos;
    struct pcvcm_eval_node *child = ctxt->eval_nodes + idx;
#ifdef PCVCM_KEEP_NAME
    if (name) {
        *name = ctxt->names[idx];
    }
#endif

    return child->result;
}

struct pcvcm_eval_stack_frame_ops *
pcvcm_eval_get_ops_by_node(struct pcvcm_node *node)
{
    UNUSED_PARAM(node);
    if (!node) {
        return NULL;
    }
    return frame_ops[node->type]();
}


