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

#include "ops.h"
#include "../hvml/hvml-gen.h"
#include "../html/parser.h"

#include "hvml-attr.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <libgen.h>

#define EVENT_TIMER_INTRVAL  10
#define DEFAULT_MOVE_BUFFER_SIZE 64

#define EVENT_SEPARATOR      ':'
#define MSG_TYPE_CHANGE     "change"
#define MSG_SUB_TYPE_CLOSE  "close"
#define COROUTINE_PREFIX    "COROUTINE"

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

void
pcintr_util_dump_document_ex(pchtml_html_document_t *doc, char **dump_buff,
    const char *file, int line, const char *func)
{
    PC_ASSERT(doc);
    UNUSED_PARAM(dump_buff);
    UNUSED_PARAM(file);
    UNUSED_PARAM(line);
    UNUSED_PARAM(func);

    char buf[1024];
    size_t nr = sizeof(buf);
    int opt = 0;
    opt |= PCHTML_HTML_SERIALIZE_OPT_UNDEF;
    opt |= PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE;
    if (!dump_buff) {
        opt |= PCHTML_HTML_SERIALIZE_OPT_WITH_HVML_HANDLE;
    }
    char *p = pchtml_doc_snprintf_ex(doc,
            (enum pchtml_html_serialize_opt)opt, buf, &nr, "");
    if (!p)
        return;

    doc = pchmtl_html_load_document_with_buf((const unsigned char*)p, nr);
    if (doc) {
        if (p != buf)
            free(p);
        nr = sizeof(buf);
        p = pchtml_doc_snprintf(doc, buf, &nr, "");
        pchtml_html_document_destroy(doc);
    }
    if (!p)
        return;

    if (dump_buff) {
        if (*dump_buff) {
            free(*dump_buff);
        }
        *dump_buff = strdup(p);
    }
#if 0
    else {
        fprintf(stderr, "%s[%d]:%s(): #document %p\n%s\n",
                pcutils_basename((char*)file), line, func, doc, p);
    }
#endif
    if (p != buf)
        free(p);
}

void
pcintr_util_dump_edom_node_ex(pcdom_node_t *node,
    const char *file, int line, const char *func)
{
    PC_ASSERT(node);

    char buf[1024];
    size_t nr = sizeof(buf);
    int opt = 0;
    opt |= PCHTML_HTML_SERIALIZE_OPT_UNDEF;
    opt |= PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE;
    char *p = pcdom_node_snprintf_ex(node,
            (enum pchtml_html_serialize_opt)opt, buf, &nr, "");
    if (p) {
        fprintf(stderr, "%s[%d]:%s():%p\n%s\n",
                pcutils_basename((char*)file), line, func, node, p);
        if (p != buf)
            free(p);
    }
}

void
pcintr_dump_frame_edom_node(pcintr_stack_t stack)
{
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(frame->edom_element);
    pcintr_dump_edom_node(stack, pcdom_interface_node(frame->edom_element));
}

static int
doc_init(pcintr_stack_t stack)
{
    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    if (!doc) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    const char *html = "<html/>";
    unsigned int r;
    r = pchtml_html_document_parse_with_buf(doc,
            (const unsigned char*)html, strlen(html));
    if (r) {
        pchtml_html_document_destroy(doc);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    stack->doc = doc;

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
loaded_vars_release(pcintr_stack_t stack)
{
    struct rb_root *root = &stack->loaded_vars;
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

#if 0 // VW
    if (stack->ops.on_cleanup) {
        stack->ops.on_cleanup(stack, stack->ctxt);
        stack->ops.on_cleanup = NULL;
        stack->ctxt = NULL;
    }
#endif
    pcintr_heap_t heap = stack->co->owner;
    if (heap->event_handler) {
        heap->event_handler(stack->co, PURC_EVENT_DESTROY,
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

    if (stack->timers) {
        pcintr_timers_destroy(stack->timers);
        stack->timers = NULL;
    }

    pcintr_destroy_observer_list(&stack->common_variant_observer_list);
    pcintr_destroy_observer_list(&stack->dynamic_variant_observer_list);
    pcintr_destroy_observer_list(&stack->native_variant_observer_list);

    if (stack->doc) {
        pchtml_html_document_destroy(stack->doc);
        stack->doc = NULL;
    }

    loaded_vars_release(stack);

    if (stack->base_uri) {
        free(stack->base_uri);
    }

    pcintr_exception_clear(&stack->exception);

#if 0 // VW
    if (stack->entry) {
        struct pcvdom_document *vdom_document;
        vdom_document = pcvdom_document_from_node(&stack->entry->node);
        PC_ASSERT(vdom_document);
        pcvdom_document_unref(vdom_document);
        stack->entry = NULL;
    }
#endif
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

        if (co->ident) {
            PC_ASSERT(co->full_name);
            purc_atom_remove_string(co->full_name);
            free(co->full_name);
            co->full_name = NULL;
        }
        if (co->mq) {
            pcinst_msg_queue_destroy(co->mq);
        }
        PURC_VARIANT_SAFE_CLEAR(co->val_from_return_or_exit);
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
    INIT_LIST_HEAD(&stack->frames);
    INIT_LIST_HEAD(&stack->common_variant_observer_list);
    INIT_LIST_HEAD(&stack->dynamic_variant_observer_list);
    INIT_LIST_HEAD(&stack->native_variant_observer_list);
    stack->scoped_variables = RB_ROOT;

    stack->stage = STACK_STAGE_FIRST_ROUND;
    stack->loaded_vars = RB_ROOT;
    stack->mode = STACK_VDOM_BEFORE_HVML;

    struct pcinst *inst = pcinst_current();
    PC_ASSERT(inst);
    struct pcintr_heap *heap = inst->intr_heap;
    PC_ASSERT(heap);
    stack->owning_heap = heap;
}

void pcintr_heap_lock(struct pcintr_heap *heap)
{
    int r = pthread_mutex_lock(&heap->locker);
    PC_ASSERT(r == 0);
}

void pcintr_heap_unlock(struct pcintr_heap *heap)
{
    int r = pthread_mutex_unlock(&heap->locker);
    PC_ASSERT(r == 0);
}

static struct list_head                       _all_heaps;

static void _cleanup_instance(struct pcinst* inst)
{
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap)
        return;

    if (heap->owning_heaps) {
        pcintr_remove_heap(&_all_heaps);
        PC_ASSERT(heap->owning_heaps == NULL);
    }

    PC_ASSERT(heap->exiting == false);
    heap->exiting = true;

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
        purc_inst_destroy_move_buffer();
        heap->move_buff = 0;
    }

    if (heap->event_timer) {
        pcintr_timer_destroy(heap->event_timer);
        heap->event_timer = NULL;
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
            PCINST_MOVE_BUFFER_BROADCAST, DEFAULT_MOVE_BUFFER_SIZE);
    if (!heap->move_buff) {
        free(heap);
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    int r;
    r = pthread_mutex_init(&heap->locker, NULL);
    if (r) {
        purc_inst_destroy_move_buffer();
        heap->move_buff = 0;
        free(heap);
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    inst->running_loop = purc_runloop_get_current();
    inst->intr_heap = heap;
    heap->owner     = inst;

    heap->coroutines = RB_ROOT;
    heap->running_coroutine = NULL;
    heap->next_coroutine_id = 1;

    heap->event_timer = pcintr_timer_create(NULL, false, true,
            NULL, event_timer_fire, inst);
    if (!heap->event_timer) {
        purc_inst_destroy_move_buffer();
        heap->move_buff = 0;
        free(heap);
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    pcintr_timer_set_interval(heap->event_timer, EVENT_TIMER_INTRVAL);
    pcintr_timer_start(heap->event_timer);

    PC_ASSERT(pcintr_get_heap());
    pcintr_add_heap(&_all_heaps);

    return 0;
}

static int _init_once(void)
{
    purc_runloop_t runloop = purc_runloop_get_current();
    PC_ASSERT(runloop);
    init_ops();

    INIT_LIST_HEAD(&_all_heaps);

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
        PC_ASSERT(heap->running_coroutine == NULL);
    }
    else {
#if 0           /* { */
        fprintf(stderr, "%s[%d]: %s(): %s\n",
            basename((char*)file), line, func,
            "<<<<<<<<<<<<<stop<<<<<<<<<<<<<<<<<<<<<<<<<<<");
#endif          /* } */
        PC_ASSERT(heap->running_coroutine);
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

    purc_variant_t at = pcdvobjs_make_elements(parent->edom_element);
    if (at == PURC_VARIANT_INVALID)
        return -1;

    int r;
    r = pcintr_set_at_var(frame, at);
    purc_variant_unref(at);

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
    PC_ASSERT(list_empty(&stack->frames));
    PC_ASSERT(stack->nr_frames == 0);

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

        child_frame->next_step = NEXT_STEP_AFTER_PUSHED;

        list_add_tail(&frame->node, &stack->frames);
        ++stack->nr_frames;

        return frame_pseudo;
    } while (0);

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
    PC_ASSERT(stack->nr_frames == 1);

    struct list_head *frames = &stack->frames;
    struct pcintr_stack_frame *frame;
    frame = list_last_entry(frames, struct pcintr_stack_frame, node);
    PC_ASSERT(frame);
    PC_ASSERT(frame->type == STACK_FRAME_TYPE_PSEUDO);

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
    const char *sv = "";

    purc_variant_t val = eval_vdom_attr(stack, attr);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    if (!purc_variant_is_undefined(val)) {
        PC_ASSERT(purc_variant_is_string(val));
        sv = purc_variant_get_string_const(val);
        PC_ASSERT(sv);
    }

    int r = pcintr_util_set_attribute(frame->edom_element, attr->key, sv);
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
    struct pcutils_map *attrs = element->attrs;
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

    int r = pcutils_map_traverse(attrs, &data, walk_attr);
    if (r)
        return r;

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

static void
dump_stack(pcintr_stack_t stack)
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
#endif                             /* } */

static void
dump_c_stack(struct pcdebug_backtrace *bt)
{
    if (!bt)
        return;

    struct pcinst *inst = pcinst_current();
    fprintf(stderr, "dumping stacks of purc instance [%p]......\n", inst);
    pcdebug_backtrace_dump(bt);
}

void
pcintr_check_insertion_mode_for_normal_element(pcintr_stack_t stack)
{
    PC_ASSERT(stack);

    if (stack->stage != STACK_STAGE_FIRST_ROUND)
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
    if (frame->ops.after_pushed) {
        void *ctxt = frame->ops.after_pushed(&co->stack, frame->pos);
        if (co->state == CO_STATE_WAIT) {
            PC_ASSERT(co->yielded_ctxt);
            PC_ASSERT(co->continuation);
        }
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

        pcdom_element_t *target;
        target = frame->edom_element;
        const char *s = purc_variant_get_string_const(content);
        pcdom_text_t *txt;
        txt = pcintr_util_append_content(target, s);
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

        struct pcintr_stack_frame *child_frame;
        child_frame = &frame_normal->frame;
        child_frame->ops = pcintr_get_ops_by_element(element);
        child_frame->pos = element;
        PC_ASSERT(element);
        child_frame->silently = pcintr_is_element_silently(child_frame->pos) ?
            1 : 0;
        child_frame->edom_element = frame->edom_element;
        child_frame->scope = NULL;

        child_frame->next_step = NEXT_STEP_AFTER_PUSHED;
    }
}

static void
exception_copy(struct pcintr_exception *exception)
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

static bool co_is_observed(pcintr_coroutine_t co)
{
    if (!list_empty(&co->stack.common_variant_observer_list))
        return true;

    if (!list_empty(&co->stack.dynamic_variant_observer_list))
        return true;

    if (!list_empty(&co->stack.native_variant_observer_list))
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

#define BUILDIN_VAR_HVML        "HVML"
#define BUILDIN_VAR_DATETIME    "DATETIME"
#define BUILDIN_VAR_T           "T"
#define BUILDIN_VAR_L           "L"
#define BUILDIN_VAR_DOC         "DOC"
#define BUILDIN_VAR_EJSON       "EJSON"
#define BUILDIN_VAR_STR         "STR"
#define BUILDIN_VAR_STREAM      "STREAM"

static bool
bind_cor_named_variable(pcintr_stack_t stack, const char* name,
        purc_variant_t var)
{
    if (var == PURC_VARIANT_INVALID) {
        return false;
    }

    if (!pcintr_bind_coroutine_variable(stack->co, name, var)) {
        purc_variant_unref(var);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }
    purc_variant_unref(var);
    return true;
}

static bool
bind_builtin_coroutine_variables(pcintr_stack_t stack)
{
    // $TIMERS
    stack->timers = pcintr_timers_init(stack);
    if (!stack->timers) {
        return false;
    }

    // $HVML
    if(!bind_cor_named_variable(stack, BUILDIN_VAR_HVML,
                purc_dvobj_hvml_new(&stack->co->hvml_ctrl_props))) {
        return false;
    }

    // $DATETIME
    if(!bind_cor_named_variable(stack, BUILDIN_VAR_DATETIME,
                purc_dvobj_datetime_new())) {
        return false;
    }

    // $T
    if(!bind_cor_named_variable(stack, BUILDIN_VAR_T,
                purc_dvobj_text_new())) {
        return false;
    }

    // $L
    if(!bind_cor_named_variable(stack, BUILDIN_VAR_L,
                purc_dvobj_logical_new())) {
        return false;
    }

    // FIXME: document-wide-variant???
    // $STR
    if(!bind_cor_named_variable(stack, BUILDIN_VAR_STR,
                purc_dvobj_string_new())) {
        return false;
    }

    // $STREAM
    if(!bind_cor_named_variable(stack, BUILDIN_VAR_STREAM,
                purc_dvobj_stream_new())) {
        return false;
    }


    // $DOC
    pchtml_html_document_t *doc = stack->doc;
    pcdom_document_t *document = (pcdom_document_t*)doc;
    if(!bind_cor_named_variable(stack, BUILDIN_VAR_DOC,
                purc_dvobj_doc_new(document))) {
        return false;
    }


    // $EJSON
    if(!bind_cor_named_variable(stack, BUILDIN_VAR_EJSON,
                purc_dvobj_ejson_new())) {
        return false;
    }
    // end

    return true;
}

int
pcintr_init_vdom_under_stack(pcintr_stack_t stack)
{
    PC_ASSERT(stack == pcintr_get_stack());

    stack->async_request_ids = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (!stack->async_request_ids) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    if (doc_init(stack)) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    if (!bind_builtin_coroutine_variables(stack))
        return -1;

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

static void on_sub_exit_event(void *ctxt)
{
    pcintr_coroutine_result_t child_result;
    child_result = (pcintr_coroutine_result_t)ctxt;
    PC_ASSERT(child_result);

    pcintr_coroutine_t co;
    co = pcintr_get_coroutine();
    PC_ASSERT(co);

    PRINT_VARIANT(child_result->as);
    PRINT_VARIANT(child_result->result);

    list_del(&child_result->node);
    PURC_VARIANT_SAFE_CLEAR(child_result->as);
    PURC_VARIANT_SAFE_CLEAR(child_result->result);
    free(child_result);
}

static void check_after_execution(pcintr_coroutine_t co);

static void on_sub_exit(void *ctxt)
{
    pcintr_coroutine_result_t child_result;
    child_result = (pcintr_coroutine_result_t)ctxt;
    PC_ASSERT(child_result);

    pcintr_coroutine_t co;
    co = pcintr_get_coroutine();
    PC_ASSERT(co);

    PRINT_VARIANT(child_result->result);

    pcintr_post_msg(ctxt, on_sub_exit_event);
    check_after_execution(co);
}

static void execute_one_step_for_exiting_co(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame == NULL);
    PC_ASSERT(stack->exited);

    PC_ASSERT(co->stack.except == 0);

    // CHECK pending requests

    PC_ASSERT(co->stack.back_anchor == NULL);

#if 0 // VW
    if (co->stack.ops.on_terminated) {
        co->stack.ops.on_terminated(&co->stack, co->stack.ctxt);
        co->stack.ops.on_terminated = NULL;
    }
    if (co->stack.ops.on_cleanup) {
        co->stack.ops.on_cleanup(&co->stack, co->stack.ctxt);
        co->stack.ops.on_cleanup = NULL;
        co->stack.ctxt = NULL;
    }
#endif

    pcintr_heap_t heap = co->owner;
    struct pcinst *inst = heap->owner;

    if (heap->event_handler) {
        heap->event_handler(co, PURC_EVENT_EXIT, stack->doc);
    }

    if (co->curator) {
        pcintr_coroutine_t parent = pcintr_coroutine_get_by_id(co->curator);
        PC_ASSERT(parent->owner == co->owner);
        co->curator = 0;
        pcintr_coroutine_result_t co_result;
        co_result = co->result;
        co->result = NULL;
        PC_ASSERT(parent);
        PC_ASSERT(co_result);
        pcintr_wakeup_target_with(parent, co_result, on_sub_exit);
    }

    pcutils_rbtree_erase(&co->node, &heap->coroutines);
    coroutine_destroy(co);

    if (inst->keep_alive == 0 &&
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
    check_after_execution(co);
}

static void run_exiting_co(void *ctxt)
{
    pcintr_coroutine_t co = (pcintr_coroutine_t)ctxt;
    PC_ASSERT(co);
    switch (co->state) {
        case CO_STATE_READY:
            co->state = CO_STATE_RUN;
            coroutine_set_current(co);
            execute_one_step_for_exiting_co(co);
            coroutine_set_current(NULL);
            break;
        case CO_STATE_RUN:
            PC_ASSERT(0);
            break;
        case CO_STATE_WAIT:
            PC_ASSERT(0);
            break;
        default:
            PC_ASSERT(0);
    }
}

static void run_ready_co(void);

static void
revoke_all_dynamic_observers(void)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    struct list_head *observers = &stack->dynamic_variant_observer_list;
    struct pcintr_observer *p, *n;
    list_for_each_entry_safe(p, n, observers, node) {
        pcintr_revoke_observer(p);
    }
}

static void
revoke_all_native_observers(void)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    struct list_head *observers = &stack->native_variant_observer_list;
    struct pcintr_observer *p, *n;
    list_for_each_entry_safe(p, n, observers, node) {
        pcintr_revoke_observer(p);
    }
}

static void
revoke_all_common_observers(void)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    struct list_head *observers = &stack->common_variant_observer_list;
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
        case CO_STATE_RUN:
            purc_set_error_with_info(PURC_ERROR_NOT_READY,
                    "coroutine context is not READY but RUN");
            return false;
        case CO_STATE_WAIT:
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

static void notify_to_stop(pcintr_coroutine_t co)
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

static void on_msg(void *ctxt)
{
    pcintr_msg_t msg;
    msg = (pcintr_msg_t)ctxt;

    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    pcintr_coroutine_t co = stack->co;
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_READY);
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame == NULL);

    co->state = CO_STATE_RUN;

    PC_ASSERT(co->msg_pending);
    co->msg_pending = 0;
    msg->on_msg(msg->ctxt);
    free(msg);
    check_after_execution(co);
}

static struct pcintr_msg            last_msg;

static void on_last_msg(void *ctxt)
{
    PC_ASSERT(ctxt == &last_msg);
    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);
    PC_ASSERT(co->stack.exited);
    PC_ASSERT(co->stack.last_msg_sent);
    PC_ASSERT(co->stack.last_msg_read == 0);
    co->stack.last_msg_read = 1;
    PC_ASSERT(co->state == CO_STATE_READY);
    co->state = CO_STATE_RUN;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(&co->stack);
    PC_ASSERT(frame == NULL);
    check_after_execution(co);
}

static void
post_callstate_success_event(pcintr_coroutine_t co, purc_variant_t with)
{
    if (!co->curator)
        return;

    pcintr_coroutine_t target = pcintr_coroutine_get_by_id(co->curator);

    PC_ASSERT(co->result);
    PC_ASSERT(co->owner && target->owner);
    PURC_VARIANT_SAFE_CLEAR(co->result->result);
    co->result->result = purc_variant_ref(with);

    purc_atom_t msg_type;
    msg_type = pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, CALLSTATE));

    purc_variant_t msg_sub_type;
    msg_sub_type = purc_variant_make_string_static("success", false);
    PC_ASSERT(msg_sub_type);

    purc_variant_t src;
    src = purc_variant_make_undefined();
    PC_ASSERT(src);

    purc_variant_t payload = purc_variant_ref(with);
    PC_ASSERT(payload);

    pcintr_fire_event_to_target(target, msg_type, msg_sub_type, src, payload);

    PURC_VARIANT_SAFE_CLEAR(msg_sub_type);
    PURC_VARIANT_SAFE_CLEAR(src);
    PURC_VARIANT_SAFE_CLEAR(payload);
}

static void
post_callstate_except_event(pcintr_coroutine_t co, const char *error_except)
{
    if (!co->curator)
        return;

    pcintr_coroutine_t target = pcintr_coroutine_get_by_id(co->curator);

    purc_atom_t msg_type;
    msg_type = pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, CALLSTATE));

    purc_variant_t msg_sub_type;
    msg_sub_type = purc_variant_make_string_static("except", false);
    PC_ASSERT(msg_sub_type);

    purc_variant_t src;
    src = purc_variant_make_undefined();
    PC_ASSERT(src);

    purc_variant_t payload = purc_variant_make_string(error_except, false);
    PC_ASSERT(payload);

    pcintr_fire_event_to_target(target, msg_type, msg_sub_type, src, payload);

    PURC_VARIANT_SAFE_CLEAR(msg_sub_type);
    PURC_VARIANT_SAFE_CLEAR(src);
    PURC_VARIANT_SAFE_CLEAR(payload);
}

static void check_after_execution(pcintr_coroutine_t co)
{
    struct pcinst *inst = pcinst_current();
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    switch (co->state) {
        case CO_STATE_READY:
            break;
        case CO_STATE_RUN:
            co->state = CO_STATE_READY;
            break;
        case CO_STATE_WAIT:
            PC_ASSERT(frame && frame->type == STACK_FRAME_TYPE_NORMAL);
            PC_ASSERT(inst->errcode == 0);
            PC_ASSERT(co->yielded_ctxt);
            PC_ASSERT(co->continuation);
            return;
        default:
            PC_ASSERT(0);
    }

    if (inst->errcode) {
        PC_ASSERT(stack->except == 0);
        exception_copy(&stack->exception);
        stack->except = 1;
        pcinst_clear_error(inst);
        PC_ASSERT(inst->errcode == 0);
#ifndef NDEBUG                     /* { */
        dump_stack(stack);
#endif                             /* } */
        PC_ASSERT(inst->errcode == 0);
    }

    if (frame) {
        if (co->execution_pending == 0) {
            co->execution_pending = 1;
            pcintr_wakeup_target(co, run_ready_co);
        }
        return;
    }

    PC_ASSERT(co->yielded_ctxt == NULL);
    PC_ASSERT(co->continuation == NULL);

    /* send doc to rdr */
    if (stack->stage == STACK_STAGE_FIRST_ROUND &&
            !pcintr_rdr_page_control_load(stack))
    {
        PC_ASSERT(0); // TODO:
        // stack->exited = 1;
        return;
    }

    pcintr_dump_document(stack);
    stack->stage = STACK_STAGE_EVENT_LOOP;


    if (co->stack.except) {
        const char *error_except = NULL;
        purc_atom_t atom;
        atom = co->stack.exception.error_except;
        PC_ASSERT(atom);
        error_except = purc_atom_to_string(atom);

        PC_ASSERT(co->error_except == NULL);
        co->error_except = error_except;

        dump_c_stack(co->stack.exception.bt);
        co->stack.except = 0;

        if (!co->stack.exited) {
            co->stack.exited = 1;
            notify_to_stop(co);
        }
    }

    if (!list_empty(&co->msgs) && co->msg_pending == 0) {
        PC_ASSERT(co->state == CO_STATE_READY);
        struct pcintr_stack_frame *frame;
        frame = pcintr_stack_get_bottom_frame(stack);
        PC_ASSERT(frame == NULL);
        pcintr_msg_t msg;
        msg = list_first_entry(&co->msgs, struct pcintr_msg, node);
        list_del(&msg->node);
        co->msg_pending = 1;
        pcintr_wakeup_target_with(co, msg, on_msg);
        return;
    }

    if (!list_empty(&co->children)) {
        return;
    }

    if (co->stack.exited) {
        revoke_all_dynamic_observers();
        PC_ASSERT(list_empty(&co->stack.dynamic_variant_observer_list));
        revoke_all_native_observers();
        PC_ASSERT(list_empty(&co->stack.native_variant_observer_list));
        revoke_all_common_observers();
        PC_ASSERT(list_empty(&co->stack.common_variant_observer_list));
    }

    bool still_observed = co_is_observed(co);
    if (!still_observed) {
        if (!co->stack.exited) {
            co->stack.exited = 1;
            notify_to_stop(co);
        }
    }

    if (!list_empty(&co->msgs) && co->msg_pending == 0) {
        PC_ASSERT(co->state == CO_STATE_READY);
        struct pcintr_stack_frame *frame;
        frame = pcintr_stack_get_bottom_frame(stack);
        PC_ASSERT(frame == NULL);
        pcintr_msg_t msg;
        msg = list_first_entry(&co->msgs, struct pcintr_msg, node);
        list_del(&msg->node);
        co->msg_pending = 1;
        pcintr_wakeup_target_with(co, msg, on_msg);
        return;
    }

    if (still_observed) {
        return;
    }

    if (!co->stack.exited) {
        co->stack.exited = 1;
        notify_to_stop(co);
    }

    if (!list_empty(&co->msgs)) {
        return;
    }

    if (co->msg_pending) {
        return;
    }

// #define PRINT_DEBUG
    if (co->stack.last_msg_sent == 0) {
        co->stack.last_msg_sent = 1;

#ifdef PRINT_DEBUG              /* { */
        PC_DEBUGX("last msg was sent");
#endif                          /* } */
        pcintr_wakeup_target_with(co, &last_msg, on_last_msg);
        return;
    }

    if (co->stack.last_msg_read == 0) {
        return;
    }


#ifdef PRINT_DEBUG              /* { */
    PC_DEBUGX("last msg was processed");
#endif                          /* } */

    if (co->curator) {
        if (co->error_except) {
            // TODO: which is error, which is except?
            // currently, we treat all as except
            post_callstate_except_event(co, co->error_except);
        }
        else {
            PC_ASSERT(co->val_from_return_or_exit);
            post_callstate_success_event(co, co->val_from_return_or_exit);
        }
    }

    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_READY);
    purc_runloop_dispatch(inst->running_loop, run_exiting_co, co);
}

void pcintr_set_exit(purc_variant_t val)
{
    PC_ASSERT(val != PURC_VARIANT_INVALID);

    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);

    PURC_VARIANT_SAFE_CLEAR(co->val_from_return_or_exit);
    co->val_from_return_or_exit = purc_variant_ref(val);

    if (co->stack.exited == 0) {
        co->stack.exited = 1;
        notify_to_stop(co);
    }
}

static void run_ready_co(void)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    pcintr_coroutine_t co = stack->co;
    PC_ASSERT(co);
    PC_ASSERT(co->execution_pending == 1);
    co->execution_pending = 0;

    switch (co->state) {
        case CO_STATE_READY:
            co->state = CO_STATE_RUN;
            pcintr_execute_one_step_for_ready_co(co);
            check_after_execution(co);
            break;
        case CO_STATE_RUN:
            PC_ASSERT(0);
            break;
        case CO_STATE_WAIT:
            PC_ASSERT(0);
            break;
        default:
            PC_ASSERT(0);
    }
}

static void execute_main_for_ready_co(pcintr_coroutine_t co)
{
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_RUN);

    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame == NULL);

    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    struct pcintr_stack_frame_normal *frame_normal;
    frame_normal = pcintr_push_stack_frame_normal(stack);
    if (!frame_normal)
        return;

    frame = &frame_normal->frame;

    frame->ops = *pcintr_get_document_ops();
}

static void run_co_main(void)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    pcintr_coroutine_t co = stack->co;
    PC_ASSERT(co);

    switch (co->state) {
        case CO_STATE_READY:
            co->state = CO_STATE_RUN;
            execute_main_for_ready_co(co);
            check_after_execution(co);
            break;
        case CO_STATE_RUN:
            PC_ASSERT(0);
            break;
        case CO_STATE_WAIT:
            PC_ASSERT(0);
            break;
        default:
            PC_ASSERT(0);
    }
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
    coroutine->full_name = p;

    coroutine->ident = purc_atom_from_string_ex(PURC_ATOM_BUCKET_USER,
            coroutine->full_name);

    return 0;
}

static int
cmp_by_atom(struct rb_node *node, void *ud)
{
    purc_atom_t *atom = (purc_atom_t*)ud;
    pcintr_coroutine_t co;
    co = container_of(node, struct pcintr_coroutine, node);
    return (*atom) - co->ident;
}

static pcintr_coroutine_t
coroutine_create(purc_vdom_t vdom, pcintr_coroutine_t parent,
        purc_variant_t as, void *user_data)
{
    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = inst->intr_heap;
    struct rb_root *coroutines = &heap->coroutines;

    pcintr_coroutine_t co = NULL;
    pcintr_stack_t stack = NULL;

    pcintr_coroutine_result_t co_result = NULL;
    if (parent) {
        co_result = (pcintr_coroutine_result_t)calloc(1, sizeof(*co_result));
        if (!co_result) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }
    }

    co = (pcintr_coroutine_t)calloc(1, sizeof(*co));
    if (!co) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fail_co;
    }
    co->val_from_return_or_exit = purc_variant_make_undefined();
    PC_ASSERT(co->val_from_return_or_exit != PURC_VARIANT_INVALID);

    if (set_coroutine_id(co)) {
        goto fail_name;
    }

    co->vdom = vdom;
    co->state = CO_STATE_READY;
    INIT_LIST_HEAD(&co->children);
    INIT_LIST_HEAD(&co->registered_cancels);
    INIT_LIST_HEAD(&co->msgs);
    co->msg_pending = 0;

    co->mq = pcinst_msg_queue_create();
    if (!co->mq) {
        goto fail_name;
    }

    if (parent) {
        co->curator = parent->ident;
        if (as != PURC_VARIANT_INVALID)
            co_result->as = purc_variant_ref(as);
        list_add_tail(&co_result->node, &parent->children);
        co->result = co_result;
        co_result = NULL;
    }

    stack = &co->stack;
    stack->co = co;
    co->owner = heap;
    co->user_data = user_data;

    int r;
    r = pcutils_rbtree_insert_only(coroutines, &co->ident,
            cmp_by_atom, &co->node);
    PC_ASSERT(r == 0);

    stack_init(stack);

#if 0 // VW
    if (ops) {
        stack->ops  = *ops;
        stack->ctxt = ctxt;
    }
#endif

    stack->vdom = vdom;
    return co;

fail_name:
    free(co);

fail_co:
    free(co_result);

    return NULL;
}

purc_coroutine_t
purc_schedule_vdom(purc_vdom_t vdom,
        purc_atom_t curator, purc_variant_t request,
        pcrdr_page_type page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_info, const char *body_id,
        void *user_data)
{
    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co == NULL);

    /* TODO: check curator here */
    UNUSED_PARAM(curator);

    co = coroutine_create(vdom, NULL, NULL, user_data);
    if (!co) {
        pcvdom_document_unref(vdom);
        purc_log_error("Failed to create coroutine\n");
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    PC_ASSERT(co->stack.vdom);

    if (page_type != PCRDR_PAGE_TYPE_NULL &&
            !pcintr_attach_to_renderer(co,
                page_type, target_workspace,
                target_group, page_name, extra_info)) {
        purc_log_warn("Failed to attach to renderer\n");
    }

    /* TODO: handle entry here */
    UNUSED_PARAM(body_id);
    UNUSED_PARAM(request);

    pcintr_wakeup_target(co, run_co_main);
    return co;
}

purc_event_handler
purc_get_event_handler(void)
{
    struct pcinst *inst = pcinst_current();
    if (inst) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return (purc_event_handler)PURC_INVPTR;
    }

    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return (purc_event_handler)PURC_INVPTR;
    }

    return heap->event_handler;
}

purc_event_handler
purc_set_event_handler(purc_event_handler handler)
{
    struct pcinst *inst = pcinst_current();
    if (inst) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return (purc_event_handler)PURC_INVPTR;
    }

    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return (purc_event_handler)PURC_INVPTR;
    }

    purc_event_handler old = heap->event_handler;
    heap->event_handler = handler;
    return old;
}

int
purc_run(purc_event_handler handler)
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

    heap->event_handler = handler;
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

    if (stack->co->hvml_ctrl_props->base_url_string) {
        pcfetcher_set_base_url(stack->co->hvml_ctrl_props->base_url_string);
    }
    purc_variant_t ret = PURC_VARIANT_INVALID;
    struct pcfetcher_resp_header resp_header = {0};
    uint32_t timeout = stack->co->hvml_ctrl_props->timeout.tv_sec;
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
    PC_ASSERT(request_id != PURC_VARIANT_INVALID);
    PC_ASSERT(ctxt);
    struct load_async_data *data;
    data = (struct load_async_data*)ctxt;
    PC_ASSERT(data);
    PC_ASSERT(data->request_id == request_id);
    PC_ASSERT(data->handler);
    PC_ASSERT(data->requesting_thread == pthread_self());

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

    if (stack->co->hvml_ctrl_props->base_url_string) {
        pcfetcher_set_base_url(stack->co->hvml_ctrl_props->base_url_string);
    }

    uint32_t timeout = stack->co->hvml_ctrl_props->timeout.tv_sec;
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

    if (stack->co->hvml_ctrl_props->base_url_string) {
        pcfetcher_set_base_url(stack->co->hvml_ctrl_props->base_url_string);
    }
    uint32_t timeout = stack->co->hvml_ctrl_props->timeout.tv_sec;
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

    purc_variant_t doc = pcintr_get_coroutine_variable(cor, BUILDIN_VAR_DOC);
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
pcintr_load_dynamic_variant(pcintr_stack_t stack,
    const char *name, size_t len)
{
    char NAME[PATH_MAX+1];
    snprintf(NAME, sizeof(NAME), "%.*s", (int)len, name);

    struct rb_root *root = &stack->loaded_vars;

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

    if (pcintr_bind_coroutine_variable(stack->co, NAME, v)) {
        return true;
    }

error:
    destroy_loaded_var(p);

    return false;
}

pcdom_element_t*
pcintr_util_append_element(pcdom_element_t* parent, const char *tag)
{
    pcdom_node_t *node = pcdom_interface_node(parent);
    pcdom_document_t *dom_doc = node->owner_document;
    pcdom_element_t *elem;
    elem = pcdom_document_create_element(dom_doc,
            (const unsigned char*)tag, strlen(tag), NULL);
    if (!elem)
        return NULL;

    pcdom_node_append_child(node, pcdom_interface_node(elem));

    return elem;
}

static pcdom_text_t*
pcintr_util_append_content_inner(pcdom_element_t* parent, const char *txt)
{
    pcdom_document_t *doc = pcdom_interface_node(parent)->owner_document;
    const unsigned char *content = (const unsigned char*)txt;
    size_t content_len = strlen(txt);

    pcdom_text_t *text_node;
    text_node = pcdom_document_create_text_node(doc, content, content_len);
    if (text_node == NULL)
        return NULL;

    pcdom_node_append_child(pcdom_interface_node(parent),
            pcdom_interface_node(text_node));

    return text_node;
}

pcdom_text_t*
pcintr_util_append_content(pcdom_element_t* parent, const char *txt)
{
    pcdom_text_t* text_node = pcintr_util_append_content_inner(parent, txt);
    if (text_node == NULL) {
        return NULL;
    }

    pcintr_rdr_dom_append_content(pcintr_get_stack(), parent, txt);
    return text_node;
}

pcdom_text_t*
pcintr_util_displace_content(pcdom_element_t* parent, const char *txt)
{
    pcdom_node_t *parent_node = pcdom_interface_node(parent);
    while (parent_node->first_child)
        pcdom_node_destroy_deep(parent_node->first_child);

    pcdom_text_t* text_node = pcintr_util_append_content_inner(parent, txt);
    if (text_node == NULL) {
        return NULL;
    }

    pcintr_rdr_dom_displace_content(pcintr_get_stack(), parent, txt);
    return text_node;
}

int
pcintr_util_set_attribute(pcdom_element_t *elem,
        const char *key, const char *val)
{
    pcdom_attr_t *attr;
    attr = pcdom_element_set_attribute(elem,
            (const unsigned char*)key, strlen(key),
            (const unsigned char*)val, strlen(val));
    if (!attr) {
        return -1;
    }
    if (!val) {
        pcintr_rdr_dom_erase_element_property(pcintr_get_stack(), elem, key);
    }
    else {
        pcintr_rdr_dom_update_element_property(pcintr_get_stack(), elem, key,
                val);
    }
    return 0;
}

int
pcintr_util_remove_attribute(pcdom_element_t *elem, const char *key)
{
    unsigned int ret = pcdom_element_remove_attribute(elem,
            (const unsigned char *)key, strlen(key));

    //TODO: send to rdr
    return ret;
}

pchtml_html_document_t*
pcintr_util_load_document(const char *html)
{
    pchtml_html_document_t *doc;
    doc = pchtml_html_document_create();
    if (!doc)
        return NULL;

    unsigned int r;
    r = pchtml_html_document_parse_with_buf(doc,
            (const unsigned char*)html, strlen(html));
    if (r) {
        pchtml_html_document_destroy(doc);
        return NULL;
    }

    return doc;
}

int
pcintr_util_comp_docs(pchtml_html_document_t *docl,
    pchtml_html_document_t *docr, int *diff)
{
    char lbuf[1024], rbuf[1024];
    size_t lsz = sizeof(lbuf), rsz = sizeof(rbuf);
    char *pl = pchtml_doc_snprintf_plain(docl, lbuf, &lsz, "");
    char *pr = pchtml_doc_snprintf_plain(docr, rbuf, &rsz, "");
    int err = -1;
    if (pl && pr) {
        *diff = strcmp(pl, pr);
        if (*diff) {
            PC_DEBUGX("diff:\n%s\n%s", pl, pr);
        }
        err = 0;
    }

    if (pl != lbuf)
        free(pl);
    if (pr != rbuf)
        free(pr);

    return err;
}

bool
pcintr_util_is_ancestor(pcdom_node_t *ancestor, pcdom_node_t *descendant)
{
    pcdom_node_t *node = descendant;
    do {
        if (node->parent && node->parent == ancestor)
            return true;
        node = node->parent;
    } while (node);

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
cleaner(void* native_entity, bool silently)
{
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)native_entity;
    PC_ASSERT(tpl);
    template_cleaner(tpl);

    UNUSED_PARAM(silently);
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

    PC_ASSERT(0 == check_template_variant(v));
    return v;
}

int
pcintr_template_set(purc_variant_t val, struct pcvcm_node *vcm,
        bool to_free)
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


int
pcintr_util_add_child_chunk(pcdom_element_t *parent, const char *chunk)
{
    int r = -1;

    size_t nr = strlen(chunk);

    pcdom_node_t *root = NULL;
    do {
        pchtml_html_document_t *doc;
        doc = pchtml_html_interface_document(
                pcdom_interface_node(parent)->owner_document);
        unsigned int ui;
        ui = pchtml_html_document_parse_fragment_chunk_begin(doc, parent);
        if (ui == 0) {
            do {
                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)"<div>", 5);
                if (ui)
                    break;

                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)chunk, nr);
                if (ui)
                    break;

                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)"</div>", 6);
            } while (0);
        }
        pcdom_node_t *div;
        root = pchtml_html_document_parse_fragment_chunk_end(doc);
        if (root) {
            PC_ASSERT(root->first_child == root->last_child);
            PC_ASSERT(root->first_child);
            PC_ASSERT(root->first_child->type == PCDOM_NODE_TYPE_ELEMENT);
            div = root->first_child;
        }
        if (ui)
            break;

        while (div->first_child) {
            pcdom_node_t *child = div->first_child;
            pcdom_node_remove(child);
            pcdom_node_append_child(pcdom_interface_node(parent), child);
            pcintr_rdr_dom_append_child(pcintr_get_stack(), parent, child);
        }
        r = 0;
    } while (0);

    if (root)
        pcdom_node_destroy(pcdom_interface_node(root));

    return r ? -1 : 0;
}

int
pcintr_util_add_child(pcdom_element_t *parent, const char *fmt, ...)
{
    char buf[1024];
    size_t nr = sizeof(buf);
    char *p;
    va_list ap;
    va_start(ap, fmt);
    p = pcutils_vsnprintf(buf, &nr, fmt, ap);
    va_end(ap);

    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    int r = pcintr_util_add_child_chunk(parent, p);

    if (p != buf)
        free(p);

    return r ? -1 : 0;
}

int
pcintr_util_set_child_chunk(pcdom_element_t *parent, const char *chunk)
{
    int r = -1;

    size_t nr = strlen(chunk);

    pcdom_node_t *root = NULL;
    do {
        pchtml_html_document_t *doc;
        doc = pchtml_html_interface_document(
                pcdom_interface_node(parent)->owner_document);
        unsigned int ui;
        ui = pchtml_html_document_parse_fragment_chunk_begin(doc, parent);
        if (ui == 0) {
            do {
                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)"<div>", 5);
                if (ui)
                    break;

                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)chunk, nr);
                if (ui)
                    break;

                ui = pchtml_html_document_parse_fragment_chunk(doc,
                        (const unsigned char*)"</div>", 6);
            } while (0);
        }
        pcdom_node_t *div;
        root = pchtml_html_document_parse_fragment_chunk_end(doc);
        if (root) {
            PC_ASSERT(root->first_child == root->last_child);
            PC_ASSERT(root->first_child);
            PC_ASSERT(root->first_child->type == PCDOM_NODE_TYPE_ELEMENT);
            div = root->first_child;
        }
        if (ui)
            break;

        pcdom_node_remove(div);
        while (pcdom_interface_node(parent)->first_child)
            pcdom_node_destroy_deep(pcdom_interface_node(parent)->first_child);

        while (div->first_child) {
            pcdom_node_t *child = div->first_child;
            pcdom_node_remove(child);
            pcdom_node_append_child(pcdom_interface_node(parent), child);
            pcintr_rdr_dom_displace_child(pcintr_get_stack(), parent, child);
        }
        r = 0;
    } while (0);

    if (root)
        pcdom_node_destroy(pcdom_interface_node(root));

    return r ? -1 : 0;
}

int
pcintr_util_set_child(pcdom_element_t *parent, const char *fmt, ...)
{
    char buf[1024];
    size_t nr = sizeof(buf);
    char *p;
    va_list ap;
    va_start(ap, fmt);
    p = pcutils_vsnprintf(buf, &nr, fmt, ap);
    va_end(ap);

    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    int r = pcintr_util_set_child_chunk(parent, p);

    if (p != buf)
        free(p);

    return r ? -1 : 0;
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
    purc_variant_t at = pcdvobjs_make_elements(frame->edom_element);
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
    purc_variant_t source_uri = purc_variant_make_string(
            stack->co->full_name, false);
    pcintr_post_event_by_ctype(stack->co->ident,
            PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY, source_uri,
            var, MSG_TYPE_CHANGE, NULL,
            PURC_VARIANT_INVALID);
    purc_variant_unref(source_uri);
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

void
pcintr_apply_routine(co_routine_f routine, pcintr_coroutine_t target)
{
    struct pcintr_heap *heap = pcintr_get_heap();
    PC_ASSERT(heap);
    struct pcinst *inst = heap->owner;
    PC_ASSERT(inst);
    purc_runloop_t runloop = purc_runloop_get_current();
    PC_ASSERT(runloop);
    PC_ASSERT(inst->running_loop == runloop);
    PC_ASSERT(heap->running_coroutine == NULL);

    coroutine_set_current(target);
    routine();
    coroutine_set_current(NULL);
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

    PC_ASSERT(pcintr_get_heap());

    struct pcinst *inst = (struct pcinst *)data;
    pcintr_dispatch_msg();

    if (inst != NULL && inst->rdr_caps != NULL) {
        pcrdr_wait_and_dispatch_message(inst->conn_to_rdr, 1);
        purc_clr_error();
    }

    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co == NULL) {
        return;
    }
    PC_ASSERT(co->state == CO_STATE_RUN);

    pcintr_stack_t stack = &co->stack;
    if (stack->exited)
        return;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame == NULL);

    struct list_head *observer_list = &stack->native_variant_observer_list;
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

void
pcintr_post_msg(void *ctxt, pcintr_msg_callback_f cb)
{
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    pcintr_coroutine_t co = stack->co;
    PC_ASSERT(co);
    if (1) {
        pcintr_post_msg_to_target(co, ctxt, cb);
        return;
    }

    pcintr_msg_t msg;
    msg = (pcintr_msg_t)calloc(1, sizeof(*msg));
    PC_ASSERT(msg);

    msg->ctxt        = ctxt;
    msg->on_msg      = cb;

    list_add_tail(&msg->node, &co->msgs);
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

void pcintr_yield(void *ctxt, void (*continuation)(void *ctxt, void *extra))
{
    PC_ASSERT(ctxt);
    PC_ASSERT(continuation);
    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_RUN);
    PC_ASSERT(co->yielded_ctxt == NULL);
    PC_ASSERT(co->continuation == NULL);
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    co->state = CO_STATE_WAIT;
    co->yielded_ctxt = ctxt;
    co->continuation = continuation;
}

void pcintr_resume(void *extra)
{
    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_WAIT);
    PC_ASSERT(co->yielded_ctxt);
    PC_ASSERT(co->continuation);
    pcintr_stack_t stack = &co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    void *ctxt = co->yielded_ctxt;
    void (*continuation)(void *ctxt, void *extra) = co->continuation;

    co->state = CO_STATE_RUN;
    co->yielded_ctxt = NULL;
    co->continuation = NULL;
    continuation(ctxt, extra);
    check_after_execution(co);
}

pcintr_coroutine_t
pcintr_create_child_co(pcvdom_element_t vdom_element,
        purc_variant_t as, purc_variant_t within)
{
    UNUSED_PARAM(within);

    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);

    PC_ASSERT(vdom_element);

    pcintr_coroutine_t child;
    child = coroutine_create(co->vdom, co, as, NULL);
    do {
        if (!child)
            break;

        PC_ASSERT(co->stack.vdom);

        child->stack.entry = vdom_element;
        // VW: we reuse the vDOM, so must increase refcount.
        pcvdom_document_ref(co->vdom);

        purc_log_debug("running parent/child: %p/%p", co, child);
        PRINT_VDOM_NODE(&vdom_element->node);
        pcintr_wakeup_target(child, run_co_main);
    } while (0);

    return child;
}

pcintr_coroutine_t
pcintr_load_child_co(const char *hvml,
        purc_variant_t as, purc_variant_t within)
{
    purc_vdom_t vdom;

    vdom = purc_load_hvml_from_string(hvml);
    if (vdom == NULL)
        return NULL;

    UNUSED_PARAM(within);

    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);

    pcintr_coroutine_t child;
    child = coroutine_create(vdom, co, as, NULL);
    do {
        if (!child)
            break;

        PC_ASSERT(co->stack.vdom);

        PC_DEBUGX("running parent/child: %p/%p", co, child);
        pcintr_wakeup_target(child, run_co_main);
    } while (0);

    return child;
}

void
pcintr_on_event(purc_atom_t msg_type, purc_variant_t msg_sub_type,
        purc_variant_t src, purc_variant_t payload)
{
    PC_ASSERT(0);
    UNUSED_PARAM(msg_sub_type);
    UNUSED_PARAM(src);
    UNUSED_PARAM(payload);

    if (msg_type == pchvml_keyword(PCHVML_KEYWORD_ENUM(MSG, CALLSTATE))) {
        if (0)
            PC_ASSERT(0);
    }
    else {
        if (0)
            PC_ASSERT(0);
    }
}

void*
pcintr_load_module(const char *module,
        const char *env_name, const char *prefix)
{
#define PRINT_DEBUG
#if OS(LINUX) || OS(UNIX) || OS(MAC_OS_X)             /* { */
    const char *ext = ".so";
#if OS(MAC_OS_X)
    ext = ".dylib";
#endif

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

        PC_ASSERT(0 == check_template_variant(v));

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

