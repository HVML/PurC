/**
 * @file fire.c
 * @author Xue Shuming
 * @date 2022/03/31
 * @brief The ops for <fire>
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

#define EVENT_SEPARATOR      ':'

struct ctxt_for_fire {
    struct pcvdom_node           *curr;
    purc_variant_t                on;
    purc_variant_t                for_var;
    purc_variant_t                at;
    purc_variant_t                with;

    char                         *msg_type;
    char                         *sub_type;
    purc_atom_t                   msg_type_atom;
};

static void
ctxt_for_fire_destroy(struct ctxt_for_fire *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->for_var);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);

        if (ctxt->msg_type) {
            free(ctxt->msg_type);
            ctxt->msg_type = NULL;
        }
        if (ctxt->sub_type) {
            free(ctxt->sub_type);
            ctxt->sub_type = NULL;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_fire_destroy((struct ctxt_for_fire*)ctxt);
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_fire *ctxt;
    ctxt = (struct ctxt_for_fire*)frame->ctxt;
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
    struct ctxt_for_fire *ctxt;
    ctxt = (struct ctxt_for_fire*)frame->ctxt;
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
process_attr_for(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_fire *ctxt;
    ctxt = (struct ctxt_for_fire*)frame->ctxt;
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

    const char *s = purc_variant_get_string_const(ctxt->for_var);
    const char *p = strchr(s, EVENT_SEPARATOR);
    if (p) {
        ctxt->msg_type = strndup(s, p-s);
        ctxt->sub_type = strdup(p+1);
    }
    else {
        ctxt->msg_type = strdup(s);
    }

    if (ctxt->msg_type == NULL) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "unknown vdom attribute '%s = %s' for element <%s>",
                purc_atom_to_string(name), s, element->tag_name);
        return -1;
    }

    ctxt->msg_type_atom = purc_atom_try_string_ex(ATOM_BUCKET_MSG,
            ctxt->msg_type);
    if (ctxt->msg_type_atom == 0) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "unknown vdom attribute '%s = %s' for element <%s>",
                purc_atom_to_string(name), s, element->tag_name);
        return -1;
    }

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_fire *ctxt;
    ctxt = (struct ctxt_for_fire*)frame->ctxt;
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
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
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

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == pcintr_get_stack());

    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_fire *ctxt;
    ctxt = (struct ctxt_for_fire*)calloc(1, sizeof(*ctxt));
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
        return ctxt;

    purc_variant_t for_var = ctxt->for_var;
    if (for_var == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "`for` not specified");
        return ctxt;
    }

    if (ctxt->on == PURC_VARIANT_INVALID &&
            ctxt->at == PURC_VARIANT_INVALID)
    {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "neither `on` nor `at` is specified");
        return ctxt;
    }

    // named var
    if (ctxt->at != PURC_VARIANT_INVALID && purc_variant_is_string(ctxt->at)) {
        const char* name = purc_variant_get_string_const(ctxt->at);
        purc_variant_t observed = pcintr_get_named_var_for_event(stack, name);
        if (observed) {
            purc_variant_t source_uri = purc_variant_make_string(
                    stack->co->full_name, false);
            int ret = pcintr_post_event_by_ctype(stack->co,
                    PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE, source_uri,
                    observed, ctxt->msg_type, ctxt->sub_type,
                    ctxt->with);
            purc_variant_unref(source_uri);
            purc_variant_unref(observed);
            if (ret != PURC_ERROR_OK) {
                return ctxt;
            }
        }
    }
    else {
        purc_variant_t on = ctxt->on;
        if (purc_variant_is_string(ctxt->on)) {
            // XXX: optimization
            // CSS selector used string
            // handle by elements.c match_observe
            purc_variant_t source_uri = purc_variant_make_string(
                    stack->co->full_name, false);
            int ret = pcintr_post_event_by_ctype(stack->co,
                    PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE, source_uri,
                    on, ctxt->msg_type, ctxt->sub_type,
                    ctxt->with);
            purc_variant_unref(source_uri);
            if (ret != PURC_ERROR_OK) {
                return ctxt;
            }
        }
        else
        {
            purc_variant_t source_uri = purc_variant_make_string(
                    stack->co->full_name, false);
            int ret = pcintr_post_event_by_ctype(stack->co,
                    PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE, source_uri,
                    on, ctxt->msg_type, ctxt->sub_type,
                    ctxt->with);
            purc_variant_unref(source_uri);
            if (ret != PURC_ERROR_OK) {
                return ctxt;
            }
        }
    }

    purc_clr_error();

    // NOTE: no element to process if succeeds
    return NULL;
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

    struct ctxt_for_fire *ctxt;
    ctxt = (struct ctxt_for_fire*)frame->ctxt;
    if (ctxt) {
        ctxt_for_fire_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    PC_ASSERT(0);
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(frame);
    PC_ASSERT(content);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    PC_ASSERT(0);
}

static int
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(comment);
    return 0;
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(frame);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    return 0;
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    if (stack->back_anchor == frame)
        stack->back_anchor = NULL;

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct ctxt_for_fire *ctxt;
    ctxt = (struct ctxt_for_fire*)frame->ctxt;

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
            PC_ASSERT(0); // Not implemented yet
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

struct pcintr_element_ops* pcintr_get_fire_ops(void)
{
    return &ops;
}

