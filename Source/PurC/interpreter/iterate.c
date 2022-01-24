/**
 * @file iterate.c
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

#include "purc-executor.h"

#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define TO_DEBUG 1

struct ctxt_for_iterate {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                by;

    struct purc_exec_ops          ops;
    purc_exec_inst_t              exec_inst;
    purc_exec_iter_t              it;
};

static void
ctxt_for_iterate_destroy(struct ctxt_for_iterate *ctxt)
{
    if (ctxt) {
        if (ctxt->exec_inst) {
            bool ok = ctxt->ops.destroy(ctxt->exec_inst);
            PC_ASSERT(ok);
            ctxt->exec_inst = NULL;
        }
        PURC_VARIANT_SAFE_CLEAR(ctxt->by);
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);

        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_iterate_destroy((struct ctxt_for_iterate*)ctxt);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    PC_ASSERT(ctxt);

    purc_variant_t on;
    on = purc_variant_object_get_by_ckey(frame->attr_vars, "on", false);
    if (on == PURC_VARIANT_INVALID)
        return -1;
    PURC_VARIANT_SAFE_CLEAR(ctxt->on);
    ctxt->on = on;
    purc_variant_ref(on);

    purc_variant_t by;
    by = purc_variant_object_get_by_ckey(frame->attr_vars, "by", false);
    if (by == PURC_VARIANT_INVALID) {
        by = purc_variant_make_string_static("RANGE: FROM 0", false);
        if (by == PURC_VARIANT_INVALID)
            return -1;
    }
    else {
        purc_variant_ref(by);
    }
    purc_clr_error();
    PURC_VARIANT_SAFE_CLEAR(ctxt->by);
    ctxt->by = by;

    const char *rule = purc_variant_get_string_const(by);
    PC_ASSERT(rule);
    bool ok = purc_get_executor(rule, &ctxt->ops);
    if (!ok)
        return -1;

    PC_ASSERT(ctxt->ops.create);
    PC_ASSERT(ctxt->ops.it_begin);
    PC_ASSERT(ctxt->ops.it_next);
    PC_ASSERT(ctxt->ops.it_value);
    PC_ASSERT(ctxt->ops.destroy);

    purc_exec_inst_t exec_inst;
    exec_inst = ctxt->ops.create(PURC_EXEC_TYPE_ITERATE, on, false);
    if (!exec_inst)
        return -1;

    ctxt->exec_inst = exec_inst;

    purc_exec_iter_t it;
    it = ctxt->ops.it_begin(exec_inst, rule);
    if (!it)
        return -1;

    ctxt->it = it;

    purc_variant_t value;
    value = ctxt->ops.it_value(exec_inst, it);
    if (value == PURC_VARIANT_INVALID)
        return -1;

    PURC_VARIANT_SAFE_CLEAR(frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK]);
    frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK] = value;
    purc_variant_ref(value);

    return 0;
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

    if (stack->fragment == NULL) {
        stack->fragment = purc_rwstream_new_buffer(1024, 1024*1024*16);
        if (!stack->fragment)
            return NULL;
    }
    else {
        off_t n = purc_rwstream_seek(stack->fragment, 0, SEEK_SET);
        PC_ASSERT(n == 0);
    }

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)calloc(1, sizeof(*ctxt));
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

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    purc_exec_inst_t exec_inst;
    exec_inst = ctxt->exec_inst;
    if (!exec_inst) {
        pcintr_stack_write_fragment(stack);
        return true;
    }

    purc_exec_iter_t it = ctxt->it;
    if (!it) {
        pcintr_stack_write_fragment(stack);
        return true;
    }

    it = ctxt->ops.it_next(exec_inst, it, NULL); // TODO: re-eval rule????
    ctxt->it = it;
    if (!it) {
        int err = purc_get_last_error();
        if (err == PURC_ERROR_NOT_EXISTS) {
            purc_clr_error();
        }
        pcintr_stack_write_fragment(stack);
        return true;
    }

    return false;
}

static bool
rerun(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    purc_exec_inst_t exec_inst;
    exec_inst = ctxt->exec_inst;
    PC_ASSERT(exec_inst);

    purc_exec_iter_t it = ctxt->it;
    PC_ASSERT(it);

    purc_variant_t value;
    value = ctxt->ops.it_value(exec_inst, it);
    if (value == PURC_VARIANT_INVALID)
        return true;

    PURC_VARIANT_SAFE_CLEAR(frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK]);
    frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK] = value;
    purc_variant_ref(value);

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
    PC_ASSERT(stack == purc_get_stack());

    pcintr_coroutine_t co = &stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

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
                PC_ASSERT(stack->except == 0);
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
    .rerun              = rerun,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_iterate_ops(void)
{
    return &ops;
}

