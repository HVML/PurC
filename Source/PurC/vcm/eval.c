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

#define PURC_ENVV_VCM_LOG_ENABLE    "PURC_VCM_LOG_ENABLE"

static const char *stepnames[] = {
    STEP_NAME_AFTER_PUSH,
    STEP_NAME_EVAL_PARAMS,
    STEP_NAME_EVAL_VCM,
    STEP_NAME_DONE
};

const char *
pcvcm_eval_stack_frame_step_name(enum pcvcm_eval_stack_frame_step type)
{
    return stepnames[type];
}

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
    return frame;

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
    if (ctxt->result) {
        purc_variant_unref(ctxt->result);
    }

    free(ctxt);
}

int
pcvcm_eval_ctxt_error_code(struct pcvcm_eval_ctxt *ctxt)
{
    return ctxt ? ctxt->err : 0;
}

#define DUMP_BUF_SIZE           128
#define MAX_LEVELS              1024
#define INDENT_UNIT             2

static ssize_t
print_indent(purc_rwstream_t rws, int level, size_t *len_expected)
{
    size_t n;
    char buff[MAX_LEVELS * 2];

    if (level <= 0 || level > MAX_LEVELS) {
        return 0;
    }

    n = level * INDENT_UNIT;
    memset(buff, ' ', n);
    if (len_expected) {
        *len_expected += n;
    }
    return purc_rwstream_write(rws, buff, n);
}

static char *
get_jsonee(struct pcvcm_node *node, size_t *len)
{
    /* TODO: used the origin jsonee */
    return pcvcm_node_serialize(node, len);
}

int
pcvcm_dump_frame(struct pcvcm_eval_stack_frame *frame, purc_rwstream_t rws,
        int level, int indent)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(rws);
    char buf[DUMP_BUF_SIZE];
    size_t len;

    print_indent(rws, indent, NULL);
    snprintf(buf, DUMP_BUF_SIZE, "#%02d: ", level);
    purc_rwstream_write(rws, buf, strlen(buf));

    char *jsonee = get_jsonee(frame->node, &len);
    purc_rwstream_write(rws, jsonee, len);
    purc_rwstream_write(rws, "\n", 1);
    free(jsonee);

#if __DEV_VCM__
    print_indent(rws, indent, NULL);
    snprintf(buf, DUMP_BUF_SIZE, "  step: %s\n",
            pcvcm_eval_stack_frame_step_name(frame->step));
    purc_rwstream_write(rws, buf, strlen(buf));
#endif

    print_indent(rws, indent, NULL);
    snprintf(buf, DUMP_BUF_SIZE, "  Variant Creation Model: ");
    purc_rwstream_write(rws, buf, strlen(buf));
    char *s = pcvcm_node_to_string(frame->node, &len);
    purc_rwstream_write(rws, s, len);
    purc_rwstream_write(rws, "\n", 1);
    free(s);

#if __DEV_VCM__
    for (size_t i = 0; i < frame->nr_params; i++) {
        print_indent(rws, indent, NULL);
        struct pcvcm_node *param = pcutils_array_get(frame->params, i);
        char *s = pcvcm_node_to_string(param, &len);

        if (i == frame->pos && frame->step == STEP_EVAL_PARAMS) {
            snprintf(buf, DUMP_BUF_SIZE, " >param_%02ld: ", i);
        }
        else {
            snprintf(buf, DUMP_BUF_SIZE, "  param_%02ld: ", i);
        }
        purc_rwstream_write(rws, buf, strlen(buf));
        purc_rwstream_write(rws, s, len);

        if (i < frame->pos) {
            purc_variant_t result = pcutils_array_get(frame->params_result, i);
            if (result) {
                const char *type = pcvariant_typename(result);
                snprintf(buf, DUMP_BUF_SIZE, ", result: %s/", type);
                purc_rwstream_write(rws, buf, strlen(buf));

                char *buf = pcvariant_to_string(result);
                purc_rwstream_write(rws, buf, strlen(buf));
                free(buf);
            }
        }

        purc_rwstream_write(rws, "\n", 1);

        free(s);
    }
#endif


    return 0;
}

int
pcvcm_dump_stack(struct pcvcm_eval_ctxt *ctxt, purc_rwstream_t rws,
        int indent, bool ignore_prefix)
{
    char buf[DUMP_BUF_SIZE];
    size_t len;
    struct list_head *stack = &ctxt->stack;

    char *s = get_jsonee(ctxt->node, &len);
    if (!ignore_prefix) {
        print_indent(rws, indent, NULL);
        snprintf(buf, DUMP_BUF_SIZE, "JSONEE: ");
        purc_rwstream_write(rws, buf, strlen(buf));
    }
    purc_rwstream_write(rws, s, len);
    purc_rwstream_write(rws, "\n", 1);
    free(s);

    /* vcm */
    print_indent(rws, indent, NULL);
    snprintf(buf, DUMP_BUF_SIZE, "  Variant Creation Model: ");
    purc_rwstream_write(rws, buf, strlen(buf));
    s = pcvcm_node_to_string(ctxt->node, &len);
    purc_rwstream_write(rws, s, len);
    purc_rwstream_write(rws, "\n", 1);
    free(s);

    if (ctxt->result) {
        print_indent(rws, indent, NULL);
        const char *type = pcvariant_typename(ctxt->result);
        if (ctxt->flags & PCVCM_EVAL_FLAG_SILENTLY) {
            snprintf(buf, DUMP_BUF_SIZE, "  Evaluated (silently) result: %s/",
                    type);
        }
        else {
            snprintf(buf, DUMP_BUF_SIZE, "  Evaluated result: %s/", type);
        }
        purc_rwstream_write(rws, buf, strlen(buf));

        char *buf = pcvariant_to_string(ctxt->result);
        purc_rwstream_write(rws, buf, strlen(buf));
        purc_rwstream_write(rws, "\n", 1);
        free(buf);
    }

    int err = ctxt->err;
    if (err) {
        print_indent(rws, indent, NULL);
        purc_atom_t except = purc_get_error_exception(err);
        const char *msg = purc_atom_to_string(except);
        if (ctxt->flags & PCVCM_EVAL_FLAG_SILENTLY) {
            snprintf(buf, DUMP_BUF_SIZE, "  Exception (ignored): ");
        }
        else {
            snprintf(buf, DUMP_BUF_SIZE, "  Exception: ");
        }
        purc_rwstream_write(rws, buf, strlen(buf));
        purc_rwstream_write(rws, msg, strlen(msg));
        purc_rwstream_write(rws, "\n", 1);
    }

    if (!list_empty(stack)) {
        print_indent(rws, indent, NULL);
        snprintf(buf, DUMP_BUF_SIZE, "  Call stack:\n");
        purc_rwstream_write(rws, buf, strlen(buf));

        struct pcvcm_eval_stack_frame *p, *n;
        int level = 0;
        list_for_each_entry_reverse_safe(p, n, stack, ln) {
            pcvcm_dump_frame(p, rws, level, indent + 2);
            level++;
        }
    }
    return 0;
}

void
pcvcm_print_stack(struct pcvcm_eval_ctxt *ctxt)
{
    purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUF_SIZE, MAX_BUF_SIZE);
    pcvcm_dump_stack(ctxt, rws, 0, false);

    char* buf = (char*) purc_rwstream_get_mem_buffer(rws, NULL);
    PLOG("\n%s\n", buf);
    purc_rwstream_destroy(rws);
}

unsigned
pcvcm_eval_ctxt_get_call_flags(struct pcvcm_eval_ctxt *ctxt)
{
    unsigned ret = PCVRT_CALL_FLAG_NONE;
    if (ctxt->flags & PCVCM_EVAL_FLAG_SILENTLY) {
        ret |= PCVRT_CALL_FLAG_SILENTLY;
    }

    if (ctxt->flags & PCVCM_EVAL_FLAG_AGAIN) {
        ret |= PCVRT_CALL_FLAG_AGAIN;
    }

    if (ctxt->flags & PCVCM_EVAL_FLAG_TIMEOUT) {
        ret |= PCVRT_CALL_FLAG_TIMEOUT;
    }
    return ret;
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
pcvcm_eval_native_wrapper_create(purc_variant_t caller_node,
        purc_variant_t param)
{
    purc_variant_t b = purc_variant_make_boolean(true);
    if (b == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (object == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_object_set_by_static_ckey(object, KEY_INNER_HANDLER, b);
    purc_variant_object_set_by_static_ckey(object, KEY_CALLER_NODE, caller_node);
    purc_variant_object_set_by_static_ckey(object, KEY_PARAM_NODE, param);
    purc_variant_unref(b);
    return object;
}

bool
pcvcm_eval_is_native_wrapper(purc_variant_t val)
{
    if (!val || !purc_variant_is_object(val)) {
        return false;
    }

    // FIXME: keep last error
    int err = purc_get_last_error();
    if (purc_variant_object_get_by_ckey(val, KEY_INNER_HANDLER)) {
        return true;
    }
    purc_set_error(err);
    return false;
}

purc_variant_t
pcvcm_eval_native_wrapper_get_caller(purc_variant_t val)
{
    return purc_variant_object_get_by_ckey(val, KEY_CALLER_NODE);
}

purc_variant_t
pcvcm_eval_native_wrapper_get_param(purc_variant_t val)
{
    return purc_variant_object_get_by_ckey(val, KEY_PARAM_NODE);
}


purc_variant_t
pcvcm_eval_call_dvariant_method(purc_variant_t root,
        purc_variant_t var, size_t nr_args, purc_variant_t *argv,
        enum pcvcm_eval_method_type type, unsigned call_flags)
{
    purc_dvariant_method func = (type == GETTER_METHOD) ?
         purc_variant_dynamic_get_getter(var) :
         purc_variant_dynamic_get_setter(var);
    if (func) {
        return func(root, nr_args, argv, call_flags);
    }
    return PURC_VARIANT_INVALID;
}

purc_variant_t
pcvcm_eval_call_nvariant_method(purc_variant_t var,
        const char *key_name, size_t nr_args, purc_variant_t *argv,
        enum pcvcm_eval_method_type type, unsigned call_flags)
{
    struct purc_native_ops *ops = purc_variant_native_get_ops(var);
    if (ops) {
        void *entity = purc_variant_native_get_entity(var);
        purc_nvariant_method native_func = (type == GETTER_METHOD) ?
            ops->property_getter(entity, key_name) :
            ops->property_setter(entity, key_name);
        if (native_func) {
            return  native_func(entity, nr_args, argv, call_flags);
        }
    }
    return PURC_VARIANT_INVALID;
}

static bool
is_action_node(struct pcvcm_node *node)
{
    return (node && (
                node->type == PCVCM_NODE_TYPE_FUNC_GET_ELEMENT ||
                node->type == PCVCM_NODE_TYPE_FUNC_CALL_GETTER ||
                node->type == PCVCM_NODE_TYPE_FUNC_CALL_SETTER
                )
            );
}

bool
pcvcm_eval_is_handle_as_getter(struct pcvcm_node *node)
{
    struct pcvcm_node *parent_node = (struct pcvcm_node *)
        pctree_node_parent(&node->tree_node);
    struct pcvcm_node *first_child = pcvcm_node_first_child(parent_node);
    if (is_action_node(parent_node) && first_child == node) {
        return false;
    }
    return true;
}

static bool
has_fatal_error(int err)
{
    return (err == PURC_ERROR_OUT_OF_MEMORY);
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
    int err = 0;

#if 0
    if (ctxt->enable_log) {
        pcvcm_print_stack(ctxt);
    }
#endif

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
                    purc_variant_t v = pcutils_array_get(frame->params_result, frame->pos);
                    if (v) {
                        continue;
                    }
                    param = frame->ops->select_param(ctxt, frame, frame->pos);
                    if (!param) {
                        if (frame->step == STEP_EVAL_PARAMS) {
                            continue;
                        }
                        ctxt->err = purc_get_last_error();
                        if (ctxt->err != 0) {
                            goto out;
                        }
                        break;
                    }
                    param_frame = push_frame(ctxt, param, frame->pos);
                    if (!param_frame) {
                        goto out;
                    }

                    val = eval_frame(ctxt, param_frame, frame->pos);
                    if (!val) {
                        goto out;
                    }
                    pcutils_array_set(frame->params_result, param_frame->return_pos, val);
                    pop_frame(ctxt);
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
    ctxt->err = purc_get_last_error();
    if ((result == PURC_VARIANT_INVALID) &&
            (ctxt->err != PURC_ERROR_AGAIN) &&
            (ctxt->flags & PCVCM_EVAL_FLAG_SILENTLY) &&
            !has_fatal_error(err)) {
        result = purc_variant_make_undefined();
    }
    frame->node->attach = (uintptr_t)result;

#if 0
    if (ctxt->enable_log) {
        pcvcm_print_stack(ctxt);
    }
#endif

    if (ctxt->enable_log && ctxt) {
        pcintr_coroutine_t co = pcintr_get_coroutine();
        size_t len;
        char *s = get_jsonee(frame->node, &len);
        if (result) {
            char *buf = pcvariant_to_string(result);
            PLOG("co=%d|vcm=%s|frame=%p|pos=%ld|nr=%ld\n", co ? co->cid : 0, s, frame, frame->pos, frame->nr_params);
            PLOG("ret=%s\n", buf);
            free(buf);
        }
        else {
            PLOG("co=%d|vcm=%s|frame=%p|pos=%ld|nr=%ld\n", co ? co->cid : 0, s, frame, frame->pos, frame->nr_params);
            PLOG("ret=null\n");
        }
        free(s);
    }
    return result;
}

purc_variant_t
eval_vcm(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently, bool timeout, bool again)
{
    purc_variant_t result = PURC_VARIANT_INVALID;
    struct pcvcm_eval_stack_frame *frame;

    ctxt->find_var = find_var;
    ctxt->find_var_ctxt = find_var_ctxt;
    if (silently) {
        ctxt->flags |= PCVCM_EVAL_FLAG_SILENTLY;
    }

    if (timeout) {
        ctxt->flags |= PCVCM_EVAL_FLAG_TIMEOUT;
    }

    if (again) {
        ctxt->flags |= PCVCM_EVAL_FLAG_AGAIN;
        frame = bottom_frame(ctxt);
    }
    else {
        frame = push_frame(ctxt, tree, 0);
    }

    if (!frame) {
        goto out;
    }

    do {
        size_t return_pos = frame->return_pos;
        result = eval_frame(ctxt, frame, return_pos);
        ctxt->err = purc_get_last_error();
        if (!result || ctxt->err) {
            goto out;
        }
        pop_frame(ctxt);
        frame = bottom_frame(ctxt);
        if (frame) {
            pcutils_array_set(frame->params_result, return_pos, result);
        }
    } while (frame);

out:
    if (result) {
        if (ctxt->result) {
            purc_variant_unref(ctxt->result);
        }
        ctxt->result = purc_variant_ref(result);
    }
#if 0
    if (ctxt->enable_log) {
        pcvcm_print_stack(ctxt);
    }
#endif
    return result;
}

static int i = 0;
purc_variant_t pcvcm_eval_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt **ctxt_out,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently)
{
    purc_variant_t result = PURC_VARIANT_INVALID;
    struct pcvcm_eval_ctxt *ctxt = NULL;
    unsigned int enable_log = 0;
    const char *env_value;
    int err;

    if ((env_value = getenv(PURC_ENVV_VCM_LOG_ENABLE))) {
        enable_log = (*env_value == '1' ||
                pcutils_strcasecmp(env_value, "true") == 0);
    }

    if (enable_log) {
        PLOG("begin vcm\n");
    }

    purc_clr_error();

    if (!tree) {
        result = silently ? purc_variant_make_undefined() :
            PURC_VARIANT_INVALID;
        goto out;
    }

    ctxt = pcvcm_eval_ctxt_create();
    if (!ctxt) {
        goto out;
    }
    ctxt->enable_log = enable_log;
    ctxt->node = tree;

    result = eval_vcm(tree, ctxt, find_var, find_var_ctxt, silently,
            false, false);

out:
    err = purc_get_last_error();
    if (!result && silently) {
        if (err == PURC_ERROR_AGAIN && ctxt_out) {
            result = PURC_VARIANT_INVALID;
        }
        result = purc_variant_make_undefined();
    }

    if (ctxt) {
        ctxt->err = err;
    }

    if (enable_log && ctxt) {
        pcintr_coroutine_t co = pcintr_get_coroutine();
        size_t len;
        char *s = get_jsonee(ctxt->node, &len);
        if (result) {
            char *buf = pcvariant_to_string(result);
            PLOG("co=%d|vcm=%s\n", co ? co->cid : 0, s);
            PLOG("ret=%s\n", buf);
            free(buf);
        }
        else {
            PLOG("co=%d|vcm=%s\n", co ? co->cid : 0, s);
            PLOG("ret=null\n");
        }
        free(s);
        PLOG("end %d\n\n", i++);
    }

    if (err && ctxt_out) {
        *ctxt_out = ctxt;
    }
    else if (ctxt) {
        pcvcm_eval_ctxt_destroy(ctxt);
    }
    return result;
}

purc_variant_t pcvcm_eval_again_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently, bool timeout)
{
    purc_variant_t result = PURC_VARIANT_INVALID;
    unsigned int enable_log = 0;
    const char *env_value;

    if ((env_value = getenv(PURC_ENVV_VCM_LOG_ENABLE))) {
        enable_log = (*env_value == '1' ||
                pcutils_strcasecmp(env_value, "true") == 0);
    }

    if (enable_log) {
        PLOG("begin vcm again\n");
    }

    if (!ctxt) {
        goto out;
    }
    ctxt->enable_log = enable_log;

    /* clear AGAIN error */
    ctxt->err = purc_get_last_error();
    if (ctxt->err == PURC_ERROR_AGAIN) {
        ctxt->err = 0;
        purc_clr_error();
    }

    result = eval_vcm(tree, ctxt, find_var, find_var_ctxt, silently,
            timeout, true);

out:
    if (enable_log) {
        size_t len;
        pcintr_coroutine_t co = pcintr_get_coroutine();
        char *s = get_jsonee(ctxt->node, &len);
        if (result) {
            char *buf = pcvariant_to_string(result);
            PLOG("co=%d|vcm=%s\n", co ? co->cid : 0, s);
            PLOG("ret=%s\n", buf);
            free(buf);
        }
        else {
            PLOG("co=%d|vcm=%s\n", co ? co->cid : 0, s);
            PLOG("ret=null|err=%s\n", purc_get_error_message(purc_get_last_error()));
        }
        free(s);
        PLOG("end %d\n\n", i++);
    }
    return result;
}


