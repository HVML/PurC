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
#include "ops.h"


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
    frame->ops = pcvcm_eval_get_ops_by_node(node);

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
        for (size_t i = 0; i < frame->nr_params; i++) {
            purc_variant_t v = pcutils_array_get(frame->params_result, i);
            if (v) {
                purc_variant_unref(v);
            }
        }
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


static struct pcvcm_eval_stack_frame *
bottom_frame(struct pcvcm_eval_ctxt *ctxt)
{
    if (list_empty(&ctxt->stack)) {
        return NULL;
    }
    return list_last_entry(&ctxt->stack, struct pcvcm_eval_stack_frame, ln);
}

static struct pcvcm_eval_stack_frame *
push_frame(struct pcvcm_eval_ctxt *ctxt, struct pcvcm_node *node,
        size_t return_pos)
{
    struct pcvcm_eval_stack_frame *frame = pcvcm_eval_stack_frame_create(
            node, return_pos);
    if (frame == NULL) {
        goto out;
    }

    list_add_tail(&frame->ln, &ctxt->stack);
out:
    return frame;
}

static void
pop_frame(struct pcvcm_eval_ctxt *ctxt)
{
    struct pcvcm_eval_stack_frame *last = list_last_entry(
            &ctxt->stack, struct pcvcm_eval_stack_frame, ln);
    list_del(&last->ln);
    pcvcm_eval_stack_frame_destroy(last);
}

purc_variant_t
eval_frame(struct pcvcm_eval_ctxt *ctxt, struct pcvcm_eval_stack_frame *frame,
        size_t return_pos)
{
    UNUSED_PARAM(return_pos);
    purc_variant_t result = PURC_VARIANT_INVALID;
    purc_variant_t val;
    struct pcvcm_eval_stack_frame *param_frame;
    struct pcvcm_node *param;
    int ret = 0;

    while (frame->step != STEP_DONE) {
        switch (frame->step) {
            case STEP_AFTER_PUSH:
                ret = frame->ops->after_pushed(ctxt, frame);
                if (ret != PURC_ERROR_OK) {
                    goto out;
                }
                frame->step = STEP_EVAL_PARAMS;
                break;

            case STEP_EVAL_PARAMS:
                for (; frame->pos < frame->nr_params; frame->pos++) {
                    param = pcutils_array_get(frame->params, frame->pos);
                    param_frame = push_frame(ctxt, param, frame->pos);
                    if (!param_frame) {
                        goto out;
                    }

                    val = eval_frame(ctxt, param_frame, frame->pos);
                    if (!val) {
                        goto out;
                    }
                    pcutils_array_set(frame->params_result, frame->pos, val);
                }
                frame->step = STEP_EVAL_VCM;
                break;

            case STEP_EVAL_VCM:
                result = frame->ops->eval(ctxt, frame);
                if (!result) {
                    goto out;
                }
                frame->step = STEP_DONE;
                break;

            case STEP_DONE:
                break;
        }
    }

out:
    return result;
}

purc_variant_t pcvcm_eval_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt **ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently)
{
    purc_variant_t result = PURC_VARIANT_INVALID;
    struct pcvcm_eval_stack_frame *frame;
    struct pcvcm_eval_ctxt *eval_ctxt = pcvcm_eval_ctxt_create();
    int err;
    if (!eval_ctxt) {
        goto out;
    }
    eval_ctxt->find_var = find_var;
    eval_ctxt->find_var_ctxt = find_var_ctxt;
    if (silently) {
        eval_ctxt->flags |= PCVCM_EVAL_FLAG_SILENTLY;
    }

    frame = push_frame(eval_ctxt, tree, 0);
    if (!frame) {
        goto out;
    }

    do {
        result = eval_frame(eval_ctxt, frame, frame->return_pos);
        err = purc_get_last_error();
        if (err == PURC_ERROR_AGAIN) {
            goto out;
        }
        pop_frame(ctxt);
        frame = bottom_frame(eval_ctxt);
    } while (frame);

out:
    if (err == PURC_ERROR_AGAIN) {
        *ctxt = eval_ctxt;
    }
    else {
        pcvcm_eval_ctxt_destroy(eval_ctxt);
    }
    return result;
}

