/*
 * @file eval.h
 * @author XueShuming
 * @date 2021/09/02
 * @brief The interfaces for vcm eval.
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

#ifndef _VCM_EVAL_H
#define _VCM_EVAL_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "private/debug.h"
#include "private/tree.h"
#include "private/list.h"
#include "private/vcm.h"
#include "purc-variant.h"


#define PCVCM_EVAL_FLAG_NONE            0x0000
#define PCVCM_EVAL_FLAG_SILENTLY        0x0001
#define PCVCM_EVAL_FLAG_AGAIN           0x0002
#define PCVCM_EVAL_FLAG_TIMEOUT         0x0004

enum pcvcm_eval_stack_frame_step {
    STEP_AFTER_PUSH = 0,
    STEP_EVAL_PARAMS,
    STEP_EVAL_VCM,
    STEP_DONE,
};

struct pcvcm_eval_stack_frame_ops;
struct pcvcm_eval_stack_frame {
    struct list_head        ln;

    struct pcvcm_node      *node;
    pcutils_array_t        *params;
    pcutils_array_t        *params_result;
    struct pcvcm_eval_stack_frame_ops *ops;

    find_var_fn             find_var;
    void                   *find_var_ctxt;

    size_t                  nr_params;
    size_t                  pos;
    size_t                  return_pos;

    enum pcvcm_eval_stack_frame_step step;
};

struct pcvcm_eval_ctxt {
    /* struct pcvcm_eval_stack_frame */
    struct list_head        stack;
    uint32_t                flags;
};

struct pcvcm_eval_stack_frame_ops {
    int (*after_pushed)(struct pcvcm_eval_ctxt *ctxt,
            struct pcvcm_eval_stack_frame *frame);

    purc_variant_t (*eval)(struct pcvcm_eval_ctxt *ctxt,
            struct pcvcm_eval_stack_frame *frame);
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pcvcm_eval_stack_frame *
pcvcm_eval_stack_frame_create(struct pcvcm_node *node, size_t return_pos);

void
pcvcm_eval_stack_frame_destroy(struct pcvcm_eval_stack_frame *);


struct pcvcm_eval_ctxt *
pcvcm_eval_ctxt_create();

void
pcvcm_eval_ctxt_destroy(struct pcvcm_eval_ctxt *ctxt);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined _VCM_EVAL_H */

