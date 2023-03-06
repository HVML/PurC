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

#define __DEV_VCM__                0

#define PCVCM_EVAL_FLAG_NONE            0x0000
#define PCVCM_EVAL_FLAG_SILENTLY        0x0001
#define PCVCM_EVAL_FLAG_AGAIN           0x0002
#define PCVCM_EVAL_FLAG_TIMEOUT         0x0004

#define KEY_INNER_HANDLER               "__vcm_native_wrapper"
#define KEY_CALLER_NODE                 "__vcm_caller_node"
#define KEY_PARAM_NODE                  "__vcm_param_node"

#define PCVCM_VARIABLE_ARGS_NAME        "_ARGS"


#define MIN_BUF_SIZE                    32
#define MAX_BUF_SIZE                    SIZE_MAX

#if (defined __DEV_VCM__ && __DEV_VCM__)
#define PLOG(format, ...)  fprintf(stderr, "#####>"format, ##__VA_ARGS__);
#else
#define PLOG               PC_DEBUG
#endif /* (defined __DEV_VCM__ && __DEV_VCM__) */

#define PLINE()            PLOG("%s:%d:%s\n", __FILE__, __LINE__, __func__)

enum pcvcm_eval_stack_frame_step {
#define STEP_NAME_AFTER_PUSH            "STEP_AFTER_PUSH"
    STEP_AFTER_PUSH = 0,
#define STEP_NAME_EVAL_PARAMS           "STEP_EVAL_PARAMS"
    STEP_EVAL_PARAMS,
#define STEP_NAME_EVAL_VCM              "STEP_EVAL_VCM"
    STEP_EVAL_VCM,
#define STEP_NAME_DONE                  "STEP_DONE"
    STEP_DONE,
};

enum pcvcm_eval_method_type {
    GETTER_METHOD,
    SETTER_METHOD
};

struct pcvcm_eval_node {
    struct pcvcm_node      *node;
    purc_variant_t          result;
    int32_t                 idx;
    int32_t                 first_child_idx;
};

struct pcvcm_eval_stack_frame_ops;
struct pcvcm_eval_stack_frame {
    struct pcvcm_eval_stack_frame_ops *ops;
    struct pcvcm_node      *node;
    purc_variant_t          args;    // named variable : _ARGS

    size_t                  eval_node_idx;
    size_t                  nr_params;
    size_t                  pos;
    size_t                  return_pos;
    int                     idx;

    enum pcvcm_eval_stack_frame_step step;
};

struct pcvcm_eval_ctxt {
    uint32_t                flags;
    find_var_fn             find_var;
    void                   *find_var_ctxt;

    struct pcvcm_node      *node;
    purc_variant_t          result;

    struct pcvcm_eval_node *eval_nodes;
    size_t                  nr_eval_nodes;
    int32_t                 eval_nodes_insert_pos;

    struct pcvcm_eval_stack_frame *frames;
    size_t                  nr_frames;
    int32_t                 frame_idx;

    const char            **names;

    int                     err;
    unsigned int            enable_log:1;
    unsigned int            free_on_destroy:1;
};

struct pcvcm_eval_stack_frame_ops {
    int (*after_pushed)(struct pcvcm_eval_ctxt *ctxt,
            struct pcvcm_eval_stack_frame *frame);

    struct pcvcm_eval_node *(*select_param)(struct pcvcm_eval_ctxt *ctxt,
            struct pcvcm_eval_stack_frame *frame, size_t pos);

    purc_variant_t (*eval)(struct pcvcm_eval_ctxt *ctxt,
            struct pcvcm_eval_stack_frame *frame, const char **name);
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pcvcm_eval_ctxt *
pcvcm_eval_ctxt_create();

void
pcvcm_eval_ctxt_destroy(struct pcvcm_eval_ctxt *ctxt);

unsigned
pcvcm_eval_ctxt_get_call_flags(struct pcvcm_eval_ctxt *ctxt);


purc_variant_t
pcvcm_eval_native_wrapper_create(purc_variant_t caller_node,
        purc_variant_t param);

bool
pcvcm_eval_is_native_wrapper(purc_variant_t val);

purc_variant_t
pcvcm_eval_native_wrapper_get_caller(purc_variant_t val);

purc_variant_t
pcvcm_eval_native_wrapper_get_param(purc_variant_t val);

purc_variant_t
pcvcm_eval_call_dvariant_method(purc_variant_t root,
        purc_variant_t var, size_t nr_args, purc_variant_t *argv,
        enum pcvcm_eval_method_type type, unsigned call_flags);

purc_variant_t
pcvcm_eval_call_nvariant_method(purc_variant_t var,
        const char *key_name, size_t nr_args, purc_variant_t *argv,
        enum pcvcm_eval_method_type type, unsigned call_flags);

purc_variant_t
pcvcm_eval_call_nvariant_getter(purc_variant_t var,
        const char *key_name, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags);

purc_variant_t
pcvcm_eval_call_nvariant_setter(purc_variant_t var,
        const char *key_name, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags);

bool
pcvcm_eval_is_handle_as_getter(struct pcvcm_node *node);

purc_variant_t pcvcm_eval_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt **ctxt_out, purc_variant_t args,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently);

purc_variant_t pcvcm_eval_again_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently, bool timeout);

purc_variant_t pcvcm_eval_sub_expr_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *ctxt, purc_variant_t args, bool silently);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined _VCM_EVAL_H */

