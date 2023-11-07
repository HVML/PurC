/**
 * @file hvml.c
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

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

#define ATTR_KEY_ID         "id"
#define BUFF_MIN                        1024
#define BUFF_MAX                        1024 * 1024 * 4
#define MIME_TYPE_TEXT_HTML             "text/html"

struct ctxt_for_hvml {
    struct pcvdom_node           *curr;
    pcvdom_element_t              body;

    purc_variant_t                template;

    pcintr_coroutine_t            co;
    purc_variant_t                sync_id;
    purc_variant_t                params;

    int                           ret_code;
    int                           err;
    purc_rwstream_t               resp;
    char                         *mime_type;
};

static void
ctxt_for_hvml_destroy(struct ctxt_for_hvml *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->template);
        PURC_VARIANT_SAFE_CLEAR(ctxt->sync_id);
        PURC_VARIANT_SAFE_CLEAR(ctxt->params);

        if (ctxt->resp) {
            purc_rwstream_destroy(ctxt->resp);
            ctxt->resp = NULL;
        }

        if (ctxt->mime_type) {
            free(ctxt->mime_type);
            ctxt->mime_type = NULL;
        }

        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_hvml_destroy((struct ctxt_for_hvml*)ctxt);
}

static int
process_attr_template(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_hvml *ctxt;
    ctxt = (struct ctxt_for_hvml*)frame->ctxt;
    if (ctxt->template != PURC_VARIANT_INVALID) {
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
    else if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> type '%s' invalid",
                purc_atom_to_string(name), element->tag_name, pcvariant_typename(val));
        return -1;
    }

    ctxt->template = purc_variant_ref(val);

    return 0;
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(name);
    UNUSED_PARAM(ud);

    const char *sv = purc_variant_get_string_const(val);

    pcintr_stack_t stack = (pcintr_stack_t) ud;
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TARGET)) == name) {
        if (stack->co->target) {
            char *target = strdup(sv);
            if (!target) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return -1;
            }
            free(stack->co->target);
            stack->co->target = target;
        }
    }
    else if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TEMPLATE)) == name) {
        return process_attr_template(frame, element, name, val);
    }
    else {
        /* VW: only set attributes other than `target` to
           the root element of eDOM */
        if (pcintr_is_hvml_attr(attr->key)) {
            return 0;
        }

        /* inherit doc do not send */
        pcintr_util_set_attribute(frame->owner->doc,
                frame->edom_element, PCDOC_OP_DISPLACE, attr->key, sv, 0,
                !stack->inherit, false);
    }

    return 0;
}

static bool
is_match_body_id(pcintr_stack_t stack, struct pcvdom_element *element)
{
    bool match = false;
    if (!stack->body_id) {
        match = true;
        goto ret;
    }

    purc_variant_t elem_id = pcvdom_element_eval_attr_val(stack, element,
            ATTR_KEY_ID);
    if (!elem_id || !purc_variant_is_string(elem_id)) {
        goto out;
    }

    const char *eid = purc_variant_get_string_const(elem_id);
    if (strcmp(eid, stack->body_id) == 0) {
        match = true;
    }

out:
    if (elem_id) {
        purc_variant_unref(elem_id);
    }

ret:
    return match;
}

static pcvdom_element_t
find_body(pcintr_stack_t stack)
{
    purc_vdom_t vdom = stack->vdom;
    struct pcvdom_element *ret = NULL;
    size_t nr = pcutils_arrlist_length(vdom->bodies);
    if (nr == 0) {
        goto out;
    }

    for (size_t i = 0; i < nr; i++) {
        void *p = pcutils_arrlist_get_idx(vdom->bodies, i);
        struct pcvdom_element *body = (struct pcvdom_element*)p;
        if (is_match_body_id(stack, body)) {
            ret = body;
            goto out;
        }
    }

    ret = pcutils_arrlist_get_idx(vdom->bodies, 0);
out:
    return ret;
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_hvml *ctxt;
    ctxt = (struct ctxt_for_hvml*)frame->ctxt;

    ctxt->body = find_body(&co->stack);
    purc_clr_error();

    return 0;
}

static int
observer_handle(pcintr_coroutine_t cor, struct pcintr_observer *observer,
        pcrdr_msg *msg, const char *type, const char *sub_type, void *data)
{
    UNUSED_PARAM(observer);
    UNUSED_PARAM(msg);
    UNUSED_PARAM(type);
    UNUSED_PARAM(sub_type);
    UNUSED_PARAM(data);
    UNUSED_PARAM(msg);

    pcintr_set_current_co(cor);

    int r;
    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)data;

    struct ctxt_for_hvml *ctxt;
    ctxt = (struct ctxt_for_hvml*)frame->ctxt;

    if (ctxt->ret_code == RESP_CODE_USER_STOP) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        goto out;
    }

    if (!ctxt->resp || ctxt->ret_code != 200) {
        if (frame->silently) {
            frame->next_step = NEXT_STEP_ON_POPPING;
            goto out;
        }
        frame->next_step = NEXT_STEP_ON_POPPING;
        // FIXME: what error to set
        purc_set_error_with_info(PURC_ERROR_REQUEST_FAILED, "%d",
                ctxt->ret_code);
        goto out;
    }

    if (strcmp(MIME_TYPE_TEXT_HTML, ctxt->mime_type) != 0) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
                "template type '%s' not implemented", ctxt->mime_type);
        goto out;
    }

    purc_rwstream_t stream = purc_rwstream_new_buffer(BUFF_MIN, BUFF_MAX);
    purc_rwstream_dump_to_another(ctxt->resp, stream, -1);

    size_t sz_content = 0;
    char *content = purc_rwstream_get_mem_buffer(stream, &sz_content);
    purc_document_t doc = pcdoc_document_new(PCDOC_K_TYPE_HTML,
            content, sz_content);
    if (!doc) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        // FIXME: what error to set
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE, "invalid template");
        goto out;
    }

    if (cor->stack.doc) {
        purc_document_unref(cor->stack.doc);
        cor->stack.doc = NULL;
    }
    cor->stack.doc = doc;

    /* FIXME: clear origin $DOC, bind new $DOC */
    purc_variant_t n_doc = purc_dvobj_doc_new(cor->stack.doc);
    pcintr_unbind_coroutine_variable(cor, PURC_PREDEF_VARNAME_DOC);
    pcintr_bind_coroutine_variable(cor, PURC_PREDEF_VARNAME_DOC, n_doc);
    purc_variant_unref(n_doc);

    purc_rwstream_destroy(stream);

    r = post_process(cor, frame);
    if (r) {
        frame->next_step = NEXT_STEP_ON_POPPING;
    }

out:
    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
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
    struct ctxt_for_hvml *ctxt = (struct ctxt_for_hvml*)frame->ctxt;

    PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
    PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
    PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);

    ctxt->ret_code = resp_header->ret_code;
    ctxt->resp = resp;
    if (resp_header->mime_type) {
        ctxt->mime_type = strdup(resp_header->mime_type);
    }

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
process_init_sync(pcintr_coroutine_t co, pcintr_stack_frame_t frame)
{
    int ret = -1;

    pcintr_stack_t stack = &co->stack;
    struct ctxt_for_hvml *ctxt = (struct ctxt_for_hvml*)frame->ctxt;
    ctxt->co = co;

    enum pcfetcher_request_method method = PCFETCHER_REQUEST_METHOD_GET;
    ctxt->params = purc_variant_make_object_0();
    const char *uri = purc_variant_get_string_const(ctxt->template);

    purc_variant_t v = pcintr_load_from_uri_async(stack, uri,
            method, ctxt->params, on_sync_complete, frame, PURC_VARIANT_INVALID);
    if (v == PURC_VARIANT_INVALID) {
        goto failed;
    }

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

    purc_clr_error();

    ret = 0;

failed:
    return ret;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    stack->mode = STACK_VDOM_BEFORE_HEAD;

    if (stack->except)
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_hvml *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_hvml*)calloc(1, sizeof(*ctxt));
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

    frame->edom_element = purc_document_root(stack->doc);
    int r;
    r = pcintr_refresh_at_var(frame);
    if (r)
        return ctxt;

    struct pcvdom_element *element = frame->pos;

    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (ctxt->template && !stack->inherit &&
            (stack->co->target_page_type != PCRDR_PAGE_TYPE_NULL) &&
            (purc_document_type(stack->doc) != PCDOC_K_TYPE_VOID)) {
        r = process_init_sync(stack->co, frame);
        return ctxt;
    }

    post_process(stack->co, frame);

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);
    switch (stack->mode) {
        case STACK_VDOM_BEFORE_HVML:
            break;
        case STACK_VDOM_BEFORE_HEAD:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_IN_HEAD:
            break;
        case STACK_VDOM_AFTER_HEAD:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_IN_BODY:
            break;
        case STACK_VDOM_AFTER_BODY:
            stack->mode = STACK_VDOM_AFTER_HVML;
            break;
        case STACK_VDOM_AFTER_HVML:
            break;
        default:
            break;
    }

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    struct ctxt_for_hvml *ctxt;
    ctxt = (struct ctxt_for_hvml*)frame->ctxt;
    if (ctxt) {
        ctxt_for_hvml_destroy(ctxt);
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

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(content);

    int err = 0;
    struct pcvcm_node *vcm = content->vcm;
    if (!vcm) {
        goto out;
    }

    purc_variant_t v = pcintr_eval_vcm(&co->stack, vcm, frame->silently);
    if (v == PURC_VARIANT_INVALID) {
        err = purc_get_last_error();
        goto out;
    }
    pcintr_set_question_var(frame, v);
    purc_variant_unref(v);

out:
    return err;
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

    struct ctxt_for_hvml *ctxt;
    ctxt = (struct ctxt_for_hvml*)frame->ctxt;

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
                enum pchvml_tag_id tag_id = element->tag_id;
                if (tag_id != PCHVML_TAG_BODY) {
                    return element;
                }
                else if (stack->mode == STACK_VDOM_AFTER_BODY) {
                    goto again;
                }
                else if (element == ctxt->body) {
                    return element;
                }
                goto again;
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

struct pcintr_element_ops* pcintr_get_hvml_ops(void)
{
    return &ops;
}

