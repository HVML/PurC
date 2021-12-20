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

#include "internal.h"

#include "private/debug.h"
#include "private/runloop.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

static int
timeout_cb(void *ctxt)
{
    pcintr_coroutine_t co = (pcintr_coroutine_t)ctxt;
    co->state = CO_STATE_READY;
    pcintr_coroutine_ready();
    return 0;
}

struct thread_arg {
    pcintr_coroutine_t co;
    int                secs;
};

static void* timer_thread(void *ctxt)
{
    struct thread_arg *arg = (struct thread_arg*)ctxt;
    sleep(arg->secs);

    pcrunloop_t runloop = pcrunloop_get_main();
    PC_ASSERT(runloop);
    pcrunloop_dispatch(runloop, timeout_cb, arg->co);

    free(arg);

    return NULL;
}

static void simulate_timeout(pcintr_coroutine_t co, int secs)
{
    struct thread_arg *arg = (struct thread_arg*)malloc(sizeof(*arg));
    PC_ASSERT(arg);
    arg->co = co;
    arg->secs = secs;

    pthread_t thread;
    int r = pthread_create(&thread, NULL, timer_thread, arg);
    PC_ASSERT(r == 0);
    pthread_detach(thread);
}

struct ctxt_for_undefined {
    struct pcvdom_node           *curr;
};

static void
ctxt_for_undefined_destroy(struct ctxt_for_undefined *ctxt)
{
    if (ctxt)
        free(ctxt);
}

static void
on_timedout(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    fprintf(stderr, "==co[%p]@%s[%d]:%s()==\n", co, basename((char*)__FILE__), __LINE__, __func__);

    struct pcvdom_element *element = frame->scope;
    PC_ASSERT(element);

    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
after_pushed(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct pcvdom_element *element = frame->scope;
    PC_ASSERT(element);

    fprintf(stderr, "==co[%p]<%s>@%s[%d]:%s()==\n", co, element->tag_name, basename((char*)__FILE__), __LINE__, __func__);

    int r = pcintr_element_eval_attrs(frame, element);
    if (r) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        frame->next_step = -1;
        co->state = CO_STATE_TERMINATED;
        return;
    }

    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        frame->next_step = -1;
        co->state = CO_STATE_TERMINATED;
        return;
    }

    if (strstr(element->tag_name, "timeout") == element->tag_name) {
        frame->ctxt = ctxt;
        frame->preemptor = on_timedout;
        co->state = CO_STATE_WAIT;
        // simulate
        int secs = atol(element->tag_name + 7);
        simulate_timeout(co, secs);
        return;
    }

    frame->ctxt = ctxt;
    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
on_popping(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct pcvdom_element *element = frame->scope;
    fprintf(stderr, "==co[%p]<%s>@%s[%d]:%s()==\n", co, element->tag_name, basename((char*)__FILE__), __LINE__, __func__);
    pcintr_stack_t stack = co->stack;
    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)frame->ctxt;
    if (ctxt) {
        ctxt_for_undefined_destroy(ctxt);
        frame->ctxt = NULL;
    }
    pop_stack_frame(stack);
    co->state = CO_STATE_READY;
}

static void
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)frame->ctxt;

    pcintr_stack_t stack = co->stack;
    struct pcintr_stack_frame *child_frame;
    child_frame = push_stack_frame(stack);
    if (!child_frame) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return;
    }
    child_frame->ops = pcintr_get_ops_by_element(element);
    child_frame->scope = element;

    ctxt->curr = &element->node;
    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)frame->ctxt;

    ctxt->curr = &content->node;
    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)frame->ctxt;

    ctxt->curr = &comment->node;
    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
select_child(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_undefined *ctxt;
    ctxt = (struct ctxt_for_undefined*)frame->ctxt;

    if (ctxt->curr == NULL) {
        struct pcvdom_element *element = frame->scope;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        ctxt->curr = node;
    }
    else {
        ctxt->curr = pcvdom_node_next_sibling(ctxt->curr);
    }

    if (ctxt->curr == NULL) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        co->state = CO_STATE_READY;
        return;
    }

    switch (ctxt->curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            on_element(co, frame, PCVDOM_ELEMENT_FROM_NODE(ctxt->curr));
            return;
        case PCVDOM_NODE_CONTENT:
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(ctxt->curr));
            return;
        case PCVDOM_NODE_COMMENT:
            on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(ctxt->curr));
            return;
        default:
            PC_ASSERT(0); // Not implemented yet
    }

    PC_ASSERT(0);
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



