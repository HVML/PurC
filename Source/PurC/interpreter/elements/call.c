/**
 * @file call.c
 * @author Xu Xiaohong
 * @date 2022/04/14
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
#include "private/instance.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

#define EVENT_SEPARATOR                 ':'
#define REQ_ARGS                        "_args"
#define REQ_CONTENT                     "_content"

#define RUNNER_NAME_SELF                "_self"
#define CALL_EVENT_HANDER               "_call_event_handler"

struct ctxt_for_call {
    struct pcvdom_node    *curr;

    purc_variant_t         on;
    purc_variant_t         with;
    purc_variant_t         within;
    char                   within_app_name[PURC_LEN_APP_NAME + 1];
    char                   within_runner_name[PURC_LEN_RUNNER_NAME + 1];

    purc_variant_t         as;
    const char            *s_as;

    purc_variant_t         at;
    const char            *s_at;

    pcvdom_element_t       define;

    char               endpoint_name_within[PURC_LEN_ENDPOINT_NAME + 1];
    purc_atom_t        endpoint_atom_within;

    unsigned int                  within_self:1;
    unsigned int                  concurrently:1;
    unsigned int                  synchronously:1;

    purc_variant_t         call_id;
};

static void
ctxt_for_call_destroy(struct ctxt_for_call *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->within);
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->call_id);
        if (ctxt->endpoint_atom_within) {
            purc_atom_remove_string_ex(PURC_ATOM_BUCKET_DEF,
                    ctxt->endpoint_name_within);
            ctxt->endpoint_atom_within = 0;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_call_destroy((struct ctxt_for_call*)ctxt);
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
    if (msg && (purc_variant_is_equal_to(observer->observed, msg->elementValue) ||
            pcintr_crtn_observed_is_match(observer->observed, msg->elementValue))) {
        goto match_observed;
    }
    else {
        goto out;
    }

match_observed:
    if (type && strcmp(type, MSG_TYPE_CALL_STATE) == 0) {
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

    pcintr_stack_frame_t frame = (pcintr_stack_frame_t)data;

    if (0 == strcmp(sub_type, MSG_SUB_TYPE_SUCCESS)) {
        purc_variant_t payload = msg->data;

        pcintr_set_question_var(frame, payload);
    }
    else if (0 == strcmp(sub_type, MSG_SUB_TYPE_EXCEPT)) {
        purc_variant_t payload = msg->data;

        const char *s = purc_variant_get_string_const(payload);
        purc_set_error_with_info(PURC_ERROR_UNKNOWN,
                "sub coroutine failed with except: %s", s);
    }

    pcintr_resume(cor, msg);
    pcintr_set_current_co(NULL);
    return 0;
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;

    if (ctxt->on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <%s>",
                frame->pos->tag_name);
        return -1;
    }

    pcvdom_element_t define = pcintr_get_vdom_from_variant(ctxt->on);
    if (define == NULL) {
        PC_WARN("define element is not found\n");
        purc_set_error(PURC_ERROR_ENTITY_NOT_FOUND);
        return -1;
    }

    if (0 && pcvdom_element_first_child_element(define) == NULL) {
        PC_WARN("define element is empty\n");
        purc_set_error(PURC_ERROR_NO_DATA);
        return -1;
    }

    if (!ctxt->synchronously) {
        if (ctxt->as == PURC_VARIANT_INVALID) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "vdom attribute 'as' for element <call> undefined");
            return -1;
        }
        if (!purc_variant_is_string(ctxt->as)) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "vdom attribute 'as' for element <call> is not string");
            return -1;
        }
    }

    if (ctxt->within == PURC_VARIANT_INVALID) {
        ctxt->within_self = 1;
    }

    /* handle call element by select_child with ctxt->define  */
    if (ctxt->within_self && ctxt->concurrently == 0) {
        ctxt->define = define;
        frame->scope = define;
        return 0;
    }

    purc_variant_t request = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (!request) {
        return  -1;
    }
    purc_variant_object_set_by_static_ckey(request, REQ_ARGS, ctxt->with);
    purc_variant_t caret = pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_CARET);
    purc_variant_object_set_by_static_ckey(request, REQ_CONTENT, caret);

    const char *runner_name = ctxt->within ?
        purc_variant_get_string_const(ctxt->within) : NULL;
    const char *as = ctxt->as ? purc_variant_get_string_const(ctxt->as) : NULL;

    purc_vdom_t vdom = pcintr_build_concurrently_call_vdom(&co->stack, define);
    if (!vdom) {
        return -1;
    }

    purc_atom_t child_cid = pcintr_schedule_child_co(vdom, co->cid,
            runner_name, NULL, request, NULL, true);
    purc_variant_unref(request);

    ctxt->call_id =  pcintr_crtn_observed_create(child_cid);

    if (as) {
        pcintr_bind_named_variable(&co->stack, frame, as, ctxt->at, false,
                false, ctxt->call_id);
    }

    if (ctxt->synchronously) {
        pcintr_yield(
                CO_STAGE_FIRST_RUN | CO_STAGE_OBSERVING,
                CO_STATE_STOPPED,
                ctxt->call_id,
                MSG_TYPE_CALL_STATE,
                MSG_SUB_TYPE_ASTERISK,
                is_observer_match,
                observer_handle,
                frame,
                true
                );
        return 0;
    }

    // ASYNC nothing to do
    return 0;
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
    if (ctxt->on != PURC_VARIANT_INVALID) {
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
    ctxt->on = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
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
process_attr_within(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
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

    const char *s = purc_variant_get_string_const(val);

    if (strcmp(s, RUNNER_NAME_SELF)) {
        ctxt->within_self = 0;
    }
    else {
        ctxt->within_self = 1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->within);
    ctxt->within = purc_variant_ref(val);

    return 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
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
    ctxt->as = purc_variant_ref(val);
    ctxt->s_as = purc_variant_get_string_const(ctxt->as);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
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
    ctxt->at = purc_variant_ref(val);
    ctxt->s_at = purc_variant_get_string_const(ctxt->at);

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

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITHIN)) == name) {
        return process_attr_within(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CONCURRENTLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CONC)) == name) {
        ctxt->concurrently = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNCHRONOUSLY)) == name) {
        ctxt->synchronously = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNC)) == name) {
        ctxt->synchronously = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNCHRONOUSLY)) == name) {
        ctxt->synchronously = 0;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNC)) == name) {
        ctxt->synchronously = 0;
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

    struct ctxt_for_call *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_call*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        ctxt->synchronously = 1;

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    if (pcintr_common_handle_attr_in(stack->co, frame)) {
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

    if (ctxt->with) {
        int r = pcintr_set_question_var(frame, ctxt->with);
        if (r) {
            return NULL;
        }

        if (purc_variant_is_object(ctxt->with)) {
            pcintr_bind_object_members_as_temp_vars(frame, ctxt->with);
        }
    }

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

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;
    if (ctxt) {
        ctxt_for_call_destroy(ctxt);
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
    UNUSED_PARAM(stack);
    UNUSED_PARAM(ud);

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_call *ctxt;
    ctxt = (struct ctxt_for_call*)frame->ctxt;

    if (stack->back_anchor == frame) {
        stack->back_anchor = NULL;
        ctxt->define = NULL;
        ctxt->curr = NULL;
    }

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        if (ctxt->define) {
            element = ctxt->define;
        }

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
        if (ctxt->define) {
            ctxt->define = NULL;
            goto again;
        }
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

struct pcintr_element_ops* pcintr_get_call_ops(void)
{
    return &ops;
}

