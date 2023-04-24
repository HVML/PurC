/**
 * @file archetype.c
 * @author Xu Xiaohong
 * @date 2021/12/06
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

#include "purc.h"

#include "../internal.h"

#include "private/debug.h"
#include "purc-runloop.h"
#include "private/stringbuilder.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>


struct ctxt_for_archetype {
    struct pcvdom_node           *curr;
    purc_variant_t                name;

    purc_variant_t                src;
    purc_variant_t                param;
    purc_variant_t                method;

    purc_variant_t                type;

    purc_variant_t                sync_id;
    pcintr_coroutine_t            co;

    int                           ret_code;
    int                           err;
    purc_rwstream_t               resp;

    struct pcvcm_node            *vcm_from_src;

    purc_variant_t                contents;
};

static void
ctxt_for_archetype_destroy(struct ctxt_for_archetype *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->name);
        PURC_VARIANT_SAFE_CLEAR(ctxt->src);
        PURC_VARIANT_SAFE_CLEAR(ctxt->param);
        PURC_VARIANT_SAFE_CLEAR(ctxt->method);
        PURC_VARIANT_SAFE_CLEAR(ctxt->type);
        PURC_VARIANT_SAFE_CLEAR(ctxt->sync_id);
        PURC_VARIANT_SAFE_CLEAR(ctxt->contents);
        if (ctxt->vcm_from_src) {
            pcvcm_node_destroy(ctxt->vcm_from_src);
        }
        if (ctxt->resp) {
            purc_rwstream_destroy(ctxt->resp);
            ctxt->resp = NULL;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_archetype_destroy((struct ctxt_for_archetype*)ctxt);
}

static int
process_attr_name(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->name != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->name = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_src(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->src != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->src = purc_variant_ref(val);

    return 0;
}

static int
process_attr_param(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->param != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_object(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not object",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->param = purc_variant_ref(val);

    return 0;
}

static int
process_attr_method(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->method != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->method = purc_variant_ref(val);

    return 0;
}

static int
process_attr_raw(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
    UNUSED_PARAM(name);
    UNUSED_PARAM(val);
    return 0;
}

static int
process_attr_type(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->type != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    ctxt->type = purc_variant_ref(val);
    return 0;
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(attr);
    UNUSED_PARAM(ud);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, NAME)) == name) {
        return process_attr_name(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SRC)) == name) {
        return process_attr_src(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, PARAM)) == name) {
        return process_attr_param(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, METHOD)) == name) {
        return process_attr_method(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, RAW)) == name) {
        return process_attr_raw(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TYPE)) == name) {
        return process_attr_type(frame, element, name, val);
    }

    /* ignore other attr */
    return 0;
}

static int
method_by_method(const char *s_method, enum pcfetcher_request_method *method)
{
    if (strcmp(s_method, "GET") == 0) {
        *method = PCFETCHER_REQUEST_METHOD_GET;
    }
    else if (strcmp(s_method, "POST") == 0) {
        *method = PCFETCHER_REQUEST_METHOD_POST;
    }
    else if (strcmp(s_method, "DELETE") == 0) {
        *method = PCFETCHER_REQUEST_METHOD_DELETE;
    }
    else {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "unknown method `%s`", s_method);
        return -1;
    }

    return 0;
}

static void on_sync_complete(purc_variant_t request_id, void *ud,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    UNUSED_PARAM(request_id);
    UNUSED_PARAM(ud);
    UNUSED_PARAM(resp_header);
    UNUSED_PARAM(resp);

    pcintr_stack_frame_t frame;
    frame = (pcintr_stack_frame_t)ud;
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

    PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
    PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
    PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);

    ctxt->ret_code = resp_header->ret_code;
    ctxt->resp = resp;

    if (ctxt->co->stack.exited) {
        return;
    }

    pcintr_coroutine_post_event(ctxt->co->cid,
        PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
        ctxt->sync_id, MSG_TYPE_FETCHER_STATE, MSG_SUB_TYPE_SUCCESS,
        PURC_VARIANT_INVALID, ctxt->sync_id);
}

static bool
is_observer_match(pcintr_coroutine_t co,
        struct pcintr_observer *observer, pcrdr_msg *msg,
        purc_variant_t observed, const char *type, const char *sub_type)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(observed);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    bool match = false;
    if (!purc_variant_is_equal_to(observer->observed, msg->elementValue)) {
        goto out;
    }

    if (type && strcmp(type, MSG_TYPE_FETCHER_STATE) == 0) {
        match = true;
        goto out;
    }

out:
    return match;
}

static int
observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, const char *type, const char *sub_type, void *data)
{
    UNUSED_PARAM(cor);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);
    UNUSED_PARAM(msg);

    pcintr_set_current_co(cor);

    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)data;

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

    purc_variant_t ret = PURC_VARIANT_INVALID;

    // struct pcvdom_element *element = frame->pos;
    if (ctxt->ret_code == RESP_CODE_USER_STOP) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        goto clean_rws;
    }

    bool has_except = true;

    if (!ctxt->resp || ctxt->ret_code != 200) {
        if (frame->silently) {
            frame->next_step = NEXT_STEP_ON_POPPING;
            goto out;
        }
        purc_set_error_with_info(PURC_ERROR_REQUEST_FAILED, "%d",
                ctxt->ret_code);
        goto dispatch_except;
    }

    ctxt->vcm_from_src = (struct pcvcm_node*)purc_variant_ejson_parse_stream(
            ctxt->resp);
    if (ctxt->vcm_from_src == NULL)
        goto dispatch_except;

    has_except = false;

dispatch_except:
    if (has_except) {
        PC_DEBUG("%s error=%s\n", __func__,
                purc_get_error_message(purc_get_last_error()));
    }

clean_rws:
    if (ctxt->resp) {
        purc_rwstream_destroy(ctxt->resp);
        ctxt->resp = NULL;
    }
    PURC_VARIANT_SAFE_CLEAR(ret);

    frame->next_step = NEXT_STEP_SELECT_CHILD;

out:
    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
    return 0;
}

static void
process_by_src(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

    const char *s_src = purc_variant_get_string_const(ctxt->src);

    const char *s_method = "GET";
    if (ctxt->method != PURC_VARIANT_INVALID) {
        s_method = purc_variant_get_string_const(ctxt->method);
    }

    int r;

    enum pcfetcher_request_method method;
    r = method_by_method(s_method, &method);
    if (r)
        return;

    purc_variant_t param;
    if (ctxt->param == PURC_VARIANT_INVALID) {
        param = purc_variant_make_object_0();
        if (param == PURC_VARIANT_INVALID)
            return;
    }
    else {
        param = purc_variant_ref(ctxt->param);
    }

    ctxt->co = stack->co;
    purc_variant_t v;
    v = pcintr_load_from_uri_async(stack, s_src, method, param,
            on_sync_complete, frame, PURC_VARIANT_INVALID);
    purc_variant_unref(param);

    if (v == PURC_VARIANT_INVALID)
        return;

    ctxt->sync_id = purc_variant_ref(v);

    pcintr_yield(
            CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
            CO_STATE_STOPPED,
            ctxt->sync_id,
            MSG_TYPE_FETCHER_STATE,
            MSG_SUB_TYPE_ASTERISK,
            is_observer_match,
            observer_handle,
            frame,
            true
            );
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_archetype *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_archetype*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, true)) {
        return NULL;
    }

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return ctxt;

    ctxt->contents = pcintr_template_make();
    if (!ctxt->contents)
        return ctxt;

    struct pcvdom_element *element = frame->pos;

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (ctxt->name == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                    "lack of vdom attribute 'name' for element <%s>",
                    frame->pos->tag_name);

        return ctxt;
    }

    if (ctxt->src != PURC_VARIANT_INVALID) {
        process_by_src(stack, frame);
        return ctxt;
    }

    if (!ctxt->type && stack->co->target) {
        ctxt->type = purc_variant_make_string(stack->co->target, false);
    }

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt) {
        ctxt_for_archetype_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

    // successfully loading from external src
    if (ctxt->vcm_from_src)
        return 0;

    struct pcvcm_node *vcm = content->vcm;
    if (!vcm)
        return 0;

    // NOTE: element is still the owner of vcm_content
    bool to_free = false;
    return pcintr_template_set(ctxt->contents, vcm, ctxt->type, to_free);
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

    purc_variant_t contents = ctxt->contents;
    if (!contents)
        return -1;

    if (ctxt->vcm_from_src) {
        bool to_free = true;
        int r = pcintr_template_set(ctxt->contents, ctxt->vcm_from_src,
                ctxt->type, to_free);
        ctxt->vcm_from_src = NULL;
        if (r) {
            return -1;
        }
    }

    PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
    frame->ctnt_var = contents;
    purc_variant_ref(contents);

    purc_variant_t name;
    name = ctxt->name;
    if (name == PURC_VARIANT_INVALID)
        return -1;

    const char *s_name = purc_variant_get_string_const(name);
    if (s_name == NULL)
        return -1;

    struct pcvdom_element *parent = pcvdom_element_parent(frame->pos);

    bool ok;
    ok = pcintr_bind_scope_variable(co, parent, s_name, frame->ctnt_var, NULL);
    if (!ok)
        return -1;

    return 0;
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

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

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
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        purc_clr_error();
        on_child_finished(co, frame);
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            break;
        case PCVDOM_NODE_ELEMENT:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            break;
        case PCVDOM_NODE_CONTENT:
            if (on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr)))
                return NULL;
            goto again;
        case PCVDOM_NODE_COMMENT:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            goto again;
        default:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            break;
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

struct pcintr_element_ops* pcintr_get_archetype_ops(void)
{
    return &ops;
}

