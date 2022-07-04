/*
 * @file cor-vars.c
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

pcvarmgr_t
pcintr_create_scoped_variables(struct pcvdom_node *node)
{
    PC_ASSERT(node);
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);

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
        const char *name, purc_variant_t variant)
{
    if (!cor || !elem || !name || !variant) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    struct pcvdom_node *node = pcvdom_ele_cast_to_node(elem);
    pcvarmgr_t scoped_variables = pcintr_create_scoped_variables(node);
    if (!scoped_variables)
        return false;

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
    pcintr_stack_t stack = pcintr_get_stack();
    PC_ASSERT(stack);
    PC_ASSERT(stack->co == cor);

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

    struct pcvdom_node *node = pcvdom_doc_cast_to_node(cor->vdom);
    pcvarmgr_t scoped_variables = pcintr_create_scoped_variables(node);
    if (!scoped_variables)
        return false;

    bool b = pcvarmgr_add(scoped_variables, name, variant);
    return b;
}

static pcvarmgr_t
purc_coroutine_get_varmgr(purc_coroutine_t cor)
{
    if (!cor || !cor->vdom) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct pcvdom_node *node = pcvdom_doc_cast_to_node(cor->vdom);
    return pcintr_get_scoped_variables(cor, node);
}

bool
purc_coroutine_unbind_variable(purc_coroutine_t cor, const char *name)
{
    if (!cor || !cor->vdom || !name) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return false;
    }

    pcvarmgr_t scoped_variables = purc_coroutine_get_varmgr(cor);
    if (!scoped_variables)
        return false;

    return pcvarmgr_remove(scoped_variables, name);
}

purc_variant_t
purc_coroutine_get_variable(purc_coroutine_t cor, const char *name)
{
    if (!cor || !cor->vdom || !name) {
        PC_ASSERT(0);
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_t scoped_variables = purc_coroutine_get_varmgr(cor);
    if (!scoped_variables)
        return false;

    return pcvarmgr_get(scoped_variables, name);
}

