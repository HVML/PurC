/**
 * @file interpreter.c
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for interpreter
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

// #undef NDEBUG

#include "config.h"

#include "purc-runloop.h"

#include "internal.h"

#include "private/debug.h"
#include "private/instance.h"
#include "private/dvobjs.h"
#include "private/fetcher.h"
#include "private/regex.h"
#include "private/stringbuilder.h"
#include "private/msg-queue.h"
#include "private/runners.h"
#include "private/channel.h"

#include "ops.h"
#include "../hvml/hvml-gen.h"
#include "../html/parser.h"

#include "hvml-attr.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <libgen.h>

#define EVENT_TIMER_INTRVAL  10

#define EVENT_SEPARATOR      ':'


#define COROUTINE_PREFIX    "COROUTINE"
#define HVML_VARIABLE_REGEX "^[A-Za-z_][A-Za-z0-9_]*$"

static void
stack_frame_release(struct pcintr_stack_frame *frame)
{
    if (!frame)
        return;

    frame->scope = NULL;
    frame->edom_element = NULL;
    frame->pos   = NULL;

    if (frame->ctxt) {
        PC_ASSERT(frame->ctxt_destroy);
        frame->ctxt_destroy(frame->ctxt);
        frame->ctxt  = NULL;
    }

    for (size_t i=0; i<PCA_TABLESIZE(frame->symbol_vars); ++i) {
        PURC_VARIANT_SAFE_CLEAR(frame->symbol_vars[i]);
    }

    PURC_VARIANT_SAFE_CLEAR(frame->attr_vars);
    PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
    PURC_VARIANT_SAFE_CLEAR(frame->result_from_child);
    PURC_VARIANT_SAFE_CLEAR(frame->except_templates);
    PURC_VARIANT_SAFE_CLEAR(frame->error_templates);
}

static void
stack_frame_pseudo_release(struct pcintr_stack_frame_pseudo *frame_pseudo)
{
    if (!frame_pseudo)
        return;

    stack_frame_release(&frame_pseudo->frame);
}

static void
stack_frame_pseudo_destroy(struct pcintr_stack_frame_pseudo *frame_pseudo)
{
    if (!frame_pseudo)
        return;

    stack_frame_pseudo_release(frame_pseudo);
    free(frame_pseudo);
}

static void
stack_frame_normal_release(struct pcintr_stack_frame_normal *frame_normal)
{
    if (!frame_normal)
        return;

    stack_frame_release(&frame_normal->frame);
}

static void
stack_frame_normal_destroy(struct pcintr_stack_frame_normal *frame_normal)
{
    if (!frame_normal)
        return;

    stack_frame_normal_release(frame_normal);
    free(frame_normal);
}

static int
doc_init(pcintr_stack_t stack)
{
    struct pcvdom_element* hvml_elem =
        pcvdom_document_get_root(stack->co->vdom);
    if (UNLIKELY(hvml_elem == NULL)) {
        purc_set_error(PURC_ERROR_INCOMPLETED);
        return -1;
    }

    // XXX: may use the coroutine-level variables.
    purc_variant_t target = pcvdom_element_eval_attr_val(stack,
            hvml_elem, "target");
    if (UNLIKELY(target == PURC_VARIANT_INVALID)) {
        purc_set_error(PURC_ERROR_INCOMPLETED);
        return -1;
    }

    const char *target_name = purc_variant_get_string_const(target);
    PC_DEBUG("Retrieved target name: %s\n", target_name);
    stack->doc = purc_document_new(purc_document_retrieve_type(target_name));
    purc_variant_unref(target);

    if (stack->doc == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        PC_ASSERT(0);
        return -1;
    }

    return 0;
}

static void
release_loaded_var(struct pcintr_loaded_var *p)
{
    if (p) {
        if (p->val != PURC_VARIANT_INVALID) {
            purc_variant_unload_dvobj(p->val);
            p->val = PURC_VARIANT_INVALID;
        }
        if (p->name) {
            free(p->name);
            p->name = NULL;
        }
    }
}

static void
destroy_loaded_var(struct pcintr_loaded_var *p)
{
    if (p) {
        release_loaded_var(p);
        free(p);
    }
}

static int
unload_dynamic_var(struct rb_node *node, void *ud)
{
    struct rb_root *root = (struct rb_root*)ud;
    struct pcintr_loaded_var *p;
    p = container_of(node, struct pcintr_loaded_var, node);
    pcutils_rbtree_erase(node, root);
    destroy_loaded_var(p);

    return 0;
}

static void
loaded_vars_release(pcintr_coroutine_t cor)
{
    struct rb_root *root = &cor->loaded_vars;
    if (RB_EMPTY_ROOT(root))
        return;

    int r;
    r = pcutils_rbtree_traverse(root, root, unload_dynamic_var);
    PC_ASSERT(r == 0);
}

void
pcintr_exception_clear(struct pcintr_exception *exception)
{
    if (!exception)
        return;

    PURC_VARIANT_SAFE_CLEAR(exception->exinfo);
    if (exception->bt) {
        pcdebug_backtrace_unref(exception->bt);
        exception->bt = NULL;
    }
    exception->error_except = 0;
    exception->err_element  = NULL;
}

void
pcintr_exception_move(struct pcintr_exception *dst,
        struct pcintr_exception *src)
{
    if (dst == src)
        return;

    if (dst->exinfo != src->exinfo) {
        PURC_VARIANT_SAFE_CLEAR(dst->exinfo);
        dst->exinfo = src->exinfo;
        src->exinfo = PURC_VARIANT_INVALID;
    }

    if (dst->bt != src->bt) {
        if (dst->bt)
            pcdebug_backtrace_unref(dst->bt);
        dst->bt = src->bt;
        src->bt = NULL;
    }

    dst->error_except = src->error_except;
    src->error_except = 0;

    dst->err_element = src->err_element;
    src->err_element = NULL;
}

static void
release_scoped_variables(pcintr_stack_t stack)
{
    if (!stack)
        return;

    struct rb_node *p, *n;
    struct rb_node *last = pcutils_rbtree_last(&stack->scoped_variables);
    pcutils_rbtree_for_each_reverse_safe(last, p, n) {
        pcvarmgr_t mgr = container_of(p, struct pcvarmgr, node);
        pcutils_rbtree_erase(p, &stack->scoped_variables);
        PC_ASSERT(p->rb_left == NULL);
        PC_ASSERT(p->rb_right == NULL);
        PC_ASSERT(p->rb_parent == NULL);
        pcvarmgr_destroy(mgr);
    }
}

static void
destroy_stack_frame(struct pcintr_stack_frame *frame)
{
    struct pcintr_stack_frame_normal *frame_normal = NULL;

    switch (frame->type) {
        case STACK_FRAME_TYPE_NORMAL:
            frame_normal = container_of(frame,
                    struct pcintr_stack_frame_normal, frame);
            stack_frame_normal_destroy(frame_normal);
            break;
        case STACK_FRAME_TYPE_PSEUDO:
            PC_ASSERT(0);
            break;
        default:
            PC_ASSERT(0);
    }
}

static void
stack_release(pcintr_stack_t stack)
{
    if (!stack)
        return;

    if (stack->async_request_ids != PURC_VARIANT_INVALID) {
        size_t sz = purc_variant_array_get_size(stack->async_request_ids);
        if (sz) {
            purc_variant_t ids = purc_variant_container_clone(
                    stack->async_request_ids);
            for (size_t i = 0; i < sz; i++) {
                pcfetcher_cancel_async(purc_variant_array_get(ids, i));
            }
            purc_variant_unref(ids);
        }
        PURC_VARIANT_SAFE_CLEAR(stack->async_request_ids);
    }

    pcintr_heap_t heap = stack->co->owner;
    if (heap->cond_handler) {
        heap->cond_handler(PURC_COND_COR_DESTROYED, stack->co,
                stack->co->user_data);
    }

    struct list_head *frames = &stack->frames;
    struct pcintr_stack_frame *p, *n;
    list_for_each_entry_reverse_safe(p, n, frames, node) {
        PC_ASSERT(p->type == STACK_FRAME_TYPE_NORMAL);
        list_del(&p->node);
        --stack->nr_frames;
        destroy_stack_frame(p);
    }
    PC_ASSERT(stack->nr_frames == 0);

    release_scoped_variables(stack);

    pcintr_destroy_observer_list(&stack->intr_observers);
    pcintr_destroy_observer_list(&stack->hvml_observers);

    if (stack->doc) {
        purc_document_unref(stack->doc);
        stack->doc = NULL;
    }

    if (stack->tag_prefix) {
        free(stack->tag_prefix);
        stack->tag_prefix = NULL;
    }

    pcintr_exception_clear(&stack->exception);

    if (stack->body_id) {
        free(stack->body_id);
        stack->body_id = NULL;
    }

    if (stack->vcm_ctxt) {
        pcvcm_eval_ctxt_destroy(stack->vcm_ctxt);
        stack->vcm_ctxt = NULL;
    }
}

enum pcintr_req_state {
    REQ_STATE_INIT,
    REQ_STATE_PENDING,
    REQ_STATE_CANCELLED,
    REQ_STATE_ACTIVATED,
    REQ_STATE_DYING,
};

struct pcintr_req {
    pcintr_coroutine_t           owner;
    void                        *ctxt;
    struct pcintr_req_ops       *ops;

    int                          refc; // TODO: atomic or thread-safe-lock?
    enum pcintr_req_state        state;
    struct list_head            *list;
    struct list_head             node;
};

static void
coroutine_release(pcintr_coroutine_t co)
{
    if (co) {
        struct pcintr_heap *heap = pcintr_get_heap();
        PC_ASSERT(heap && co->owner == heap);

        stack_release(&co->stack);
        pcvdom_document_unref(co->vdom);

        PURC_VARIANT_SAFE_CLEAR(co->doc_contents);
        PURC_VARIANT_SAFE_CLEAR(co->doc_wrotten_len);

        struct list_head *children = &co->children;
        struct list_head *p, *n;
        list_for_each_safe(p, n, children) {
            pcintr_coroutine_child_t child;
            child = list_entry(p, struct pcintr_coroutine_child, ln);
            free(child);
        }

        if (co->cid) {
            const char *uri = pcintr_coroutine_get_uri(co);
            purc_atom_remove_string_ex(PURC_ATOM_BUCKET_DEF, uri);
        }
        if (co->mq) {
            pcinst_msg_queue_destroy(co->mq);
        }

        pcintr_coroutine_clear_tasks(co);

        if (co->variables) {
            pcvarmgr_destroy(co->variables);
        }

        struct purc_broken_down_url *url = &co->base_url_broken_down;

        if (url->schema) {
            free(url->schema);
        }

        if (url->user) {
            free(url->user);
        }

        if (url->passwd) {
            free(url->passwd);
        }

        if (url->host) {
            free(url->host);
        }

        if (url->path) {
            free(url->path);
        }

        if (url->query) {
            free(url->query);
        }

        if (url->fragment) {
            free(url->fragment);
        }

        if (co->target) {
            free(co->target);
        }

        if (co->base_url_string) {
            free(co->base_url_string);
        }

        if (co->timers) {
            pcintr_timers_destroy(co->timers);
            co->timers = NULL;
        }

        loaded_vars_release(co);
    }
}

static void
coroutine_destroy(pcintr_coroutine_t co)
{
    if (co) {
        coroutine_release(co);
        free(co);
    }
}

static void
stack_init(pcintr_stack_t stack)
{
    list_head_init(&stack->frames);
    list_head_init(&stack->intr_observers);
    list_head_init(&stack->hvml_observers);
    stack->scoped_variables = RB_ROOT;

    stack->mode = STACK_VDOM_BEFORE_HVML;
}

static void _cleanup_instance(struct pcinst* inst)
{
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap)
        return;

    struct rb_root *coroutines = &heap->coroutines;

    struct rb_node *p, *n;
    struct rb_node *first = pcutils_rbtree_first(coroutines);
    pcutils_rbtree_for_each_safe(first, p, n) {
        pcintr_coroutine_t co;
        co = container_of(p, struct pcintr_coroutine, node);

        pcutils_rbtree_erase(p, coroutines);
        coroutine_destroy(co);
    }

    if (heap->move_buff) {
        size_t n = purc_inst_destroy_move_buffer();
        PC_DEBUG("Instance is quiting, %u messages discarded\n", (unsigned)n);
        heap->move_buff = 0;
    }

    if (heap->event_timer) {
        pcintr_timer_destroy(heap->event_timer);
        heap->event_timer = NULL;
    }

    if (heap->name_chan_map) {
        pcutils_map_destroy(heap->name_chan_map);
        heap->name_chan_map = NULL;
    }

    free(heap);
    inst->intr_heap = NULL;
}


static void
event_timer_fire(pcintr_timer_t timer, const char* id, void* data);

static int _init_instance(struct pcinst* inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(extra_info);
    inst->intr_heap = NULL;

    struct pcintr_heap *heap = inst->intr_heap;
    PC_ASSERT(heap == NULL);

    heap = (struct pcintr_heap*)calloc(1, sizeof(*heap));
    if (!heap)
        return PURC_ERROR_OUT_OF_MEMORY;

    heap->move_buff = purc_inst_create_move_buffer(
            PCINST_MOVE_BUFFER_BROADCAST, PCINTR_MOVE_BUFFER_SIZE);
    if (!heap->move_buff) {
        free(heap);
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    if (!pcintr_bind_builtin_runner_variables()) {
        free(heap);
        return purc_get_last_error();
    }

    inst->running_loop = purc_runloop_get_current();
    inst->intr_heap = heap;
    heap->owner     = inst;

    heap->coroutines = RB_ROOT;
    heap->running_coroutine = NULL;

    heap->name_chan_map =
        pcutils_map_create(NULL, NULL, NULL,
                (free_val_fn)pcchan_destroy, comp_key_string, false);

    heap->event_timer = pcintr_timer_create(NULL, NULL, event_timer_fire, inst);
    if (!heap->event_timer) {
        purc_inst_destroy_move_buffer();
        heap->move_buff = 0;
        free(heap);
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    pcintr_timer_set_interval(heap->event_timer, EVENT_TIMER_INTRVAL);
    pcintr_timer_start(heap->event_timer);

    return 0;
}

static int _init_once(void)
{
    purc_runloop_t runloop = purc_runloop_get_current();
    PC_ASSERT(runloop);
    init_ops();

    return pcintr_init_loader_once();
}

struct pcmodule _module_interpreter = {
    .id              = PURC_HAVE_HVML,
    .module_inited   = 0,

    .init_once              = _init_once,
    .init_instance          = _init_instance,
    .cleanup_instance       = _cleanup_instance,
};

struct pcintr_heap* pcintr_get_heap(void)
{
    struct pcinst *inst = pcinst_current();

    return inst ? inst->intr_heap : NULL;
}

pcintr_coroutine_t
pcintr_get_coroutine(void)
{
    struct pcintr_heap *heap = pcintr_get_heap();
    return heap ? heap->running_coroutine : NULL;
}

purc_runloop_t
pcintr_get_runloop(void)
{
    pcintr_coroutine_t co = pcintr_get_coroutine();
    pcintr_heap_t heap = co ? co->owner : NULL;
    struct pcinst *inst = heap ? heap->owner : NULL;
    return inst ? inst->running_loop : NULL;
}

static void
coroutine_set_current_with_location(struct pcintr_coroutine *co,
        const char *file, int line, const char *func)
{
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);
    UNUSED_PARAM(func);

    struct pcintr_heap *heap = pcintr_get_heap();
    if (co) {
#if 0           /* { */
        fprintf(stderr, "%s[%d]: %s(): %s\n",
            basename((char*)file), line, func,
            ">>>>>>>>>>>>>start>>>>>>>>>>>>>>>>>>>>>>>>>>");
#endif          /* } */
        //PC_ASSERT(heap->running_coroutine == NULL);
    }
    else {
#if 0           /* { */
        fprintf(stderr, "%s[%d]: %s(): %s\n",
            basename((char*)file), line, func,
            "<<<<<<<<<<<<<stop<<<<<<<<<<<<<<<<<<<<<<<<<<<");
#endif          /* } */
        //PC_ASSERT(heap->running_coroutine);
    }

    heap->running_coroutine = co;
}

#define coroutine_set_current(co) \
    coroutine_set_current_with_location(co, __FILE__, __LINE__, __func__)

void pcintr_set_current_co_with_location(pcintr_coroutine_t co,
        const char *file, int line, const char *func)
{
    coroutine_set_current_with_location(co, file, line, func);
}

pcintr_stack_t pcintr_get_stack(void)
{
    struct pcintr_coroutine *co = pcintr_get_coroutine();
    if (!co)
        return NULL;

    return &co->stack;
}

static void
pop_stack_frame(pcintr_stack_t stack)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack->nr_frames > 0);

    struct list_head *tail = stack->frames.prev;
    PC_ASSERT(tail != NULL);
    PC_ASSERT(tail != &stack->frames);

    list_del(tail);

    struct pcintr_stack_frame *frame;
    frame = container_of(tail, struct pcintr_stack_frame, node);

    struct pcintr_stack_frame_normal *frame_normal = NULL;
    struct pcintr_stack_frame_pseudo *frame_pseudo = NULL;

    switch (frame->type) {
        case STACK_FRAME_TYPE_NORMAL:
            frame_normal = container_of(frame,
                    struct pcintr_stack_frame_normal, frame);
            stack_frame_normal_destroy(frame_normal);
            break;
        case STACK_FRAME_TYPE_PSEUDO:
            frame_pseudo = container_of(frame,
                    struct pcintr_stack_frame_pseudo, frame);
            stack_frame_pseudo_destroy(frame_pseudo);
            break;
        default:
            PC_ASSERT(0);
    }

    --stack->nr_frames;
}

static int
set_lessthan_symval(struct pcintr_stack_frame *frame, purc_variant_t val)
{
    if (val != PURC_VARIANT_INVALID) {
        PURC_VARIANT_SAFE_CLEAR(
                frame->symbol_vars[PURC_SYMBOL_VAR_LESS_THAN]);
        frame->symbol_vars[PURC_SYMBOL_VAR_LESS_THAN] = val;
        purc_variant_ref(val);
    }
    else {
        purc_variant_t undefined = purc_variant_make_undefined();
        if (undefined == PURC_VARIANT_INVALID)
            return -1;
        PURC_VARIANT_SAFE_CLEAR(
                frame->symbol_vars[PURC_SYMBOL_VAR_LESS_THAN]);
        frame->symbol_vars[PURC_SYMBOL_VAR_LESS_THAN] = undefined;
    }

    return 0;
}

static int
init_percent_symval(struct pcintr_stack_frame *frame)
{
    purc_variant_t idx;
    idx = purc_variant_make_ulongint(0);
    if (idx == PURC_VARIANT_INVALID)
        return -1;

    enum purc_symbol_var symbol = PURC_SYMBOL_VAR_PERCENT_SIGN;
    PURC_VARIANT_SAFE_CLEAR(frame->symbol_vars[symbol]);
    frame->symbol_vars[symbol] = idx;

    return 0;
}

static int
init_at_symval(struct pcintr_stack_frame *frame)
{
    struct pcintr_stack_frame *parent;
    parent = pcintr_stack_frame_get_parent(frame);
    if (!parent || !parent->edom_element)
        return 0;

    purc_variant_t at = pcintr_get_at_var(parent);
    if (at == PURC_VARIANT_INVALID)
        return -1;

    int r;
    r = pcintr_set_at_var(frame, at);

    return r ? -1 : 0;
}

static int
init_exclamation_symval(struct pcintr_stack_frame *frame)
{
    purc_variant_t exclamation_var;
    exclamation_var = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    if (exclamation_var == PURC_VARIANT_INVALID)
        return -1;

    int r;
    r = pcintr_set_exclamation_var(frame, exclamation_var);
    purc_variant_unref(exclamation_var);

    return r ? -1 : 0;
}

static int
init_undefined_symvals(struct pcintr_stack_frame *frame)
{
    purc_variant_t undefined = purc_variant_make_undefined();
    if (undefined == PURC_VARIANT_INVALID)
        return -1;

    for (size_t i=0; i<PCA_TABLESIZE(frame->symbol_vars); ++i) {
        frame->symbol_vars[i] = undefined;
        purc_variant_ref(undefined);
    }
    purc_variant_unref(undefined);

    return 0;
}

int
pcintr_calc_and_set_caret_symbol(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame)
{
    int ret = 0;
    int last_err = purc_get_last_error();
    pcvdom_element_t elem = frame->pos;
    if (!elem) {
        goto out;
    }

    bool operation = pcvdom_element_is_hvml_operation(elem);
    if (!operation) {
        goto out;
    }

    struct pcvdom_node *node = &elem->node;
    node = pcvdom_node_first_child(node);
    if (!node || node->type != PCVDOM_NODE_CONTENT) {
        purc_clr_error();
        goto out;
    }

    struct pcvdom_content *content = PCVDOM_CONTENT_FROM_NODE(node);
    struct pcvcm_node *vcm = content->vcm;
    if (!vcm) {
        goto out;
    }

    purc_variant_t v = pcvcm_eval(vcm, stack, frame->silently);
    if (v == PURC_VARIANT_INVALID) {
        purc_clr_error();
        goto out;
    }

    ret = pcintr_set_symbol_var(frame, PURC_SYMBOL_VAR_CARET, v);
    purc_variant_unref(v);

out:
    if (last_err) {
        purc_set_error(last_err);
    }
    return ret;
}

static int
init_symvals_with_vals(struct pcintr_stack_frame *frame)
{
    if (frame->type == STACK_FRAME_TYPE_PSEUDO)
        return 0;

    // $0%
    if (init_percent_symval(frame))
        return -1;

    // $0@
    if (init_at_symval(frame))
        return -1;

    // $0!
    if (init_exclamation_symval(frame))
        return -1;

    return 0;
}

static int
init_stack_frame(pcintr_stack_t stack, struct pcintr_stack_frame* frame)
{
    frame->owner           = stack;
    frame->silently        = 0;

    frame->except_templates = purc_variant_make_object_0();
    frame->error_templates  = purc_variant_make_object_0();

    if (frame->except_templates == PURC_VARIANT_INVALID ||
            frame->error_templates == PURC_VARIANT_INVALID)
    {
        return -1;
    }

    return 0;
}

static int
init_stack_frame_pseudo(pcintr_stack_t stack,
        struct pcintr_stack_frame_pseudo *frame_pseudo)
{
    do {
        if (init_stack_frame(stack, &frame_pseudo->frame))
            break;

        if (init_undefined_symvals(&frame_pseudo->frame))
            break;

        return 0;
    } while (0);

    return -1;
}

static struct pcintr_stack_frame_pseudo*
stack_frame_pseudo_create(pcintr_stack_t stack)
{
    struct pcintr_stack_frame_pseudo *frame_pseudo;
    frame_pseudo = (struct pcintr_stack_frame_pseudo*)calloc(1,
            sizeof(*frame_pseudo));
    if (!frame_pseudo) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    struct pcintr_stack_frame *frame = &frame_pseudo->frame;
    frame->type = STACK_FRAME_TYPE_PSEUDO;

    if (init_stack_frame_pseudo(stack, frame_pseudo))
        goto fail_init;

    return frame_pseudo;

fail_init:
    stack_frame_pseudo_destroy(frame_pseudo);

    return NULL;
}

static struct pcintr_stack_frame_pseudo*
push_stack_frame_pseudo(pcintr_stack_t stack,
        pcvdom_element_t vdom_element)
{
    PC_ASSERT(vdom_element);

    struct pcintr_stack_frame_pseudo *frame_pseudo;
    frame_pseudo = stack_frame_pseudo_create(stack);
    if (!frame_pseudo)
        return NULL;

    struct pcintr_stack_frame *frame = &frame_pseudo->frame;

    do {
        struct pcintr_element_ops ops = {};

        struct pcintr_stack_frame *child_frame;
        child_frame = &frame_pseudo->frame;
        child_frame->ops = ops;
        child_frame->pos = vdom_element;
        child_frame->edom_element = NULL;
        child_frame->scope = NULL;
        child_frame->silently = pcintr_is_element_silently(child_frame->pos) ?
            1 : 0;

        child_frame->next_step = NEXT_STEP_AFTER_PUSHED;

        list_add_tail(&frame->node, &stack->frames);
        ++stack->nr_frames;

        return frame_pseudo;
    } while (0);

    // FIXME:  ??  reached here!!!
    PC_ASSERT(0);
    pop_stack_frame(stack);
    return NULL;
}

void
pcintr_push_stack_frame_pseudo(pcvdom_element_t vdom_element)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);

    struct pcintr_stack_frame_pseudo *frame_pseudo;
    frame_pseudo = push_stack_frame_pseudo(stack, vdom_element);

    PC_ASSERT(frame_pseudo);
    PC_ASSERT(frame_pseudo->frame.type == STACK_FRAME_TYPE_PSEUDO);
}

void
pcintr_pop_stack_frame_pseudo(void)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    pop_stack_frame(stack);
}

static int
init_stack_frame_normal(pcintr_stack_t stack,
        struct pcintr_stack_frame_normal *frame_normal)
{
    do {
        if (init_stack_frame(stack, &frame_normal->frame))
            break;

        if (init_undefined_symvals(&frame_normal->frame))
            break;

        return 0;
    } while (0);

    return -1;
}

static struct pcintr_stack_frame_normal*
stack_frame_normal_create(pcintr_stack_t stack)
{
    struct pcintr_stack_frame_normal *frame_normal;
    frame_normal = (struct pcintr_stack_frame_normal*)calloc(1,
            sizeof(*frame_normal));
    if (!frame_normal) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    struct pcintr_stack_frame *frame = &frame_normal->frame;
    frame->type = STACK_FRAME_TYPE_NORMAL;

    if (init_stack_frame_normal(stack, frame_normal))
        goto fail_init;

    return frame_normal;

fail_init:
    stack_frame_normal_destroy(frame_normal);

    return NULL;
}

struct pcintr_stack_frame_normal *
pcintr_push_stack_frame_normal(pcintr_stack_t stack)
{
    struct pcintr_stack_frame_normal *frame_normal;
    frame_normal = stack_frame_normal_create(stack);
    if (!frame_normal)
        return NULL;

    struct pcintr_stack_frame *frame = &frame_normal->frame;
    frame->type = STACK_FRAME_TYPE_NORMAL;

    list_add_tail(&frame->node, &stack->frames);
    ++stack->nr_frames;

    do {
        if (init_symvals_with_vals(&frame_normal->frame))
            break;

        return frame_normal;
    } while (0);

    pop_stack_frame(stack);
    return NULL;
}

void
pcintr_set_input_var(pcintr_stack_t stack, purc_variant_t val)
{
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    set_lessthan_symval(frame, val);
}

purc_variant_t
eval_vdom_attr(pcintr_stack_t stack, struct pcvdom_attr *attr)
{
    PC_ASSERT(attr);
    PC_ASSERT(attr->key);
    if (!attr->val)
        return purc_variant_make_undefined();

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    return pcvcm_eval(attr->val, stack, frame->silently ? true : false);
}

int
pcintr_set_edom_attribute(pcintr_stack_t stack, struct pcvdom_attr *attr)
{
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(frame->edom_element);

    PC_ASSERT(attr);
    PC_ASSERT(attr->key);
    size_t len = 0;
    const char *sv = "";

    purc_variant_t val = eval_vdom_attr(stack, attr);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    if (!purc_variant_is_undefined(val)) {
        PC_ASSERT(purc_variant_is_string(val));
        sv = purc_variant_get_string_const_ex(val, &len);
        PC_ASSERT(sv);
    }

    int r = pcdoc_element_set_attribute(stack->doc, frame->edom_element,
            PCDOC_OP_DISPLACE, attr->key, sv, len);
    PC_ASSERT(r == 0);
    PURC_VARIANT_SAFE_CLEAR(val);

    return r ? -1 : 0;
}

purc_variant_t
pcintr_eval_vdom_attr(pcintr_stack_t stack, struct pcvdom_attr *attr)
{
    return eval_vdom_attr(stack, attr);
}

struct pcintr_walk_attrs_ud {
    struct pcintr_stack_frame       *frame;
    struct pcvdom_element           *element;
    void                            *ud;
    pcintr_attr_f                    cb;
};

static int
walk_attr(void *key, void *val, void *ud)
{
    PC_ASSERT(key);
    PC_ASSERT(val);
    PC_ASSERT(ud);

    struct pcintr_walk_attrs_ud *data = (struct pcintr_walk_attrs_ud*)ud;

    struct pcintr_stack_frame *frame = data->frame;
    PC_ASSERT(frame);

    struct pcvdom_attr *attr = (struct pcvdom_attr*)val;
    PC_ASSERT(attr->key);
    PC_ASSERT(attr->key == key);

    struct pcvdom_element *element = data->element;
    PC_ASSERT(element);

    purc_atom_t atom = PCHVML_KEYWORD_ATOM(HVML, attr->key);
    // NOTE: we only dispatch those keyworded-attr to caller
    return data->cb(frame, element, atom, attr, data->ud);
}

int
pcintr_vdom_walk_attrs(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element, void *ud, pcintr_attr_f cb)
{
    pcutils_array_t *attrs = element->attrs;
    if (!attrs)
        return 0;

    PC_ASSERT(frame->pos == element);

    if (frame->attr_vars == PURC_VARIANT_INVALID) {
        frame->attr_vars = purc_variant_make_object(0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        if (frame->attr_vars == PURC_VARIANT_INVALID)
            return -1;
    }

    struct pcintr_walk_attrs_ud data = {
        .frame        = frame,
        .element      = element,
        .ud           = ud,
        .cb           = cb,
    };

    size_t nr = pcutils_array_length(element->attrs);
    for (size_t i = 0; i < nr; i++) {
        struct pcvdom_attr *attr = pcutils_array_get(element->attrs, i);
        int r = walk_attr(attr->key, attr, &data);
        if (r) {
            return r;
        }
    }

    return 0;
}

bool
pcintr_is_element_silently(struct pcvdom_element *element)
{
    return element ? pcvdom_element_is_silently(element) : false;
}

#ifndef NDEBUG                     /* { */
static void
dump_stack_frame(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        size_t level)
{
    UNUSED_PARAM(stack);

    if (level == 0) {
        fprintf(stderr, "document\n");
        return;
    }
    pcvdom_element_t scope = frame->scope;
    pcvdom_element_t pos = frame->pos;
    for (size_t i=0; i<level; ++i) {
        fprintf(stderr, "  ");
    }
    fprintf(stderr, "scope:<%s>; pos:<%s>\n",
        (scope ? scope->tag_name : NULL),
        (pos ? pos->tag_name : NULL));
}

void
pcintr_dump_stack(pcintr_stack_t stack)
{
    fprintf(stderr, "dumping stacks of corroutine [%p] ......\n", &stack->co);
    PC_ASSERT(stack);
    struct pcintr_exception *exception = &stack->exception;
    struct pcdebug_backtrace *bt = exception->bt;

    if (bt) {
        fprintf(stderr, "error_except: generated @%s[%d]:%s()\n",
                pcutils_basename((char*)bt->file), bt->line, bt->func);
    }
    purc_atom_t     error_except = exception->error_except;
    purc_variant_t  err_except_info = exception->exinfo;
    if (error_except) {
        fprintf(stderr, "error_except: %s\n",
                purc_atom_to_string(error_except));
    }
    if (err_except_info) {
        pcinst_dump_err_except_info(err_except_info);
    }
    fprintf(stderr, "nr_frames: %zd\n", stack->nr_frames);
    struct list_head *frames = &stack->frames;
    size_t level = 0;
    if (!list_empty(frames)) {
        struct list_head *p;
        list_for_each(p, frames) {
            struct pcintr_stack_frame *frame;
            frame = container_of(p, struct pcintr_stack_frame, node);
            dump_stack_frame(stack, frame, level++);
        }
    }
}

void
pcintr_dump_c_stack(struct pcdebug_backtrace *bt)
{
    if (!bt)
        return;

    struct pcinst *inst = pcinst_current();
    fprintf(stderr, "dumping stacks of purc instance [%p]......\n", inst);
    pcdebug_backtrace_dump(bt);
}
#endif                             /* } */

void
pcintr_check_insertion_mode_for_normal_element(pcintr_stack_t stack)
{
    PC_ASSERT(stack);

    if (stack->co->stage != CO_STAGE_FIRST_RUN)
        return;

    switch (stack->mode) {
        case STACK_VDOM_BEFORE_HVML:
            PC_ASSERT(0);
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
            PC_ASSERT(0);
            break;
        case STACK_VDOM_AFTER_HVML:
            PC_ASSERT(0);
            break;
        default:
            PC_ASSERT(0);
            break;
    }
}

static void
after_pushed(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    //pcintr_coroutine_dump(co);
    if (frame->ops.after_pushed) {
        void *ctxt = frame->ops.after_pushed(&co->stack, frame->pos);
        if (!ctxt) {
            if (purc_get_last_error() == 0) {
                frame->next_step = NEXT_STEP_ON_POPPING;
                return;
            }
        }
    }

    frame->next_step = NEXT_STEP_SELECT_CHILD;
}

static void
on_popping(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    bool ok = true;
    pcintr_stack_t stack = &co->stack;
    do {
        if (!stack->except)
            break;

        purc_variant_t except_templates = frame->except_templates;
        if (except_templates == PURC_VARIANT_INVALID)
            break;

        purc_atom_t error_except = stack->exception.error_except;

        purc_variant_t v = PURC_VARIANT_INVALID;
        pcintr_match_template(except_templates, error_except, &v);

        if (v == PURC_VARIANT_INVALID)
            break;

        purc_variant_t content;
        content = pcintr_template_expansion(v);
        PURC_VARIANT_SAFE_CLEAR(v);

        pcintr_exception_clear(&stack->exception);
        stack->except = 0;

        pcdoc_element_t target;
        target = frame->edom_element;
        size_t len;
        const char *s = purc_variant_get_string_const_ex(content, &len);

        pcdoc_text_node_t txt;
        txt = pcdoc_element_new_text_content(stack->doc, target,
                PCDOC_OP_APPEND, s, len);
        PURC_VARIANT_SAFE_CLEAR(content);

        if (txt == NULL) {
            // FIXME: continue or abortion?
        }
    } while (0);

    if (frame->ops.on_popping) {
        ok = frame->ops.on_popping(&co->stack, frame->ctxt);
        if (co->stack.exited)
            PC_ASSERT(ok);
    }

    if (ok) {
        pcintr_stack_t stack = &co->stack;
        pop_stack_frame(stack);
    } else {
        frame->next_step = NEXT_STEP_RERUN;
    }
}

static void
on_rerun(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    PC_ASSERT(!co->stack.exited);

    bool ok = false;
    if (frame->ops.rerun) {
        ok = frame->ops.rerun(&co->stack, frame->ctxt);
    }

    PC_ASSERT(ok);

    frame->next_step = NEXT_STEP_SELECT_CHILD;
}

static void
on_select_child(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct pcvdom_element *element = NULL;
    if (!co->stack.exited && frame->ops.select_child) {
        element = frame->ops.select_child(&co->stack, frame->ctxt);
    }

    if (element == NULL) {
        frame->next_step = NEXT_STEP_ON_POPPING;
    }
    else {
        frame->next_step = NEXT_STEP_SELECT_CHILD;

        // push child frame
        pcintr_stack_t stack = &co->stack;
        struct pcintr_stack_frame_normal *frame_normal;
        frame_normal = pcintr_push_stack_frame_normal(stack);
        if (!frame_normal)
            return;

        purc_variant_t at = pcintr_get_at_var(frame);
        PC_ASSERT(at != PURC_VARIANT_INVALID);

        pcdoc_element_t edom_element = NULL;
        if (!purc_variant_is_undefined(at))
            edom_element = pcdvobjs_get_element_from_elements(at, 0);

        struct pcintr_stack_frame *child_frame;
        child_frame = &frame_normal->frame;
        child_frame->ops = pcintr_get_ops_by_element(element);
        child_frame->pos = element;
        PC_ASSERT(element);
        child_frame->silently = pcintr_is_element_silently(child_frame->pos) ?
            1 : 0;
        child_frame->edom_element = edom_element;
        child_frame->scope = NULL;

        child_frame->next_step = NEXT_STEP_AFTER_PUSHED;
    }
}

void
pcintr_exception_copy(struct pcintr_exception *exception)
{
    if (!exception)
        return;

    const struct pcinst *inst = pcinst_current();
    exception->errcode        = inst->errcode;
    exception->error_except   = inst->error_except;
    exception->err_element    = inst->err_element;

    if (inst->err_exinfo)
        purc_variant_ref(inst->err_exinfo);
    PURC_VARIANT_SAFE_CLEAR(exception->exinfo);
    exception->exinfo = inst->err_exinfo;

    if (inst->bt)
        pcdebug_backtrace_ref(inst->bt);
    if (exception->bt) {
        pcdebug_backtrace_unref(exception->bt);
    }
    exception->bt = inst->bt;
}

bool pcintr_co_is_observed(pcintr_coroutine_t co)
{
    if (!list_empty(&co->stack.hvml_observers))
        return true;

    return false;
}

struct pcintr_stack_frame*
pcintr_stack_get_bottom_frame(pcintr_stack_t stack)
{
    if (!stack)
        return NULL;

    if (stack->nr_frames < 1)
        return NULL;

    struct list_head *tail = stack->frames.prev;
    return container_of(tail, struct pcintr_stack_frame, node);
}

struct pcintr_stack_frame*
pcintr_stack_frame_get_parent(struct pcintr_stack_frame *frame)
{
    if (!frame)
        return NULL;

    if (frame->type == STACK_FRAME_TYPE_PSEUDO)
        return NULL;

    if (list_is_first(&frame->node, &frame->owner->frames))
        return NULL;

    struct list_head *n = frame->node.prev;
    PC_ASSERT(n);

    return container_of(n, struct pcintr_stack_frame, node);
}

#define BUILTIN_VAR_CRTN        PURC_PREDEF_VARNAME_CRTN
#define BUILTIN_VAR_T           PURC_PREDEF_VARNAME_T
#define BUILTIN_VAR_DOC         PURC_PREDEF_VARNAME_DOC
#define BUILTIN_VAR_REQ         PURC_PREDEF_VARNAME_REQ

static bool
bind_cor_named_variable(purc_coroutine_t cor, const char* name,
        purc_variant_t var)
{
    if (var == PURC_VARIANT_INVALID) {
        return false;
    }

    if (!pcintr_bind_coroutine_variable(cor, name, var)) {
        purc_variant_unref(var);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }
    purc_variant_unref(var);
    return true;
}

static bool
bind_builtin_coroutine_variables(purc_coroutine_t cor, purc_variant_t request)
{
    // $TIMERS
    cor->timers = pcintr_timers_init(cor);
    if (!cor->timers) {
        return false;
    }

    // $REQ
    if (request && !pcintr_bind_coroutine_variable(
                cor, BUILTIN_VAR_REQ, request)) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    // $CRTN
    if(!bind_cor_named_variable(cor, BUILTIN_VAR_CRTN,
                purc_dvobj_coroutine_new(cor))) {
        return false;
    }

    // $T
    if(!bind_cor_named_variable(cor, BUILTIN_VAR_T,
                purc_dvobj_text_new())) {
        return false;
    }

    return true;
}

int
pcintr_init_vdom_under_stack(pcintr_stack_t stack)
{
    stack->async_request_ids = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (!stack->async_request_ids) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    // $DOC
    if(!bind_cor_named_variable(stack->co, BUILTIN_VAR_DOC,
                purc_dvobj_doc_new(stack->doc))) {
        return -1;
    }

    return 0;
}

void pcintr_execute_one_step_for_ready_co(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    if (frame == NULL)
        return;

    switch (frame->next_step) {
        case NEXT_STEP_AFTER_PUSHED:
            after_pushed(co, frame);
            break;
        case NEXT_STEP_ON_POPPING:
            on_popping(co, frame);
            break;
        case NEXT_STEP_RERUN:
            on_rerun(co, frame);
            break;
        case NEXT_STEP_SELECT_CHILD:
            on_select_child(co, frame);
            break;
        default:
            PC_ASSERT(0);
            break;
    }
}

static void
execute_one_step_for_exiting_co(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = &co->stack;
    PC_ASSERT(stack->exited);
    PC_ASSERT(co->stack.except == 0);

    // CHECK pending requests
    PC_ASSERT(co->stack.back_anchor == NULL);

    pcintr_heap_t heap = co->owner;
    struct pcinst *inst = heap->owner;

    purc_variant_t result = pcintr_coroutine_get_result(co);

    if (heap->cond_handler) {
        /* TODO: pass real result here */
        struct purc_cor_exit_info info = {
            result,
            stack->doc };
        heap->cond_handler(PURC_COND_COR_EXITED, co, &info);
    }

    if (co->curator) {
        // XXX: the curator may live in another thread!
        purc_atom_t cid = co->curator;
        co->curator = 0;

        purc_variant_t element_value = purc_variant_make_ulongint(co->cid);
        pcintr_coroutine_post_event(cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                element_value,
                MSG_TYPE_SUB_EXIT, NULL,
                result, PURC_VARIANT_INVALID);
        purc_variant_unref(element_value);

    }

    pcutils_rbtree_erase(&co->node, &heap->coroutines);
    coroutine_destroy(co);

    if (heap->keep_alive == 0 &&
            pcutils_rbtree_first(&heap->coroutines) == NULL)
    {
        purc_runloop_stop(inst->running_loop);
    }

    return;
}

void pcintr_check_after_execution(void)
{
    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);
    pcintr_check_after_execution_full(pcinst_current(), co);
}

void pcintr_run_exiting_co(void *ctxt)
{
    pcintr_coroutine_t co = (pcintr_coroutine_t)ctxt;
    PC_ASSERT(co);
    switch (co->state) {
        case CO_STATE_READY:
        case CO_STATE_EXITED:
            pcintr_coroutine_set_state(co, CO_STATE_RUNNING);
            coroutine_set_current(co);
            execute_one_step_for_exiting_co(co);
            coroutine_set_current(NULL);
            break;
        case CO_STATE_RUNNING:
            PC_ASSERT(0);
            break;
        case CO_STATE_STOPPED:
            PC_ASSERT(0);
            break;
        default:
            PC_ASSERT(0);
    }
}

void
pcintr_revoke_all_hvml_observers(pcintr_stack_t stack)
{
    PC_ASSERT(stack);
    struct list_head *observers = &stack->hvml_observers;
    struct pcintr_observer *p, *n;
    list_for_each_entry_safe(p, n, observers, node) {
        pcintr_revoke_observer(p);
    }
}

bool pcintr_is_ready_for_event(void)
{
    struct pcinst *inst = pcinst_current();
    if (!inst) {
        fprintf(stderr,
                "purc instance not initialized or already cleaned up\n");
        abort();
    }

    pcintr_heap_t heap = pcintr_get_heap();
    if (!heap) {
        PC_DEBUG("purc instance not fully initialized\n");
        abort();
    }

    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (!co) {
        PC_DEBUG(
                "running in a purc thread "
                "but not in a correct coroutine context\n"
                );
        abort();
    }

    switch (co->state) {
        case CO_STATE_READY:
            break;
        case CO_STATE_RUNNING:
            purc_set_error_with_info(PURC_ERROR_NOT_READY,
                    "coroutine context is not READY but RUN");
            return false;
        case CO_STATE_STOPPED:
            purc_set_error_with_info(PURC_ERROR_NOT_READY,
                    "coroutine context is not READY but WAIT");
            return false;
        default:
            PC_ASSERT(0);
    }

    pcintr_stack_t stack = &co->stack;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    if (frame) {
        purc_set_error_with_info(PURC_ERROR_NOT_READY,
                "coroutine context is not READY for event/msg to be fired");
        return false;
    }

    return true;
}

void
pcintr_notify_to_stop(pcintr_coroutine_t co)
{
    if (!co)
        return;
    pcintr_cancel_t p, n;
    list_for_each_entry_reverse_safe(p, n, &co->registered_cancels, node) {
        PC_ASSERT(p->list);
        list_del(&p->node);
        p->list = NULL;
        p->cancel(p->ctxt);
    }
}

void pcintr_set_exit(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);

    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);

    pcintr_coroutine_set_result(co, val);

    if (co->stack.exited == 0) {
        co->stack.exited = 1;
        pcintr_notify_to_stop(co);
    }
}

static void init_frame_for_co(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame == NULL);

    struct pcintr_stack_frame_normal *frame_normal;
    frame_normal = pcintr_push_stack_frame_normal(stack);
    if (!frame_normal) {
        return;
    }

    frame = &frame_normal->frame;

    frame->ops = *pcintr_get_document_ops();
    co->stage = CO_STAGE_FIRST_RUN;
}

static int set_coroutine_id(pcintr_coroutine_t coroutine)
{
    pcintr_heap_t heap = pcintr_get_heap();
    PC_ASSERT(heap);
    struct pcinst *inst = pcinst_current();
    PC_ASSERT(inst && inst == heap->owner);
    PC_ASSERT(inst->runner_name);

    size_t len = PURC_LEN_ENDPOINT_NAME + PURC_LEN_UNIQUE_ID + 4;
    char *p = (char*)malloc(len);
    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    char id_buf[PURC_LEN_UNIQUE_ID + 1];
    purc_generate_unique_id(id_buf, COROUTINE_PREFIX);

    sprintf(p, "%s/%s", inst->endpoint_name, id_buf);
    coroutine->cid = purc_atom_from_string_ex(PURC_ATOM_BUCKET_DEF, p);
    free(p);

    return 0;
}

static int
cmp_by_atom(struct rb_node *node, void *ud)
{
    purc_atom_t *atom = (purc_atom_t*)ud;
    pcintr_coroutine_t co;
    co = container_of(node, struct pcintr_coroutine, node);
    return (*atom) - co->cid;
}

static pcintr_coroutine_t
coroutine_create(purc_vdom_t vdom, pcintr_coroutine_t parent,
        pcrdr_page_type page_type, void *user_data)
{
    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = inst->intr_heap;
    struct rb_root *coroutines = &heap->coroutines;

    pcintr_coroutine_t co = NULL;
    pcintr_stack_t stack = NULL;

    co = (pcintr_coroutine_t)calloc(1, sizeof(*co));
    if (!co) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fail;
    }

    if (set_coroutine_id(co)) {
        goto fail_co;
    }

    pcvdom_document_ref(vdom);
    co->vdom = vdom;
    pcintr_coroutine_set_state(co, CO_STATE_READY);
    list_head_init(&co->children);
    list_head_init(&co->ln_stopped);
    list_head_init(&co->registered_cancels);
    list_head_init(&co->tasks);

    co->mq = pcinst_msg_queue_create();
    if (!co->mq) {
        goto fail_co;
    }

    co->variables = pcvarmgr_create();
    if (!co->variables) {
        goto fail_variables;
    }

    stack = &co->stack;
    stack->co = co;
    co->owner = heap;
    co->user_data = user_data;
    co->loaded_vars = RB_ROOT;

    int r;
    r = pcutils_rbtree_insert_only(coroutines, &co->cid,
            cmp_by_atom, &co->node);
    PC_ASSERT(r == 0);

    stack_init(stack);
    pcintr_coroutine_add_sub_exit_observer(co);
    pcintr_coroutine_add_last_msg_observer(co);

    if (parent && page_type == PCRDR_PAGE_TYPE_INHERIT) {
        stack->doc = purc_document_ref(parent->stack.doc);
    }
    else if (doc_init(stack)) {
        goto fail_variables;
    }

    if (parent) {
        co->curator = parent->cid;
        pcintr_coroutine_child_t child;
        child = (pcintr_coroutine_child_t)calloc(1, sizeof(*child));
        if (!child) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fail_variables;
        }
        child->cid = co->cid;
        list_add_tail(&child->ln, &parent->children);
    }
    else {
        // set curator in caller
    }

    stack->vdom = vdom;
    if (heap->cond_handler) {
        heap->cond_handler(PURC_COND_COR_CREATED, co,
                (void *)(uintptr_t)co->cid);
    }

    return co;

fail_variables:
    pcinst_msg_queue_destroy(co->mq);

fail_co:
    free(co);

fail:
    return NULL;
}

static void
set_body_entry(pcintr_stack_t stack, const char *body_id)
{
    stack->body_id = strdup(body_id);
}

purc_coroutine_t
purc_schedule_vdom(purc_vdom_t vdom,
        purc_atom_t curator, purc_variant_t request,
        pcrdr_page_type page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_info, const char *body_id,
        void *user_data)
{
    pcintr_coroutine_t co;

    struct pcintr_heap* intr = pcintr_get_heap();
    PC_ASSERT(intr);

    pcintr_coroutine_t parent = NULL;
    if (curator) {
        struct rb_node* node = pcutils_rbtree_find(&intr->coroutines,
                &curator, cmp_by_atom);
        if (node) {
            parent = container_of(node, struct pcintr_coroutine, node);
        }
    }

    co = coroutine_create(vdom, parent, page_type, user_data);
    if (!co) {
        purc_log_error("Failed to create coroutine\n");
        goto failed;
    }

    if (parent == NULL) {
        co->curator = curator;
    }

    co->stage = CO_STAGE_SCHEDULED;

    /* Attach to rdr only if the document needs rdr,
       the document is newly created, and the page type is not null. */
    if (co->stack.doc->need_rdr &&
            purc_document_get_refc(co->stack.doc) == 1 &&
            page_type != PCRDR_PAGE_TYPE_NULL) {

        if (!pcintr_attach_to_renderer(co,
                page_type, target_workspace,
                target_group, page_name, extra_info)) {
            purc_log_warn("Failed to attach to renderer\n");
        }
    }
    else if (co->stack.doc->need_rdr &&
            purc_document_get_refc(co->stack.doc) > 1) {
        /* use same rdr parameters with parent */
        PC_ASSERT(parent);

        co->target_page_type = parent->target_page_type;
        co->target_workspace_handle = parent->target_workspace_handle;
        co->target_page_handle = parent->target_page_handle;
        co->target_dom_handle = parent->target_dom_handle;
    }

    if (body_id && body_id[0] != '\0') {
        set_body_entry(&co->stack, body_id);
    }

    if (!bind_builtin_coroutine_variables(co, request)) {
        goto failed;
    }

    init_frame_for_co(co);
    return co;

failed:
    if (co == NULL) {
        pcvdom_document_unref(vdom);
    }
    else {
        coroutine_destroy(co);
    }

    return NULL;
}

purc_cond_handler
purc_get_cond_handler(void)
{
    struct pcinst *inst = pcinst_current();
    if (inst) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return (purc_cond_handler)PURC_INVPTR;
    }

    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return (purc_cond_handler)PURC_INVPTR;
    }

    return heap->cond_handler;
}

purc_cond_handler
purc_set_cond_handler(purc_cond_handler handler)
{
    struct pcinst *inst = pcinst_current();
    if (inst) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return (purc_cond_handler)PURC_INVPTR;
    }

    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return (purc_cond_handler)PURC_INVPTR;
    }

    purc_cond_handler old = heap->cond_handler;
    heap->cond_handler = handler;
    return old;
}

int
purc_run(purc_cond_handler handler)
{
    struct pcinst *inst = pcinst_current();
    PC_ASSERT(inst);
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return -1;
    }

    purc_runloop_t runloop = purc_runloop_get_current();
    if (inst->running_loop != runloop) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return -1;
    }

    heap->keep_alive = 0;
    heap->cond_handler = handler;

    purc_runloop_set_idle_func(runloop, pcintr_schedule, inst);
    purc_runloop_run();

    return 0;
}

static bool
set_object_by(purc_variant_t obj, struct pcintr_dynamic_args *arg)
{
    purc_variant_t dynamic;
    dynamic = purc_variant_make_dynamic(arg->getter, arg->setter);
    if (dynamic == PURC_VARIANT_INVALID)
        return false;

    bool ok = purc_variant_object_set_by_static_ckey(obj, arg->name, dynamic);
    if (!ok) {
        purc_variant_unref(dynamic);
        return false;
    }

    return true;
}

purc_variant_t
pcintr_make_object_of_dynamic_variants(size_t nr_args,
    struct pcintr_dynamic_args *args)
{
    purc_variant_t obj;
    obj = purc_variant_make_object_by_static_ckey(0,
            NULL, PURC_VARIANT_INVALID);

    if (obj == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (size_t i=0; i<nr_args; ++i) {
        struct pcintr_dynamic_args *arg = args + i;
        if (!set_object_by(obj, arg)) {
            purc_variant_unref(obj);
            return false;
        }
    }

    return obj;
}

// type:sub_type
bool
pcintr_parse_event(const char *event, purc_variant_t *type,
        purc_variant_t *sub_type)
{
    if (event == NULL || type == NULL) {
        goto out;
    }

    const char *p = strchr(event, EVENT_SEPARATOR);
    if (p) {
        *type = purc_variant_make_string_ex(event, p - event, true);
        if (*type == PURC_VARIANT_INVALID) {
            goto out;
        }
        if (sub_type) {
            *sub_type = purc_variant_make_string(p + 1, true);
            if (*sub_type == PURC_VARIANT_INVALID) {
                goto out_unref_type;
            }
        }
    }
    else {
        *type = purc_variant_make_string(event, true);
        if (*type == PURC_VARIANT_INVALID) {
            goto out;
        }
    }

    return true;

out_unref_type:
    if (*type) {
        purc_variant_unref(*type);
        *type = NULL;
    }

out:
    return false;
}

purc_variant_t
pcintr_load_from_uri(pcintr_stack_t stack, const char* uri)
{
    if (uri == NULL) {
        return PURC_VARIANT_INVALID;
    }

    if (stack->co->base_url_string) {
        pcfetcher_set_base_url(stack->co->base_url_string);
    }
    purc_variant_t ret = PURC_VARIANT_INVALID;
    struct pcfetcher_resp_header resp_header = {0};
    uint32_t timeout = stack->co->timeout.tv_sec;
    purc_rwstream_t resp = pcfetcher_request_sync(
            uri,
            PCFETCHER_REQUEST_METHOD_GET,
            NULL,
            timeout,
            &resp_header);
    if (resp_header.ret_code == 200) {
        size_t sz_content = 0;
        char* buf = (char*)purc_rwstream_get_mem_buffer(resp, &sz_content);
        // FIXME:
        purc_clr_error();
        ret = purc_variant_make_from_json_string(buf, sz_content);
    }

    if (resp_header.mime_type) {
        free(resp_header.mime_type);
    }

    if (resp) {
        purc_rwstream_destroy(resp);
        resp = NULL;
    }

    return ret;
}

struct load_async_data {
    pcfetcher_response_handler handler;
    void* ctxt;
    pthread_t                  requesting_thread;
    pcintr_stack_t             requesting_stack;
    purc_variant_t             request_id;
};

static void
release_load_async_data(struct load_async_data *data)
{
    if (data) {
        PURC_VARIANT_SAFE_CLEAR(data->request_id);
        data->handler           = NULL;
        data->ctxt              = NULL;
        data->requesting_thread = 0;
        data->requesting_stack  = NULL;
    }
}

static void
destroy_load_async_data(struct load_async_data *data)
{
    if (data) {
        release_load_async_data(data);
        free(data);
    }
}

static void
on_load_async_done(
        purc_variant_t request_id, void* ctxt,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    struct load_async_data *data;
    data = (struct load_async_data*)ctxt;
    data->handler(request_id, data->ctxt, resp_header, resp);
    destroy_load_async_data(data);
}

purc_variant_t
pcintr_load_from_uri_async(pcintr_stack_t stack, const char* uri,
        enum pcfetcher_request_method method, purc_variant_t params,
        pcfetcher_response_handler handler, void* ctxt)
{
    PC_ASSERT(stack);
    PC_ASSERT(uri);
    PC_ASSERT(handler);

    PC_ASSERT(pcintr_get_stack() == stack);

    struct load_async_data *data;
    data = (struct load_async_data*)malloc(sizeof(*data));
    if (!data) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }
    data->handler              = handler;
    data->ctxt                 = ctxt;
    data->requesting_thread    = pthread_self();
    data->requesting_stack     = stack;
    data->request_id           = PURC_VARIANT_INVALID;

    if (stack->co->base_url_string) {
        pcfetcher_set_base_url(stack->co->base_url_string);
    }

    uint32_t timeout = stack->co->timeout.tv_sec;
    data->request_id = pcfetcher_request_async(
            uri,
            method,
            params,
            timeout,
            on_load_async_done,
            data);

    if (data->request_id == PURC_VARIANT_INVALID) {
        destroy_load_async_data(data);
        return PURC_VARIANT_INVALID;
    }

    return data->request_id;
}

bool
pcintr_save_async_request_id(pcintr_stack_t stack, purc_variant_t req_id)
{
    if (!stack || !req_id) {
        return false;
    }

    return purc_variant_array_append(stack->async_request_ids, req_id);
}

bool
pcintr_remove_async_request_id(pcintr_stack_t stack, purc_variant_t req_id)
{
    if (!stack || !req_id) {
        return false;
    }
    size_t sz = purc_variant_array_get_size(stack->async_request_ids);
    for (size_t i = 0; i < sz; i++) {
        if (req_id == purc_variant_array_get(stack->async_request_ids, i)) {
            purc_variant_array_remove(stack->async_request_ids, i);
            break;
        }
    }
    return true;
}

purc_variant_t
pcintr_load_vdom_fragment_from_uri(pcintr_stack_t stack, const char* uri)
{
    if (uri == NULL) {
        return PURC_VARIANT_INVALID;
    }

    if (stack->co->base_url_string) {
        pcfetcher_set_base_url(stack->co->base_url_string);
    }
    uint32_t timeout = stack->co->timeout.tv_sec;
    purc_variant_t ret = PURC_VARIANT_INVALID;
    struct pcfetcher_resp_header resp_header = {0};
    purc_rwstream_t resp = pcfetcher_request_sync(
            uri,
            PCFETCHER_REQUEST_METHOD_GET,
            NULL,
            timeout,
            &resp_header);
    if (resp_header.ret_code == 200) {
        size_t sz_content = 0;
        char* buf = (char*)purc_rwstream_get_mem_buffer(resp, &sz_content);
        (void)buf;
        purc_clr_error();
        // TODO: modify vdom in place????
        // ret = purc_variant_make_from_json_string(buf, sz_content);
        purc_rwstream_destroy(resp);
        PC_ASSERT(0);
    }

    if (resp_header.mime_type) {
        free(resp_header.mime_type);
    }
    return ret;
}

#define DOC_QUERY         "query"

purc_variant_t
pcintr_doc_query(purc_coroutine_t cor, const char* css, bool silently)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    if (cor == NULL || css == NULL) {
        goto end;
    }

    purc_variant_t doc = pcintr_get_coroutine_variable(cor, BUILTIN_VAR_DOC);
    if (doc == PURC_VARIANT_INVALID) {
        PC_ASSERT(0);
        goto end;
    }

    struct purc_native_ops *ops = purc_variant_native_get_ops(doc);
    if (ops == NULL) {
        PC_ASSERT(0);
        goto end;
    }

    purc_nvariant_method native_func = ops->property_getter(DOC_QUERY);
    if (!native_func) {
        PC_ASSERT(0);
        goto end;
    }

    purc_variant_t arg = purc_variant_make_string(css, false);
    if (arg == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto end;
    }

    // TODO: silenly
    ret = native_func (purc_variant_native_get_entity(doc), 1, &arg, silently);
    purc_variant_unref(arg);
end:
    return ret;
}

bool
pcintr_load_dynamic_variant(pcintr_coroutine_t cor,
    const char *name, size_t len)
{
    char NAME[PATH_MAX+1];
    snprintf(NAME, sizeof(NAME), "%.*s", (int)len, name);

    struct rb_root *root = &cor->loaded_vars;

    struct rb_node **pnode = &root->rb_node;
    struct rb_node *parent = NULL;
    struct rb_node *entry = NULL;
    while (*pnode) {
        struct pcintr_loaded_var *p;
        p = container_of(*pnode, struct pcintr_loaded_var, node);

        int ret = strcmp(NAME, p->name);

        parent = *pnode;

        if (ret < 0)
            pnode = &parent->rb_left;
        else if (ret > 0)
            pnode = &parent->rb_right;
        else{
            return true;
        }
    }

    struct pcintr_loaded_var *p = NULL;

    purc_variant_t v = purc_variant_load_dvobj_from_so(NULL, NAME);
    if (v == PURC_VARIANT_INVALID)
        return false;

    p = (struct pcintr_loaded_var*)calloc(1, sizeof(*p));
    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }

    p->val = v;

    p->name = strdup(NAME);
    if (!p->name) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }

    entry = &p->node;

    pcutils_rbtree_link_node(entry, parent, pnode);
    pcutils_rbtree_insert_color(entry, root);

    if (pcintr_bind_coroutine_variable(cor, NAME, v)) {
        return true;
    }

error:
    destroy_loaded_var(p);

    return false;
}

static struct pcvdom_template*
template_create(void)
{
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)calloc(1, sizeof(*tpl));
    if (!tpl) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    return tpl;
}

static void
template_cleaner(struct pcvdom_template *tpl)
{
    if (!tpl)
        return;

    if (tpl->vcm && tpl->to_free) {
        pcvcm_node_destroy(tpl->vcm);
    }
    PURC_VARIANT_SAFE_CLEAR(tpl->type);
    tpl->vcm = NULL;
    tpl->to_free = false;
}

static void
template_destroy(struct pcvdom_template *tpl)
{
    if (!tpl)
        return;

    template_cleaner(tpl);
    free(tpl);
}

// the cleaner to clear the content represented by the native entity.
static purc_variant_t
cleaner(void* native_entity, unsigned call_flags)
{
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)native_entity;
    PC_ASSERT(tpl);
    template_cleaner(tpl);

    UNUSED_PARAM(call_flags);
    return purc_variant_make_boolean(true);
}

// the callback to release the native entity.
static void
on_release(void* native_entity)
{
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)native_entity;
    PC_ASSERT(tpl);
    template_destroy(tpl);
}

static struct purc_native_ops ops_tpl = {
    .cleaner                      = cleaner,
    .on_release                   = on_release,
};

static int
check_template_variant(purc_variant_t val)
{
    if (val == PURC_VARIANT_INVALID ||
            purc_variant_is_native(val) == false)
    {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    struct purc_native_ops *ops;
    ops = (struct purc_native_ops*)val->ptr_ptr[1];

    if (ops != &ops_tpl) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    return 0;
}

purc_variant_t
pcintr_template_make(void)
{
    struct pcvdom_template *tpl;
    tpl = template_create();
    if (!tpl)
        return PURC_VARIANT_INVALID;

    purc_variant_t v = purc_variant_make_native(tpl, &ops_tpl);
    if (!v) {
        template_destroy(tpl);
        return PURC_VARIANT_INVALID;
    }

    int r = check_template_variant(v);
    PC_ASSERT(0 == r);
    return v;
}

int
pcintr_template_set(purc_variant_t val, struct pcvcm_node *vcm,
        purc_variant_t type, bool to_free)
{
    PC_ASSERT(val);
    PC_ASSERT(vcm);

    int r;
    r = check_template_variant(val);
    if (r)
        return -1;

    void *native_entity = purc_variant_native_get_entity(val);
    PC_ASSERT(native_entity);
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)native_entity;

    PC_ASSERT(tpl->vcm == NULL);
    tpl->vcm = vcm;
    if (type) {
        tpl->type = purc_variant_ref(type);
    }
    tpl->to_free = to_free;

    return 0;
}

void
pcintr_template_walk(purc_variant_t val, void *ctxt,
        pcintr_template_walk_cb cb)
{
    int r;
    r = check_template_variant(val);
    // FIXME: modify pcintr_template_walk function-signature
    PC_ASSERT(r == 0);

    void *native_entity = purc_variant_native_get_entity(val);
    PC_ASSERT(native_entity);
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)native_entity;

    cb(tpl->vcm, ctxt);
}


static purc_variant_t
attribute_assign(purc_variant_t left, purc_variant_t right)
{
    UNUSED_PARAM(left);

    purc_variant_ref(right);

    return right;
}

static purc_variant_t
attribute_addition(purc_variant_t left, purc_variant_t right)
{
    return pcvdom_tokenwised_eval_attr(PCHVML_ATTRIBUTE_ADDITION_OPERATOR,
            left, right);
}

static purc_variant_t
attribute_subtraction(purc_variant_t left, purc_variant_t right)
{
    return pcvdom_tokenwised_eval_attr(PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR,
            left, right);
}

static purc_variant_t
attribute_asterisk(purc_variant_t left, purc_variant_t right)
{
    return pcvdom_tokenwised_eval_attr(PCHVML_ATTRIBUTE_ASTERISK_OPERATOR,
            left, right);
}

static purc_variant_t
attribute_regex(purc_variant_t left, purc_variant_t right)
{
    return pcvdom_tokenwised_eval_attr(PCHVML_ATTRIBUTE_REGEX_OPERATOR,
            left, right);
}

static purc_variant_t
attribute_precise(purc_variant_t left, purc_variant_t right)
{
    return pcvdom_tokenwised_eval_attr(PCHVML_ATTRIBUTE_PRECISE_OPERATOR,
            left, right);
}

static purc_variant_t
attribute_replace(purc_variant_t left, purc_variant_t right)
{
    return pcvdom_tokenwised_eval_attr(PCHVML_ATTRIBUTE_REPLACE_OPERATOR,
            left, right);
}

static purc_variant_t
attribute_head_addition(purc_variant_t left, purc_variant_t right)
{
    return pcvdom_tokenwised_eval_attr(PCHVML_ATTRIBUTE_HEAD_OPERATOR,
            left, right);
}

static purc_variant_t
attribute_tail_addition(purc_variant_t left, purc_variant_t right)
{
    return pcvdom_tokenwised_eval_attr(PCHVML_ATTRIBUTE_TAIL_OPERATOR,
            left, right);
}

pcintr_attribute_op
pcintr_attribute_get_op(enum pchvml_attr_operator op)
{
    switch (op) {
        case PCHVML_ATTRIBUTE_OPERATOR:
            return attribute_assign;

        case PCHVML_ATTRIBUTE_ADDITION_OPERATOR:
            return attribute_addition;

        case PCHVML_ATTRIBUTE_SUBTRACTION_OPERATOR:
            return attribute_subtraction;

        case PCHVML_ATTRIBUTE_ASTERISK_OPERATOR:
            return attribute_asterisk;

        case PCHVML_ATTRIBUTE_REGEX_OPERATOR:
            return attribute_regex;

        case PCHVML_ATTRIBUTE_PRECISE_OPERATOR:
            return attribute_precise;

        case PCHVML_ATTRIBUTE_REPLACE_OPERATOR:
            return attribute_replace;

        case PCHVML_ATTRIBUTE_HEAD_OPERATOR:
            return attribute_head_addition;

        case PCHVML_ATTRIBUTE_TAIL_OPERATOR:
            return attribute_tail_addition;

        default:
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
            return NULL;
    }
}

int
pcintr_set_symbol_var(struct pcintr_stack_frame *frame,
        enum purc_symbol_var symbol, purc_variant_t val)
{
    PC_ASSERT(frame);
    PC_ASSERT(symbol >= 0);
    PC_ASSERT(symbol < PURC_SYMBOL_VAR_MAX);
    PC_ASSERT(val != PURC_VARIANT_INVALID);

    purc_variant_ref(val);
    PURC_VARIANT_SAFE_CLEAR(frame->symbol_vars[symbol]);
    frame->symbol_vars[symbol] = val;

    return 0;
}

purc_variant_t
pcintr_get_symbol_var(struct pcintr_stack_frame *frame,
        enum purc_symbol_var symbol)
{
    PC_ASSERT(frame);
    PC_ASSERT(symbol >= 0);
    PC_ASSERT(symbol < PURC_SYMBOL_VAR_MAX);

    return frame->symbol_vars[symbol];
}

int
pcintr_refresh_at_var(struct pcintr_stack_frame *frame)
{
    purc_variant_t at =
        pcdvobjs_make_elements(frame->owner->doc, frame->edom_element);
    if (at == PURC_VARIANT_INVALID)
        return -1;

    int r;
    r = pcintr_set_at_var(frame, at);
    purc_variant_unref(at);

    return r ? -1 : 0;
}

int
pcintr_set_at_var(struct pcintr_stack_frame *frame, purc_variant_t val)
{
    return pcintr_set_symbol_var(frame, PURC_SYMBOL_VAR_AT_SIGN, val);
}

purc_variant_t
pcintr_get_at_var(struct pcintr_stack_frame *frame)
{
    return pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_AT_SIGN);
}

int
pcintr_set_question_var(struct pcintr_stack_frame *frame, purc_variant_t val)
{
    return pcintr_set_symbol_var(frame, PURC_SYMBOL_VAR_QUESTION_MARK, val);
}

purc_variant_t
pcintr_get_question_var(struct pcintr_stack_frame *frame)
{
    return pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_QUESTION_MARK);
}

int
pcintr_set_exclamation_var(struct pcintr_stack_frame *frame,
        purc_variant_t val)
{
    return pcintr_set_symbol_var(frame, PURC_SYMBOL_VAR_EXCLAMATION, val);
}

purc_variant_t
pcintr_get_exclamation_var(struct pcintr_stack_frame *frame)
{
    return pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_EXCLAMATION);
}

int
pcintr_inc_percent_var(struct pcintr_stack_frame *frame)
{
    purc_variant_t v;
    v = pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_PERCENT_SIGN);
    PC_ASSERT(v != PURC_VARIANT_INVALID);
    PC_ASSERT(purc_variant_is_ulongint(v));
    v->u64 += 1;

    return 0;
}

purc_variant_t
pcintr_get_percent_var(struct pcintr_stack_frame *frame)
{
    return pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_PERCENT_SIGN);
}

void
pcintr_observe_vcm_ev(pcintr_stack_t stack, struct pcintr_observer* observer,
        purc_variant_t var, struct purc_native_ops *ops)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(observer);
    UNUSED_PARAM(var);
    UNUSED_PARAM(ops);

    void *native_entity = purc_variant_native_get_entity(var);

    // create virtual frame
    struct pcintr_stack_frame_normal *frame_normal;
    frame_normal = pcintr_push_stack_frame_normal(stack);
    if (!frame_normal)
        return;

    struct pcintr_stack_frame *frame;
    frame = &frame_normal->frame;

    frame->ops = pcintr_get_ops_by_element(observer->pos);
    frame->scope = observer->scope;
    frame->pos = observer->pos;
    frame->silently = pcintr_is_element_silently(frame->pos) ? 1 : 0;
    frame->edom_element = observer->edom_element;

    // eval value
    purc_nvariant_method eval_getter = ops->property_getter(
            PCVCM_EV_PROPERTY_EVAL);
    purc_variant_t new_val = eval_getter(native_entity, 0, NULL,
            frame->silently ? true : false);
    pop_stack_frame(stack);

    if (!new_val) {
        return;
    }

    // get last value
    purc_nvariant_method last_value_getter = ops->property_getter(
            PCVCM_EV_PROPERTY_LAST_VALUE);
    purc_variant_t last_value = last_value_getter(native_entity, 0, NULL,
            frame->silently ? true : false);
    int cmp = purc_variant_compare_ex(new_val, last_value,
            PCVARIANT_COMPARE_OPT_AUTO);
    if (cmp == 0) {
        purc_variant_unref(new_val);
        return;
    }

    purc_nvariant_method last_value_setter = ops->property_setter(
            PCVCM_EV_PROPERTY_LAST_VALUE);
    last_value_setter(native_entity, 1, &new_val,
            frame->silently ? true : false);

    // dispatch change event
    pcintr_coroutine_post_event(stack->co->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
            var, MSG_TYPE_CHANGE, NULL,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
}

purc_runloop_t
pcintr_co_get_runloop(pcintr_coroutine_t co)
{
    if (!co)
        return NULL;

    pcintr_heap_t heap = co->owner;
    if (!heap)
        return NULL;

    struct pcinst *inst = heap->owner;
    if (!inst)
        return NULL;

    return inst->running_loop;
}

struct timer_data {
    pcintr_timer_t               timer;
    char                        *id;
};

static void
event_timer_fire(pcintr_timer_t timer, const char* id, void* data)
{
    UNUSED_PARAM(timer);
    UNUSED_PARAM(id);
    UNUSED_PARAM(data);

    PC_ASSERT(pcintr_get_heap());

    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co == NULL) {
        return;
    }

    if (co->state != CO_STATE_OBSERVING)
    {
        return;
    }

    pcintr_stack_t stack = &co->stack;
    if (stack->exited)
        return;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame == NULL);

    struct list_head *observer_list = &stack->hvml_observers;
    struct pcintr_observer *p, *n;
    list_for_each_entry_safe(p, n, observer_list, node) {
        purc_variant_t var = p->observed;
        struct purc_native_ops *ops = purc_variant_native_get_ops(var);
        if (ops && ops->property_getter) {
            purc_nvariant_method is_vcm_ev = ops->property_getter(
                    PCVCM_EV_PROPERTY_VCM_EV);
            if (is_vcm_ev) {
                pcintr_observe_vcm_ev(stack, p, var, ops);
            }
        }
    }
}

static struct purc_native_ops ops_vdom = {};

purc_variant_t
pcintr_wrap_vdom(pcvdom_element_t vdom)
{
    PC_ASSERT(vdom != NULL);

    purc_variant_t val;
    val = purc_variant_make_native(vdom, &ops_vdom);

    return val;
}

pcvdom_element_t
pcintr_get_vdom_from_variant(purc_variant_t val)
{
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    if (purc_variant_is_native(val) == false) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    void *native = purc_variant_native_get_entity(val);
    if (native == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct purc_native_ops *ops;
    ops = (struct purc_native_ops*)val->ptr_ptr[1];
    if (ops != &ops_vdom) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    return (pcvdom_element_t)native;
}

void pcintr_cancel_init(pcintr_cancel_t cancel,
        void *ctxt, void (*cancel_routine)(void *ctxt))
{
    PC_ASSERT(ctxt);
    PC_ASSERT(cancel_routine);
    PC_ASSERT(cancel->ctxt   == NULL);
    PC_ASSERT(cancel->cancel == NULL);
    PC_ASSERT(cancel->list == NULL);

    cancel->ctxt   = ctxt;
    cancel->cancel = cancel_routine;
}

void pcintr_register_cancel(pcintr_cancel_t cancel)
{
    PC_ASSERT(cancel);
    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);

    PC_ASSERT(cancel->list == NULL);
    list_add_tail(&cancel->node, &co->registered_cancels);
    cancel->list = &co->registered_cancels;
}

void pcintr_unregister_cancel(pcintr_cancel_t cancel)
{
    PC_ASSERT(cancel);
    if (cancel->list == NULL)
        return;

    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);

    PC_ASSERT(cancel->list == &co->registered_cancels);
    list_del(&cancel->node);
    cancel->list = NULL;
}

void*
pcintr_load_module(const char *module,
        const char *env_name, const char *prefix)
{
#define PRINT_DEBUG
#if OS(LINUX) || OS(UNIX) || OS(DARWIN)             /* { */
    const char *ext = ".so";
#   if OS(DARWIN)
    ext = ".dylib";
#   endif

    if (!prefix)
        prefix = "";

    void *library_handle = NULL;

    char so[PATH_MAX+1];
    int n;
    do {
        /* XXX: the order of searching directories:
         *
         * 1. the valid directories contains in the environment variable:
         *      $(env_name)
         * 2. /usr/local/lib/purc-<purc-api-version>/
         * 3. /usr/lib/purc-<purc-api-version>/
         * 4. /lib/purc-<purc-api-version>/
         */

        // step1: search in directories defined by the env var
        const char *env = NULL;
        if (env_name)
            env = getenv(env_name);

        if (env) {
            char *path = strdup(env);
            char *str1;
            char *saveptr1;
            char *dir;

            for (str1 = path; ; str1 = NULL) {
                dir = strtok_r(str1, ":;", &saveptr1);
                if (dir == NULL || dir[0] != '/') {
                    break;
                }

                n = snprintf(so, sizeof(so),
                        "%s/%s%s%s", dir, prefix, module, ext);
                PC_ASSERT(n>0 && (size_t)n<sizeof(so));
                library_handle = dlopen(so, RTLD_LAZY);
                if (library_handle) {
#ifdef PRINT_DEBUG        /* { */
                    PC_DEBUGX("Loaded from %s\n", so);
#endif                    /* } */
                    break;
                }
            }

            free(path);

            if (library_handle)
                break;
        }

        static const char *ver = PURC_API_VERSION_STRING;

        // try in system directories.
        static const char *other_tries[] = {
            "/usr/local/lib/purc-%s/%s%s%s",
            "/usr/lib/purc-%s/%s%s%s",
            "/lib/purc-%s/%s%s%s",
        };

        for (size_t i = 0; i < PCA_TABLESIZE(other_tries); i++) {
            n = snprintf(so, sizeof(so), other_tries[i], ver,
                    prefix, module, ext);
            PC_ASSERT(n>0 && (size_t)n<sizeof(so));
            library_handle = dlopen(so, RTLD_LAZY);
            if (library_handle) {
#ifdef PRINT_DEBUG        /* { */
                PC_DEBUGX("Loaded from %s\n", so);
#endif                    /* } */
                break;
            }
        }

    } while (0);

    if (!library_handle) {
        purc_set_error_with_info(PURC_ERROR_BAD_SYSTEM_CALL,
                "failed to load: %s", so);
        return NULL;
    }
#ifdef PRINT_DEBUG        /* { */
    PC_DEBUGX("loaded: %s", so);
#endif                    /* } */

    return library_handle;

#else                                                 /* }{ */
    UNUSED_PARAM(module);
    UNUSED_PARAM(env_name);
    UNUSED_PARAM(prefix);

    // TODO: Add codes for other OS.
    pcinst_set_error (PURC_ERROR_NOT_SUPPORTED);
    PC_ASSERT(0); // Not implemented yet
#endif                                                /* } */
#undef PRINT_DEBUG
}

void
pcintr_unload_module(void *handle)
{
    if (!handle)
        return;

    if (1)
        return;

    // FIXME: we don't close for the moment
    int r;
    r = dlclose(handle);

    PC_ASSERT(r == 0);
}

int
pcintr_bind_template(purc_variant_t templates,
        purc_variant_t type, purc_variant_t contents)
{
    PC_ASSERT(templates != PURC_VARIANT_INVALID);
    PC_ASSERT(type != PURC_VARIANT_INVALID);
    PC_ASSERT(contents != PURC_VARIANT_INVALID);

    bool ok;
    ok = purc_variant_object_set(templates, type, contents);

    return ok ? 0 : -1;
}

void
pcintr_match_template(purc_variant_t templates,
        purc_atom_t type, purc_variant_t *content)
{
    PC_ASSERT(content);
    *content = NULL;

    if (templates == PURC_VARIANT_INVALID)
        return;

    PC_ASSERT(purc_variant_is_object(templates));

    PC_ASSERT(type);

    const char *s_type = purc_atom_to_string(type);
    PC_ASSERT(s_type);

    purc_variant_t k, v;
    foreach_key_value_in_variant_object(templates, k, v) {
        PC_ASSERT(k != PURC_VARIANT_INVALID);
        PC_ASSERT(purc_variant_is_string(k));
        PC_ASSERT(v != PURC_VARIANT_INVALID);

        const char *sk = purc_variant_get_string_const(k);
        int wild = 1;
        if (strcmp(sk, "*")) {
            wild = 0;
            if (strcmp(sk, s_type)) {
                continue;
            }
        }

        int r = check_template_variant(v);
        PC_ASSERT(0 == r);

        PURC_VARIANT_SAFE_CLEAR(*content);
        *content = purc_variant_ref(v);

        if (wild)
            continue;

        break;
    }
    end_foreach;
}

struct template_walk_data {
    pcintr_stack_t               stack;

    int                          r;
    purc_variant_t               val;
};

static int
template_walker(struct pcvcm_node *vcm, void *ctxt)
{
    struct template_walk_data *ud;
    ud = (struct template_walk_data*)ctxt;
    PC_ASSERT(ud);
    PC_ASSERT(ud->val == PURC_VARIANT_INVALID);

    pcintr_stack_t stack = ud->stack;
    PC_ASSERT(stack);

    // TODO: silently
    purc_variant_t v = pcvcm_eval(vcm, stack, false);
    PC_ASSERT(v != PURC_VARIANT_INVALID);

    if (purc_variant_is_string(v)) {
        const char *s = purc_variant_get_string_const(v);

        size_t chunk = 128;
        struct pcutils_stringbuilder sb;
        pcutils_stringbuilder_init(&sb, chunk);
        int n = pcutils_stringbuilder_snprintf(&sb, "%s", s);
        if (n < 0 || (size_t)n != strlen(s)) {
            pcutils_stringbuilder_reset(&sb);
            purc_variant_unref(v);
            ud->r = -1;
            return -1;
        }

        char *ssv = pcutils_stringbuilder_build(&sb);
        if (ssv) {
            ud->val = purc_variant_make_string_reuse_buff(ssv, strlen(ssv), true);
            PC_ASSERT(v);
        }
        pcutils_stringbuilder_reset(&sb);
    }
    else {
        ud->val = v;
        purc_variant_ref(ud->val);
    }

    purc_variant_unref(v);
    return 0;
}

purc_variant_t
pcintr_template_expansion(purc_variant_t val)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);

    struct template_walk_data ud = {
        .stack        = stack,
        .r            = 0,
        .val          = PURC_VARIANT_INVALID,
    };

    pcintr_template_walk(val, &ud, template_walker);

    int r = ud.r;
    purc_variant_t v = PURC_VARIANT_INVALID;

    if (r == 0) {
        v = ud.val;
        PC_ASSERT(v);
    }

    return v;
}

purc_variant_t
pcintr_template_get_type(purc_variant_t val)
{
    int r;
    r = check_template_variant(val);
    // FIXME: modify pcintr_template_walk function-signature
    PC_ASSERT(r == 0);

    void *native_entity = purc_variant_native_get_entity(val);
    PC_ASSERT(native_entity);
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)native_entity;
    return tpl->type;
}

void
pcintr_coroutine_set_state_with_location(pcintr_coroutine_t co,
        enum pcintr_coroutine_state state,
        const char *file, int line, const char *func)
{
    //PLOG(">%s:%d:%s state=%d\n", file, line, func, state);
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);
    UNUSED_PARAM(func);
    co->state = state;
}

pcdoc_element_t
pcintr_util_new_element(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_operation op, const char *tag, bool self_close)
{
    pcdoc_element_t new_elem;

    new_elem = pcdoc_element_new_element(doc, elem, op, tag, self_close);
    if (new_elem) {
        // TODO check stage and send message to rdr
    }

    return new_elem;
}

pcdoc_text_node_t
pcintr_util_new_text_content(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_operation op, const char *txt, size_t len)
{
    pcdoc_text_node_t text_node;

    text_node = pcdoc_element_new_text_content(doc, elem, op,
            txt, len);

    // TODO: append/prepend textContent?
    pcintr_stack_t stack = pcintr_get_stack();
    if (text_node && stack && stack->co->target_page_handle) {
        pcintr_rdr_send_dom_req_simple_raw(stack, op,
                elem, "textContent", PCRDR_MSG_DATA_TYPE_PLAIN,
                txt, len);
    }

    return text_node;
}

pcdoc_node
pcintr_util_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *content, size_t len, purc_variant_t data_type)
{
    pcdoc_node node;
    node = pcdoc_element_new_content(doc, elem, op, content, len);

    pcrdr_msg_data_type type = doc->def_text_type;
    if (data_type) {
        /* use the type from archetype `type` attribute */
        type = pcintr_rdr_retrieve_data_type(
                purc_variant_get_string_const(data_type));
    }

    pcintr_stack_t stack = pcintr_get_stack();
    if (node.type != PCDOC_NODE_VOID &&
            stack && stack->co->target_page_handle) {
        pcintr_rdr_send_dom_req_simple_raw(stack, op,
                elem, NULL, type, content, len);
    }

    return node;
}

int
pcintr_util_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *name, const char *val, size_t len)
{
    if (pcdoc_element_set_attribute(doc, elem, op, name, val, len))
        return -1;

    pcintr_stack_t stack = pcintr_get_stack();
    if (stack && stack->co->target_page_handle) {
        char property[strlen(name) + 8];
        strcpy(property, "attr.");
        strcat(property, name);

        pcintr_rdr_send_dom_req_simple_raw(stack, op,
                elem, property, PCRDR_MSG_DATA_TYPE_PLAIN, val, len);
    }

    return 0;
}

void
pcintr_coroutine_set_result(pcintr_coroutine_t co, purc_variant_t result)
{
    if (!result) {
        return;
    }

    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame = pcintr_stack_get_bottom_frame(stack);
    while (frame && frame->pos && frame->pos->tag_id != PCHVML_TAG_HVML) {
        frame = pcintr_stack_frame_get_parent(frame);
    }

    if (!frame) {
        PC_ASSERT(0);
        return;  // NOTE: never reached here!!!
    }
    pcintr_set_question_var(frame, result);
}

purc_variant_t
pcintr_coroutine_get_result(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame = pcintr_stack_get_bottom_frame(stack);
    while (frame && frame->pos && frame->pos->tag_id != PCHVML_TAG_HVML) {
        frame = pcintr_stack_frame_get_parent(frame);
    }

    if (frame) {
        return pcintr_get_question_var(frame);
    }
    PC_ASSERT(0);
    return PURC_VARIANT_INVALID; // NOTE: never reached here!!!
}

bool
pcintr_is_variable_token(const char *str)
{
    return pcregex_is_match(HVML_VARIABLE_REGEX, str);
}

