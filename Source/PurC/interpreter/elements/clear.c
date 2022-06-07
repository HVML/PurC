/**
 * @file clear.c
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

struct ctxt_for_clear {
    struct pcvdom_node           *curr;
    purc_variant_t                on;
};

static void
ctxt_for_clear_destroy(struct ctxt_for_clear *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_clear_destroy((struct ctxt_for_clear*)ctxt);
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_clear *ctxt;
    ctxt = (struct ctxt_for_clear*)frame->ctxt;
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
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
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

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    struct ctxt_for_clear *ctxt;
    ctxt = (struct ctxt_for_clear*)calloc(1, sizeof(*ctxt));
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

    if (ctxt->on == PURC_VARIANT_INVALID) {
        return NULL;
    }

    purc_variant_t ret;
    enum purc_variant_type type = purc_variant_get_type(ctxt->on);
    switch (type) {
    case PURC_VARIANT_TYPE_STRING:
        {
            const char *s = purc_variant_get_string_const(ctxt->on);
            pchtml_html_document_t *doc = stack->doc;
            purc_variant_t elems = pcdvobjs_elements_by_css(doc, s);
            if (!elems) {
                ret = purc_variant_make_boolean(false);
                break;
            }

            struct purc_native_ops *ops = purc_variant_native_get_ops(elems);
            if (!ops || ops->cleaner == NULL) {
                ret = purc_variant_make_boolean(false);
            }
            else {
                void *entity = purc_variant_native_get_entity(elems);
                ret = ops->cleaner(entity, frame->silently);
            }
            purc_variant_unref(elems);
        }
        break;

    case PURC_VARIANT_TYPE_OBJECT:
        {
            bool result = pcvariant_object_clear(ctxt->on, frame->silently);
            ret = purc_variant_make_boolean(result);
        }
        break;

    case PURC_VARIANT_TYPE_ARRAY:
        {
            bool result = pcvariant_array_clear(ctxt->on, frame->silently);
            ret = purc_variant_make_boolean(result);
        }
        break;

    case PURC_VARIANT_TYPE_SET:
        {
            bool result = pcvariant_set_clear(ctxt->on, frame->silently);
            ret = purc_variant_make_boolean(result);
        }
        break;

    case PURC_VARIANT_TYPE_NATIVE:
        {
            struct purc_native_ops *ops = purc_variant_native_get_ops(ctxt->on);
            if (!ops || ops->cleaner == NULL) {
                ret = purc_variant_make_boolean(false);
            }
            else {
                void *entity = purc_variant_native_get_entity(ctxt->on);
                ret = ops->cleaner(entity, frame->silently);
            }
        }
        break;

    default:
        ret = purc_variant_make_boolean(false);
        break;
    }

    // TODO : set ret as result data
    purc_variant_unref(ret);
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

    struct ctxt_for_clear *ctxt;
    ctxt = (struct ctxt_for_clear*)frame->ctxt;
    if (ctxt) {
        ctxt_for_clear_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = NULL,
};

struct pcintr_element_ops* pcintr_get_clear_ops(void)
{
    return &ops;
}

