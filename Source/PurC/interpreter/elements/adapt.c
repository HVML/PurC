/**
 * @file adapt.c
 * @author Xue Shuming
 * @date 2025/06/09
 * @brief
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
 *
 */

#include "purc-variant.h"
#include "purc.h"
#include "../internal.h"

#include "private/debug.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

struct ctxt_for_adapt {
    struct pcvdom_node           *curr;
    struct pcvcm_node            *content_vcm;
    struct pcvcm_node            *tpl_vcm;

    purc_variant_t                tpl_native;
    purc_variant_t                on;
    bool                          individually;
};

static void
ctxt_for_adapt_destroy(struct ctxt_for_adapt *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_adapt_destroy((struct ctxt_for_adapt*)ctxt);
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        struct pcvdom_attr *attr,
        size_t idx,
        void *ud)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(idx);
    UNUSED_PARAM(ud);

    struct ctxt_for_adapt *ctxt;
    ctxt = (struct ctxt_for_adapt*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        ctxt->on = pcintr_eval_vcm((pcintr_stack_t)ud, attr->val,
                frame->silently);
        if (ctxt->on == PURC_VARIANT_INVALID) {
            return -1;
        }
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        ctxt->tpl_vcm = attr->val;
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, INDIVIDUALLY)) == name) {
        ctxt->individually = true;
        return 0;
    }

    /* ignore other attr */
    return 0;
}

static int
eval_attr(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    pcutils_array_t *attrs = frame->pos->attrs;
    size_t nr_params = pcutils_array_length(attrs);
    struct pcvdom_attr *attr = NULL;
    purc_atom_t name = 0;
    int err = 0;

    for (; frame->eval_attr_pos < nr_params; frame->eval_attr_pos++) {
        attr = pcutils_array_get(attrs, frame->eval_attr_pos);
        name = PCHVML_KEYWORD_ATOM(HVML, attr->key);
        err = attr_found_val(frame, frame->pos, name, attr,
                frame->eval_attr_pos, stack);
        if (err) {
            goto out;
        }
    }

out:
    return err;
}

static int
eval_content(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(stack);
    struct pcvdom_node *node = &frame->pos->node;
    node = pcvdom_node_first_child(node);
    if (!node || node->type != PCVDOM_NODE_CONTENT) {
        purc_clr_error();
        goto out;
    }

    struct pcvdom_content *content = PCVDOM_CONTENT_FROM_NODE(node);
    struct ctxt_for_adapt *ctxt = frame->ctxt;
    ctxt->content_vcm = content->vcm;

out:
    return 0;
}

static purc_variant_t eval_tpl_vcm(pcintr_stack_t stack,
                                   struct pcintr_stack_frame* frame,
                                   struct ctxt_for_adapt* ctxt)
{
    if (ctxt->tpl_native) {
        return pcintr_template_expansion(ctxt->tpl_native, frame->silently);
    }
    purc_variant_t val = pcintr_eval_vcm(stack, ctxt->tpl_vcm, frame->silently);
    if (val && purc_variant_is_native(val)) {
        ctxt->tpl_native = purc_variant_ref(val);
        purc_variant_unref(val);
        return pcintr_template_expansion(ctxt->tpl_native, frame->silently);
    }
    return val;
}

static int process_ds_as_single(pcintr_stack_t stack,
                               struct pcintr_stack_frame *frame,
                               struct ctxt_for_adapt *ctxt)
{
    int ret = 0;
    struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(frame);
    assert(parent);
    purc_variant_t origin_val = pcintr_get_question_var(parent);
    purc_variant_ref(origin_val);

    pcintr_set_question_var(parent, ctxt->on);
    purc_variant_t val = eval_tpl_vcm(stack, frame, ctxt);
    if (val == PURC_VARIANT_INVALID) {
        ret = -1;
        goto out;
    }

    pcintr_set_question_var(frame, val);
    purc_variant_unref(val);

out:
    pcintr_set_question_var(parent, origin_val);
    purc_variant_unref(origin_val);
    return ret;
}

static int
process_ds_array(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
     struct ctxt_for_adapt *ctxt)
{
    int ret = -1;
    struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(frame);
    assert(parent);
    purc_variant_t origin_val = pcintr_get_question_var(parent);
    purc_variant_ref(origin_val);

    purc_variant_t ret_val = purc_variant_make_array_0();
    if (ret_val == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    size_t nr_size = purc_variant_array_get_size(ctxt->on);
    for (size_t i = 0; i < nr_size; ++i) {
        purc_variant_t v = purc_variant_array_get(ctxt->on, i);
        pcintr_set_question_var(parent, v);
        purc_variant_t val = eval_tpl_vcm(stack, frame, ctxt);
        if (val == PURC_VARIANT_INVALID) {
            goto out;
        }
        purc_variant_array_append(ret_val, val);
        purc_variant_unref(val);
    }
    pcintr_set_question_var(frame, ret_val);
    purc_variant_unref(ret_val);
    ret = 0;

out:
    if (origin_val) {
        pcintr_set_question_var(parent, origin_val);
        purc_variant_unref(origin_val);
    }
    return ret;
}

static int
process_ds_set(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
     struct ctxt_for_adapt *ctxt)
{
    int ret = -1;
    struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(frame);
    assert(parent);
    purc_variant_t origin_val = pcintr_get_question_var(parent);
    purc_variant_ref(origin_val);

    const char *keys = NULL;
    purc_variant_set_unique_keys(ctxt->on, &keys);
    size_t nr_size = purc_variant_set_get_size(ctxt->on);
    purc_variant_t ret_val = PURC_VARIANT_INVALID;

    for (size_t i = 0; i < nr_size; ++i) {
        purc_variant_t v = purc_variant_set_get_by_index(ctxt->on, i);
        pcintr_set_question_var(parent, v);
        purc_variant_t val = eval_tpl_vcm(stack, frame, ctxt);
        if (val == PURC_VARIANT_INVALID) {
            goto out;
        }
        if (ret_val) {
            if (purc_variant_is_set(ret_val)) {
                purc_variant_set_add(ret_val, val, PCVRNT_CR_METHOD_OVERWRITE);
            }
            else {
                purc_variant_array_append(ret_val, val);
            }
            purc_variant_unref(val);
            continue;
        }

        if (keys && purc_variant_is_object(val) &&
            purc_variant_object_get_by_ckey(val, keys)) {
            ret_val = purc_variant_make_set_by_ckey(0, keys,
                 PURC_VARIANT_INVALID);
            if (ret_val == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto out;
            }
            purc_variant_set_add(ret_val, val, PCVRNT_CR_METHOD_OVERWRITE);
            purc_variant_unref(val);
        }
        else {
            ret_val = purc_variant_make_array_0();
            if (ret_val == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto out;
            }
            purc_variant_array_append(ret_val, val);
            purc_variant_unref(val);
        }
    }
    pcintr_set_question_var(frame, ret_val);
    purc_variant_unref(ret_val);
    ret = 0;

out:
    if (origin_val) {
        pcintr_set_question_var(parent, origin_val);
        purc_variant_unref(origin_val);
    }
    return ret;
}

static int
process_ds_tuple(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
     struct ctxt_for_adapt *ctxt)
{
    int ret = -1;
    struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(frame);
    assert(parent);
    purc_variant_t origin_val = pcintr_get_question_var(parent);
    purc_variant_ref(origin_val);

    size_t nr_size = purc_variant_tuple_get_size(ctxt->on);

    purc_variant_t ret_val = purc_variant_make_tuple(nr_size, NULL);
    if (ret_val == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    for (size_t i = 0; i < nr_size; ++i) {
        purc_variant_t v = purc_variant_tuple_get(ctxt->on, i);
        pcintr_set_question_var(parent, v);
        purc_variant_t val = eval_tpl_vcm(stack, frame, ctxt);
        if (val == PURC_VARIANT_INVALID) {
            goto out;
        }
        purc_variant_tuple_set(ret_val, i, val);
        purc_variant_unref(val);
    }
    pcintr_set_question_var(frame, ret_val);
    purc_variant_unref(ret_val);
    ret = 0;

out:
    if (origin_val) {
        pcintr_set_question_var(parent, origin_val);
        purc_variant_unref(origin_val);
    }
    return ret;
}

static int
process_ds_object(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
     struct ctxt_for_adapt *ctxt)
{
    int ret = -1;
    struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(frame);
    assert(parent);
    purc_variant_t origin_val = pcintr_get_question_var(parent);
    purc_variant_ref(origin_val);

    purc_variant_t ret_val = purc_variant_make_object_0();
    if (ret_val == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    purc_variant_t k, v;
    foreach_in_variant_object_safe_x(ctxt->on, k, v)
        pcintr_set_question_var(parent, v);
        purc_variant_t val = eval_tpl_vcm(stack, frame, ctxt);
        if (val == PURC_VARIANT_INVALID) {
            goto out;
        }
        purc_variant_object_set(ret_val, k, val);
        purc_variant_unref(val);
    end_foreach;

    pcintr_set_question_var(frame, ret_val);
    purc_variant_unref(ret_val);
    ret = 0;

out:
    if (origin_val) {
        pcintr_set_question_var(parent, origin_val);
        purc_variant_unref(origin_val);
    }
    return ret;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except) {
        return NULL;
    }

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_adapt *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_adapt*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;
        frame->pos = pos; // ATTENTION!!
    }

    int err = eval_attr(stack, frame);
    if (err) {
        return NULL;
    }

    err = eval_content(stack, frame);
    if (err) {
        return NULL;
    }

    if (!ctxt->tpl_vcm) {
        ctxt->tpl_vcm = ctxt->content_vcm;
    }
    else if (ctxt->content_vcm) {
        purc_variant_t val = pcintr_eval_vcm(stack, ctxt->content_vcm,
                frame->silently);
        if (val == PURC_VARIANT_INVALID) {
            return NULL;
        }
        pcintr_set_symbol_var(frame, PURC_SYMBOL_VAR_CARET, val);
        purc_variant_unref(val);
    }

    if (!ctxt->tpl_vcm) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'with' for element <%s>",
                frame->pos->tag_name);
        return NULL;
    }

    if (!ctxt->on) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <%s>",
                frame->pos->tag_name);
        return NULL;
    }

    if (!ctxt->individually) {
        err = process_ds_as_single(stack, frame, ctxt);
        if (err) {
            return NULL;
        }
        return ctxt;
    }

    enum purc_variant_type type = purc_variant_get_type(ctxt->on);
    switch (type) {
    case PURC_VARIANT_TYPE_ARRAY:
        err = process_ds_array(stack, frame, ctxt);
        if (err) {
            return NULL;
        }
        break;

    case PURC_VARIANT_TYPE_SET:
        err = process_ds_set(stack, frame, ctxt);
        if (err) {
            return NULL;
        }
        break;

    case PURC_VARIANT_TYPE_TUPLE:
        err = process_ds_tuple(stack, frame, ctxt);
        if (err) {
            return NULL;
        }
        break;

    case PURC_VARIANT_TYPE_OBJECT:
        err = process_ds_object(stack, frame, ctxt);
        if (err) {
            return NULL;
        }
        break;

    default:
        err = process_ds_as_single(stack, frame, ctxt);
        if (err) {
            return NULL;
        }
        break;
    }

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL) {
        return true;
    }

    struct ctxt_for_adapt *ctxt;
    ctxt = (struct ctxt_for_adapt*)frame->ctxt;
    if (ctxt) {
        ctxt_for_adapt_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static void
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
}

static void
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(content);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(comment);
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (stack->back_anchor == frame)
        stack->back_anchor = NULL;

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct ctxt_for_adapt *ctxt;
    ctxt = (struct ctxt_for_adapt*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
        purc_clr_error();
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        purc_clr_error();
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            break;
        case PCVDOM_NODE_ELEMENT:
        {
            pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
            on_element(co, frame, element);
            return element;
        }
        case PCVDOM_NODE_CONTENT:
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr));
            goto again;
        case PCVDOM_NODE_COMMENT:
            on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr));
            goto again;
        default:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    }

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL; // NOTE: never reached here!!!
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_adapt_ops(void)
{
    return &ops;
}
