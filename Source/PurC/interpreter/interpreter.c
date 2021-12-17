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

#include "private/debug.h"
#include "private/instance.h"
#include "private/interpreter.h"
#include "private/runloop.h"

#include "element-ops.h"
#include "../hvml/hvml-gen.h"

void pcintr_stack_init_once(void)
{
    pcrunloop_init_main();
    pcrunloop_t runloop = pcrunloop_get_main();
    PC_ASSERT(runloop);
}

void pcintr_stack_init_instance(struct pcinst* inst)
{
    INIT_LIST_HEAD(&inst->coroutines);
    inst->running_coroutine = NULL;

    // struct pcintr_stack *intr_stack = &inst->intr_stack;
    // memset(intr_stack, 0, sizeof(*intr_stack));

    // struct list_head *frames = &intr_stack->frames;
    // INIT_LIST_HEAD(frames);
    // intr_stack->ret_var = PURC_VARIANT_INVALID;
}

static inline void
stack_frame_release(struct pcintr_stack_frame *frame)
{
    frame->scope = NULL;
    frame->pos   = NULL;

    frame->ctxt  = NULL;

    for (size_t i=0; i<PCA_TABLESIZE(frame->symbol_vars); ++i) {
        PURC_VARIANT_SAFE_CLEAR(frame->symbol_vars[i]);
    }

    PURC_VARIANT_SAFE_CLEAR(frame->attr_vars);

    PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);

    PURC_VARIANT_SAFE_CLEAR(frame->mid_vars);
}

static inline void
vdom_release(purc_vdom_t vdom)
{
    if (vdom->document) {
        pcvdom_document_destroy(vdom->document);
        vdom->document = NULL;
    }
}

static inline void
vdom_destroy(purc_vdom_t vdom)
{
    if (!vdom)
        return;

    vdom_release(vdom);

    free(vdom);
}

static inline void
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
}

static inline void
stack_init(pcintr_stack_t stack)
{
    INIT_LIST_HEAD(&stack->frames);
}

void pcintr_stack_cleanup_instance(struct pcinst* inst)
{
    struct list_head *coroutines = &inst->coroutines;
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


static inline pcintr_coroutine_t
coroutine_get_current(void)
{
    struct pcinst *inst = pcinst_current();
    return inst->running_coroutine;
}

pcintr_stack_t purc_get_stack(void)
{
    struct pcintr_coroutine *co = coroutine_get_current();
    if (!co)
        return NULL;

    return co->stack;
}

static inline void
run_coroutine(pcintr_coroutine_t co)
{
    co->execute(co);
}

static inline int run_coroutines(void *ctxt)
{
    UNUSED_PARAM(ctxt);

    fprintf(stderr, "===%s[%d]===\n", __FILE__, __LINE__);
    struct pcinst *inst = pcinst_current();
    struct list_head *coroutines = &inst->coroutines;
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
                    run_coroutine(co);
                    ++readies;
                    break;
                case CO_STATE_WAIT:
                    ++waits;
                    break;
                default:
                    PC_ASSERT(0);
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

static inline struct pcintr_stack_frame*
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

struct pcintr_element_ops*
pcintr_get_element_ops(pcvdom_element_t element)
{
    PC_ASSERT(element);

    switch (element->tag_id) {
        case PCHVML_TAG_HVML:
            return pcintr_hvml_get_ops();
        case PCHVML_TAG_ITERATE:
            return pcintr_iterate_get_ops();
        default:
            fprintf(stderr, "==tag_id:%d==\n", element->tag_id);
            PC_ASSERT(0); // Not implemented yet
            return NULL;
    }
}

struct frame_element
{
    struct pcintr_stack_frame     *frame;
    struct pcvdom_element         *element;
};

// static inline purc_variant_t
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

// static inline int
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

// static inline int
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
// static inline int
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

static inline void
comment_eval(struct pcvdom_comment *comment)
{
    UNUSED_PARAM(comment);
}

static inline void
content_eval(struct pcvdom_content *content)
{
    UNUSED_PARAM(content);
    abort();
}

static inline void
element_eval(struct pcvdom_element *element)
{
    UNUSED_PARAM(element);
}

// static inline int
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

// static inline int
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

static inline void
doctype_eval(struct pcvdom_doctype *doctype)
{
    const char *system_info = doctype->system_info;
    fprintf(stderr, "system_info: [%s]\n", system_info);
}

static inline void
children_eval(struct pcvdom_element *parent)
{
    struct pcvdom_node *p;
    p = pcvdom_node_first_child(&parent->node);
    for (; p; p = pcvdom_node_next_sibling(p)) {
        switch (p->type) {
            case PCVDOM_NODE_COMMENT:
                comment_eval(PCVDOM_COMMENT_FROM_NODE(p));
                break;
            case PCVDOM_NODE_CONTENT:
                content_eval(PCVDOM_CONTENT_FROM_NODE(p));
                break;
            case PCVDOM_NODE_ELEMENT:
                element_eval(PCVDOM_ELEMENT_FROM_NODE(p));
                break;
            default:
                PC_ASSERT(0);
        }
    }
}

static inline void
head_eval(struct pcvdom_element *head)
{
    children_eval(head);
}

static inline void
body_eval(struct pcvdom_element *body)
{
    children_eval(body);
}

static inline void
hvml_eval(struct pcvdom_element *hvml)
{
    pcintr_stack_t stack = purc_get_stack();
    PC_ASSERT(stack);

    struct pcvdom_element *p;

    p = pcvdom_element_first_child_element(hvml);
    for (; p; p = pcvdom_element_next_sibling_element(p)) {
        pcvdom_tag_id tag_id = p->tag_id;
        if (tag_id != PCHVML_TAG_HEAD)
            continue;

        struct pcintr_stack_frame *frame;
        frame = push_stack_frame(stack);
        PC_ASSERT(frame);
        frame->scope = hvml;
        head_eval(p);
        if (pcintr_stack_is_waiting(stack))
            return;
        pop_stack_frame(stack);
    }

    p = pcvdom_element_first_child_element(hvml);
    for (; p; p = pcvdom_element_next_sibling_element(p)) {
        pcvdom_tag_id tag_id = p->tag_id;
        if (tag_id != PCHVML_TAG_BODY)
            continue;

        struct pcintr_stack_frame *frame;
        frame = push_stack_frame(stack);
        PC_ASSERT(frame);
        frame->scope = hvml;
        // TODO: check id
        body_eval(p);
        if (pcintr_stack_is_waiting(stack))
            return;
        pop_stack_frame(stack);
    }
}

static inline void
document_eval(struct pcvdom_document *document)
{
    pcintr_stack_t stack = purc_get_stack();
    PC_ASSERT(stack);

    struct pcvdom_doctype *doctype = &document->doctype;
    doctype_eval(doctype);
    if (pcintr_stack_is_waiting(stack))
        return;

    struct pcintr_stack_frame *frame;
    frame = push_stack_frame(stack);
    PC_ASSERT(frame);

    struct pcvdom_element *hvml = document->root;
    PC_ASSERT(hvml);
    hvml_eval(hvml);
    if (pcintr_stack_is_waiting(stack))
        return;

    pop_stack_frame(stack);
    return;

    // struct pcvdom_node *node = &document->node;
    // struct pcvdom_node *p = pcvdom_node_first_child(node);

    // struct pcvdom_element *element;
    // int r = 0;
    // for (; p; p = pcvdom_node_next_sibling(p)) {
    //     switch (p->type) {
    //         case PCVDOM_NODE_DOCUMENT:
    //             PC_ASSERT(0);
    //             return -1;
    //         case PCVDOM_NODE_ELEMENT:
    //             element = container_of(p, struct pcvdom_element, node);
    //             r = element_eval(document, element);
    //             break;
    //         case PCVDOM_NODE_CONTENT:
    //             // FIXME: output to edom?
    //             break;
    //         case PCVDOM_NODE_COMMENT:
    //             // FIXME:
    //             break;
    //         default:
    //             PC_ASSERT(0);
    //             return -1;
    //     }
    //     if (r)
    //         return -1;
    // }

    // return -1;
}

static inline void
vdom_eval(purc_vdom_t vdom)
{
    PC_ASSERT(vdom);
    pcintr_stack_t stack = purc_get_stack();
    PC_ASSERT(stack->nr_frames == 0);
    PC_ASSERT(stack->except == 0);

    struct pcvdom_document *document = vdom->document;
    document_eval(document);
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

static inline struct pcvdom_document*
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

static inline void vdom_main(pcintr_coroutine_t co)
{
    PC_ASSERT(co->state == CO_STATE_RUN);
    pcintr_stack_t stack = co->stack;
    PC_ASSERT(stack);

    list_del(&co->node);
    // TODO: who's responsible to free resources???
    stack_release(stack);
    free(stack);

    // vdom_eval(vdom);
    // if (pcintr_stack_is_waiting(stack))
    //     return 0;

    // PC_ASSERT(stack->nr_frames == 0);

    // stack->state |= STACK_STATE_TERMINATED;

    // if (pcintr_stack_is_terminated(stack)) {
    //     pcvdom_document_destroy(vdom->document);
    //     free(vdom);

    //     pcrunloop_t runloop = pcrunloop_get_current();
    //     PC_ASSERT(runloop);
    //     pcrunloop_stop(runloop);
    // }

    // return 0;
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
    stack->co.execute = vdom_main;

    struct pcinst *inst = pcinst_current();
    struct list_head *coroutines = &inst->coroutines;
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

static inline bool
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
