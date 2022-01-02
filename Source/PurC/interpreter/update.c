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
    UNUSED_PARAM(frame);
    if (purc_variant_is_type(with, PURC_VARIANT_TYPE_ULONGINT)) {
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
        purc_variant_ref(with);
        return with;
    }
    else {
        PC_ASSERT(0); // Not implemented yet
        return PURC_VARIANT_INVALID;
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

static purc_variant_t
get_source(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    purc_variant_t with;
    with = purc_variant_object_get_by_ckey(frame->attr_vars, "with");
    if (with != PURC_VARIANT_INVALID)
        return get_source_by_with(co, frame, with);

    purc_variant_t from;
    from = purc_variant_object_get_by_ckey(frame->attr_vars, "from");
    if (from != PURC_VARIANT_INVALID)
        return get_source_by_from(co, frame, from);

    if (frame->ctnt_var != PURC_VARIANT_INVALID)
        purc_variant_ref(frame->ctnt_var);

    if (frame->ctnt_var)
        purc_clr_error();

    return frame->ctnt_var;
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
        purc_variant_t v;
        struct rb_node *n;
        foreach_value_in_variant_set_safe_x(on, v, n)
            bool ok = purc_variant_set_remove(on, v);
            PC_ASSERT(ok); // FIXME: debug-only-now
        end_foreach;
        foreach_value_in_variant_array(src, v)
            char buf[1024];
            pcvariant_serialize(buf, sizeof(buf), v);
            D("v: %s", buf);
            bool ok = purc_variant_set_add(on, v, true);
            if (!ok)
                return -1;
        end_foreach;
        return 0;
    }
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
        pcintr_printf_to_fragment(co->stack, on, to, "%s", s);
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
        pcedom_element_t *body = (pcedom_element_t*)doc->body;
        pcedom_document_t *document = (pcedom_document_t*)doc;
        pcedom_collection_t *collection;
        collection = pcedom_collection_create(document);
        PC_ASSERT(collection);
        unsigned int ui;
        ui = pcedom_collection_init(collection, 10);
        PC_ASSERT(ui == 0);
        ui = pcedom_elements_by_attr(body, collection,
                (const unsigned char*)"id", 2,
                (const unsigned char*)s+1, strlen(s+1),
                false);
        PC_ASSERT(ui == 0);
        for (unsigned int i=0; i<pcedom_collection_length(collection); ++i) {
            pcedom_node_t *node;
            node = pcedom_collection_node(collection, i);
            PC_ASSERT(node);
            pcedom_element_t *elem = (pcedom_element_t*)node;
            purc_variant_t o = pcintr_make_element_variant(elem);
            PC_ASSERT(o != PURC_VARIANT_INVALID);
            const char *content = purc_variant_get_string_const(src);
            pcintr_printf_to_fragment(co->stack, o, at, "%s", content);
        }
        pcedom_collection_destroy(collection, true);
        return 0;
    }
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);

    // TODO: '$@'
    purc_variant_t on;
    on = purc_variant_object_get_by_ckey(frame->attr_vars, "on");
    if (on == PURC_VARIANT_INVALID)
        return -1;
    PURC_VARIANT_SAFE_CLEAR(ctxt->on);
    ctxt->on = on;
    purc_variant_ref(on);

    purc_variant_t to;
    to = purc_variant_object_get_by_ckey(frame->attr_vars, "to");
    if (to != PURC_VARIANT_INVALID) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->to);
        ctxt->to = to;
        purc_variant_ref(to);
    }

    purc_variant_t at;
    at = purc_variant_object_get_by_ckey(frame->attr_vars, "at");
    if (at != PURC_VARIANT_INVALID) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        ctxt->at = at;
        purc_variant_ref(at);
    }

    purc_variant_t src = get_source(co, frame);
    if (src == PURC_VARIANT_INVALID)
        return -1;
    PURC_VARIANT_SAFE_CLEAR(ctxt->src);
    ctxt->src = src;

    purc_clr_error();
    return process(co, frame);
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

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);
    D("<%s>", element->tag_name);

    int r;
    r = pcintr_element_eval_attrs(frame, element);
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

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;
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

