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
#include "private/runloop.h"

#include "ops.h"
#include "../hvml/hvml-gen.h"
#include "hvml-attr.h"

#define TO_DEBUG 0

void pcintr_stack_init_once(void)
{
    pcrunloop_init_main();
    pcrunloop_t runloop = pcrunloop_get_main();
    PC_ASSERT(runloop);
    init_ops();
}

void pcintr_stack_init_instance(struct pcinst* inst)
{
    struct pcintr_heap *heap = &inst->intr_heap;
    INIT_LIST_HEAD(&heap->coroutines);
    heap->running_coroutine = NULL;
}

static void
stack_frame_release(struct pcintr_stack_frame *frame)
{
    frame->scope = NULL;
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
    PURC_VARIANT_SAFE_CLEAR(frame->mid_vars);
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

static void
stack_release(pcintr_stack_t stack)
{
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
        vdom_destroy(stack->vdom);
        stack->vdom = NULL;
    }

    if (stack->common_observer_list) {
        pcutils_arrlist_free(stack->common_observer_list);
        stack->common_observer_list = NULL;
    }

    if (stack->special_observer_list) {
        pcutils_arrlist_free(stack->special_observer_list);
        stack->special_observer_list = NULL;
    }

    if (stack->native_observer_list) {
        pcutils_arrlist_free(stack->native_observer_list);
        stack->native_observer_list = NULL;
    }
}

static void
stack_init(pcintr_stack_t stack)
{
    INIT_LIST_HEAD(&stack->frames);
    stack->stage = STACK_STAGE_FIRST_ROUND;
}

void pcintr_stack_cleanup_instance(struct pcinst* inst)
{
    struct pcintr_heap *heap = &inst->intr_heap;
    struct list_head *coroutines = &heap->coroutines;
    if (list_empty(coroutines))
        return;

    struct list_head *p, *n;
    list_for_each_safe(p, n, coroutines) {
        pcintr_coroutine_t co;
        co = container_of(p, struct pcintr_coroutine, node);
        list_del(p);
        struct pcintr_stack *stack = co->stack;
        stack_release(stack);
        free(stack);
    }
}


static pcintr_coroutine_t
coroutine_get_current(void)
{
    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = &inst->intr_heap;
    return heap->running_coroutine;
}

static void
coroutine_set_current(struct pcintr_coroutine *co)
{
    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = &inst->intr_heap;
    heap->running_coroutine = co;
}

pcintr_stack_t purc_get_stack(void)
{
    struct pcintr_coroutine *co = coroutine_get_current();
    if (!co)
        return NULL;

    return co->stack;
}

struct pcintr_stack_frame*
pcintr_push_stack_frame(pcintr_stack_t stack)
{
    PC_ASSERT(stack);
    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)calloc(1, sizeof(*frame));
    if (!frame)
        return NULL;

    purc_variant_t undefined = purc_variant_make_undefined();
    if (undefined == PURC_VARIANT_INVALID) {
        free(frame);
        return NULL;
    }
    for (size_t i=0; i<PCA_TABLESIZE(frame->symbol_vars); ++i) {
        frame->symbol_vars[i] = undefined;
        purc_variant_ref(undefined);
    }

    list_add_tail(&frame->node, &stack->frames);
    ++stack->nr_frames;

    return frame;
}

void
pcintr_pop_stack_frame(pcintr_stack_t stack)
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
visit_attr(void *key, void *val, void *ud)
{
    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)ud;
    if (frame->attr_vars == PURC_VARIANT_INVALID) {
        frame->attr_vars = purc_variant_make_object(0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        if (frame->attr_vars == PURC_VARIANT_INVALID)
            return -1;
    }

    struct pcvdom_attr *attr = (struct pcvdom_attr*)val;
    struct pcvcm_node *vcm = attr->val;
    purc_variant_t value;
    if (!vcm) {
        struct pcvdom_element *element = frame->pos;
        D("<%s>attr: [%s:]", element->tag_name, attr->key);
        value = purc_variant_make_undefined();
        if (value == PURC_VARIANT_INVALID) {
            return -1;
        }
    }
    else {
        PC_ASSERT(attr->key == key);
        PC_ASSERT(vcm);

        struct pcvdom_element *element = frame->pos;
        PC_ASSERT(element);
        char *s = pcvcm_node_to_string(vcm, NULL);
        D("<%s>attr: [%s:%s]", element->tag_name, attr->key, s);
        free(s);
        purc_clr_error();

        pcintr_stack_t stack = purc_get_stack();
        PC_ASSERT(stack);
        value = pcvcm_eval(vcm, stack);
        if (value == PURC_VARIANT_INVALID) {
            return -1;
        }
    }

    const struct pchvml_attr_entry *pre_defined = attr->pre_defined;
    bool ok;
    if (pre_defined) {
        ok = purc_variant_object_set_by_static_ckey(frame->attr_vars,
                pre_defined->name, value);
        purc_variant_unref(value);
    }
    else {
        PC_ASSERT(attr->key);
        purc_variant_t k = purc_variant_make_string(attr->key, true);
        if (k == PURC_VARIANT_INVALID) {
            purc_variant_unref(value);
            return -1;
        }
        ok = purc_variant_object_set(frame->attr_vars, k, value);
        purc_variant_unref(value);
        purc_variant_unref(k);
    }

    return ok ? 0 : -1;
}

int
pcintr_element_eval_attrs(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    struct pcutils_map *attrs = element->attrs;
    if (!attrs)
        return 0;

    PC_ASSERT(frame->pos == element);

    int r = pcutils_map_traverse(attrs, frame, visit_attr);
    if (r)
        return r;

    return 0;
}

int
pcintr_element_eval_vcm_content(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    struct pcvcm_node *vcm_content = element->vcm_content;
    if (vcm_content == NULL)
        return 0;

    char *s = pcvcm_node_to_string(vcm_content, NULL);
    D("<%s>vcm_content: [%s]", element->tag_name, s);
    free(s);
    purc_clr_error();

    purc_variant_t v; /* = pcvcm_eval(vcm_content, stack) */
    // NOTE: element is still the owner of vcm_content
    v = purc_variant_make_ulongint((uint64_t)vcm_content);
    if (v == PURC_VARIANT_INVALID)
        return -1;

    PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
    frame->ctnt_var = v;

    return 0;
}

static void
after_pushed(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    if (frame->ops.after_pushed) {
        frame->ops.after_pushed(co->stack, frame->pos);
    }

    frame->next_step = NEXT_STEP_SELECT_CHILD;
}

static void
on_popping(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    bool ok = true;
    if (frame->ops.on_popping) {
        ok = frame->ops.on_popping(co->stack, frame->ctxt);
        D("ok: %s", ok ? "true" : "false");
    }

    if (ok) {
        pcintr_stack_t stack = co->stack;
        pcintr_pop_stack_frame(stack);
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
        child_frame = pcintr_push_stack_frame(stack);
        if (!child_frame) {
            pcintr_pop_stack_frame(stack);
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }

        child_frame->ops = pcintr_get_ops_by_element(element);
        child_frame->pos = element;
        if (pcvdom_element_is_hvml_native(element)) {
            child_frame->scope = frame->scope;
            PC_ASSERT(child_frame->scope);
            // child_frame->scope = element;
        }
        else {
            purc_clr_error();
            child_frame->scope = element;
        }
        child_frame->next_step = NEXT_STEP_AFTER_PUSHED;
    }
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
        }
    }

    PC_ASSERT(co->state == CO_STATE_RUN);
    co->state = CO_STATE_READY;
    bool no_frames = list_empty(&co->stack->frames);
    if (no_frames)
        co->stack->stage = STACK_STAGE_EVENT_LOOP;
    if (co->waits)
        return;
    if (no_frames)
        co->state = CO_STATE_TERMINATED;
}

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
    PC_ASSERT(scope);
    pcvdom_element_t pos = frame->pos;
    for (size_t i=0; i<level; ++i) {
        fprintf(stderr, "  ");
    }
    fprintf(stderr, "scope:<%s>; pos:<%s>\n",
        scope->tag_name, pos ? pos->tag_name : NULL);
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
    fprintf(stderr, "dumping stack @[%p]\n", stack);
    PC_ASSERT(stack);
    fprintf(stderr, "error_except: generated @%s[%d]:%s()\n",
            basename((char*)stack->file), stack->lineno, stack->func);
    purc_atom_t     error_except = stack->error_except;
    purc_variant_t  err_except_info = stack->err_except_info;
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

static int run_coroutines(void *ctxt)
{
    UNUSED_PARAM(ctxt);

    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = &inst->intr_heap;
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
            pcintr_stack_t stack = co->stack;
            PC_ASSERT(stack);
            if (stack->except) {
                dump_stack(stack);
                co->state = CO_STATE_TERMINATED;
            }
            if (co->state == CO_STATE_TERMINATED) {
                stack->stage = STACK_STAGE_TERMINATING;
                list_del(&co->node);
                stack_release(stack);
                free(stack);
            }
        }
    }

    if (readies) {
        pcrunloop_t runloop = pcrunloop_get_current();
        PC_ASSERT(runloop);
        pcrunloop_dispatch(runloop, run_coroutines, NULL);
    }
    else if (waits==0) {
        pcrunloop_t runloop = pcrunloop_get_current();
        PC_ASSERT(runloop);
        pcrunloop_stop(runloop);
    }

    return 0;
}

void pcintr_coroutine_ready(void)
{
    pcrunloop_t runloop = pcrunloop_get_current();
    PC_ASSERT(runloop);
    pcrunloop_dispatch(runloop, run_coroutines, NULL);
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

    struct list_head *n = frame->node.prev;
    if (!n)
        return NULL;

    return container_of(n, struct pcintr_stack_frame, node);
}

purc_vdom_t
purc_load_hvml_from_string(const char* string)
{
    purc_rwstream_t in;
    in = purc_rwstream_new_from_mem ((void*)string, strlen(string));
    if (!in)
        return NULL;
    purc_vdom_t vdom = purc_load_hvml_from_rwstream(in);
    purc_rwstream_destroy(in);
    return vdom;
}

purc_vdom_t
purc_load_hvml_from_file(const char* file)
{
    purc_rwstream_t in;
    in = purc_rwstream_new_from_file(file, "r");
    if (!in)
        return NULL;
    purc_vdom_t vdom = purc_load_hvml_from_rwstream(in);
    purc_rwstream_destroy(in);
    return vdom;
}

PCA_EXPORT purc_vdom_t
purc_load_hvml_from_url(const char* url)
{
    UNUSED_PARAM(url);
    PC_ASSERT(0); // Not implemented yet
    return NULL;
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

purc_vdom_t
purc_load_hvml_from_rwstream(purc_rwstream_t stream)
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

    stack->vdom = vdom;
    stack->co.stack = stack;
    stack->co.state = CO_STATE_READY;

    struct pcintr_stack_frame *frame;
    frame = pcintr_push_stack_frame(stack);
    if (!frame) {
        stack_release(stack);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    // frame->next_step = on_vdom_start;
    frame->ops = *pcintr_get_document_ops();

    struct pcinst *inst = pcinst_current();
    struct pcintr_heap *heap = &inst->intr_heap;
    struct list_head *coroutines = &heap->coroutines;
    list_add_tail(&stack->co.node, coroutines);

    pcrunloop_t runloop = pcrunloop_get_current();
    PC_ASSERT(runloop);
    pcrunloop_dispatch(runloop, run_coroutines, NULL);

    // FIXME: double-free, potentially!!!
    return vdom;
}

bool
purc_run(purc_variant_t request, purc_event_handler handler)
{
    UNUSED_PARAM(request);
    UNUSED_PARAM(handler);
    pcrunloop_run();

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

int add_observer_into_list(struct pcutils_arrlist* list,
        struct pcintr_observer* observer)
{
    observer->list = list;
    return pcutils_arrlist_add(list, observer);
}

void del_observer_from_list(struct pcutils_arrlist* list,
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
    }
}

void observer_free_func(void *data)
{
    if (data) {
        struct pcintr_observer* observer = (struct pcintr_observer*)data;
        free(observer->msg_type);
        free(observer->sub_type);
        free(observer);
    }
}

struct pcintr_observer*
pcintr_register_observer(enum pcintr_observer_type type, purc_variant_t observed,
        purc_variant_t for_value, pcvdom_element_t ele)
{
    UNUSED_PARAM(for_value);

    pcintr_stack_t stack = purc_get_stack();
    struct pcutils_arrlist* list = NULL;
    switch (type) {
        case PCINTR_OBSERVER_TYPE_SPECIAL:
            if (!stack->special_observer_list) {
                stack->special_observer_list =  pcutils_arrlist_new(
                        observer_free_func);
            }
            list = stack->special_observer_list;
            break;

        case PCINTR_OBSERVER_TYPE_NATIVE:
            if (!stack->native_observer_list) {
                stack->native_observer_list =  pcutils_arrlist_new(
                        observer_free_func);
            }
            list = stack->native_observer_list;
            break;

        case PCINTR_OBSERVER_TYPE_COMMON:
        default:
            if (!stack->common_observer_list) {
                stack->common_observer_list =  pcutils_arrlist_new(
                        observer_free_func);
            }
            list = stack->common_observer_list;
            break;
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

    char* msg_type = strtok(value, ":");
    if (!msg_type) {
        //TODO : purc_set_error();
        free(value);
        return NULL;
    }

    char* sub_type = strtok(NULL, ":");
    if (!sub_type) {
        //TODO : purc_set_error();
        free(value);
        return NULL;
    }

    struct pcintr_observer* observer =  (struct pcintr_observer*)calloc(1,
            sizeof(struct pcintr_observer));
    if (!observer) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        free(value);
        return NULL;
    }
    observer->observed = observed;
    observer->obs_ele = ele;
    observer->msg_type = strdup(msg_type);
    observer->sub_type = strdup(sub_type);
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
