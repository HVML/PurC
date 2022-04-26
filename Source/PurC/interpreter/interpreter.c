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

#include "internal.h"

#include "private/debug.h"
#include "private/instance.h"
#include "purc-runloop.h"
#include "private/dvobjs.h"
#include "private/fetcher.h"
#include "private/regex.h"

#include "ops.h"
#include "../hvml/hvml-gen.h"
#include "../html/parser.h"

#include "hvml-attr.h"

#include <stdarg.h>

#define EVENT_SEPARATOR      ":"
#define EVENT_TIMER_INTRVAL  10

#define MSG_TYPE_CHANGE     "change"

void pcintr_stack_init_once(void)
{
    purc_runloop_t runloop = purc_runloop_get_current();
    PC_ASSERT(runloop);
    init_ops();
}

void pcintr_stack_init_instance(struct pcinst* inst)
{
    struct pcintr_heap *heap = inst->intr_heap;
    PC_ASSERT(heap == NULL);

    heap = (struct pcintr_heap*)calloc(1, sizeof(*heap));
    if (!heap)
        return;

    inst->intr_heap = heap;

    INIT_LIST_HEAD(&heap->coroutines);
    heap->running_coroutine = NULL;
}

static void
stack_frame_release(struct pcintr_stack_frame *frame)
{
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
}

static void
vdom_release(purc_vdom_t vdom)
{
    if (vdom->document) {
        pcvdom_document_destroy(vdom->document);
        vdom->document = NULL;
    }
}

static void
vdom_destroy(purc_vdom_t vdom)
{
    if (!vdom)
        return;

    vdom_release(vdom);

    free(vdom);
}

void
pcintr_util_dump_document_ex(pchtml_html_document_t *doc,
    const char *file, int line, const char *func)
{
    PC_ASSERT(doc);

    char buf[1024];
    size_t nr = sizeof(buf);
    int opt = 0;
    opt |= PCHTML_HTML_SERIALIZE_OPT_UNDEF;
    opt |= PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE;
    opt |= PCHTML_HTML_SERIALIZE_OPT_WITH_HVML_HANDLE;
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

    fprintf(stderr, "%s[%d]:%s(): #document %p\n%s\n",
            pcutils_basename((char*)file), line, func, doc, p);
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
}

static void
stack_release(pcintr_stack_t stack)
{
    if (stack->ops.on_cleanup) {
        stack->ops.on_cleanup(stack, stack->ctxt);
        stack->ops.on_cleanup = NULL;
        stack->ctxt = NULL;
    }

    struct list_head *frames = &stack->frames;
    if (!list_empty(frames)) {
        struct pcintr_stack_frame *p, *n;
        list_for_each_entry_reverse_safe(p, n, frames, node) {
            list_del(&p->node);
            --stack->nr_frames;
            stack_frame_release(p);
            free(p);
        }
        PC_ASSERT(stack->nr_frames == 0);
    }

    if (stack->vdom) {
        if (stack->vdom->timers) {
            pcintr_timers_destroy(stack->vdom->timers);
        }
        vdom_destroy(stack->vdom);
        stack->vdom = NULL;
    }

    if (stack->common_variant_observer_list) {
        pcutils_arrlist_free(stack->common_variant_observer_list);
        stack->common_variant_observer_list = NULL;
    }

    if (stack->dynamic_variant_observer_list) {
        pcutils_arrlist_free(stack->dynamic_variant_observer_list);
        stack->dynamic_variant_observer_list = NULL;
    }

    if (stack->native_variant_observer_list) {
        pcutils_arrlist_free(stack->native_variant_observer_list);
        stack->native_variant_observer_list = NULL;
    }

    if (stack->doc) {
        pchtml_html_document_destroy(stack->doc);
        stack->doc = NULL;
    }

    loaded_vars_release(stack);

    if (stack->base_uri) {
        free(stack->base_uri);
    }

    pcintr_exception_clear(&stack->exception);

    if (stack->event_timer) {
        pcintr_timer_destroy(stack->event_timer);
        stack->event_timer = NULL;
    }
}

static void
stack_destroy(pcintr_stack_t stack)
{
    if (stack) {
        stack_release(stack);
        free(stack);
    }
}

static void
stack_init(pcintr_stack_t stack)
{
    INIT_LIST_HEAD(&stack->frames);
    stack->stage = STACK_STAGE_FIRST_ROUND;
    stack->loaded_vars = RB_ROOT;
    stack->mode = STACK_VDOM_BEFORE_HVML;
}

void pcintr_stack_cleanup_instance(struct pcinst* inst)
{
    struct pcintr_heap *heap = inst->intr_heap;
    if (!heap)
        return;

    struct list_head *coroutines = &heap->coroutines;
    if (!list_empty(coroutines)) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, coroutines) {
            pcintr_coroutine_t co;
            co = container_of(p, struct pcintr_coroutine, node);
            list_del(p);
            struct pcintr_stack *stack = co->stack;
            stack_destroy(stack);
        }
    }

    free(heap);
    inst->intr_heap = NULL;
}

static pcintr_coroutine_t
coroutine_get_current(void)
{
    struct pcinst *inst = pcinst_current();
    if (inst && inst->intr_heap)
        return inst->intr_heap->running_coroutine;

    return NULL;
}

static void
coroutine_set_current(struct pcintr_coroutine *co)
{
    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = inst->intr_heap;
    heap->running_coroutine = co;
}

pcintr_stack_t pcintr_get_stack(void)
{
    struct pcintr_coroutine *co = coroutine_get_current();
    if (!co)
        return NULL;

    return co->stack;
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

    stack_frame_release(frame);
    free(frame);
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
init_symvals_with_vals(struct pcintr_stack_frame *frame)
{
    purc_variant_t undefined = purc_variant_make_undefined();
    if (undefined == PURC_VARIANT_INVALID)
        return -1;

    for (size_t i=0; i<PCA_TABLESIZE(frame->symbol_vars); ++i) {
        frame->symbol_vars[i] = undefined;
        purc_variant_ref(undefined);
    }
    purc_variant_unref(undefined);

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

static struct pcintr_stack_frame*
push_stack_frame(pcintr_stack_t stack)
{
    PC_ASSERT(stack);
    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)calloc(1, sizeof(*frame));
    if (!frame) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    PC_ASSERT(NULL == pcintr_stack_frame_get_parent(frame));

    list_add_tail(&frame->node, &stack->frames);
    ++stack->nr_frames;

    if (init_symvals_with_vals(frame)) {
        pop_stack_frame(stack);
        return NULL;
    }

    frame->owner           = stack;
    frame->silently        = 0;
    return frame;
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
dump_err_except_info(purc_variant_t err_except_info)
{
    if (purc_variant_is_type(err_except_info, PURC_VARIANT_TYPE_STRING)) {
        fprintf(stderr, "err_except_info: %s\n",
                purc_variant_get_string_const(err_except_info));
    }
    else {
        char buf[1024];
        buf[0] = '\0';
        int r = pcvariant_serialize(buf, sizeof(buf), err_except_info);
        PC_ASSERT(r >= 0);
        if ((size_t)r>=sizeof(buf)) {
            buf[sizeof(buf)-1] = '\0';
            buf[sizeof(buf)-2] = '.';
            buf[sizeof(buf)-3] = '.';
            buf[sizeof(buf)-4] = '.';
        }
        fprintf(stderr, "err_except_info: %s\n", buf);
    }
}

static void
dump_stack(pcintr_stack_t stack)
{
    fprintf(stderr, "dumping stacks of corroutine [%p] ......\n", &stack->co);
    PC_ASSERT(stack);
    struct pcintr_exception *exception = &stack->exception;
    struct pcdebug_backtrace *bt = exception->bt;
    if (!bt)
        return;

    fprintf(stderr, "error_except: generated @%s[%d]:%s()\n",
            pcutils_basename((char*)bt->file), bt->line, bt->func);
    purc_atom_t     error_except = exception->error_except;
    purc_variant_t  err_except_info = exception->exinfo;
    if (error_except) {
        fprintf(stderr, "error_except: %s\n",
                purc_atom_to_string(error_except));
    }
    if (err_except_info) {
        dump_err_except_info(err_except_info);
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

int
pcintr_check_insertion_mode_for_normal_element(pcintr_stack_t stack)
{
    PC_ASSERT(stack);

    if (stack->stage != STACK_STAGE_FIRST_ROUND)
        return 0;

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

    return 0;
}

static void
after_pushed(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    if (frame->ops.after_pushed) {
        void *ctxt = frame->ops.after_pushed(co->stack, frame->pos);
        if (!ctxt) {
            frame->next_step = NEXT_STEP_ON_POPPING;
            return;
        }
    }

    frame->next_step = NEXT_STEP_SELECT_CHILD;
}

static void
on_popping(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    bool ok = true;
    if (frame->ops.on_popping) {
        ok = frame->ops.on_popping(co->stack, frame->ctxt);
    }

    if (ok) {
        pcintr_stack_t stack = co->stack;
        pop_stack_frame(stack);
    } else {
        frame->next_step = NEXT_STEP_RERUN;
    }
}

static void
on_rerun(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    bool ok = false;
    if (frame->ops.rerun) {
        ok = frame->ops.rerun(co->stack, frame->ctxt);
    }

    PC_ASSERT(ok);

    frame->next_step = NEXT_STEP_SELECT_CHILD;
}

static void
on_select_child(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct pcvdom_element *element = NULL;
    if (frame->ops.select_child) {
        element = frame->ops.select_child(co->stack, frame->ctxt);
    }

    if (element == NULL) {
        frame->next_step = NEXT_STEP_ON_POPPING;
    }
    else {
        frame->next_step = NEXT_STEP_SELECT_CHILD;

        // push child frame
        pcintr_stack_t stack = co->stack;
        struct pcintr_stack_frame *child_frame;
        child_frame = push_stack_frame(stack);
        if (!child_frame)
            return;

        child_frame->ops = pcintr_get_ops_by_element(element);
        child_frame->pos = element;
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

static void
execute_one_step(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    if (frame->preemptor) {
        preemptor_f preemptor = frame->preemptor;
        frame->preemptor = NULL;
        preemptor(co, frame);
    }
    else {
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

    PC_ASSERT(co->state == CO_STATE_RUN);
    co->state = CO_STATE_READY;
    PC_ASSERT(co->stack);

    struct pcinst *inst = pcinst_current();
    PC_ASSERT(inst);
    if (inst->errcode) {
        PC_ASSERT(co->stack->except == 0);
        exception_copy(&co->stack->exception);
        co->stack->except = 1;
        pcinst_clear_error(inst);
        PC_ASSERT(inst->errcode == 0);
#ifndef NDEBUG                     /* { */
        dump_stack(co->stack);
#endif                             /* } */
        PC_ASSERT(inst->errcode == 0);
    }

    bool no_frames = list_empty(&co->stack->frames);
    if (no_frames) {
        /* send doc to rdr */
        if (co->stack->stage == STACK_STAGE_FIRST_ROUND &&
            !pcintr_rdr_page_control_load(stack)) {
            co->state = CO_STATE_TERMINATED;
            return;
        }

        pcintr_dump_document(stack);
        co->stack->stage = STACK_STAGE_EVENT_LOOP;
        // do not run execute-one-step until event's fired if co->waits > 0
        if (co->stack->except == 0 && co->waits) { // FIXME:
            co->state = CO_STATE_WAIT;
            return;
        }

        co->state = CO_STATE_TERMINATED;
        PC_DEBUGX("co terminating: %p", co);
    }
    else {
        frame = pcintr_stack_get_bottom_frame(stack);
        if (frame && frame->preemptor) {
            PC_ASSERT(0); // Not implemented yet
        }
        // continue coroutine even if it's in wait state
    }
}

static int run_coroutines(void *ctxt)
{
    UNUSED_PARAM(ctxt);

    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = inst->intr_heap;
    struct list_head *coroutines = &heap->coroutines;
    size_t readies = 0;
    size_t waits = 0;
    if (!list_empty(coroutines)) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, coroutines) {
            struct pcintr_coroutine *co;
            co = container_of(p, struct pcintr_coroutine, node);
            switch (co->state) {
                case CO_STATE_READY:
                    co->state = CO_STATE_RUN;
                    coroutine_set_current(co);
                    pcvariant_push_gc();
                    execute_one_step(co);
                    //PC_ASSERT(purc_get_last_error() == PURC_ERROR_OK);
                    if (co->state == CO_STATE_TERMINATED) {
                        if (co->stack->except) {
                            dump_c_stack(co->stack->exception.bt);
                        }
                        PC_ASSERT(co->stack->back_anchor == NULL);

                        if (co->stack->ops.on_terminated) {
                            co->stack->ops.on_terminated(co->stack, co->stack->ctxt);
                            co->stack->ops.on_terminated = NULL;
                        }
                        if (co->stack->ops.on_cleanup) {
                            co->stack->ops.on_cleanup(co->stack, co->stack->ctxt);
                            co->stack->ops.on_cleanup = NULL;
                            co->stack->ctxt = NULL;
                        }
                    }
                    pcvariant_pop_gc();
                    coroutine_set_current(NULL);
                    ++readies;
                    break;
                case CO_STATE_WAIT:
                    ++waits;
                    break;
                case CO_STATE_RUN:
                    PC_ASSERT(0);
                    break;
                case CO_STATE_TERMINATED:
                    PC_ASSERT(0);
                    break;
                default:
                    PC_ASSERT(0);
            }
            if (co->state == CO_STATE_TERMINATED) {
                co->stack->stage = STACK_STAGE_TERMINATING;
                list_del(&co->node);
                stack_destroy(co->stack);
            }
        }
    }

    if (readies) {
        pcintr_coroutine_ready();
    }
    else if (waits==0) {
        purc_runloop_t runloop = purc_runloop_get_current();
        PC_ASSERT(runloop);
        purc_runloop_stop(runloop);
    }

    return 0;
}

void pcintr_coroutine_ready(void)
{
    purc_runloop_t runloop = purc_runloop_get_current();
    PC_ASSERT(runloop);
    purc_runloop_dispatch(runloop, run_coroutines, NULL);
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

    if (list_is_first(&frame->node, &frame->owner->frames))
        return NULL;

    struct list_head *n = frame->node.prev;
    PC_ASSERT(n);

    return container_of(n, struct pcintr_stack_frame, node);
}

purc_vdom_t
purc_load_hvml_from_string(const char* string)
{
    return purc_load_hvml_from_string_ex(string, NULL, NULL);
}

purc_vdom_t
purc_load_hvml_from_string_ex(const char* string,
        struct pcintr_supervisor_ops *ops, void *ctxt)
{
    purc_rwstream_t in;
    in = purc_rwstream_new_from_mem ((void*)string, strlen(string));
    if (!in)
        return NULL;
    purc_vdom_t vdom = purc_load_hvml_from_rwstream_ex(in, ops, ctxt);
    purc_rwstream_destroy(in);
    return vdom;
}

purc_vdom_t
purc_load_hvml_from_file(const char* file)
{
    return purc_load_hvml_from_file_ex(file, NULL, NULL);
}

purc_vdom_t
purc_load_hvml_from_file_ex(const char* file,
        struct pcintr_supervisor_ops *ops, void *ctxt)
{
    purc_rwstream_t in;
    in = purc_rwstream_new_from_file(file, "r");
    if (!in)
        return NULL;
    purc_vdom_t vdom = purc_load_hvml_from_rwstream_ex(in, ops, ctxt);
    purc_rwstream_destroy(in);
    return vdom;
}

purc_vdom_t
purc_load_hvml_from_url(const char* url)
{
    return purc_load_hvml_from_url_ex(url, NULL, NULL);
}

purc_vdom_t
purc_load_hvml_from_url_ex(const char* url,
        struct pcintr_supervisor_ops *ops, void *ctxt)
{
    purc_vdom_t vdom = NULL;
    struct pcfetcher_resp_header resp_header = {0};
    purc_rwstream_t resp = pcfetcher_request_sync(
            url,
            PCFETCHER_REQUEST_METHOD_GET,
            NULL,
            10,
            &resp_header);
    if (resp_header.ret_code == 200) {
        vdom = purc_load_hvml_from_rwstream_ex(resp, ops, ctxt);
        purc_rwstream_destroy(resp);
    }

    if (resp_header.mime_type) {
        free(resp_header.mime_type);
    }
    return vdom;
}

static struct pcvdom_document*
load_document(purc_rwstream_t in)
{
    struct pchvml_parser *parser = NULL;
    struct pcvdom_gen *gen = NULL;
    struct pcvdom_document *doc = NULL;
    struct pchvml_token *token = NULL;
    parser = pchvml_create(0, 0);
    if (!parser)
        goto error;

    gen = pcvdom_gen_create();
    if (!gen)
        goto error;

again:
    if (token)
        pchvml_token_destroy(token);

    token = pchvml_next_token(parser, in);
    if (!token)
        PC_ASSERT(0);

    if (!token)
        goto error;

    if (pcvdom_gen_push_token(gen, parser, token))
        goto error;

    if (!pchvml_token_is_type(token, PCHVML_TOKEN_EOF)) {
        goto again;
    }

    doc = pcvdom_gen_end(gen);
    goto end;

error:
    doc = pcvdom_gen_end(gen);
    if (doc) {
        pcvdom_document_destroy(doc);
        doc = NULL;
    }

end:
    if (token)
        pchvml_token_destroy(token);

    if (gen)
        pcvdom_gen_destroy(gen);

    if (parser)
        pchvml_destroy(parser);

    return doc;
}

#define BUILDIN_VAR_HVML        "HVML"
#define BUILDIN_VAR_SYSTEM      "SYSTEM"
#define BUILDIN_VAR_DATETIME    "DATETIME"
#define BUILDIN_VAR_T           "T"
#define BUILDIN_VAR_L           "L"
#define BUILDIN_VAR_DOC         "DOC"
#define BUILDIN_VAR_SESSION     "SESSION"
#define BUILDIN_VAR_EJSON       "EJSON"
#define BUILDIN_VAR_STR         "STR"
#define BUILDIN_VAR_STREAM      "STREAM"

static bool
bind_doc_named_variable(pcintr_stack_t stack, const char* name,
        purc_variant_t var)
{
    if (var == PURC_VARIANT_INVALID) {
        return false;
    }

    if (!pcintr_bind_document_variable(stack->vdom, name, var)) {
        purc_variant_unref(var);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }
    purc_variant_unref(var);
    return true;
}

static bool
init_buidin_doc_variable(pcintr_stack_t stack)
{
    // $TIMERS
    stack->vdom->timers = pcintr_timers_init(stack);
    if (!stack->vdom->timers) {
        return false;
    }

    // $HVML
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_HVML,
                purc_dvobj_hvml_new(&stack->vdom->hvml_ctrl_props))) {
        return false;
    }

    // $SYSTEM
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_SYSTEM,
                purc_dvobj_system_new())) {
        return false;
    }

    // $DATETIME
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_DATETIME,
                purc_dvobj_datetime_new())) {
        return false;
    }

    // $T
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_T,
                purc_dvobj_text_new())) {
        return false;
    }

    // $L
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_L,
                purc_dvobj_logical_new())) {
        return false;
    }

    // FIXME: document-wide-variant???
    // $STR
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_STR,
                purc_dvobj_string_new())) {
        return false;
    }

    // $STREAM
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_STREAM,
                purc_dvobj_stream_new())) {
        return false;
    }


    // $DOC
    pchtml_html_document_t *doc = stack->doc;
    pcdom_document_t *document = (pcdom_document_t*)doc;
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_DOC,
                purc_dvobj_doc_new(document))) {
        return false;
    }

    // TODO : bind by  purc_bind_variable
    // begin
    // $SESSION
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_SESSION,
                purc_dvobj_session_new())) {
        return false;
    }

    // $EJSON
    if(!bind_doc_named_variable(stack, BUILDIN_VAR_EJSON,
                purc_dvobj_ejson_new())) {
        return false;
    }
    // end

    return true;
}

purc_vdom_t
purc_load_hvml_from_rwstream(purc_rwstream_t stream)
{
    return purc_load_hvml_from_rwstream_ex(stream, NULL, NULL);
}

purc_vdom_t
purc_load_hvml_from_rwstream_ex(purc_rwstream_t stream,
        struct pcintr_supervisor_ops *ops, void *ctxt)
{
    struct pcvdom_document *doc = NULL;
    doc = load_document(stream);
    if (!doc)
        return NULL;

    purc_vdom_t vdom = (purc_vdom_t)calloc(1, sizeof(*vdom));
    if (!vdom) {
        pcvdom_document_destroy(doc);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    vdom->document = doc;

    pcintr_stack_t stack = (pcintr_stack_t)calloc(1, sizeof(*stack));
    if (!stack) {
        vdom_destroy(vdom);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    stack_init(stack);

    stack->event_timer = pcintr_timer_create(NULL, NULL, stack,
            pcintr_event_timer_fire);
    if (!stack->event_timer) {
        vdom_destroy(vdom);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    pcintr_timer_set_interval(stack->event_timer, EVENT_TIMER_INTRVAL);
    pcintr_timer_start(stack->event_timer);

    stack->vdom = vdom;
    stack->co.stack = stack;
    stack->co.state = CO_STATE_READY;

    if (doc_init(stack)) {
        stack_destroy(stack);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    if(!init_buidin_doc_variable(stack)) {
        stack_destroy(stack);
        return NULL;
    }

    struct pcintr_stack_frame *frame;
    frame = push_stack_frame(stack);
    if (!frame) {
        stack_destroy(stack);
        return NULL;
    }
    // frame->next_step = on_vdom_start;
    frame->ops = *pcintr_get_document_ops();

    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = inst->intr_heap;
    struct list_head *coroutines = &heap->coroutines;
    list_add_tail(&stack->co.node, coroutines);

    pcintr_coroutine_ready();

    if (ops) {
        stack->ops  = *ops;
        stack->ctxt = ctxt;
    }

    // FIXME: double-free, potentially!!!
    return vdom;
}

bool
purc_run(purc_variant_t request, purc_event_handler handler)
{
    UNUSED_PARAM(request);
    UNUSED_PARAM(handler);

    purc_runloop_run();

    return true;
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

static int
add_observer_into_list(struct pcutils_arrlist* list,
        struct pcintr_observer* observer)
{
    observer->list = list;
    int r = pcutils_arrlist_append(list, observer);
    PC_ASSERT(r == 0);

    // TODO:
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    PC_ASSERT(stack->co.waits >= 0);
    stack->co.waits++;

    return r;
}

static void
del_observer_from_list(struct pcutils_arrlist* list,
        struct pcintr_observer* observer)
{
    size_t n = pcutils_arrlist_length(list);
    int pos = -1;
    for (size_t i = 0; i < n; i++) {
        if (observer == pcutils_arrlist_get_idx(list, i)) {
            pos = i;
            break;
        }
    }

    if (pos > -1) {
        pcutils_arrlist_del_idx(list, pos, 1);

        // TODO:
        pcintr_stack_t stack = pcintr_get_stack();
        PC_ASSERT(stack);
        PC_ASSERT(stack->co.waits >= 1);
        stack->co.waits--;
    }
}

void observer_free_func(void *data)
{
    if (data) {
        struct pcintr_observer* observer = (struct pcintr_observer*)data;
        if (observer->listener) {
            purc_variant_revoke_listener(observer->observed,
                    observer->listener);
        }
        if (purc_variant_is_native(observer->observed)) {
            struct purc_native_ops *ops = purc_variant_native_get_ops(
                    observer->observed);
            if (ops && ops->on_forget) {
                void *native_entity = purc_variant_native_get_entity(
                        observer->observed);
                ops->on_forget(native_entity, observer->msg_type,
                        observer->sub_type);
            }
        }
        purc_variant_unref(observer->observed);
        free(observer->msg_type);
        free(observer->sub_type);
        free(observer);
    }
}

struct pcintr_observer*
pcintr_register_observer(purc_variant_t observed,
        purc_variant_t for_value, pcvdom_element_t scope,
        pcdom_element_t *edom_element,
        pcvdom_element_t pos,
        struct pcvar_listener* listener
        )
{
    UNUSED_PARAM(for_value);

    pcintr_stack_t stack = pcintr_get_stack();
    struct pcutils_arrlist* list = NULL;
    if (purc_variant_is_type(observed, PURC_VARIANT_TYPE_DYNAMIC)) {
        if (stack->dynamic_variant_observer_list == NULL) {
            stack->dynamic_variant_observer_list = pcutils_arrlist_new(
                    observer_free_func);
        }
        list = stack->dynamic_variant_observer_list;
    }
    else if (purc_variant_is_type(observed, PURC_VARIANT_TYPE_NATIVE)) {
        if (stack->native_variant_observer_list == NULL) {
            stack->native_variant_observer_list = pcutils_arrlist_new(
                    observer_free_func);
        }
        list = stack->native_variant_observer_list;
    }
    else {
        if (stack->common_variant_observer_list == NULL) {
            stack->common_variant_observer_list = pcutils_arrlist_new(
                    observer_free_func);
        }
        list = stack->common_variant_observer_list;
    }

    if (!list) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    const char* for_value_str = purc_variant_get_string_const(for_value);
    char* value = strdup(for_value_str);
    if (!value) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    char* p = value;
    char* msg_type = strtok_r(p, EVENT_SEPARATOR, &p);
    if (!msg_type) {
        //TODO : purc_set_error();
        free(value);
        return NULL;
    }

    char* sub_type = strtok_r(p, EVENT_SEPARATOR, &p);

    struct pcintr_observer* observer =  (struct pcintr_observer*)calloc(1,
            sizeof(struct pcintr_observer));
    if (!observer) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(value);
        return NULL;
    }
    observer->observed = observed;
    purc_variant_ref(observed);
    observer->scope = scope;
    observer->edom_element = edom_element;
    observer->pos = pos;
    observer->msg_type = strdup(msg_type);
    observer->msg_type_atom = purc_atom_try_string_ex(ATOM_BUCKET_MSG,
            msg_type);
    observer->sub_type = sub_type ? strdup(sub_type) : NULL;
    observer->listener = listener;
    add_observer_into_list(list, observer);

    free(value);

    return observer;
}

bool
pcintr_revoke_observer(struct pcintr_observer* observer)
{
    if (!observer) {
        return true;
    }

    del_observer_from_list(observer->list, observer);
    return true;
}

bool
pcintr_revoke_observer_ex(purc_variant_t observed, purc_variant_t for_value)
{
    pcintr_stack_t stack = pcintr_get_stack();
    const char* for_value_str = purc_variant_get_string_const(for_value);
    char* value = strdup(for_value_str);
    if (!value) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    char* p = value;
    char* msg_type = strtok_r(p, EVENT_SEPARATOR, &p);
    if (!msg_type) {
        //TODO : purc_set_error();
        free(value);
        return false;
    }

    char* sub_type = strtok_r(p, EVENT_SEPARATOR, &p);
    purc_variant_t msg_type_var = purc_variant_make_string(msg_type, false);
    purc_variant_t sub_type_var = PURC_VARIANT_INVALID;
    if (sub_type) {
        sub_type_var = purc_variant_make_string(sub_type, false);
    }

    struct pcutils_arrlist* observers = pcintr_find_all_observer(stack,
            observed, msg_type_var, sub_type_var);
    if (observers != NULL) {
        size_t n = pcutils_arrlist_length(observers);
        for (size_t i = 0; i < n; i++) {
            struct pcintr_observer* observer = pcutils_arrlist_get_idx(
                    observers, i);
            if (observer) {
                pcintr_revoke_observer(observer);
            }
        }
        pcutils_arrlist_free(observers);
    }

    purc_variant_unref(msg_type_var);
    if (sub_type) {
        purc_variant_unref(sub_type_var);
    }
    free(value);
    return true;
}

bool is_observer_match(struct pcintr_observer *observer,
        purc_variant_t observed, purc_atom_t type_atom, const char *sub_type)
{
    if ((observer->observed == observed) &&
                (observer->msg_type_atom == type_atom)) {
        if (observer->sub_type == sub_type ||
                pcregex_is_match(observer->sub_type, sub_type)) {
            return true;
        }
    }
    return false;
}

struct pcutils_arrlist*
pcintr_find_all_observer(pcintr_stack_t stack, purc_variant_t observed,
        purc_variant_t msg_type, purc_variant_t sub_type)
{
    if (observed == PURC_VARIANT_INVALID ||
            msg_type == PURC_VARIANT_INVALID) {
        return NULL;
    }
    const char* msg = purc_variant_get_string_const(msg_type);
    purc_atom_t msg_atom = purc_atom_try_string_ex(ATOM_BUCKET_MSG, msg);
    if (msg_atom == 0) {
        return NULL;
    }

    const char* sub = (sub_type != PURC_VARIANT_INVALID) ?
        purc_variant_get_string_const(sub_type) : NULL;

    struct pcutils_arrlist* list = NULL;
    if (purc_variant_is_type(observed, PURC_VARIANT_TYPE_DYNAMIC)) {
        list = stack->dynamic_variant_observer_list;
    }
    else if (purc_variant_is_type(observed, PURC_VARIANT_TYPE_NATIVE)) {
        list = stack->native_variant_observer_list;
    }
    else {
        list = stack->common_variant_observer_list;
    }

    if (!list) {
        return NULL;
    }

    struct pcutils_arrlist* ret_list = pcutils_arrlist_new(NULL);
    if (ret_list == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    size_t n = pcutils_arrlist_length(list);
    for (size_t i = 0; i < n; i++) {
        struct pcintr_observer* observer = pcutils_arrlist_get_idx(list, i);
        if (is_observer_match(observer, observed, msg_atom, sub)) {
            pcutils_arrlist_append(ret_list, observer);
        }
    }
    n = pcutils_arrlist_length(ret_list);
    if (n) {
        return ret_list;
    }
    pcutils_arrlist_free(ret_list);
    return NULL;
}

bool
pcintr_is_observer_empty(pcintr_stack_t stack)
{
    if (!stack) {
        return false;
    }

    if (stack->native_variant_observer_list
            && pcutils_arrlist_length(stack->native_variant_observer_list)) {
        return false;
    }

    if (stack->dynamic_variant_observer_list
            && pcutils_arrlist_length(stack->dynamic_variant_observer_list)) {
        return false;
    }

    if (stack->common_variant_observer_list
            && pcutils_arrlist_length(stack->common_variant_observer_list)) {
        return false;
    }
    return true;
}

struct pcintr_message {
    pcintr_stack_t stack;
    purc_variant_t source;
    purc_variant_t type;
    purc_variant_t sub_type;
    purc_variant_t extra;
};

struct pcintr_message*
pcintr_message_create(pcintr_stack_t stack, purc_variant_t source,
        purc_variant_t type, purc_variant_t sub_type, purc_variant_t extra)
{
    struct pcintr_message* msg = (struct pcintr_message*)malloc(
            sizeof(struct pcintr_message));
    if (!msg) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    msg->stack = stack;

    msg->source = source;
    purc_variant_ref(msg->source);

    msg->type = type;
    purc_variant_ref(msg->type);

    msg->sub_type = sub_type;
    if (sub_type != PURC_VARIANT_INVALID) {
        purc_variant_ref(msg->sub_type);
    }

    msg->extra = extra;
    if (extra != PURC_VARIANT_INVALID) {
        purc_variant_ref(msg->extra);
    }
    return msg;
}

void
pcintr_message_destroy(struct pcintr_message* msg)
{
    if (msg) {
        PURC_VARIANT_SAFE_CLEAR(msg->source);
        PURC_VARIANT_SAFE_CLEAR(msg->type);
        PURC_VARIANT_SAFE_CLEAR(msg->sub_type);
        PURC_VARIANT_SAFE_CLEAR(msg->extra);
        free(msg);
    }
}

static int
pcintr_handle_message(void *ctxt)
{
    pcintr_stack_t stack = NULL;

    struct pcutils_arrlist* observers = NULL;

    struct pcintr_message* msg = (struct pcintr_message*) ctxt;
    PC_ASSERT(msg);

    stack = msg->stack;
    PC_ASSERT(stack);

    observers = pcintr_find_all_observer(msg->stack,
            msg->source, msg->type, msg->sub_type);
    pcintr_message_destroy(msg);
    if (observers == NULL) {
        return 0;
    }

    size_t n = pcutils_arrlist_length(observers);
    for (size_t i = 0; i < n; i++) {
        struct pcintr_observer* observer = pcutils_arrlist_get_idx(observers, i);

        // FIXME:
        // push stack frame
        struct pcintr_stack_frame *frame;
        frame = push_stack_frame(stack);
        if (!frame)
            return -1;

        frame->ops = pcintr_get_ops_by_element(observer->pos);
        frame->scope = observer->scope;
        frame->pos = observer->pos;
        frame->silently = pcintr_is_element_silently(frame->pos) ? 1 : 0;
        frame->edom_element = observer->edom_element;
        frame->next_step = NEXT_STEP_AFTER_PUSHED;

        stack->co.state = CO_STATE_READY;
       // pcintr_coroutine_ready();
        run_coroutines(NULL);
    }

    pcutils_arrlist_free(observers);
    return 0;
}

int
pcintr_dispatch_message(pcintr_stack_t stack, purc_variant_t source,
        purc_variant_t for_value, purc_variant_t extra)
{
    int ret = -1;
    const char *for_value_str = purc_variant_get_string_const(for_value);
    char *value = strdup(for_value_str);
    if (!value) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return ret;
    }

    char *p = value;
    char *s_type = strtok_r(p, EVENT_SEPARATOR, &p);
    if (!s_type) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out_free_value;
    }

    purc_variant_t type = purc_variant_make_string(s_type, true);
    if (type == PURC_VARIANT_INVALID) {
        goto out_free_value;
    }

    purc_variant_t sub_type = PURC_VARIANT_INVALID;
    char *s_sub_type = strtok_r(p, EVENT_SEPARATOR, &p);
    if (s_sub_type) {
        sub_type = purc_variant_make_string(s_sub_type, true);
        if (sub_type == PURC_VARIANT_INVALID) {
            goto out_unref_type;
        }
    }

    ret = pcintr_dispatch_message_ex(stack, source, type, sub_type, extra);

    if (sub_type) {
        purc_variant_unref(sub_type);
    }

out_unref_type:
    purc_variant_unref(type);

out_free_value:
    free(value);

    return ret;
}

int
pcintr_dispatch_message_ex(pcintr_stack_t stack, purc_variant_t source,
        purc_variant_t type, purc_variant_t sub_type, purc_variant_t extra)
{
    struct pcintr_message* msg = pcintr_message_create(stack, source, type,
            sub_type, extra);
    if (!msg) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    purc_runloop_t runloop = purc_runloop_get_current();
    PC_ASSERT(runloop);
    purc_runloop_dispatch(runloop, pcintr_handle_message, msg);
    return PURC_ERROR_OK;
}

purc_variant_t
pcintr_load_from_uri(pcintr_stack_t stack, const char* uri)
{
    if (uri == NULL) {
        return PURC_VARIANT_INVALID;
    }

    if (stack->vdom->hvml_ctrl_props->base_url_string) {
        pcfetcher_set_base_url(stack->vdom->hvml_ctrl_props->base_url_string);
    }
    purc_variant_t ret = PURC_VARIANT_INVALID;
    struct pcfetcher_resp_header resp_header = {0};
    purc_rwstream_t resp = pcfetcher_request_sync(
            uri,
            PCFETCHER_REQUEST_METHOD_GET,
            NULL,
            10,
            &resp_header);
    if (resp_header.ret_code == 200) {
        size_t sz_content = 0;
        char* buf = (char*)purc_rwstream_get_mem_buffer(resp, &sz_content);
        // FIXME:
        purc_clr_error();
        ret = purc_variant_make_from_json_string(buf, sz_content);
        purc_rwstream_destroy(resp);
    }

    if (resp_header.mime_type) {
        free(resp_header.mime_type);
    }
    return ret;
}

purc_variant_t
pcintr_load_from_uri_async(pcintr_stack_t stack, const char* uri,
        pcfetcher_response_handler handler, void* ctxt)
{
    if (uri == NULL || handler == NULL) {
        return PURC_VARIANT_INVALID;
    }

    if (stack->vdom->hvml_ctrl_props->base_url_string) {
        pcfetcher_set_base_url(stack->vdom->hvml_ctrl_props->base_url_string);
    }
    return pcfetcher_request_async(
            uri,
            PCFETCHER_REQUEST_METHOD_GET,
            NULL,
            10,
            handler,
            ctxt);
}

purc_variant_t
pcintr_load_vdom_fragment_from_uri(pcintr_stack_t stack, const char* uri)
{
    if (uri == NULL) {
        return PURC_VARIANT_INVALID;
    }

    if (stack->vdom->hvml_ctrl_props->base_url_string) {
        pcfetcher_set_base_url(stack->vdom->hvml_ctrl_props->base_url_string);
    }
    purc_variant_t ret = PURC_VARIANT_INVALID;
    struct pcfetcher_resp_header resp_header = {0};
    purc_rwstream_t resp = pcfetcher_request_sync(
            uri,
            PCFETCHER_REQUEST_METHOD_GET,
            NULL,
            10,
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
pcintr_doc_query(purc_vdom_t vdom, const char* css, bool silently)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    if (vdom == NULL || css == NULL) {
        goto end;
    }

    purc_variant_t doc = pcvdom_document_get_variable(vdom, BUILDIN_VAR_DOC);
    if (doc == PURC_VARIANT_INVALID) {
        PC_ASSERT(0);
        goto end;
    }

    struct purc_native_ops *ops = purc_variant_native_get_ops (doc);
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

    if (pcintr_bind_document_variable(stack->vdom, NAME, v)) {
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
    pcintr_rdr_dom_update_element_property(pcintr_get_stack(), elem, key, val);
    return 0;
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

static struct pcvdom_template_node*
template_node_create(struct pcvcm_node *vcm)
{
    PC_ASSERT(vcm);

    struct pcvdom_template_node *node;
    node = (struct pcvdom_template_node*)calloc(1, sizeof(*node));
    if (!node) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    node->vcm = vcm;
    return node;
}

static void
template_node_destroy(struct pcvdom_template_node *node)
{
    node->vcm = NULL;
    free(node);
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

    INIT_LIST_HEAD(&tpl->list);

    return tpl;
}

static void
template_cleaner(struct pcvdom_template *tpl)
{
    if (!tpl)
        return;

    struct pcvdom_template_node *p, *n;
    list_for_each_entry_safe(p, n, &tpl->list, node) {
        list_del(&p->node);

        template_node_destroy(p);
    }
}

static void
template_destroy(struct pcvdom_template *tpl)
{
    if (!tpl)
        return;

    template_cleaner(tpl);
    free(tpl);
}

static int
template_append(struct pcvdom_template *tpl, struct pcvcm_node *vcm)
{
    struct pcvdom_template_node *p;
    list_for_each_entry(p, &tpl->list, node) {
        if (p->vcm == vcm) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vcm alread in templates");
            return -1;
        }
    }

    p = template_node_create(vcm);
    if (!p)
        return -1;

    p->vcm = vcm;
    list_add_tail(&p->node, &tpl->list);
    return 0;
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

    return v;
}

int
is_template_variant(purc_variant_t val)
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

int
pcintr_template_append(purc_variant_t val, struct pcvcm_node *vcm)
{
    PC_ASSERT(val);
    PC_ASSERT(vcm);

    int r;
    r = is_template_variant(val);
    if (r)
        return -1;

    void *native_entity = purc_variant_native_get_entity(val);
    PC_ASSERT(native_entity);
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)native_entity;

    return template_append(tpl, vcm);
}

void
pcintr_template_walk(purc_variant_t val, void *ctxt,
        pcintr_template_walk_cb cb)
{
    int r;
    r = is_template_variant(val);
    // FIXME: modify pcintr_template_walk function-signature
    PC_ASSERT(r == 0);

    void *native_entity = purc_variant_native_get_entity(val);
    PC_ASSERT(native_entity);
    struct pcvdom_template *tpl;
    tpl = (struct pcvdom_template*)native_entity;

    struct pcvdom_template_node *p;
    list_for_each_entry(p, &tpl->list, node) {
        PC_ASSERT(p->vcm);
        if (cb(p->vcm, ctxt))
            return;
    }
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
    struct pcintr_stack_frame *frame;
    frame = push_stack_frame(stack);
    if (!frame)
        return;

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
    purc_variant_t type = purc_variant_make_string(MSG_TYPE_CHANGE, false);
    purc_variant_t sub_type = PURC_VARIANT_INVALID;

    pcintr_dispatch_message_ex(stack, var, type, sub_type, PURC_VARIANT_INVALID);

    purc_variant_unref(type);
}

void
pcintr_event_timer_fire(const char* id, void* ctxt)
{
    UNUSED_PARAM(id);
    UNUSED_PARAM(ctxt);

    if (!ctxt) {
        return;
    }

    pcintr_stack_t stack = (pcintr_stack_t)ctxt;
    if (stack->native_variant_observer_list == NULL) {
        return;
    }

    size_t n = pcutils_arrlist_length(stack->native_variant_observer_list);
    for (size_t i = 0; i < n; i++) {
        struct pcintr_observer* observer = pcutils_arrlist_get_idx(
                stack->native_variant_observer_list, i);
        purc_variant_t var = observer->observed;
        struct purc_native_ops *ops = purc_variant_native_get_ops(var);
        if (!ops) {
            continue;
        }
        if (ops->property_getter) {
            purc_nvariant_method is_vcm_ev = ops->property_getter(
                    PCVCM_EV_PROPERTY_VCM_EV);
            if (is_vcm_ev) {
                pcintr_observe_vcm_ev(stack, observer, var, ops);
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

