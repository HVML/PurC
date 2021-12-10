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

#include "iterate.h"

void pcintr_stack_init_once(void)
{
}

void pcintr_stack_init_instance(struct pcinst* inst)
{
    struct pcintr_stack *intr_stack = &inst->intr_stack;
    memset(intr_stack, 0, sizeof(*intr_stack));

    struct list_head *frames = &intr_stack->frames;
    INIT_LIST_HEAD(frames);
    intr_stack->ret_var = PURC_VARIANT_INVALID;
}

static inline void
intr_stack_frame_release(struct pcintr_stack_frame *frame)
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

void pcintr_stack_cleanup_instance(struct pcinst* inst)
{
    struct pcintr_stack *intr_stack = &inst->intr_stack;
    struct list_head *frames = &intr_stack->frames;
    if (!list_empty(frames)) {
        struct pcintr_stack_frame *p, *n;
        list_for_each_entry_safe(p, n, frames, node) {
            list_del(&p->node);
            --intr_stack->nr_frames;
            intr_stack_frame_release(p);
            free(p);
        }
        PC_ASSERT(intr_stack->nr_frames == 0);
    }
}

pcintr_stack_t purc_get_stack(void)
{
    struct pcinst *inst = pcinst_current();
    struct pcintr_stack *intr_stack = &inst->intr_stack;
    return intr_stack;
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

    return 0;
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

    intr_stack_frame_release(frame);
    free(frame);
    --stack->nr_frames;
}

struct pcintr_element_ops*
pcintr_get_element_ops(pcvdom_element_t element)
{
    PC_ASSERT(element);

    switch (element->tag_id) {
        case PCHVML_TAG_ITERATE:
            return pcintr_iterate_get_ops();
        default:
            return NULL;
    }
}

struct frame_element_doc
{
    struct pcintr_stack_frame     *frame;
    struct pcvdom_element         *element;
    struct pcvdom_document        *document;
};

static inline purc_variant_t
fed_eval_attr(struct frame_element_doc *fed,
        enum pchvml_attr_assignment  op,
        struct pcvcm_node           *val)
{
    UNUSED_PARAM(fed);
    UNUSED_PARAM(op);
    UNUSED_PARAM(val);
    PC_ASSERT(0); // Not implemented yet
    return PURC_VARIANT_INVALID;
}

static inline int
init_frame_append_attr(void *key, void *val, void *ud)
{
    PC_ASSERT(ud);

    struct frame_element_doc *fed = (struct frame_element_doc*)ud;

    purc_variant_t attr_vars = fed->frame->attr_vars;

    struct pcvdom_attr *attr = (struct pcvdom_attr*)val;
    PC_ASSERT(key == attr->key);
    enum pchvml_attr_assignment  op   = attr->op;
    struct pcvcm_node           *node = attr->val;

    purc_variant_t k = purc_variant_make_string(attr->key, true);
    purc_variant_t v = fed_eval_attr(fed, op, node);

    PC_ASSERT(k!=PURC_VARIANT_INVALID);
    PC_ASSERT(v!=PURC_VARIANT_INVALID);

    purc_variant_t o = purc_variant_make_object(1, k, v);
    purc_variant_unref(k);
    purc_variant_unref(v);
    PC_ASSERT(o!=PURC_VARIANT_INVALID);

    bool ok = purc_variant_array_append(attr_vars, o);
    purc_variant_unref(o);

    PC_ASSERT(ok);

    return 0;
}

static inline int
init_frame_attr_vars(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element, struct pcvdom_document *document)
{
    UNUSED_PARAM(document);

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return -1;

    struct pcutils_map *attrs = element->attrs;
    if (!attrs)
        return 0;

    struct frame_element_doc ud = {
        .frame          = frame,
        .element        = element,
        .document       = document,
    };

    int r = pcutils_map_traverse(attrs, &ud, init_frame_append_attr);

    return r ? -1 : 0;
}

static inline int
init_frame_by_element(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element, struct pcvdom_document *document)
{
    frame->scope = element; // FIXME: archetype, where to store `scope`
    frame->pos = element;

    if (init_frame_attr_vars(frame, element, document))
        return -1;

    // TODO:
    // frame->ctnt_vars = ????;
    return 0;
}

static inline int
element_post_load(struct pcvdom_document *document,
        struct pcvdom_element *element);

static inline int
element_post_load_in_frame(struct pcvdom_document *document,
        struct pcvdom_element *element,
        struct pcintr_element_ops *ops,
        pcintr_stack_t stack,
        struct pcintr_stack_frame *frame)
{
    PC_ASSERT(document);
    PC_ASSERT(element);

    int r = init_frame_by_element(frame, element, document);
    if (r) {
        pop_stack_frame(stack);
        return -1;
    }

    if (ops->after_pushed) {
        void *ctxt = ops->after_pushed(stack, element);
        frame->ctxt = ctxt;
    }

rerun:
    if (ops->select_child) {
        struct pcvdom_element *child;
        child = ops->select_child(stack, frame->ctxt);
        while (child) {
            r = element_post_load(document, child);
            PC_ASSERT(r==0); // TODO: what if failed????
            child = ops->select_child(stack, frame->ctxt);
        }
    }

    if (ops->on_popping) {
        bool ok = ops->on_popping(stack, frame->ctxt);
        if (ok) {
            return 0;
        }
    }

    if (ops->rerun) {
        bool ok = ops->rerun(stack, frame->ctxt);
        PC_ASSERT(ok); // TODO: what if failed????
        goto rerun;
    }

    return 0;
}

static inline int
element_post_load(struct pcvdom_document *document,
        struct pcvdom_element *element)
{
    PC_ASSERT(document);
    PC_ASSERT(element);

    struct pcintr_element_ops *ops;
    ops = pcintr_get_element_ops(element);
    if (!ops)
        return 0;

    pcintr_stack_t stack = purc_get_stack();
    PC_ASSERT(stack);

    struct pcintr_stack_frame *frame;
    frame = push_stack_frame(stack);
    if (!frame)
        return -1;

    int r = element_post_load_in_frame(document, element, ops, stack, frame);

    pop_stack_frame(stack);

    return r ? -1 : 0;
}

static inline int
document_post_load(struct pcvdom_document *document)
{
    struct pcvdom_node *node = &document->node;
    struct pcvdom_node *p = pcvdom_node_first_child(node);
    (void)p;
    PC_ASSERT(0); // Not implemented yet

    struct pcvdom_element *element;
    int r = 0;
    for (; p; p = pcvdom_node_next_sibling(p)) {
        switch (p->type) {
            case PCVDOM_NODE_DOCUMENT:
                PC_ASSERT(0);
                return -1;
            case PCVDOM_NODE_ELEMENT:
                element = container_of(p, struct pcvdom_element, node);
                r = element_post_load(document, element);
                break;
            case PCVDOM_NODE_CONTENT:
                // FIXME: output to edom?
                break;
            case PCVDOM_NODE_COMMENT:
                // FIXME:
                break;
            default:
                PC_ASSERT(0);
                return -1;
        }
        if (r)
            return -1;
    }

    return -1;
}

int pcintr_post_load(purc_vdom_t vdom)
{
    PC_ASSERT(vdom);
    pcintr_stack_t stack = purc_get_stack();
    PC_ASSERT(stack->nr_frames == 0);
    PC_ASSERT(stack->except == 0);

    struct pcvdom_document *document = vdom->document;
    return document_post_load(document);
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
