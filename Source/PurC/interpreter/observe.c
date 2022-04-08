/**
 * @file observe.c
 * @author Xue Shuming
 * @date 2021/12/28
 * @brief The ops for <observe>
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

#include "internal.h"

#include "private/debug.h"
#include "purc-runloop.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_observe {
    struct pcvdom_node           *curr;
    purc_variant_t                on;
    purc_variant_t                for_var;
    purc_variant_t                at;
    purc_variant_t                as;
};

static void
ctxt_for_observe_destroy(struct ctxt_for_observe *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->for_var);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_observe_destroy((struct ctxt_for_observe*)ctxt);
}

#define MSG_TYPE_GROW       "grow"
#define MSG_TYPE_SHRINK     "shrink"
#define MSG_TYPE_CHANGE     "change"

bool base_variant_msg_listener(purc_variant_t source, pcvar_op_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    const char *smsg = NULL;
    switch (msg_type) {
        case PCVAR_OPERATION_GROW:
            smsg = MSG_TYPE_GROW;
            break;
        case PCVAR_OPERATION_SHRINK:
            smsg = MSG_TYPE_SHRINK;
            break;
        case PCVAR_OPERATION_CHANGE:
            smsg = MSG_TYPE_CHANGE;
            break;
        default:
            PC_ASSERT(0);
            break;
    }

    purc_variant_t type = purc_variant_make_string(
            smsg, false);

    pcintr_dispatch_message_ex((pcintr_stack_t)ctxt,
            source, type, PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    purc_variant_unref(type);
    return true;
}

#define TIMERS_EXPIRED_PREFIX                "expired:"
#define TIMERS_ACTIVATED_PREFIX              "activated:"
#define TIMERS_DEACTIVATED_PREFIX            "deactivated:"


static inline bool
is_base_variant_msg(purc_atom_t msg)
{
    if (msg == pcvariant_atom_grow ||
            msg == pcvariant_atom_shrink ||
            msg == pcvariant_atom_change/* ||
            msg == pcvariant_atom_reference ||
            msg == pcvariant_atom_unreference*/) {
        return true;
    }
    return false;
}

static inline bool
is_mmutable_variant_msg(purc_atom_t msg)
{
    return is_base_variant_msg(msg);
}

static inline bool
is_immutable_variant_msg(purc_atom_t msg)
{
#if 0
    if (msg == pcvariant_atom_reference ||
            msg == pcvariant_atom_unreference) {
        return true;
    }
#else
    UNUSED_PARAM(msg);
#endif
    return false;
}

static bool
regist_variant_listener(pcintr_stack_t stack, purc_variant_t observed,
        purc_atom_t op, struct pcvar_listener** listener)
{
    if (op == pcvariant_atom_grow) {
        *listener = purc_variant_register_post_listener(observed,
                PCVAR_OPERATION_GROW, base_variant_msg_listener, stack);
    }
    else if (op == pcvariant_atom_shrink) {
        *listener = purc_variant_register_post_listener(observed,
                PCVAR_OPERATION_SHRINK, base_variant_msg_listener, stack);
    }
    else if (op == pcvariant_atom_change) {
        *listener = purc_variant_register_post_listener(observed,
                PCVAR_OPERATION_CHANGE, base_variant_msg_listener, stack);
    }
    else {
        PC_ASSERT(0);
        return false;
    }

    if (*listener != NULL) {
        return true;
    }
    return false;
}

#define EVENT_SEPARATOR      ":"

static bool
regist_inner_data(pcintr_stack_t stack, purc_variant_t observed,
        purc_variant_t event, struct pcvar_listener** listener)
{
    UNUSED_PARAM(listener);

    if (!purc_variant_is_string(event)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return false;
    }

    const char* msg = purc_variant_get_string_const(event);
    purc_atom_t t = purc_atom_try_string(msg);

    switch (purc_variant_get_type(observed)) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_EXCEPTION:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (is_immutable_variant_msg(t)) {
                return regist_variant_listener(stack, observed, t, listener);
            }
            break;

        case PURC_VARIANT_TYPE_DYNAMIC:
            if (is_immutable_variant_msg(t)) {
                return regist_variant_listener(stack, observed, t, listener);
            }
            break;

        case PURC_VARIANT_TYPE_NATIVE:
            if (is_immutable_variant_msg(t)) {
                return regist_variant_listener(stack, observed, t, listener);
            }
            struct purc_native_ops *ops = purc_variant_native_get_ops(observed);
            if (ops && ops->on_observe) {
                void *native_entity = purc_variant_native_get_entity(observed);
                char *msg_dup = strdup(msg);
                if (!msg_dup) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    return false;
                }
                char *key;
                char *value;
                char *saveptr;
                key = strtok_r(msg_dup, EVENT_SEPARATOR, &saveptr);
                if (key == NULL) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    return false;
                }
                value = strtok_r(NULL, EVENT_SEPARATOR, &saveptr);
                bool ret = ops->on_observe(native_entity, key, value);
                free(msg_dup);
                return ret;
            }
            break;

        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
            if (is_mmutable_variant_msg(t)) {
                return regist_variant_listener(stack, observed, t, listener);
            }
            break;

        case PURC_VARIANT_TYPE_SET:
            if (is_mmutable_variant_msg(t)) {
                return regist_variant_listener(stack, observed, t, listener);
            }
            else if (pcintr_is_timers(stack, observed)) {
                if ((strncmp(msg, TIMERS_EXPIRED_PREFIX,
                                strlen(TIMERS_EXPIRED_PREFIX)) == 0)
                        || (strncmp(msg, TIMERS_ACTIVATED_PREFIX,
                                strlen(TIMERS_ACTIVATED_PREFIX)) == 0)
                        || (strncmp(msg, TIMERS_DEACTIVATED_PREFIX,
                                strlen(TIMERS_DEACTIVATED_PREFIX)) == 0)) {
                    return true;
                }
            }
            break;

        default:
            break;
    }

    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
    return false;
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
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
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
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
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
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
process_attr_for(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
    if (ctxt->for_var != PURC_VARIANT_INVALID) {
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
    ctxt->for_var = val;
    purc_variant_ref(val);

    return 0;
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FOR)) == name) {
        return process_attr_for(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "vdom attribute '%s' for element <%s>",
            purc_atom_to_string(name), element->tag_name);

    return -1;
}

static int
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        struct pcvdom_attr *attr,
        void *ud)
{
    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    purc_variant_t val = pcintr_eval_vdom_attr(pcintr_get_stack(), attr);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    int r = attr_found_val(frame, element, name, val, attr, ud);
    purc_variant_unref(val);

    return r ? -1 : 0;
}

static void
on_named_observe_release(void* native_entity)
{
    struct pcintr_observer *observer = (struct pcintr_observer*)native_entity;
    pcintr_revoke_observer(observer);
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == pcintr_get_stack());

    if (stack->except)
        return NULL;

    if (pcintr_check_insertion_mode_for_normal_element(stack))
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return NULL;

    purc_variant_t on;
    on = ctxt->on;
    if (on == PURC_VARIANT_INVALID)
        return NULL;

    purc_variant_t for_var;
    for_var = ctxt->for_var;
    if (for_var == PURC_VARIANT_INVALID)
        return NULL;

    if (stack->stage != STACK_STAGE_FIRST_ROUND) {
        purc_clr_error();
        return ctxt;
    }

    struct pcvar_listener* listener = NULL;
    purc_variant_t observed = PURC_VARIANT_INVALID;
    if (ctxt->at != PURC_VARIANT_INVALID && purc_variant_is_string(ctxt->at)) {
        const char* name = purc_variant_get_string_const(ctxt->at);
        const char* event = purc_variant_get_string_const(for_var);
        observed = pcintr_add_named_var_observer(stack, name, event);
        if (observed == PURC_VARIANT_INVALID) {
            return NULL;
        }
    }
    else {
// TODO : css selector
#if 0
        if (purc_variant_is_string(ctxt->on)) {
            const char* at_str = purc_variant_get_string_const(ctxt->on);
            if (at_str[0] == '#') {
            }
        }
        else
#endif
        {
            observed = ctxt->on;
            if (!regist_inner_data(stack, on, for_var, &listener)) {
                return NULL;
            }
        }
    }

    struct pcintr_observer* observer;
    observer = pcintr_register_observer(observed, for_var, frame->scope,
            frame->edom_element, pos, listener);
    if (observer == NULL) {
        return NULL;
    }

    if (ctxt->as != PURC_VARIANT_INVALID && purc_variant_is_string(ctxt->as)) {
        const char* name = purc_variant_get_string_const(ctxt->as);
        static struct purc_native_ops ops = {
            .on_release                   = on_named_observe_release,
        };

        purc_variant_t v = purc_variant_make_native(observer, &ops);
        if (v == PURC_VARIANT_INVALID) {
            pcintr_revoke_observer(observer);
            return NULL;
        }
        if(!pcintr_bind_document_variable(stack->vdom, name, v)) {
            purc_variant_unref(v); // on_release
            return NULL;
        }
        purc_variant_unref(v);
    }

    purc_clr_error();

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
    if (ctxt) {
        ctxt_for_observe_destroy(ctxt);
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
    PC_ASSERT(content);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(comment);
}


static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    if (stack->stage == STACK_STAGE_FIRST_ROUND) {
        return NULL;
    }

    pcintr_coroutine_t co = &stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return NULL;

    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

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
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                on_element(co, frame, element);
// FIXME:
//                PC_ASSERT(stack->except == 0);
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr));
            goto again;
        case PCVDOM_NODE_COMMENT:
            on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr));
            goto again;
        default:
            PC_ASSERT(0); // Not implemented yet
    }

    PC_ASSERT(0);
    return NULL; // NOTE: never reached here!!!
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_observe_ops(void)
{
    return &ops;
}

