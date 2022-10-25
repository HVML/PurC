/*
 * @file coroutine.c
 * @author Vincent Wei
 * @date 2022/07/03
 * @brief The management of coroutine-level variables.
 *  Derived from interperter.c and ../vdom/vdom.c authored by
 *      Xu Xiaohong and Xue Shuming.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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
 */

#include "purc.h"

#include "private/errors.h"
#include "private/debug.h"
#include "private/interpreter.h"
#include "private/vdom.h"
#include "private/instance.h"

static int
cmp_f(struct rb_node *node, void *ud)
{
    pcvarmgr_t mgr = container_of(node, struct pcvarmgr, node);
    PC_ASSERT(mgr->vdom_node);
    PC_ASSERT(ud);
    struct pcvdom_node *v = (struct pcvdom_node*)ud;
    if (mgr->vdom_node < v)
        return -1;
    if (mgr->vdom_node > v)
        return 1;
    return 0;
}

static struct rb_node*
new_varmgr(void *ud)
{
    PC_ASSERT(ud);
    struct pcvdom_node *v = (struct pcvdom_node*)ud;

    pcvarmgr_t mgr = pcvarmgr_create();
    if (!mgr)
        return NULL;

    mgr->vdom_node = v;

    return &mgr->node;
}

static pcvarmgr_t
create_scoped_variables(purc_coroutine_t cor, struct pcvdom_node *node)
{
    pcintr_stack_t stack = &cor->stack;

    /* vdom level manage by coroutine */
    if (node == (void*)stack->vdom) {
        return stack->co->variables;
    }

    struct rb_node *p;
    int r = pcutils_rbtree_insert_or_get(&stack->scoped_variables, node,
            cmp_f, new_varmgr, &p);
    if (r) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    PC_ASSERT(p);

    return container_of(p, struct pcvarmgr, node);
}

bool
pcintr_bind_scope_variable(purc_coroutine_t cor, struct pcvdom_element *elem,
        const char *name, purc_variant_t variant, pcvarmgr_t *mgr)
{
    if (!cor || !elem) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    struct pcvdom_node *node = pcvdom_ele_cast_to_node(elem);
    pcvarmgr_t scoped_variables = create_scoped_variables(cor, node);
    if (!scoped_variables)
        return false;

    if (mgr) {
        *mgr = scoped_variables;
    }

    if (!name || !variant) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    return pcvarmgr_add(scoped_variables, name, variant);
}

bool
pcintr_unbind_scope_variable(purc_coroutine_t cor, struct pcvdom_element *elem,
        const char *name)
{
    if (!cor || !elem || !name) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    pcvarmgr_t scoped_variables = pcintr_get_scoped_variables(cor,
            pcvdom_ele_cast_to_node(elem));
    if (!scoped_variables)
        return false;

    return pcvarmgr_remove(scoped_variables, name);
}

purc_variant_t
pcintr_get_scope_variable(purc_coroutine_t cor, struct pcvdom_element *elem,
        const char *name)
{
    if (!elem || !name) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_t scoped_variables = pcintr_get_scoped_variables(cor,
            pcvdom_ele_cast_to_node(elem));
    if (!scoped_variables)
        return PURC_VARIANT_INVALID;

    return pcvarmgr_get(scoped_variables, name);
}

pcvarmgr_t
pcintr_get_scoped_variables(purc_coroutine_t cor, struct pcvdom_node *node)
{
    PC_ASSERT(node);
    pcintr_stack_t stack = &cor->stack;

    /* vdom level manage by coroutine */
    if (node == (void *)stack->vdom) {
        return cor->variables;
    }

    struct rb_node *p;
    struct rb_node *first = pcutils_rbtree_first(&stack->scoped_variables);
    pcutils_rbtree_for_each(first, p) {
        pcvarmgr_t mgr = container_of(p, struct pcvarmgr, node);
        if (mgr->vdom_node == node)
            return mgr;
    }

    return NULL;
}

bool
purc_coroutine_bind_variable(purc_coroutine_t cor, const char *name,
        purc_variant_t variant)
{
    if (!cor || !cor->vdom || !name || !variant) {
        PC_ASSERT(0);
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    return pcvarmgr_add(cor->variables, name, variant);
}

bool
purc_coroutine_unbind_variable(purc_coroutine_t cor, const char *name)
{
    if (!cor || !cor->vdom || !name) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    return pcvarmgr_remove(cor->variables, name);
}

purc_variant_t
purc_coroutine_get_variable(purc_coroutine_t cor, const char *name)
{
    if (!cor || !cor->vdom || !name) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return pcvarmgr_get(cor->variables, name);
}

void *
purc_coroutine_set_user_data(purc_coroutine_t cor, void *user_data)
{
    void *old = cor->user_data;
    cor->user_data = user_data;
    return old;
}

void *
purc_coroutine_get_user_data(purc_coroutine_t cor)
{
    return cor->user_data;
}

purc_atom_t
purc_coroutine_identifier(purc_coroutine_t cor)
{
    return cor->cid;
}

static
pcintr_coroutine_t
get_coroutine_by_id(struct pcinst *inst, purc_atom_t id)
{
    struct pcintr_heap *heap = inst->intr_heap;
    struct list_head *crtns = &heap->crtns;
    pcintr_coroutine_t p, q;
    list_for_each_entry_safe(p, q, crtns, ln) {
        if (p->cid == id) {
            return p;
        }
    }

    crtns = &heap->stopped_crtns;
    list_for_each_entry_safe(p, q, crtns, ln) {
        if (p->cid == id) {
            return p;
        }
    }

    return NULL;
}

pcintr_coroutine_t
pcintr_coroutine_get_by_id(purc_atom_t id)
{
    struct pcinst *inst = pcinst_current();
    if (!inst) {
        return NULL;
    }
    return get_coroutine_by_id(inst, id);
}

