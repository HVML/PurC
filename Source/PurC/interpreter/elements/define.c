/**
 * @file define.c
 * @author Xu Xiaohong
 * @date 2022/04/13
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

#define MIN_BUFFER         512
static const char temp_header[] = "<hvml>\n";
static const char temp_footer[] = "</hvml>\n";

struct ctxt_for_define {
    struct pcvdom_node           *curr;

    purc_variant_t                as;
    purc_variant_t                at;
    purc_variant_t                from;
    purc_variant_t                from_result;
    purc_variant_t                with;

    enum VIA                      via;
    purc_variant_t                sync_id;
    purc_variant_t                params;
    pcintr_coroutine_t            co;

    int                           ret_code;
    int                           err;
    purc_rwstream_t               resp;


    unsigned int                  under_head:1;
    unsigned int                  async:1;
};

static void
ctxt_for_define_destroy(struct ctxt_for_define *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from_result);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->sync_id);
        PURC_VARIANT_SAFE_CLEAR(ctxt->params);
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
    ctxt_for_define_destroy((struct ctxt_for_define*)ctxt);
}

static const char*
get_name(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;

    purc_variant_t name = ctxt->as;

    if (name == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    if (purc_variant_is_string(name) == false) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    const char *s_name = purc_variant_get_string_const(name);
    return s_name;
}

static void on_sync_complete(
        struct pcfetcher_session *session,
        purc_variant_t request_id,
        void *ud,
        enum pcfetcher_resp_type type,
        const char *data, size_t sz_data)
{
    UNUSED_PARAM(session);
    UNUSED_PARAM(request_id);
    UNUSED_PARAM(ud);

    pcintr_stack_frame_t frame;
    frame = (pcintr_stack_frame_t)ud;
    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;

    switch (type) {
    case PCFETCHER_RESP_TYPE_HEADER:
    {
        struct pcfetcher_resp_header *resp_header =
            (struct pcfetcher_resp_header *)(void *)data;
        ctxt->ret_code = resp_header->ret_code;
        PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
        PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
        PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);
        break;
    }

    case PCFETCHER_RESP_TYPE_DATA:
    {
        if (ctxt->resp == NULL) {
            ctxt->resp = purc_rwstream_new_buffer(sz_data, 0);
        }
        purc_rwstream_write(ctxt->resp, data, sz_data);
        break;
    }

    case PCFETCHER_RESP_TYPE_ERROR:
    {
        struct pcfetcher_resp_header *resp_header =
            (struct pcfetcher_resp_header *)(void *)data;
        ctxt->ret_code = resp_header->ret_code;

        if (ctxt->co->stack.exited) {
            return;
        }

        if (ctxt->resp) {
            purc_rwstream_seek(ctxt->resp, 0, SEEK_SET);
        }
        pcintr_coroutine_post_event(ctxt->co->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
            ctxt->sync_id, MSG_TYPE_FETCHER_STATE, MSG_SUB_TYPE_SUCCESS,
            PURC_VARIANT_INVALID, ctxt->sync_id);
        break;
    }

    case PCFETCHER_RESP_TYPE_FINISH:
    {
        if (ctxt->co->stack.exited) {
            return;
        }
        if (ctxt->resp) {
            purc_rwstream_seek(ctxt->resp, 0, SEEK_SET);
        }

        pcintr_coroutine_post_event(ctxt->co->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
            ctxt->sync_id, MSG_TYPE_FETCHER_STATE, MSG_SUB_TYPE_SUCCESS,
            PURC_VARIANT_INVALID, ctxt->sync_id);
        break;
    }
    }
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
post_process_src(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t src);

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

    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;

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

    purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUFFER, 0);
    if (!rws) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        goto out;
    }

    purc_rwstream_write(rws, temp_header, strlen(temp_header));
    purc_rwstream_dump_to_another(ctxt->resp, rws, -1);
    purc_rwstream_write(rws, temp_footer, strlen(temp_footer));

    size_t nr_hvml = 0;
    char *hvml = purc_rwstream_get_mem_buffer(rws, &nr_hvml);
    purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
    purc_rwstream_destroy(rws);

    if (!vdom) {
        int err = purc_get_last_error();
        if (err) {
            const char* uri = purc_variant_get_string_const(ctxt->from);
            PC_ERROR("Failed to parse HVML from %s\n", uri);
            fprintf(stderr, "Failed to parse HVML from %s\n", uri);
            purc_variant_t ext = purc_get_last_error_ex();
            if (ext) {
                const char *err_msg = purc_variant_get_string_const(ext);
                PC_ERROR("%s\n", err_msg);
                fprintf(stderr, "%s\n", err_msg);
            }
        }
        else {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE ,
                    "load vdom from on/from failed");
        }

        frame->next_step = NEXT_STEP_ON_POPPING;
        goto out;
    }

    struct pcvdom_element *root = pcvdom_document_get_root(vdom);
    purc_variant_t v = pcintr_wrap_vdom(root);
    if (v == PURC_VARIANT_INVALID)
        return -1;

    post_process_src(cor, frame, v);
    purc_variant_unref(v);

out:
    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
    return 0;
}

static purc_variant_t
params_from_with(struct ctxt_for_define *ctxt)
{
    purc_variant_t with = ctxt->with;

    purc_variant_t params;
    if (with == PURC_VARIANT_INVALID) {
        params = purc_variant_make_object_0();
    }
    else if (purc_variant_is_object(with)) {
        params = purc_variant_ref(with);
    }
    else {
        // TODO VW: raise exceptioin for no suitable value.
        params = purc_variant_make_object_0();
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->params);
    ctxt->params = params;

    return params;
}

static int
get_source_by_from(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct ctxt_for_define *ctxt)
{
    UNUSED_PARAM(frame);

    const char* uri = purc_variant_get_string_const(ctxt->from);

    enum pcfetcher_method method;
    method = pcintr_method_from_via(ctxt->via);

    purc_variant_t params;
    params = params_from_with(ctxt);

    ctxt->co = co;
    purc_variant_t v = pcintr_load_from_uri_async(&co->stack, uri,
            method, params, on_sync_complete, frame, PURC_VARIANT_INVALID);
    if (v == PURC_VARIANT_INVALID)
        return -1;

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
    return 0;
}

int
post_process_src(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t src)
{
    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;

    const char *s_name = get_name(co, frame);
    if (!s_name) {
        return -1;
    }

    return pcintr_bind_named_variable(&co->stack, frame, s_name, ctxt->at,
            false, true, src);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);

    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;

    if (ctxt->as == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                    "lack of vdom attribute 'as' for element <%s>",
                    frame->pos->tag_name);

        return -1;
    }

    if (purc_variant_is_string(ctxt->as) == false) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "vdom attribute 'as' for element <%s> "
                    "is not of string type",
                    frame->pos->tag_name);

        return -1;
    }

    int r;

    purc_variant_t v = pcintr_wrap_vdom(frame->pos);
    if (v == PURC_VARIANT_INVALID)
        return -1;

    r = post_process_src(co, frame, v);
    purc_variant_unref(v);

    purc_variant_t from = ctxt->from;
    if (from != PURC_VARIANT_INVALID && purc_variant_is_string(from)) {
        if (!pcfetcher_is_init()) {
            purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
                    "pcfetcher not initialized");
            return -1;
        }
        r = get_source_by_from(co, frame, ctxt);
    }

    return r ? -1 : 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;
    if (ctxt->as != PURC_VARIANT_INVALID) {
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
    ctxt->as = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;
    if (ctxt->from != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (ctxt->with != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                "vdom attribute '%s' for element <%s> conflicts with '%s'",
                purc_atom_to_string(name), element->tag_name,
                pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, FROM)));
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->from = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;
    if (ctxt->with != PURC_VARIANT_INVALID) {
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
    ctxt->with = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;
    if (ctxt->at != PURC_VARIANT_INVALID) {
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
    ctxt->at = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_via(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    const char *s_val = purc_variant_get_string_const(val);
    if (!s_val)
        return -1;

    if (strcmp(s_val, "LOAD") == 0) {
        ctxt->via = VIA_LOAD;
        return 0;
    }

    if (strcmp(s_val, "GET") == 0) {
        ctxt->via = VIA_GET;
        return 0;
    }

    if (strcmp(s_val, "POST") == 0) {
        ctxt->via = VIA_POST;
        return 0;
    }

    if (strcmp(s_val, "DELETE") == 0) {
        ctxt->via = VIA_DELETE;
        return 0;
    }

    purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
            "unknown vdom attribute '%s = %s' for element <%s>",
            purc_atom_to_string(name), s_val, element->tag_name);
    return -1;
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

    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;


    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FROM)) == name) {
        return process_attr_from(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, VIA)) == name) {
        return process_attr_via(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNCHRONOUSLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNC)) == name) {
        ctxt->async = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNCHRONOUSLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNC)) == name) {
        ctxt->async = 0;
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    /* ignore other attr */
    return 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{

    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_define *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_define*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        ctxt->via = VIA_GET;

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    struct pcvdom_element *element = frame->pos;

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (!ctxt->with) {
        purc_variant_t caret = pcintr_get_symbol_var(frame,
                PURC_SYMBOL_VAR_CARET);
        if (caret && !purc_variant_is_undefined(caret)) {
            ctxt->with = caret;
            purc_variant_ref(ctxt->with);
        }
    }

    while ((element=pcvdom_element_parent(element))) {
        if (element->tag_id == PCHVML_TAG_HEAD) {
            ctxt->under_head = 1;
        }
    }

    purc_clr_error(); // pcvdom_element_parent

    r = post_process(stack->co, frame);
    if (r)
        return ctxt;

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

    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;
    if (ctxt) {
        ctxt_for_define_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);

    return 0;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(content);

    return 0;
}

static int
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(comment);
    return 0;
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);

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

    if (stack->except == 0)
        return NULL;

    struct ctxt_for_define *ctxt;
    ctxt = (struct ctxt_for_define*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
        purc_clr_error();
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
        purc_clr_error();
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        on_child_finished(co, frame);
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                if (on_element(co, frame, element))
                    return NULL;
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            if (on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr)))
                return NULL;
            goto again;
        case PCVDOM_NODE_COMMENT:
            if (on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr)))
                return NULL;
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

struct pcintr_element_ops* pcintr_get_define_ops(void)
{
    return &ops;
}


