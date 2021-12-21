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
        struct list_head *p, *n;
        list_for_each_safe(p, n, frames) {
            struct pcintr_stack_frame *frame;
            frame = container_of(p, struct pcintr_stack_frame, node);
            list_del(p);
            --stack->nr_frames;
            stack_frame_release(frame);
            free(frame);
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

pcintr_stack_t purc_get_stack(void)
{
    struct pcintr_coroutine *co = coroutine_get_current();
    if (!co)
        return NULL;

    return co->stack;
}

struct pcintr_stack_frame*
push_stack_frame(pcintr_stack_t stack)
{
    PC_ASSERT(stack);
    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)calloc(1, sizeof(*frame));
    if (!frame)
        return NULL;

    list_add_tail(&frame->node, &stack->frames);
    ++stack->nr_frames;

    return frame;
}

void
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
        value = purc_variant_make_undefined();
        if (value == PURC_VARIANT_INVALID) {
            return -1;
        }
    }
    else {
        PC_ASSERT(attr->key == key);
        PC_ASSERT(vcm);

        pcintr_stack_t stack = purc_get_stack();
        value = pcvcm_eval(vcm, stack);
        if (value == PURC_VARIANT_INVALID) {
            return -1;
        }
    }

    const char *s = purc_variant_get_string_const(value);
    fprintf(stderr, "==%s[%d]:%s()==[%s/%s]\n",
            __FILE__, __LINE__, __func__,
            attr->key, s);

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

    pcintr_stack_t stack = purc_get_stack();
    purc_variant_t v = pcvcm_eval(vcm_content, stack);
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
        frame->ops.after_pushed(co, frame);
        return;
    }

    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
on_popping(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    if (frame->ops.on_popping) {
        frame->ops.on_popping(co, frame);
        return;
    }

    pcintr_stack_t stack = co->stack;
    pop_stack_frame(stack);
    co->state = CO_STATE_READY;
}

static void
on_rerun(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    if (frame->ops.rerun) {
        frame->ops.rerun(co, frame);
        return;
    }

    frame->next_step = NEXT_STEP_SELECT_CHILD;
    co->state = CO_STATE_READY;
}

static void
on_select_child(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    if (frame->ops.select_child) {
        frame->ops.select_child(co, frame);
        return;
    }

    frame->next_step = NEXT_STEP_ON_POPPING;
    co->state = CO_STATE_READY;
}

static void
run_coroutine(pcintr_coroutine_t co)
{
    pcintr_stack_t stack = co->stack;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    if (frame->preemptor) {
        preemptor_f preemptor = frame->preemptor;
        frame->preemptor = NULL;
        preemptor(co, frame);
        return;
    }

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

    if (co->waits)
        return;
    struct list_head *frames = &stack->frames;
    if (!list_empty(frames))
        return;
    co->state = CO_STATE_TERMINATED;
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
                    pcvariant_push_gc();
                    run_coroutine(co);
                    pcvariant_pop_gc();
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
                list_del(&co->node);
                stack_release(co->stack);
                free(co->stack);
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

static struct pcintr_element_ops default_ops = {
//     .after_pushed = default_element_pushed,
//     .on_popping   = default_element_popping,
//     .rerun        = NULL,
//     .select_child = default_element_select_child,
};

struct pcintr_element_ops*
pcintr_get_element_ops(pcvdom_element_t element)
{
    PC_ASSERT(element);

    switch (element->tag_id) {
        // case PCHVML_TAG_HVML:
        //     return pcintr_hvml_get_ops();
        // case PCHVML_TAG_ITERATE:
        //     return pcintr_iterate_get_ops();
        default:
            return &default_ops;
    }
}

struct frame_element
{
    struct pcintr_stack_frame     *frame;
    struct pcvdom_element         *element;
};

// static purc_variant_t
// fed_eval_attr(struct frame_element *fed,
//         enum pchvml_attr_assignment  op,
//         struct pcvcm_node           *val)
// {
//     UNUSED_PARAM(fed);
//     UNUSED_PARAM(op);
//     UNUSED_PARAM(val);
//     PC_ASSERT(0); // Not implemented yet
//     return PURC_VARIANT_INVALID;
// }

// static int
// init_frame_append_attr(void *key, void *val, void *ud)
// {
//     PC_ASSERT(ud);
// 
//     struct frame_element *fed = (struct frame_element*)ud;
// 
//     purc_variant_t attr_vars = fed->frame->attr_vars;
// 
//     struct pcvdom_attr *attr = (struct pcvdom_attr*)val;
//     PC_ASSERT(key == attr->key);
//     enum pchvml_attr_assignment  op   = attr->op;
//     struct pcvcm_node           *node = attr->val;
// 
//     purc_variant_t k = purc_variant_make_string(attr->key, true);
//     purc_variant_t v = fed_eval_attr(fed, op, node);
// 
//     PC_ASSERT(k!=PURC_VARIANT_INVALID);
//     PC_ASSERT(v!=PURC_VARIANT_INVALID);
// 
//     purc_variant_t o = purc_variant_make_object(1, k, v);
//     purc_variant_unref(k);
//     purc_variant_unref(v);
//     PC_ASSERT(o!=PURC_VARIANT_INVALID);
// 
//     bool ok = purc_variant_array_append(attr_vars, o);
//     purc_variant_unref(o);
// 
//     PC_ASSERT(ok);
// 
//     return 0;
// }

// static int
// init_frame_attr_vars(struct pcintr_stack_frame *frame,
//         struct pcvdom_element *element)
// {
//     frame->attr_vars = purc_variant_make_object(0,
//             PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
// 
//     if (frame->attr_vars == PURC_VARIANT_INVALID)
//         return -1;
// 
//     struct pcutils_map *attrs = element->attrs;
//     if (!attrs)
//         return 0;
// 
//     struct frame_element ud = {
//         .frame          = frame,
//         .element        = element,
//     };
// 
//     int r = pcutils_map_traverse(attrs, &ud, init_frame_append_attr);
// 
//     return r ? -1 : 0;
// }
// 
// static int
// init_frame_by_element(struct pcintr_stack_frame *frame,
//         struct pcvdom_element *element)
// {
//     frame->scope = element; // FIXME: archetype, where to store `scope`
//     frame->pos = element;
// 
//     if (init_frame_attr_vars(frame, element))
//         return -1;
// 
//     // TODO:
//     // frame->ctnt_vars = ????;
//     return 0;
// }

// static int
// element_eval_in_frame(struct pcvdom_element *element,
//         struct pcintr_element_ops *ops,
//         pcintr_stack_t stack,
//         struct pcintr_stack_frame *frame)
// {
//     PC_ASSERT(element);
// 
//     int r = init_frame_by_element(frame, element);
//     if (r) {
//         return -1;
//     }
// 
//     if (ops->after_pushed) {
//         void *ctxt = ops->after_pushed(stack, element);
//         frame->ctxt = ctxt;
//     }
// 
// rerun:
//     if (ops->select_child) {
//         struct pcvdom_element *child;
//         child = ops->select_child(stack, frame->ctxt);
//         while (child) {
//             r = element_eval(child);
//             PC_ASSERT(r==0); // TODO: what if failed????
//             child = ops->select_child(stack, frame->ctxt);
//         }
//     }
// 
//     if (ops->on_popping) {
//         bool ok = ops->on_popping(stack, frame->ctxt);
//         if (ok) {
//             return 0;
//         }
//     }
// 
//     if (ops->rerun) {
//         bool ok = ops->rerun(stack, frame->ctxt);
//         PC_ASSERT(ok); // TODO: what if failed????
//         goto rerun;
//     }
// 
//     return 0;
// }

// static int
// element_eval(struct pcvdom_element *element)
// {
//     PC_ASSERT(element);
// 
//     struct pcintr_element_ops *ops;
//     ops = pcintr_get_element_ops(element);
//     if (!ops)
//         return 0;
// 
//     pcintr_stack_t stack = purc_get_stack();
//     PC_ASSERT(stack);
// 
//     struct pcintr_stack_frame *frame;
//     frame = push_stack_frame(stack);
//     if (!frame)
//         return -1;
// 
//     int r = element_eval_in_frame(element, ops, stack, frame);
// 
//     pop_stack_frame(stack);
// 
//     return r ? -1 : 0;
// }

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
    frame = push_stack_frame(stack);
    if (!frame) {
        stack_release(stack);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    // frame->next_step = on_vdom_start;
    frame->ops = pcintr_get_document_ops();

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

bool
pcintr_bind_buildin_variable(struct pcvdom_document* doc, const char* name,
        purc_variant_t variant)
{
    return pcvdom_document_bind_variable(doc, name, variant);
}

bool
pcintr_unbind_buildin_variable(struct pcvdom_document* doc,
        const char* name)
{
    return pcvdom_document_unbind_variable(doc, name);
}

bool
pcintr_bind_scope_variable(pcvdom_element_t elem, const char* name,
        purc_variant_t variant)
{
    return pcvdom_element_bind_variable(elem, name, variant);
}

bool
pcintr_unbind_scope_variable(pcvdom_element_t elem, const char* name)
{
    return pcvdom_element_unbind_variable(elem, name);
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
                stack->special_observer_list =  pcutils_arrlist_new(NULL);
            }
            list = stack->special_observer_list;
            break;

        case PCINTR_OBSERVER_TYPE_NATIVE:
            if (!stack->native_observer_list) {
                stack->native_observer_list =  pcutils_arrlist_new(NULL);
            }
            list = stack->native_observer_list;
            break;

        case PCINTR_OBSERVER_TYPE_COMMON:
        default:
            if (!stack->common_observer_list) {
                stack->common_observer_list =  pcutils_arrlist_new(NULL);
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
    free(observer->msg_type);
    free(observer->sub_type);
    free(observer);
    return true;
}
