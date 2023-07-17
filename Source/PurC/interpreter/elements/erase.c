/**
 * @file erase.c
 * @author Xue Shuming
 * @date 2022/05/24
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
#include "private/dvobjs.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>
#include <errno.h>

struct ctxt_for_erase {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                at;
    purc_variant_t                in;
};

static void
ctxt_for_erase_destroy(struct ctxt_for_erase *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->in);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_erase_destroy((struct ctxt_for_erase*)ctxt);
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_erase *ctxt;
    ctxt = (struct ctxt_for_erase*)frame->ctxt;
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
    struct ctxt_for_erase *ctxt;
    ctxt = (struct ctxt_for_erase*)frame->ctxt;
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
process_attr_in(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_erase *ctxt;
    ctxt = (struct ctxt_for_erase*)frame->ctxt;
    if (ctxt->in != PURC_VARIANT_INVALID) {
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
    ctxt->in = val;
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
    UNUSED_PARAM(attr);
    UNUSED_PARAM(ud);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, IN)) == name) {
        return process_attr_in(frame, element, name, val);
    }

    /* ignore other attr */
    return 0;
}

static purc_variant_t
element_erase(pcintr_stack_t stack, purc_variant_t on, purc_variant_t at,
        bool silently)
{
    UNUSED_PARAM(on);
    UNUSED_PARAM(at);
    UNUSED_PARAM(silently);
    purc_variant_t ret = PURC_VARIANT_INVALID;
    const char *s = purc_variant_get_string_const(on);
    purc_document_t doc = stack->doc;
    purc_variant_t elems = pcdvobjs_elements_by_css(doc, s);
    if (!elems) {
        ret = purc_variant_make_ulongint(0);
        goto out;
    }

    if (at == PURC_VARIANT_INVALID) {
        struct purc_native_ops *ops = purc_variant_native_get_ops(elems);
        if (!ops || ops->eraser == NULL) {
            ret = purc_variant_make_ulongint(0);
        }
        else {
            void *entity = purc_variant_native_get_entity(elems);
            ret = ops->eraser(entity, silently ? PCVRT_CALL_FLAG_SILENTLY : 0);
        }
    }
    else {
        if (!purc_variant_is_string(at)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }
        const char *s_at = purc_variant_get_string_const(at);
        if (!s_at) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }

        if (strncmp(s_at, "attr.", 5) != 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }
        s_at += 5;
        int nr_remove = 0;
        size_t idx = 0;
        while (1) {
            pcdoc_element_t target;
            target = pcdvobjs_get_element_from_elements(elems, idx++);
            if (!target) {
                break;
            }

            if (pcintr_util_remove_attribute(doc, target, s_at, true, false) == 0) {
                nr_remove++;
            }
        }
        ret = purc_variant_make_ulongint(nr_remove);
    }
out:
    PURC_VARIANT_SAFE_CLEAR(elems);
    return ret;
}

static purc_variant_t
object_erase(purc_variant_t on, purc_variant_t at, bool silently)
{
    purc_variant_t ret;
    if (at) {
        if (!purc_variant_is_string(at)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }
        const char *s_at = purc_variant_get_string_const(at);
        char *names = strdup(s_at);
        if (!names) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }

        int nr_remove = 0;
        char *ctxt = names;
        char *token;
        while ((token = strtok_r(ctxt, " ", &ctxt))) {
            if (strlen(token) > 0 && token[0] == '.'
                    && purc_variant_object_remove_by_ckey(on, token + 1,
                        silently)) {
                    nr_remove++;
            }
        }
        free(names);
        ret = purc_variant_make_ulongint(nr_remove);
    }
    else {
        size_t sz = 0;
        if (purc_variant_object_size(on, &sz)
                && sz > 0
                && pcvariant_object_clear(on, silently)
                ) {
            ret = purc_variant_make_ulongint(sz);
        }
        else {
            ret = purc_variant_make_ulongint(0);
        }
    }
out:
    return ret;
}

static purc_variant_t
array_erase(purc_variant_t on, purc_variant_t at, bool silently)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    if (at) {
        if (!purc_variant_is_array(at)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }
        purc_variant_t idx = purc_variant_array_get(at, 0);
        int64_t index = -1;
        bool cast = purc_variant_cast_to_longint(idx, &index, false);

        if (!cast || index < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }

        size_t nr_on = purc_variant_array_get_size(on);
        if (((size_t)index < nr_on) && purc_variant_array_remove(on, index)) {
            ret = purc_variant_make_ulongint(0);
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
        }
    }
    else {
        bool result = pcvariant_array_clear(on, silently);
        ret = purc_variant_make_boolean(result);
    }
out:
    return ret;
}

static purc_variant_t
set_erase(purc_variant_t on, purc_variant_t at, bool silently)
{
    purc_variant_t ret;
    if (at) {
        if (!purc_variant_is_array(at)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }
        purc_variant_t idx = purc_variant_array_get(at, 0);
        int64_t index = -1;
        bool cast = purc_variant_cast_to_longint(idx, &index, false);

        if (!cast || index < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
            goto out;
        }

        size_t nr_on = purc_variant_set_get_size(on);
        purc_variant_t v_removed = PURC_VARIANT_INVALID;
        if (((size_t)index < nr_on) &&
                (v_removed=purc_variant_set_remove_by_index(on, index)))
        {
            ret = purc_variant_make_ulongint(0);
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret = PURC_VARIANT_INVALID;
        }
        PURC_VARIANT_SAFE_CLEAR(v_removed);
    }
    else {
        bool result = pcvariant_set_clear(on, silently);
        ret = purc_variant_make_boolean(result);
    }
out:
    return ret;
}

static purc_variant_t
native_erase(purc_variant_t on, purc_variant_t at, bool silently)
{
    UNUSED_PARAM(at);
    struct purc_native_ops *ops = purc_variant_native_get_ops(on);
    if (!ops || ops->eraser == NULL) {
        return purc_variant_make_ulongint(0);
    }
    void *entity = purc_variant_native_get_entity(on);
    return ops->eraser(entity, silently ? PCVRT_CALL_FLAG_SILENTLY : 0);
}


static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except)
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_erase *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_erase*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

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

    if (ctxt->on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "`on` not specified");
        return ctxt;
    }

    purc_variant_t in;
    in = ctxt->in;
    if (in != PURC_VARIANT_INVALID) {
        if (!purc_variant_is_string(in)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return ctxt;
        }

        purc_variant_t elements = pcintr_doc_query(stack->co,
                purc_variant_get_string_const(in), frame->silently);
        if (elements == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return ctxt;
        }

        pcintr_set_at_var(frame, elements);
        purc_variant_unref(elements);
    }

    purc_variant_t ret;
    enum purc_variant_type type = purc_variant_get_type(ctxt->on);
    switch (type) {
    case PURC_VARIANT_TYPE_STRING:
        ret = element_erase(stack, ctxt->on, ctxt->at, frame->silently);
        break;

    case PURC_VARIANT_TYPE_OBJECT:
        ret = object_erase(ctxt->on, ctxt->at, frame->silently);
        break;

    case PURC_VARIANT_TYPE_ARRAY:
        ret = array_erase(ctxt->on, ctxt->at, frame->silently);
        break;

    case PURC_VARIANT_TYPE_SET:
        ret = set_erase(ctxt->on, ctxt->at, frame->silently);
        break;

    case PURC_VARIANT_TYPE_NATIVE:
        ret = native_erase(ctxt->on, ctxt->at, frame->silently);
        break;

    default:
        ret = purc_variant_make_ulongint(0);
        break;
    }

    if (ret) {
        pcintr_set_question_var(frame, ret);
        purc_variant_unref(ret);
    }
    purc_clr_error();

    // NOTE: no elements to process since it succeeds
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

    struct ctxt_for_erase *ctxt;
    ctxt = (struct ctxt_for_erase*)frame->ctxt;
    if (ctxt) {
        ctxt_for_erase_destroy(ctxt);
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

    return 0;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(content);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    return -1;
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
    UNUSED_PARAM(frame);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

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

    struct ctxt_for_erase *ctxt;
    ctxt = (struct ctxt_for_erase*)frame->ctxt;

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
                if (on_element(co, frame, element)) {
                    return NULL;
                }
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

struct pcintr_element_ops* pcintr_get_erase_ops(void)
{
    return &ops;
}

