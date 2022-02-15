/**
 * @file init.c
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

#include "internal.h"

#include "private/debug.h"
#include "private/runloop.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define TO_DEBUG 1

struct ctxt_for_init {
    struct pcvdom_node           *curr;

    purc_variant_t                as;
    purc_variant_t                uniquely;
    purc_variant_t                from;
    purc_variant_t                with;
    purc_variant_t                via;

    unsigned int                  under_head:1;
};

static void
ctxt_for_init_destroy(struct ctxt_for_init *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->uniquely);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->via);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_init_destroy((struct ctxt_for_init*)ctxt);
}

static int
post_process_bind_scope_var(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame,
        purc_variant_t name, purc_variant_t val)
{
    struct pcvdom_element *element = frame->scope;
    PC_ASSERT(element);

    const char *s_name = purc_variant_get_string_const(name);
    PC_ASSERT(s_name);

    bool ok;
    struct ctxt_for_init *ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->under_head) {
        ok = purc_bind_document_variable(co->stack->vdom, s_name, val);
    } else {
        element = pcvdom_element_parent(element);
        PC_ASSERT(element);
        ok = pcintr_bind_scope_variable(element, s_name, val);
    }

    return ok ? 0 : -1;
}

static int
post_process_array(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t name)
{
    struct ctxt_for_init *ctxt = (struct ctxt_for_init*)frame->ctxt;
    purc_variant_t via;
    via = ctxt->via;

    if (via == PURC_VARIANT_INVALID) {
        return post_process_bind_scope_var(co, frame, name, frame->ctnt_var);
    }
    purc_variant_t set;
    set = purc_variant_make_set(0, via, PURC_VARIANT_INVALID);
    if (set == PURC_VARIANT_INVALID)
        return -1;

    // TODO
#if 1
    if (!purc_variant_container_displace(set, frame->ctnt_var, true)) {
        purc_variant_unref(set);
        return -1;
    }
#else
    purc_variant_t val;
    size_t idx;
    foreach_value_in_variant_array(frame->ctnt_var, val, idx)
        (void)idx;
        bool ok = purc_variant_is_type(val, PURC_VARIANT_TYPE_OBJECT);
        if (ok) {
            ok = purc_variant_set_add(set, val, true);
        }
        if (!ok) {
            purc_variant_unref(set);
            return -1;
        }
    end_foreach;
#endif

    int r = post_process_bind_scope_var(co, frame, name, set);
    purc_variant_unref(set);
    return r ? -1 : 0;
}

static int
post_process_object(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t name)
{
    purc_variant_t val = frame->ctnt_var;

    return post_process_bind_scope_var(co, frame, name, val);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    purc_variant_t name;
    name = ctxt->as;
    if (name == PURC_VARIANT_INVALID)
        return -1;

    if (frame->ctnt_var != PURC_VARIANT_INVALID) {
        if (purc_variant_is_type(frame->ctnt_var, PURC_VARIANT_TYPE_ARRAY)) {
            return post_process_array(co, frame, name);
        }
        if (purc_variant_is_type(frame->ctnt_var, PURC_VARIANT_TYPE_OBJECT)) {
            return post_process_object(co, frame, name);
        }
        purc_set_error(PURC_ERROR_NOT_EXISTS);
        return -1;
   }

    return 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
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
process_attr_uniquely(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->uniquely != PURC_VARIANT_INVALID) {
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
    ctxt->uniquely = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
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
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->with != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (ctxt->from != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                "vdom attribute '%s' for element <%s> conflicts with '%s'",
                purc_atom_to_string(name), element->tag_name,
                pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, WITH)));
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
process_attr_via(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->via != PURC_VARIANT_INVALID) {
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
    ctxt->via = val;
    purc_variant_ref(val);

    return 0;
}

static int
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_ASSIGNMENT);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, UNIQUELY)) == name) {
        return process_attr_uniquely(frame, element, name, val);
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

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "vdom attribute '%s' for element <%s>",
            purc_atom_to_string(name), element->tag_name);

    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == purc_get_stack());
    if (pcintr_check_insertion_mode_for_normal_element(stack))
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return NULL;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return NULL;

    purc_variant_t with;
    with = ctxt->with;
    if (with != PURC_VARIANT_INVALID) {
        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = with;
        purc_variant_ref(with);
    }

    // FIXME
    // load from network
    purc_variant_t from;
    from = ctxt->from;
    if (from != PURC_VARIANT_INVALID && purc_variant_is_string(from)) {
        const char* uri = purc_variant_get_string_const(from);
        purc_variant_t v = pcintr_load_from_uri(stack, uri);
        if (v == PURC_VARIANT_INVALID)
            return NULL;
        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = v;
    }

    struct pcvcm_node *vcm_content = element->vcm_content;
    if (vcm_content) {
        purc_variant_t v = pcvcm_eval(vcm_content, stack);
        if (v == PURC_VARIANT_INVALID)
            return NULL;

        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = v;
    }

    while ((element=pcvdom_element_parent(element))) {
        if (element->tag_id == PCHVML_TAG_HEAD) {
            ctxt->under_head = 1;
        }
    }
    purc_clr_error();

    r = post_process(&stack->co, frame);
    if (r)
        return NULL;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt) {
        ctxt_for_init_destroy(ctxt);
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

struct pcintr_element_ops* pcintr_get_init_ops(void)
{
    return &ops;
}

