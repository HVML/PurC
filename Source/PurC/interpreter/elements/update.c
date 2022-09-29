/**
 * @file update.c
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
#include "private/dvobjs.h"
#include "purc-runloop.h"
#include "private/stringbuilder.h"

#include "html/interfaces/document.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_update {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                to;
    purc_variant_t                at;
    purc_variant_t                from;
    purc_variant_t                from_result;
    purc_variant_t                with;
    enum pchvml_attr_operator     with_op;
    pcintr_attribute_op           with_eval;

    purc_variant_t                literal;
    purc_variant_t                template_data_type;
};

static void
ctxt_for_update_destroy(struct ctxt_for_update *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->to);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from_result);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->literal);
        PURC_VARIANT_SAFE_CLEAR(ctxt->template_data_type);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_update_destroy((struct ctxt_for_update*)ctxt);
}

static purc_variant_t
get_source_by_with(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
    purc_variant_t with)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(co);
    if (purc_variant_is_type(with, PURC_VARIANT_TYPE_STRING)) {
        purc_variant_ref(with);
        return with;
    }
    else if (purc_variant_is_native(with)) {
        purc_variant_t type = pcintr_template_get_type(with);
        if (type) {
            struct ctxt_for_update *ctxt;
            ctxt = (struct ctxt_for_update*)frame->ctxt;
            ctxt->template_data_type = purc_variant_ref(type);
        }
        return pcintr_template_expansion(with);
    }
    else {
        purc_variant_ref(with);
        return with;
    }
}

static purc_variant_t
get_source_by_from(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
    purc_variant_t from, purc_variant_t with)
{
    UNUSED_PARAM(frame);
    PC_ASSERT(with == PURC_VARIANT_INVALID);

    const char* uri = purc_variant_get_string_const(from);
    return pcintr_load_from_uri(&co->stack, uri);
}

static int
merge_object(pcintr_stack_t stack,
        purc_variant_t on, purc_variant_t at,
        purc_variant_t src)
{
    UNUSED_PARAM(stack);

    const char *s_at = "";
    if (at != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(at));
        s_at = purc_variant_get_string_const(at);
    }

    if (s_at[0] == '\0') {
        bool ok;
        ok = purc_variant_object_merge_another(on, src, true);
        if (!ok) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        return 0;
    }

    PC_DEBUGX("s_at: %s", s_at);
    PC_ASSERT(0);
    return -1;
}

static int
displace_object(pcintr_stack_t stack,
        purc_variant_t on, purc_variant_t at,
        purc_variant_t src,
        pcintr_attribute_op with_eval)
{
    UNUSED_PARAM(stack);

    const char *s_at = "";
    if (at != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(at));
        s_at = purc_variant_get_string_const(at);
    }

    if (s_at[0] == '.') {
        s_at += 1;
        purc_variant_t k = purc_variant_make_string(s_at, true);
        if (k == PURC_VARIANT_INVALID)
            return -1;
        purc_variant_t o = purc_variant_object_get(on, k);
        PC_ASSERT(o != PURC_VARIANT_INVALID);
        purc_variant_t v = with_eval(o, src);
        PC_ASSERT(v != PURC_VARIANT_INVALID);

        bool ok;
        ok = purc_variant_object_set(on, k, v);
        purc_variant_unref(v);
        purc_variant_unref(k);
        if (!ok) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        return 0;
    }

    PC_DEBUGX("s_at: %s", s_at);
    PC_ASSERT(0);
    return -1;
}

static int
update_object(pcintr_stack_t stack,
        purc_variant_t on, purc_variant_t at, purc_variant_t to,
        purc_variant_t src,
        pcintr_attribute_op with_eval)
{
    const char *s_to = "displace";
    if (to != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(to));
        s_to = purc_variant_get_string_const(to);
    }

    if (strcmp(s_to, "merge") == 0) {
        return merge_object(stack, on, at, src);
    }

    if (strcmp(s_to, "displace") == 0) {
        return displace_object(stack, on, at, src, with_eval);
    }

    PC_DEBUGX("s_to: %s", s_to);
    PC_ASSERT(0);
    return -1;
}

static int
update_array(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t src,
        pcintr_attribute_op with_eval)
{
    UNUSED_PARAM(with_eval);
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(to != PURC_VARIANT_INVALID);

    purc_variant_t target = on;
    purc_variant_t at  = ctxt->at;
    if (at != PURC_VARIANT_INVALID) {
        double d = purc_variant_numberify(at);
        size_t idx = d;
        purc_variant_t v = purc_variant_array_get(on, idx);
        PC_ASSERT(v != PURC_VARIANT_INVALID);
        if (v == PURC_VARIANT_INVALID)
            return -1;
        PC_ASSERT(v != PURC_VARIANT_INVALID); // Not implemented yet
        target = v;
    }

    const char *op = purc_variant_get_string_const(to);
    PC_ASSERT(op);

    if (strcmp(op, "append") == 0) {
        bool ok = purc_variant_array_append(target, src);
        return ok ? 0 : -1;
    }

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
            "vdom attribute '%s'='%s' for element <%s>",
            pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, TO)),
            op, element->tag_name);
    PC_ASSERT(0);

    return -1;
}

static int
update_set(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t src,
        pcintr_attribute_op with_eval)
{
    UNUSED_PARAM(with_eval);
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(to != PURC_VARIANT_INVALID);

    purc_variant_t at  = ctxt->at;
    if (at != PURC_VARIANT_INVALID) {
        PC_ASSERT(0); // Not implemented yet
        return -1;
    }

    const char *op = purc_variant_get_string_const(to);
    PC_ASSERT(op);
    if (strcmp(op, "displace")==0) {
        if (!purc_variant_is_type(src, PURC_VARIANT_TYPE_ARRAY)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_SET)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }

        // TODO
        if (!purc_variant_container_displace(on, src, frame->silently)) {
            return -1;
        }
        return 0;
    }
    if (strcmp(op, "unite")==0) {
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_SET)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }

        // TODO
        if (!purc_variant_set_unite(on, src, frame->silently)) {
            return -1;
        }
        return 0;
    }
    if (strcmp(op, "overwrite")==0) {
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_SET)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }

        // TODO
        if (!purc_variant_set_overwrite(on, src, frame->silently)) {
            return -1;
        }
        return 0;
    }
    PC_DEBUGX("op: %s", op);
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static pcdoc_operation convert_operation(const char *to)
{
    pcdoc_operation op;
    if (strcmp(to, "append") == 0) {
        op = PCDOC_OP_APPEND;
    }
    else if (strcmp(to, "prepend") == 0) {
        op = PCDOC_OP_PREPEND;
    }
    else if (strcmp(to, "insertBefore") == 0) {
        op = PCDOC_OP_INSERTBEFORE;
    }
    else if (strcmp(to, "insertAfter") == 0) {
        op = PCDOC_OP_INSERTAFTER;
    }
    else if (strcmp(to, "displace") == 0) {
        op = PCDOC_OP_DISPLACE;
    }
    else {
        op = PCDOC_OP_UNKNOWN;
    }

    return op;
}

static int
update_target_child(pcintr_stack_t stack, pcdoc_element_t target,
        const char *to, purc_variant_t src,
        pcintr_attribute_op with_eval, purc_variant_t template_data_type)
{
    UNUSED_PARAM(stack);
    char *t = NULL;
    const char *s = "undefined";
    if (purc_variant_is_undefined(src)) {
        s = "undefined";
    }
    else if (purc_variant_is_string(src)) {
        s = purc_variant_get_string_const(src);
    }
    else {
        ssize_t n = purc_variant_stringify_alloc(&t, src);
        if (n<=0) {
            PC_ASSERT(purc_get_last_error());
            return -1;
        }
        s = t;
    }

    UNUSED_PARAM(with_eval);

    pcdoc_operation op = convert_operation(to);
    if (op != PCDOC_OP_UNKNOWN) {
        pcintr_util_new_content(stack->doc, target, op, s, 0,
                template_data_type, true);
        if (t)
            free(t);

        return 0;
    }

    if (t)
        free(t);

    PC_DEBUGX("to: %s", to);
    PC_ASSERT(0);
    return -1;
}

static int
update_target_content(pcintr_stack_t stack, pcdoc_element_t target,
        const char *to, purc_variant_t src,
        pcintr_attribute_op with_eval)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(with_eval);

    if (purc_variant_is_string(src)) {
        size_t len;
        const char *s = purc_variant_get_string_const_ex(src, &len);

        pcdoc_operation op = convert_operation(to);
        if (op != PCDOC_OP_UNKNOWN) {
            pcdoc_text_node_t node = pcintr_util_new_text_content(stack->doc,
                    target, op, s, len, true);
            PC_ASSERT(node);
            return 0;
        }

        PC_DEBUGX("to: %s", to);
        PC_ASSERT(0);
        return -1;
    }
    PRINT_VARIANT(src);
    PC_ASSERT(0);
    return -1;
}

static int
displace_target_attr(pcintr_stack_t stack, pcdoc_element_t target,
        const char *at, purc_variant_t src,
        pcintr_attribute_op with_eval)
{
    UNUSED_PARAM(stack);
    PC_ASSERT(with_eval);
    const char *origin = NULL;
    size_t len;
    pcdoc_element_get_attribute(stack->doc, target,
            (const char*)at, &origin, &len);

    purc_variant_t v;
    if (origin) {
        purc_variant_t l = purc_variant_make_string_static(origin, true);
        if (l == PURC_VARIANT_INVALID)
            return -1;

        v = with_eval(l, src);
        purc_variant_unref(l);
        if (v == PURC_VARIANT_INVALID)
            return -1;
    }
    else {
        v = purc_variant_ref(src);
    }

    size_t sz;
    const char *s = purc_variant_get_string_const_ex(v, &sz);
    if (!s) {
        purc_variant_unref(v);
        return -1;
    }

    int r;
    r = pcintr_util_set_attribute(stack->doc, target,
            PCDOC_OP_DISPLACE, at, s, sz, true);
    purc_variant_unref(v);
    return r ? -1 : 0;
}

static int
update_target_attr(pcintr_stack_t stack, pcdoc_element_t target,
        const char *at, const char *to, purc_variant_t src,
        pcintr_attribute_op with_eval)
{
    UNUSED_PARAM(stack);
    if (purc_variant_is_string(src)) {
        if (strcmp(to, "displace") == 0) {
            return displace_target_attr(stack, target, at, src, with_eval);
        }
        PC_DEBUGX("to: %s", to);
        PC_ASSERT(0);
        return -1;
    }
    char *sv = pcvariant_to_string(src);
    PC_ASSERT(sv);

    int r;
    r = pcintr_util_set_attribute(stack->doc, target,
            PCDOC_OP_DISPLACE, at, sv, 0, true);
    PC_ASSERT(r == 0);
    free(sv);

    return 0;
}

static int
update_target(pcintr_stack_t stack, pcdoc_element_t target,
        purc_variant_t at, purc_variant_t to, purc_variant_t src,
        pcintr_attribute_op with_eval, purc_variant_t template_data_type)
{
    const char *s_to = "displace";
    if (to != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(to));
        s_to = purc_variant_get_string_const(to);
    }

    const char *s_at = NULL;
    if (at != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(at));
        s_at = purc_variant_get_string_const(at);
        PC_ASSERT(s_at);
    }

    if (!s_at) {
        return update_target_child(stack, target, s_to, src, with_eval,
                template_data_type);
    }
    if (strcmp(s_at, "textContent") == 0) {
        return update_target_content(stack, target, s_to, src, with_eval);
    }
    if (strncmp(s_at, "attr.", 5) == 0) {
        s_at += 5;
        return update_target_attr(stack, target, s_at, s_to, src, with_eval);
    }

    fprintf(stderr, "at: %s\n", s_at);

    PRINT_VARIANT(at);
    PC_ASSERT(0);
    return -1;
}

static int
update_elements(pcintr_stack_t stack,
        purc_variant_t elems, purc_variant_t at, purc_variant_t to,
        purc_variant_t src,
        pcintr_attribute_op with_eval,
        purc_variant_t template_data_type)
{
    PC_ASSERT(purc_variant_is_native(elems));
    size_t idx = 0;
    while (1) {
        pcdoc_element_t target;
        target = pcdvobjs_get_element_from_elements(elems, idx++);
        if (!target)
            break;
        int r = update_target(stack, target, at, to, src, with_eval,
                template_data_type);
        if (r)
            return -1;
    }

    return 0;
}

static int
process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t src,
        pcintr_attribute_op with_eval)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t at  = ctxt->at;
    purc_variant_t template_data_type  = ctxt->template_data_type;
    PC_ASSERT(on != PURC_VARIANT_INVALID);

    /* FIXME: what if array of elements? */
    enum purc_variant_type type = purc_variant_get_type(on);
    if (type == PURC_VARIANT_TYPE_NATIVE) {
        // const char *s = purc_variant_get_string_const(src);
        // PC_ASSERT(to != PURC_VARIANT_INVALID);
        return update_elements(&co->stack, on, at, to, src, with_eval,
                template_data_type);
    }
    if (type == PURC_VARIANT_TYPE_OBJECT) {
        return update_object(&co->stack, on, at, to, src, with_eval);
    }
    if (type == PURC_VARIANT_TYPE_ARRAY) {
        return update_array(co, frame, src, with_eval);
    }
    if (type == PURC_VARIANT_TYPE_SET) {
        return update_set(co, frame, src, with_eval);
    }
    if (type == PURC_VARIANT_TYPE_STRING) {
        const char *s = purc_variant_get_string_const(on);

        purc_document_t doc = co->stack.doc;
        purc_variant_t elems = pcdvobjs_elements_by_css(doc, s);
        PC_ASSERT(elems != PURC_VARIANT_INVALID);
        pcdoc_element_t elem;
        elem = pcdvobjs_get_element_from_elements(elems, 0);
        int r = 0;
        if (elem) {
            r = update_elements(&co->stack, elems, at, to, src, with_eval,
                    template_data_type);
        }
        purc_variant_unref(elems);
        return r ? -1 : 0;
    }
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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
process_attr_to(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt->to != PURC_VARIANT_INVALID) {
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
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    const char *s_to = purc_variant_get_string_const(val);
    if (strcmp(s_to, "displace")) {
        if (ctxt->with_op != PCHVML_ATTRIBUTE_OPERATOR) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
    }

    ctxt->to = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt->with != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (attr->op != PCHVML_ATTRIBUTE_OPERATOR) {
        if (ctxt->to != PURC_VARIANT_INVALID) {
            const char *s_to = purc_variant_get_string_const(ctxt->to);
            if (strcmp(s_to, "displace")) {
                purc_set_error(PURC_ERROR_NOT_SUPPORTED);
                return -1;
            }
        }
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (ctxt->from != PURC_VARIANT_INVALID) {
        if (!purc_variant_is_string(val)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        if (attr->op != PCHVML_ATTRIBUTE_OPERATOR) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
    }

    ctxt->with = val;
    purc_variant_ref(val);

    ctxt->with_op   = attr->op;
    ctxt->with_eval = pcintr_attribute_get_op(attr->op);
    if (!ctxt->with_eval)
        return -1;

    return 0;
}

static int
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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

    if (ctxt->with != PURC_VARIANT_INVALID) {
        if (!purc_variant_is_string(ctxt->with)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        if (ctxt->with_op != PCHVML_ATTRIBUTE_OPERATOR) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
    }

    ctxt->from = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val, attr);
    }

    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TO)) == name) {
        return process_attr_to(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FROM)) == name) {
        return process_attr_from(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
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

    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    ctxt->with_op = PCHVML_ATTRIBUTE_OPERATOR;

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return ctxt;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (ctxt->on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <%s>",
                element->tag_name);
        return ctxt;
    }

    // FIXME
    // load from network
    purc_variant_t from = ctxt->from;
    if (from != PURC_VARIANT_INVALID && purc_variant_is_string(from)) {
        //PC_ASSERT(0);
        if (ctxt->with != PURC_VARIANT_INVALID) {
            PC_ASSERT(ctxt->with_op == PCHVML_ATTRIBUTE_OPERATOR);
        }
        purc_variant_t v;
        v = get_source_by_from(stack->co, frame, ctxt->from, ctxt->with);
        if (v == PURC_VARIANT_INVALID) {
            PC_ASSERT(purc_get_last_error());
            return ctxt;
        }
        PURC_VARIANT_SAFE_CLEAR(ctxt->from_result);
        ctxt->from_result = v;
    }

    purc_variant_t content = pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_CARET);
    if (content && !purc_variant_is_undefined(content)) {
        ctxt->literal = purc_variant_ref(content);
    }
    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt) {
        ctxt_for_update_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(element);

    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);

    if (ctxt->from || ctxt->with) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "no element is permitted "
                "since `from/with` attribute already set");
        return -1;
    }

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
    PC_ASSERT(comment);
    return 0;
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    pcintr_stack_t stack = &co->stack;

    if (stack->except)
        return 0;

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);

    if (ctxt->from) {
        if (ctxt->from_result != PURC_VARIANT_INVALID) {
            PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
            frame->ctnt_var = ctxt->from_result;
            purc_variant_ref(ctxt->from_result);

            return process(co, frame, ctxt->from_result, ctxt->with_eval);
        }
    }
    if (!ctxt->from && ctxt->with) {
        purc_variant_t src;
        src = get_source_by_with(co, frame, ctxt->with);
        PC_ASSERT(src != PURC_VARIANT_INVALID);

        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = src;
        purc_variant_ref(src);

        int r = process(co, frame, src, ctxt->with_eval);
        purc_variant_unref(src);
        return r ? -1 : 0;
    }
    if (ctxt->literal != PURC_VARIANT_INVALID) {
        pcintr_attribute_op with_eval;
        with_eval = pcintr_attribute_get_op(PCHVML_ATTRIBUTE_OPERATOR);
        if (!with_eval) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        frame->ctnt_var = ctxt->literal;
        purc_variant_ref(ctxt->literal);
        return process(co, frame, ctxt->literal, with_eval);
    }

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'with/from' for element <%s>",
                element->tag_name);

    return -1;
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);

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

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

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

struct pcintr_element_ops* pcintr_get_update_ops(void)
{
    return &ops;
}

