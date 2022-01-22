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

#include "internal.h"

#include "private/debug.h"
#include "private/dvobjs.h"
#include "private/runloop.h"

#include "html/interfaces/document.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define TO_DEBUG 1

struct ctxt_for_update {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                to;
    purc_variant_t                at;
    purc_variant_t                src;
};

static void
ctxt_for_update_destroy(struct ctxt_for_update *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->to);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->src);
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
    D("");
    UNUSED_PARAM(frame);
    if (purc_variant_is_type(with, PURC_VARIANT_TYPE_ULONGINT)) {
        D("");
        bool ok;
        uint64_t u64;
        ok = purc_variant_cast_to_ulongint(with, &u64, false);
        PC_ASSERT(ok);
        struct pcvcm_node *vcm_content;
        vcm_content = (struct pcvcm_node*)u64;
        PC_ASSERT(vcm_content);

        pcintr_stack_t stack = co->stack;
        PC_ASSERT(stack);

        return pcvcm_eval(vcm_content, stack);
    }
    else if (purc_variant_is_type(with, PURC_VARIANT_TYPE_STRING)) {
        D("");
        purc_variant_ref(with);
        return with;
    }
    else {
        D("");
        purc_variant_ref(with);
        return with;
    }
}

static purc_variant_t
get_source_by_from(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
    purc_variant_t from)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    const char* uri = purc_variant_get_string_const(from);
    return pcintr_load_from_uri(uri);
}

static int
process_object(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t src = ctxt->src;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(to != PURC_VARIANT_INVALID);
    PC_ASSERT(src != PURC_VARIANT_INVALID);

    const char *op = purc_variant_get_string_const(to);
    PC_ASSERT(op);
    if (strcmp(op, "merge")==0) {
    // TODO
#if 1
        if (!purc_variant_object_merge_another(on, src, true)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
#else
        if (!purc_variant_is_type(src, PURC_VARIANT_TYPE_OBJECT)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_OBJECT)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        purc_variant_t k, v;
        foreach_key_value_in_variant_object(src, k, v)
            bool ok = purc_variant_object_set(on, k, v);
            PC_ASSERT(ok); // TODO: debug-only-now
        end_foreach;
#endif
        return 0;
    }
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static int
process_set(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t src = ctxt->src;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(to != PURC_VARIANT_INVALID);
    PC_ASSERT(src != PURC_VARIANT_INVALID);

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
#if 1
        if (!purc_variant_container_displace(on, src, true)) {
            return -1;
        }
#else
        purc_variant_t v;
        foreach_value_in_variant_set_safe(on, v)
            bool ok = purc_variant_set_remove(on, v, false);
            PC_ASSERT(ok); // FIXME: debug-only-now
        end_foreach;

        size_t curr;
        foreach_value_in_variant_array(src, v, curr)
            (void)curr;
            char buf[1024];
            pcvariant_serialize(buf, sizeof(buf), v);
            D("v: %s", buf);
            bool ok = purc_variant_set_add(on, v, true);
            if (!ok)
                return -1;
        end_foreach;
#endif
        return 0;
    }
    if (strcmp(op, "unite")==0) {
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_SET)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }

    // TODO
#if 1
        if (!purc_variant_set_unite(on, src, true)) {
            return -1;
        }
#endif
        return 0;
    }
    if (strcmp(op, "overwrite")==0) {
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_SET)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }

    // TODO
#if 1
        if (!purc_variant_set_overwrite(on, src, true)) {
            return -1;
        }
#endif
        return 0;
    }
    D("op: %s", op);
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static int
process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t at  = ctxt->at;
    purc_variant_t src = ctxt->src;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(src != PURC_VARIANT_INVALID);

    /* FIXME: what if array of elements? */
    enum purc_variant_type type = purc_variant_get_type(on);
    if (type == PURC_VARIANT_TYPE_NATIVE) {
        const char *s = purc_variant_get_string_const(src);
        // fprintf(stderr, "[%s]\n", s);
        // pcintr_printf_to_edom(stack, "%s", s);
        D("[%s]", s);
        PC_ASSERT(to != PURC_VARIANT_INVALID);
        pcintr_printf_to_fragment(co->stack, on, to, at, "%s", s);
        return 0;
    }
    if (type == PURC_VARIANT_TYPE_OBJECT) {
        return process_object(co, frame);
    }
    if (type == PURC_VARIANT_TYPE_ARRAY) {
        PC_ASSERT(0); // Not implemented yet
        return -1;
    }
    if (type == PURC_VARIANT_TYPE_SET) {
        return process_set(co, frame);
    }
    if (type == PURC_VARIANT_TYPE_STRING) {
        const char *s = purc_variant_get_string_const(on);
        PC_ASSERT(s);
        PC_ASSERT(s[0] == '#'); // TODO:
        pchtml_html_document_t *doc = co->stack->edom_gen.doc;
        pcdom_element_t *body = (pcdom_element_t*)doc->body;
        pcdom_document_t *document = (pcdom_document_t*)doc;
        pcdom_collection_t *collection;
        collection = pcdom_collection_create(document);
        PC_ASSERT(collection);
        unsigned int ui;
        ui = pcdom_collection_init(collection, 10);
        PC_ASSERT(ui == 0);
        ui = pcdom_elements_by_attr(body, collection,
                (const unsigned char*)"id", 2,
                (const unsigned char*)s+1, strlen(s+1),
                false);
        PC_ASSERT(ui == 0);
        for (unsigned int i=0; i<pcdom_collection_length(collection); ++i) {
            pcdom_node_t *node;
            node = pcdom_collection_node(collection, i);
            PC_ASSERT(node);
            pcdom_element_t *elem = (pcdom_element_t*)node;
            purc_variant_t o = pcdvobjs_make_elements(elem);
            PC_ASSERT(o != PURC_VARIANT_INVALID);
            if (purc_variant_is_string(src)) {
                const char *content = purc_variant_get_string_const(src);
                pcintr_printf_to_fragment(co->stack, o, to, at, "%s", content);
            }
            else if (purc_variant_is_number(src)) {
                double d;
                bool ok;
                ok = purc_variant_cast_to_number(src, &d, false);
                PC_ASSERT(ok);
                pcintr_printf_to_fragment(co->stack, o, to, at, "%g", d);
            }
            else {
                PC_ASSERT(0); // Not implemented yet
            }
        }
        pcdom_collection_destroy(collection, true);
        return 0;
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
    ctxt->to = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt->src != PURC_VARIANT_INVALID) {
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

    pcintr_stack_t stack = purc_get_stack();
    pcintr_coroutine_t co = &stack->co;

    ctxt->src = get_source_by_with(co, frame, val);
    PC_ASSERT(ctxt->src != PURC_VARIANT_INVALID);

    return 0;
}

static int
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt->src != PURC_VARIANT_INVALID) {
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

    pcintr_stack_t stack = purc_get_stack();
    pcintr_coroutine_t co = &stack->co;

    ctxt->src = get_source_by_from(co, frame, val);
    PC_ASSERT(ctxt->src != PURC_VARIANT_INVALID);

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
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val, void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(TO)) == name) {
        return process_attr_to(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(FROM)) == name) {
        return process_attr_from(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "vdom attribute '%s' for element <%s>",
            purc_atom_to_string(name), element->tag_name);

    return -1;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    frame->pos = pos; // ATTENTION!!

    if (pcintr_set_symbol_var_at_sign())
        return NULL;

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return NULL;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);
    D("<%s>", element->tag_name);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return NULL;

    struct pcvcm_node *vcm_content = element->vcm_content;
    if (vcm_content) {
        purc_variant_t v = pcvcm_eval(vcm_content, stack);
        if (v == PURC_VARIANT_INVALID)
            return NULL;

        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = v;
    }

    if (ctxt->on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <%s>",
                element->tag_name);
        return NULL;
    }

    if (ctxt->src == PURC_VARIANT_INVALID) {
        if (frame->ctnt_var == PURC_VARIANT_INVALID) {
            purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                    "lack of vdom attribute 'with'/'from' for element <%s>",
                    element->tag_name);
            return NULL;
        }
        ctxt->src = frame->ctnt_var;
        purc_variant_ref(frame->ctnt_var);
    }

    r = process(&stack->co, frame);
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

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt) {
        ctxt_for_update_destroy(ctxt);
        frame->ctxt = NULL;
    }

    D("</%s>", element->tag_name);
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
    char *text = content->text;
    D("content: [%s]", text);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(comment);
    char *text = comment->text;
    D("comment: [%s]", text);
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    pcintr_coroutine_t co = &stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

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
                D("");
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                on_element(co, frame, element);
                PC_ASSERT(stack->except == 0);
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            D("");
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr));
            goto again;
        case PCVDOM_NODE_COMMENT:
            D("");
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

struct pcintr_element_ops* pcintr_get_update_ops(void)
{
    return &ops;
}

