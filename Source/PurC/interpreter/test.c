/**
 * @file test.c
 * @author Xue Shuming
 * @date 2022/01/11
 * @brief The ops for <test>
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

#include "purc-executor.h"

#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define TO_DEBUG 0

struct ctxt_for_test {
    struct pcvdom_node *curr;
    purc_variant_t on;
    purc_variant_t by;
    purc_variant_t in;

    struct purc_exec_ops          ops;
    purc_exec_inst_t              exec_inst;
    purc_exec_iter_t              it;
};

static void
ctxt_for_test_destroy(struct ctxt_for_test *ctxt)
{
    if (ctxt) {
        if (ctxt->exec_inst) {
            bool ok = ctxt->ops.destroy(ctxt->exec_inst);
            PC_ASSERT(ok);
            ctxt->exec_inst = NULL;
        }
        PURC_VARIANT_SAFE_CLEAR(ctxt->by);
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->in);

        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_test_destroy((struct ctxt_for_test*)ctxt);
}

static int
post_process_dest_data(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_test *ctxt;
    ctxt = (struct ctxt_for_test*)frame->ctxt;
    PC_ASSERT(ctxt);

    purc_variant_t on;
    on = ctxt->on;
    if (on == PURC_VARIANT_INVALID)
        return -1;
    PURC_VARIANT_SAFE_CLEAR(ctxt->on);
    ctxt->on = on;
    purc_variant_ref(on);

    PURC_VARIANT_SAFE_CLEAR(frame->result_var);
    frame->result_var = on;
    purc_variant_ref(on);

    return 0;
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_test *ctxt;
    ctxt = (struct ctxt_for_test*)frame->ctxt;
    PC_ASSERT(ctxt);

    int r = post_process_dest_data(co, frame);
    if (r)
        return r;

    purc_variant_t in;
    in = purc_variant_object_get_by_ckey(frame->attr_vars, "in", true);
    if (in != PURC_VARIANT_INVALID) {
        if (!purc_variant_is_string(in)) {
            purc_set_error(PURC_EXCEPT_INVALID_VALUE);
            return -1;
        }

        purc_variant_t elements = pcintr_doc_query(co->stack->vdom,
                purc_variant_get_string_const(in));
        if (elements == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_EXCEPT_INVALID_VALUE);
            return -1;
        }

        PURC_VARIANT_SAFE_CLEAR(ctxt->in);
        ctxt->in = in;
        purc_variant_ref(in);

        PURC_VARIANT_SAFE_CLEAR(frame->symbol_vars[PURC_SYMBOL_VAR_AT_SIGN]);
        frame->symbol_vars[PURC_SYMBOL_VAR_AT_SIGN] = elements;
    }

    return 0;
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_test *ctxt;
    ctxt = (struct ctxt_for_test*)frame->ctxt;
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
process_attr_in(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_test *ctxt;
    ctxt = (struct ctxt_for_test*)frame->ctxt;
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
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val, void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(IN)) == name) {
        return process_attr_in(frame, element, name, val);
    }
    // if (pchvml_keyword(PCHVML_KEYWORD_ENUM(WITH)) == name) {
    //     return process_attr_with(frame, element, name, val);
    // }
    // if (pchvml_keyword(PCHVML_KEYWORD_ENUM(FROM)) == name) {
    //     return process_attr_from(frame, element, name, val);
    // }
    // if (pchvml_keyword(PCHVML_KEYWORD_ENUM(AT)) == name) {
    //     return process_attr_at(frame, element, name, val);
    // }

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

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

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

    struct ctxt_for_test *ctxt;
    ctxt = (struct ctxt_for_test*)calloc(1, sizeof(*ctxt));
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

    if (stack->fragment == NULL) {
        stack->fragment = purc_rwstream_new_buffer(1024, 1024*1024*16);
        if (!stack->fragment)
            return NULL;
    }
    else {
        off_t n = purc_rwstream_seek(stack->fragment, 0, SEEK_SET);
        PC_ASSERT(n == 0);
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

    struct ctxt_for_test *ctxt;
    ctxt = (struct ctxt_for_test*)frame->ctxt;
    if (ctxt) {
        ctxt_for_test_destroy(ctxt);
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
    PC_ASSERT(ud);

    if (frame->result_from_child)
        return NULL;

    struct ctxt_for_test *ctxt;
    ctxt = (struct ctxt_for_test*)frame->ctxt;
    if (!ctxt)
        return NULL;

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

struct pcintr_element_ops* pcintr_get_test_ops(void)
{
    return &ops;
}

