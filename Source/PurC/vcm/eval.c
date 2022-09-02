/*
 * @file eval.c
 * @author XueShuming
 * @date 2021/09/02
 * @brief The impl of vcm evaluate.
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

#include "eval.h"


struct pcvcm_eval_stack_frame *
pcvcm_eval_stack_frame_create(struct pcvcm_node *node, size_t return_pos)
{
    struct pcvcm_eval_stack_frame *frame;
    frame = (struct pcvcm_eval_stack_frame*)calloc(1,sizeof(*frame));
    if (!frame) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    frame->node = node;
    frame->pos = 0;
    frame->return_pos = return_pos;
    frame->nr_params = pcvcm_node_children_count(node);
    if (frame->nr_params) {
        frame->params = pcutils_array_create();
        if (!frame->params) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out_destroy_frame;
        }
        frame->params_result = pcutils_array_create();
        if (!frame->params_result) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out_destroy_params;
        }

        struct pctree_node *child = pctree_node_child(
                (struct pctree_node*)node);
        while (child) {
            int ret = pcutils_array_push(frame->params, child);
            if (ret != PURC_ERROR_OK) {
                goto out_destroy_params_result;
            }
            child = pctree_node_next(child);
        }

    }
//    frame->ops = pcvcm_eval_get_ops_by_node(node);

out_destroy_params_result:
    pcutils_array_destroy(frame->params_result, true);

out_destroy_params:
    pcutils_array_destroy(frame->params, true);

out_destroy_frame:
    free(frame);

out:
    return frame;
}

void
pcvcm_eval_stack_frame_destroy(struct pcvcm_eval_stack_frame *frame)
{
    if (!frame) {
        return;
    }
    if (frame->params) {
        pcutils_array_destroy(frame->params, true);
    }
    if (frame->params_result) {
        pcutils_array_destroy(frame->params_result, true);
    }
    free(frame);
}

struct pcvcm_eval_ctxt *
pcvcm_eval_ctxt_create()
{
    struct pcvcm_eval_ctxt *ctxt;
    ctxt = (struct pcvcm_eval_ctxt*)calloc(1,sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    list_head_init(&ctxt->stack);
out:
    return ctxt;
}

void
pcvcm_eval_ctxt_destroy(struct pcvcm_eval_ctxt *ctxt)
{
    if (!ctxt) {
        return;
    }
    struct list_head *stack = &ctxt->stack;
    struct pcvcm_eval_stack_frame *p, *n;
    list_for_each_entry_safe(p, n, stack, ln) {
        pcvcm_eval_stack_frame_destroy(p);
    }
}

purc_variant_t
eval_node(struct pcvcm_node *node, struct pcvcm_eval_ctxt *ctxt,
        size_t return_pos, cb_find_var find_var, void *find_var_ctxt)
{
    purc_variant_t result = PURC_VARIANT_INVALID;
    struct pcvcm_eval_stack_frame *frame = pcvcm_eval_stack_frame_create(
            node, return_pos);
    if (frame == NULL) {
        goto out;
    }

    int ret = frame->ops->after_pushed(ctxt, frame);
    if (ret != PURC_ERROR_OK) {
        goto out;
    }

    for (; frame->pos < frame->nr_params; frame->pos++) {
        struct pcvcm_node *param = pcutils_array_get(frame->params, frame->pos);
        purc_variant_t v = eval_node(param, ctxt, frame->pos, find_var,
                find_var_ctxt);
        if (!v) {
            goto out;
        }
        pcutils_array_set(frame->params_result, frame->pos, v);
    }

    result = frame->ops->eval(ctxt, frame);
out:
    return result;
}


