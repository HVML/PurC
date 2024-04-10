/**
 * @file undefined.c
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

struct ctxt_for_undefined {
    struct pcvdom_node           *curr;
    purc_variant_t                href;
};

static void
ctxt_for_undefined_destroy(struct ctxt_for_undefined *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->href);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_undefined_destroy((struct ctxt_for_undefined*)ctxt);
}

static int
process_attr_href(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)frame->ctxt;
    if (ctxt->href != PURC_VARIANT_INVALID) {
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
    ctxt->href = val;
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

    const char *sv = "";
    char *value = NULL;

    if (purc_variant_is_string(val)) {
        sv = purc_variant_get_string_const(val);
    }
    else if (purc_variant_is_undefined(val)) {
        /* no action to take */
    }
    else {
        ssize_t ret = purc_variant_stringify_alloc(&value, val);
        if (ret > 0) {
            sv = value;
        }
    }

    /* VW: do not set attributes having `hvml:` prefix to eDOM */
    if (strncmp(attr->key, "hvml:", 5) == 0) {
        goto done;
    }

    pcintr_stack_t stack = pcintr_get_stack();
    pcintr_util_set_attribute(frame->owner->doc,
            frame->edom_element, PCDOC_OP_DISPLACE, attr->key, sv, 0,
            !stack->inherit, false);
    if (value) {
        free(value);
        value = NULL;
    }

    if (name) {
        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, HREF)) == name) {
            return process_attr_href(frame, element, name, val);
        }
        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TYPE)) == name) {
            return 0;
        }
        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, REL)) == name) {
            return 0;
        }
        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, NAME)) == name) {
            return 0;
        }
        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
            return 0;
        }
        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SRC)) == name) {
            if (strcasecmp(element->tag_name, "img") == 0) {
                pcintr_coroutine_t co = pcintr_get_coroutine();
                if (co->base_url_string) {
                    pcintr_util_set_attribute(frame->owner->doc,
                            frame->edom_element, PCDOC_OP_DISPLACE, "data-base-url", co->base_url_string, 0,
                            !stack->inherit, false);
                }
            }
            return 0;
        }
        PC_DEBUGX("name: %s", purc_atom_to_string(name));
        return 0;
    }

done:
    if (value) {
        free(value);
    }
    return 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    switch (stack->mode) {
        case STACK_VDOM_BEFORE_HVML:
            break;
        case STACK_VDOM_BEFORE_HEAD:
            stack->mode = STACK_VDOM_IN_BODY;
            break;
        case STACK_VDOM_IN_HEAD:
            break;
        case STACK_VDOM_AFTER_HEAD:
            stack->mode = STACK_VDOM_IN_BODY;
            break;
        case STACK_VDOM_IN_BODY:
            break;
        case STACK_VDOM_AFTER_BODY:
            break;
        case STACK_VDOM_AFTER_HVML:
            break;
        default:
            break;
    }

    if (stack->except)
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct pcintr_stack_frame *parent_frame = pcintr_stack_frame_get_parent(
            frame);
    if (parent_frame) {
        purc_variant_t v = pcintr_get_question_var(parent_frame);
        pcintr_set_question_var(frame, v);
    }

    struct ctxt_for_undefined *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_undefined*)calloc(1, sizeof(*ctxt));
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

    struct pcvdom_element *element = frame->pos;

    pcdoc_element_t child;
    const char *tag_name = frame->pos->tag_name;
    if (stack->tag_prefix) {
        size_t nr = strlen(stack->tag_prefix);
        if (strncmp(stack->tag_prefix, tag_name, nr) == 0 &&
                tag_name[nr] == ':') {
            tag_name = frame->pos->tag_name + nr + 1;
        }
    }

    child = pcintr_util_new_element(frame->owner->doc, frame->edom_element,
            PCDOC_OP_APPEND, tag_name, frame->pos->self_closing,
            !stack->inherit);
    frame->edom_element = child;
    int r;
    r = pcintr_refresh_at_var(frame);
    if (r)
        return ctxt;

    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

#if 0
    purc_variant_t with = frame->ctnt_var;
    if (with != PURC_VARIANT_INVALID) {
        // FIXME: unify
        bool ok;
        uint64_t u64;
        ok = purc_variant_cast_to_ulongint(with, &u64, false);
        struct pcvcm_node *vcm_content;
        vcm_content = (struct pcvcm_node*)u64;

        purc_variant_t v = pcintr_eval_vcm(stack, vcm_content, frame->silently);
        if (purc_variant_is_string(v)) {
            size_t sz;
            const char *sv = purc_variant_get_string_const_ex(v, &sz);
            pcintr_util_new_content(frame->owner->doc,
                    frame->edom_element, PCDOC_OP_DISPLACE, sv, sz,
                    PURC_VARIANT_INVALID);
        }
        else {
            char *sv = pcvariant_to_string(v);
            pcintr_util_new_content(frame->owner->doc,
                    frame->edom_element, PCDOC_OP_DISPLACE, sv, 0,
                    PURC_VARIANT_INVALID);
            free(sv);
        }
        purc_variant_unref(v);
    }

    purc_clr_error();
#endif

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

    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)frame->ctxt;
    if (ctxt) {
        ctxt_for_undefined_destroy(ctxt);
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
    UNUSED_PARAM(frame);
    UNUSED_PARAM(content);

    int err = 0;
    pcintr_stack_t stack = &co->stack;
    if (stack->except) {
        goto out;
    }

    // int r;
    struct pcvcm_node *vcm = content->vcm;
    if (!vcm) {
        goto out;
    }

    purc_variant_t v = pcintr_eval_vcm(&co->stack, vcm, frame->silently);
    if (v == PURC_VARIANT_INVALID) {
        err = purc_get_last_error();
        goto out;
    }

    if (purc_variant_is_string(v)) {
        size_t sz;
        const char *text = purc_variant_get_string_const_ex(v, &sz);
        pcintr_util_new_text_content(frame->owner->doc,
                frame->edom_element, PCDOC_OP_APPEND, text, sz,
                !stack->inherit, false);
        purc_variant_unref(v);
    }
    else {
        char *sv = pcvariant_to_string(v);
        pcintr_util_new_content(frame->owner->doc,
                frame->edom_element, PCDOC_OP_APPEND, sv, 0,
                PURC_VARIANT_INVALID, !stack->inherit, false);
        free(sv);
        purc_variant_unref(v);
    }

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

    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)frame->ctxt;

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

struct pcintr_element_ops* pcintr_get_undefined_ops(void)
{
    return &ops;
}

