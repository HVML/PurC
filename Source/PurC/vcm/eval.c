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
#include "private/tkz-helper.h"

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

struct pcvcm_eval_ctxt *
pcvcm_eval_ctxt_create()
{
    struct pcvcm_eval_ctxt *ctxt;
    ctxt = (struct pcvcm_eval_ctxt*)calloc(1,sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    ctxt->free_on_destroy = 1;
out:
    return ctxt;
}

struct pcvcm_eval_ctxt *
pcvcm_eval_ctxt_dup(struct pcvcm_eval_ctxt *src)
{
    struct pcvcm_eval_ctxt *ctxt = pcvcm_eval_ctxt_create();
    if (!ctxt) {
        goto out;
    }
    *ctxt = *src;

    size_t nr_bytes = ctxt->nr_eval_nodes * sizeof(struct pcvcm_eval_node);
    ctxt->eval_nodes = (struct pcvcm_eval_node *) malloc(nr_bytes);
    memcpy(ctxt->eval_nodes, src->eval_nodes, nr_bytes);

    nr_bytes = ctxt->nr_frames * sizeof(struct pcvcm_eval_stack_frame);
    ctxt->frames = (struct pcvcm_eval_stack_frame *) malloc(nr_bytes);
    memcpy(ctxt->frames, src->frames, nr_bytes);

#ifdef PCVCM_KEEP_NAME
    nr_bytes = ctxt->nr_frames * sizeof(char *);
    ctxt->names = (const char **) malloc (nr_bytes);
    memcpy(ctxt->names, src->names, nr_bytes);
#endif

    ctxt->free_on_destroy = 1;
out:
    return ctxt;
}

void
pcvcm_eval_ctxt_destroy(struct pcvcm_eval_ctxt *ctxt)
{
    if (!ctxt) {
        return;
    }
    if (ctxt->result) {
        purc_variant_unref(ctxt->result);
    }

    for (size_t i = 0; i < ctxt->nr_eval_nodes; i++) {
        struct pcvcm_eval_node *p = ctxt->eval_nodes + i;
        if (p->result) {
            purc_variant_unref(p->result);
        }
    }

    for (int32_t i = 0; i < ctxt->frame_idx; i++) {
        struct pcvcm_eval_stack_frame *p = ctxt->frames + i;
        if (p->args) {
            purc_variant_unref(p->args);
        }
    }

    if (ctxt->free_on_destroy) {
        free(ctxt->eval_nodes);
        free(ctxt->frames);
#ifdef PCVCM_KEEP_NAME
        free(ctxt->names);
#endif
        free(ctxt);
    }
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
get_jsonee(struct pcvcm_node *node, size_t *len, struct pcvcm_node *err_node,
        char **err_msg, size_t *nr_err_msg)
{
#if 0
    char *s = NULL;
    if (node->ucs) {
        s = tkz_ucs_to_string(node->ucs, len);
    }
    return s ? s : pcvcm_node_serialize(node, len);
#else
    return pcvcm_node_serialize_ex(node, len, err_node, err_msg, nr_err_msg);
#endif
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

    char *jsonee = get_jsonee(frame->node, &len, NULL, NULL, NULL);
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
        struct pcvcm_node *param = frame->ops->select_param(i);
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
            purc_variant_t result = pcvcm_get_frame_result(ctxt, frame_idx, i, NULL);
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

    purc_rwstream_write(rws, "<<<<\n", 5);

    char *err_msg = NULL;
    size_t nr_err_msg = 0;
    char *s = get_jsonee(ctxt->node, &len, ctxt->err_node, &err_msg, &nr_err_msg);
    if (!ignore_prefix) {
        print_indent(rws, indent, NULL);
        snprintf(buf, DUMP_BUF_SIZE, "JSONEE: ");
        purc_rwstream_write(rws, buf, strlen(buf));
    }
    purc_rwstream_write(rws, s, len);
    purc_rwstream_write(rws, "\n", 1);
    purc_rwstream_write(rws, err_msg, nr_err_msg);
    purc_rwstream_write(rws, "\n", 1);
    free(s);
    free(err_msg);

    purc_rwstream_write(rws, "====\n", 5);

    /* vcm */
    const char *title = "The equivalent variant creation model:\n";
    purc_rwstream_write(rws, title, strlen(title));
    err_msg = NULL;
    nr_err_msg = 0;
    s = pcvcm_node_to_string_ex(ctxt->node, &len, ctxt->err_node, &err_msg,
            &nr_err_msg);
    purc_rwstream_write(rws, s, len);
    purc_rwstream_write(rws, "\n", 1);
    purc_rwstream_write(rws, err_msg, nr_err_msg);
    purc_rwstream_write(rws, "\n", 1);
    free(s);
    free(err_msg);

    purc_rwstream_write(rws, ">>>>\n", 5);

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

#if 0
    /* 25-04-08 disable not impl call stack */
    if (ctxt->frame_idx >= 0) {
        print_indent(rws, indent, NULL);
        snprintf(buf, DUMP_BUF_SIZE, "  Call stack:\n");
        purc_rwstream_write(rws, buf, strlen(buf));

#if 0
        int level = 0;
        for (int i = ctxt->frame_idx; i >= 0; i++) {
            struct pcvcm_eval_stack_frame *p = ctxt->frames + i;
            pcvcm_dump_frame(p, rws, level, indent + 2);
            level++;
        }
#endif
    }
#endif
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
    if (ctxt->frame_idx < 0) {
        return NULL;
    }
    return ctxt->frames + ctxt->frame_idx;
}

static struct pcvcm_eval_stack_frame *
push_frame(struct pcvcm_eval_ctxt *ctxt, struct pcvcm_eval_node *node,
        size_t return_pos)
{
    int idx = ++ctxt->frame_idx;
    struct pcvcm_eval_stack_frame *frame = ctxt->frames + idx;
    frame->idx = ctxt->frame_idx;
    frame->eval_node_idx = node->idx;
    frame->node = node->node;
    frame->pos = 0;
    frame->return_pos = return_pos;
    frame->nr_params = pcvcm_node_children_count(frame->node);
    frame->ops = pcvcm_eval_get_ops_by_node(frame->node);
    frame->args = PURC_VARIANT_INVALID;
    frame->step = STEP_AFTER_PUSH;
    return frame;
}

static void
pop_frame(struct pcvcm_eval_ctxt *ctxt)
{
    if (ctxt->frame_idx < 0) {
        return;
    }

    struct pcvcm_eval_stack_frame *frame = ctxt->frames + ctxt->frame_idx;
    if (frame->args) {
        purc_variant_unref(frame->args);
        frame->args = PURC_VARIANT_INVALID;
    }
    ctxt->frame_idx--;
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

    if (purc_variant_object_get_by_ckey_ex(val, KEY_INNER_HANDLER, true)) {
        return true;
    }

    return false;
}

purc_variant_t
pcvcm_eval_native_wrapper_get_caller(purc_variant_t val)
{
    return purc_variant_object_get_by_ckey_ex(val, KEY_CALLER_NODE, false);
}

purc_variant_t
pcvcm_eval_native_wrapper_get_param(purc_variant_t val)
{
    return purc_variant_object_get_by_ckey_ex(val, KEY_PARAM_NODE, false);
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
            return native_func(entity, key_name, nr_args, argv, call_flags);
        }
    }
    return PURC_VARIANT_INVALID;
}

purc_variant_t
pcvcm_eval_call_nvariant_getter(purc_variant_t var,
        const char *key_name, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    struct purc_native_ops *ops = purc_variant_native_get_ops(var);
    UNUSED_PARAM(key_name);

    if (ops && ops->property_getter) {
        void *entity = purc_variant_native_get_entity(var);
        purc_nvariant_method self_getter;
        self_getter = ops->property_getter(entity, NULL);
        if (self_getter) {
            return self_getter(entity, NULL, nr_args, argv, call_flags);
        }
    }

    return PURC_VARIANT_INVALID;

#if 0
    if (ops && ops->getter) {
        void *entity = purc_variant_native_get_entity(var);
        return ops->getter(entity, key_name, nr_args, argv, call_flags);
    }
    return PURC_VARIANT_INVALID;
#endif
}

purc_variant_t
pcvcm_eval_call_nvariant_setter(purc_variant_t var,
        const char *key_name, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    struct purc_native_ops *ops = purc_variant_native_get_ops(var);
    UNUSED_PARAM(key_name);

    if (ops && ops->property_setter) {
        void *entity = purc_variant_native_get_entity(var);
        purc_nvariant_method self_setter;
        self_setter = ops->property_setter(entity, NULL);
        if (self_setter) {
            return self_setter(entity, NULL, nr_args, argv, call_flags);
        }
    }
    return PURC_VARIANT_INVALID;

#if 0
    if (ops && ops->setter) {
        void *entity = purc_variant_native_get_entity(var);
        return ops->setter(entity, key_name, nr_args, argv, call_flags);
    }
    return PURC_VARIANT_INVALID;
#endif
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
eval_frame(struct pcvcm_eval_ctxt *ctxt, int32_t frame_idx, size_t return_pos,
        const char **name)
{
    UNUSED_PARAM(return_pos);
    purc_variant_t result = PURC_VARIANT_INVALID;
    purc_variant_t val;
    struct pcvcm_eval_stack_frame *param_frame;
    struct pcvcm_eval_node *param;
    int ret = 0;
    int err = 0;
    struct pcvcm_eval_stack_frame frame_tmp = ctxt->frames[frame_idx];
    struct pcvcm_eval_stack_frame *frame = &frame_tmp;

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
                    int err = purc_get_last_error();
                    if (err && err != PURC_ERROR_AGAIN && !ctxt->err_node) {
                        ctxt->err_node = frame->node;
                    }
                    goto out;
                }
                frame->step = STEP_EVAL_PARAMS;
                ctxt->frames[frame_idx] = frame_tmp;
                break;

            case STEP_EVAL_PARAMS:
                for (; frame->pos < frame->nr_params; frame->pos++) {
                    purc_variant_t v = pcvcm_get_frame_result(ctxt, frame_idx,
                            frame->pos, NULL);
                    if (v) {
                        continue;
                    }
                    param = frame->ops->select_param(ctxt, frame, frame->pos);
                    if (!param) {
                        if (frame->step == STEP_EVAL_PARAMS) {
                            continue;
                        }
                        ctxt->err = purc_get_last_error();
                        if (ctxt->err) {
                            if (ctxt->err != PURC_ERROR_AGAIN &&
                                    !ctxt->err_node) {
                                ctxt->err_node = frame->node;
                            }
                            goto out;
                        }
                        break;
                    }
                    param_frame = push_frame(ctxt, param, frame->pos);
                    if (!param_frame) {
                        goto out;
                    }

                    const char *name = NULL;
                    val = eval_frame(ctxt, param_frame->idx, frame->pos, &name);
                    if (!val) {
                        goto out;
                    }
                    pcvcm_set_frame_result(ctxt, frame_idx,
                            param_frame->return_pos, val, name);
                    pop_frame(ctxt);
                }
                frame->step = STEP_EVAL_VCM;
                ctxt->frames[frame_idx] = frame_tmp;
                break;

            case STEP_EVAL_VCM:
                result = frame->ops->eval(ctxt, frame, name);
                if (!result) {
                    int err = purc_get_last_error();
                    if (err && err != PURC_ERROR_AGAIN && !ctxt->err_node) {
                        ctxt->err_node = frame->node;
                    }
                    goto out;
                }
                frame->step = STEP_DONE;
                ctxt->frames[frame_idx] = frame_tmp;
                break;

            case STEP_DONE:
                break;
        }
    }

out:
    ctxt->frames[frame_idx] = frame_tmp;
    ctxt->err = purc_get_last_error();
    if ((result == PURC_VARIANT_INVALID) &&
            (ctxt->err != PURC_ERROR_AGAIN) &&
            (ctxt->flags & PCVCM_EVAL_FLAG_SILENTLY) &&
            !has_fatal_error(err)) {
        result = purc_variant_make_undefined();
    }

#if 0
    if (ctxt->enable_log) {
        pcvcm_print_stack(ctxt);
    }
#endif

    if (ctxt->enable_log && ctxt) {
        pcintr_coroutine_t co = pcintr_get_coroutine();
        size_t len;
        char *s = get_jsonee(frame->node, &len, NULL, NULL, NULL);
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

static purc_variant_t
eval_vcm(struct pcvcm_eval_node *tree,
        struct pcvcm_eval_ctxt *ctxt, purc_variant_t args,
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

    if (args) {
        PURC_VARIANT_SAFE_CLEAR(frame->args);
        frame->args = purc_variant_ref(args);
    }

    do {
        const char *name = NULL;
        size_t return_pos = frame->return_pos;
        result = eval_frame(ctxt, frame->idx, return_pos, &name);
        ctxt->err = purc_get_last_error();
        if (!result || ctxt->err) {
            if (ctxt->err && ctxt->err != PURC_ERROR_AGAIN && !ctxt->err_node) {
                ctxt->err_node = frame->node;
            }
            goto out;
        }
        pop_frame(ctxt);
        frame = bottom_frame(ctxt);
        if (frame) {
            pcvcm_set_frame_result(ctxt, frame->idx, return_pos, result, name);
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

static void assign_idx_cb(struct pctree_node *node,  void *data)
{
    struct pcvcm_node *n = (struct pcvcm_node *)node;
    int *idx = (int *)data;
    n->idx = (*idx)++;
}

static void build_eval_nodes_cb(struct pctree_node *node, void *data)
{
    struct pcvcm_eval_ctxt *ctxt = (struct pcvcm_eval_ctxt *) data;
    struct pcvcm_eval_node *p = ctxt->eval_nodes + ctxt->eval_nodes_insert_pos;
    p->node = (struct pcvcm_node *)node;
    p->idx = ctxt->eval_nodes_insert_pos;
    p->result = PURC_VARIANT_INVALID;
    p->first_child_idx = -1;
    ctxt->eval_nodes_insert_pos++;
}

static void build_eval_node_children(struct pctree_node *node, void *data)
{
    if (!node->first_child) {
        return;
    }

    struct pcvcm_eval_ctxt *ctxt = (struct pcvcm_eval_ctxt *) data;
    int32_t pos = ctxt->eval_nodes_insert_pos;
    for (int32_t i = pos - 1 ; i >= 0; i--) {
        struct pcvcm_eval_node *p = ctxt->eval_nodes + i;
        if (p->node == (struct pcvcm_node *)node) {
            p->first_child_idx = pos;
            break;
        }
    }

    pctree_node_children_for_each(node, build_eval_nodes_cb, data);
    pctree_node_children_for_each(node, build_eval_node_children, data);
}

static void build_eval_nodes(struct pcvcm_eval_ctxt *ctxt,
        struct pcvcm_node *node)
{
    struct pcvcm_eval_node *p = ctxt->eval_nodes + ctxt->eval_nodes_insert_pos;
    p->node = (struct pcvcm_node *)node;
    p->idx = ctxt->eval_nodes_insert_pos;
    p->result = PURC_VARIANT_INVALID;
    ctxt->eval_nodes_insert_pos++;
    build_eval_node_children(&node->tree_node, ctxt);
}

static bool _init_by_env = false;
static bool _enable_log = false;

static bool
is_log_enable()
{
    if (!_init_by_env) {
        _init_by_env = true;
        const char *env_value;
        if ((env_value = getenv(PURC_ENVV_VCM_LOG_ENABLE))) {
            _enable_log = (*env_value == '1' ||
                    pcutils_strcasecmp(env_value, "true") == 0);
        }
    }
    return _enable_log;
}


static int i = 0;
purc_variant_t pcvcm_eval_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt **ctxt_out, purc_variant_t args,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently)
{
    purc_variant_t result = PURC_VARIANT_INVALID;
    struct pcvcm_eval_ctxt contxt = {0};
    struct pcvcm_eval_ctxt *ctxt = &contxt;
    unsigned int enable_log = is_log_enable();
    int err;
    int32_t nr_nodes = 0;

    if (enable_log) {
        size_t len;
        char *s = pcvcm_node_to_string(tree, &len);
        PLOG("begin vcm : %s\n", s);
        free(s);
    }

    purc_clr_error();

    if (!tree) {
        result = silently ? purc_variant_make_undefined() :
            PURC_VARIANT_INVALID;
    }
    else {
        if (tree->nr_nodes == -1) {
            int idx = 0;
            pctree_node_level_order_traversal(&tree->tree_node, assign_idx_cb,
                    &idx);
            tree->nr_nodes = idx;
        }
        nr_nodes = tree->nr_nodes;
    }

    struct pcvcm_eval_node eval_nodes[nr_nodes];
    struct pcvcm_eval_stack_frame frames[nr_nodes];
#ifdef PCVCM_KEEP_NAME
    const char *names[nr_nodes];
#endif

    if (nr_nodes) {
        ctxt->enable_log = enable_log;
        ctxt->node = tree;
        ctxt->frame_idx = -1;
        ctxt->nr_eval_nodes = nr_nodes;
        ctxt->eval_nodes = eval_nodes;
        ctxt->nr_frames = nr_nodes;
        ctxt->frames = frames;
#ifdef PCVCM_KEEP_NAME
        ctxt->names = names;
        memset(names, 0, sizeof(names));
#endif

        build_eval_nodes(ctxt, tree);

        result = eval_vcm(ctxt->eval_nodes, ctxt, args, find_var, find_var_ctxt, silently,
                false, false);
    }

    err = purc_get_last_error();
    if (result && !silently && err) {
        purc_clr_error();
    }
    else if (!result && silently) {
        if (err == PURC_ERROR_AGAIN && ctxt_out) {
            result = PURC_VARIANT_INVALID;
        }
        else {
            result = purc_variant_make_undefined();
        }
    }

    if (ctxt) {
        ctxt->err = err;
    }

    if (enable_log && ctxt) {
        pcintr_coroutine_t co = pcintr_get_coroutine();
        size_t len;
        char *s = get_jsonee(ctxt->node, &len, NULL, NULL, NULL);
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
        *ctxt_out = pcvcm_eval_ctxt_dup(ctxt);
    }
    else if (ctxt) {
        pcvcm_eval_ctxt_destroy(ctxt);
        if (ctxt_out) {
            *ctxt_out = NULL;
        }
    }
    return result;
}

purc_variant_t pcvcm_eval_again_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *ctxt,
        find_var_fn find_var, void *find_var_ctxt,
        bool silently, bool timeout)
{
    purc_variant_t result = PURC_VARIANT_INVALID;
    unsigned int enable_log = is_log_enable();

    if (enable_log) {
        size_t len;
        char *s = pcvcm_node_to_string(tree, &len);
        PLOG("begin vcm again : %s\n", s);
        free(s);
    }

    if (ctxt) {
        ctxt->enable_log = enable_log;
        if (ctxt->node->nr_nodes == -1) {
            int idx = 0;
            pctree_node_level_order_traversal(&ctxt->node->tree_node, assign_idx_cb,
                    &idx);
            tree->nr_nodes = idx;
        }

        /* clear AGAIN error */
        ctxt->err = purc_get_last_error();
        if (ctxt->err == PURC_ERROR_AGAIN) {
            ctxt->err = 0;
            ctxt->err_node = NULL;
            purc_clr_error();
        }

        result = eval_vcm(ctxt->eval_nodes, ctxt, PURC_VARIANT_INVALID, find_var,
                find_var_ctxt, silently, timeout, true);
    }

    if (enable_log) {
        size_t len;
        pcintr_coroutine_t co = pcintr_get_coroutine();
        char *s = get_jsonee(ctxt->node, &len, NULL, NULL, NULL);
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


purc_variant_t pcvcm_eval_sub_expr_full(struct pcvcm_node *tree,
        struct pcvcm_eval_ctxt *ctxt, purc_variant_t args, bool silently)
{
    UNUSED_PARAM(silently);

    purc_variant_t result = PURC_VARIANT_INVALID;
    if (!ctxt) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }
    else {
        if (tree->nr_nodes == -1) {
            int idx = 0;
            pctree_node_level_order_traversal(&tree->tree_node, assign_idx_cb,
                    &idx);
            tree->nr_nodes = idx;
        }

        ctxt->nr_eval_nodes = ctxt->nr_eval_nodes + tree->nr_nodes;
        ctxt->eval_nodes = (struct pcvcm_eval_node *) realloc(ctxt->eval_nodes,
                ctxt->nr_eval_nodes * sizeof(struct pcvcm_eval_node));


        if (ctxt->nr_frames < ctxt->nr_eval_nodes) {
            ctxt->nr_frames = ctxt->nr_eval_nodes;
            ctxt->frames = (struct pcvcm_eval_stack_frame *) realloc(ctxt->frames,
                    ctxt->nr_frames * sizeof(struct pcvcm_eval_stack_frame));

#ifdef PCVCM_KEEP_NAME
            ctxt->names = (const char **) realloc(ctxt->names,
                    ctxt->nr_frames * sizeof(char *));
#endif
        }

        size_t pos = ctxt->eval_nodes_insert_pos;
        build_eval_nodes(ctxt, tree);

        struct pcvcm_eval_stack_frame *frame = push_frame(ctxt,
                ctxt->eval_nodes + pos, 0);
        if (!frame) {
            goto out;
        }

        if (args) {
            PURC_VARIANT_SAFE_CLEAR(frame->args);
            frame->args = purc_variant_ref(args);
        }

        const char *name = NULL;
        result = eval_frame(ctxt, frame->idx, 0, &name);

        pop_frame(ctxt);
    }


out:
    return result;
}

