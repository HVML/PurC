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
#include "private/regex.h"
#include "internal.h"

#define HVML_CRTN_TOKEN_REGEX "^[A-Za-z0-9_]+$"

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

bool
pcintr_is_valid_crtn_token(const char *token)
{
    return pcregex_is_match(HVML_CRTN_TOKEN_REGEX, token);
}

const char *
pcintr_coroutine_get_token(pcintr_coroutine_t cor)
{
    return cor->token;
}

int
pcintr_coroutine_set_token(pcintr_coroutine_t cor, const char *token)
{
    int ret = -1;
    if (!token) {
        goto out;
    }

    size_t nr = strlen(token);
    if (nr > CRTN_TOKEN_LEN) {
        purc_set_error(PURC_ERROR_TOO_LONG);
        goto out;
    }

    if (!pcintr_is_valid_crtn_token(token)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    pcintr_heap_t heap = pcintr_get_heap();
    if (pcutils_map_insert(heap->token_crtn_map, token, cor)) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    pcutils_map_erase(heap->token_crtn_map, cor->token);
    strcpy(cor->token, token);
    ret = 0;

out:
    return ret;
}

struct pcintr_coroutine_rdr_conn *
pcintr_coroutine_get_rdr_conn(pcintr_coroutine_t cor,
        struct pcrdr_conn *conn)
{
    struct pcintr_coroutine_rdr_conn *ret = NULL;
    if (!cor) {
        goto out;
    }

    struct list_head *conns = &cor->conns;
    struct pcintr_coroutine_rdr_conn *p;
    struct pcintr_coroutine_rdr_conn *q;
    list_for_each_entry_safe(p, q, conns, ln) {
        if (p->conn == conn) {
            ret = p;
            goto out;
        }
    }

out:
    return ret;
}

struct pcintr_coroutine_rdr_conn *
pcintr_coroutine_create_or_get_rdr_conn(pcintr_coroutine_t cor,
        struct pcrdr_conn *conn)
{
    struct pcintr_coroutine_rdr_conn *ret = NULL;
    ret = pcintr_coroutine_get_rdr_conn(cor, conn);
    if (ret) {
        goto out;
    }

    ret = (struct pcintr_coroutine_rdr_conn *)calloc(1, sizeof(*ret));
    ret->conn = conn;
    list_add_tail(&ret->ln, &cor->conns);

out:
    return ret;
}

bool
pcintr_coroutine_is_match_page_handle(pcintr_coroutine_t cor, uint64_t handle)
{
    struct list_head *conns = &cor->conns;
    struct pcintr_coroutine_rdr_conn *p;
    struct pcintr_coroutine_rdr_conn *q;
    list_for_each_entry_safe(p, q, conns, ln) {
        if (handle == p->page_handle) {
            return true;
        }
    }
    return false;
}

bool
pcintr_coroutine_is_match_dom_handle(pcintr_coroutine_t cor, uint64_t handle)
{
    struct list_head *conns = &cor->conns;
    struct pcintr_coroutine_rdr_conn *p;
    struct pcintr_coroutine_rdr_conn *q;
    list_for_each_entry_safe(p, q, conns, ln) {
        if (handle == p->dom_handle) {
            return true;
        }
    }
    return false;
}

bool
pcintr_coroutine_is_rdr_attached(pcintr_coroutine_t cor)
{
    struct list_head *conns = &cor->conns;
    struct pcintr_coroutine_rdr_conn *p;
    struct pcintr_coroutine_rdr_conn *q;
    list_for_each_entry_safe(p, q, conns, ln) {
        if (p->page_handle) {
            return true;
        }
    }
    return false;
}

pcintr_coroutine_t
pcintr_get_first_crtn(struct pcinst *inst)
{
    pcintr_coroutine_t crtn = NULL;
    pcintr_heap_t heap = inst->intr_heap;
    struct pcutils_map_iterator it;
    it = pcutils_map_it_begin_first(heap->token_crtn_map);
    pcutils_map_entry *entry = pcutils_map_it_value(&it);
    if (entry) {
        crtn = (pcintr_coroutine_t) entry->val;
    }
    pcutils_map_it_end(&it);
    return crtn;
}

pcintr_coroutine_t
pcintr_get_last_crtn(struct pcinst *inst)
{
    pcintr_coroutine_t crtn = NULL;
    pcintr_heap_t heap = inst->intr_heap;
    struct pcutils_map_iterator it;
    it = pcutils_map_it_begin_last(heap->token_crtn_map);
    pcutils_map_entry *entry = pcutils_map_it_value(&it);
    if (entry) {
        crtn = (pcintr_coroutine_t) entry->val;
    }
    pcutils_map_it_end(&it);
    return crtn;
}

pcintr_coroutine_t
pcintr_get_main_crtn(struct pcinst *inst)
{
    pcintr_coroutine_t ret = NULL;
    struct pcutils_map_iterator it;
    pcutils_map_entry *entry;

    pcintr_heap_t heap = inst->intr_heap;
    it = pcutils_map_it_begin_first(heap->token_crtn_map);
    while ((entry = pcutils_map_it_value(&it))) {
        pcintr_coroutine_t crtn = (pcintr_coroutine_t) entry->val;
        if (crtn->is_main) {
            ret = crtn;
            break;
        }
        pcutils_map_it_next(&it);
    }
    pcutils_map_it_end(&it);

    return ret;
}

pcintr_coroutine_t
pcintr_get_crtn_by_token(struct pcinst *inst, const char *token)
{
    UNUSED_PARAM(inst);
    UNUSED_PARAM(token);
    if (!token) {
        return NULL;
    }
    if (strcmp(token, CRTN_TOKEN_MAIN) == 0) {
        return pcintr_get_main_crtn(inst);
    }
    else if (strcmp(token, CRTN_TOKEN_FIRST) == 0) {
        return pcintr_get_first_crtn(inst);
    }
    else if (strcmp(token, CRTN_TOKEN_LAST) == 0) {
        return pcintr_get_last_crtn(inst);
    }
    pcintr_heap_t heap = inst->intr_heap;
    struct pcutils_map_entry *entry;
    entry = pcutils_map_find(heap->token_crtn_map, token);
    if (entry) {
        return (pcintr_coroutine_t) entry->val;
    }
    return NULL;
}

bool
pcintr_register_crtn_to_doc(struct pcinst *inst, pcintr_coroutine_t co)
{
    pcintr_heap_t heap = inst->intr_heap;
    if (pcutils_sorted_array_add(heap->loaded_crtn_handles, co,
            co->stack.doc, NULL)) {
        purc_log_warn("Failed to register coroutine as a loaded one: %p\n", co);
        return false;
    }

    list_add(&co->doc_node, &co->stack.doc->owner_list);
    co->stack.doc->ldc++;

    return true;
}

void
pcintr_inherit_udom_handle(struct pcinst *inst, pcintr_coroutine_t co)
{
    (void)inst;

    struct pcintr_coroutine_rdr_conn *rdr_conn = NULL;
    rdr_conn = pcintr_coroutine_get_rdr_conn(co, inst->conn_to_rdr);

    if (rdr_conn->dom_handle) {
        co->stack.doc->udom = rdr_conn->dom_handle;
    }

    /* inherit the udom handle to others sharing the document */
    if (co->stack.doc->udom) {
        pcintr_coroutine_t p;
        list_for_each_entry(p, &co->stack.doc->owner_list, doc_node) {
            rdr_conn = pcintr_coroutine_get_rdr_conn(p, inst->conn_to_rdr);
            rdr_conn->dom_handle = co->stack.doc->udom;
        }
    }
}

bool
pcintr_revoke_crtn_from_doc(struct pcinst *inst, pcintr_coroutine_t co)
{
    pcintr_heap_t heap = inst->intr_heap;
    if (pcutils_sorted_array_remove(heap->loaded_crtn_handles, co)) {
        list_del(&co->doc_node);
        co->stack.doc->ldc--;

        if (co->stack.doc->ldc == 0) {
            co->stack.doc->udom = 0;
            pcintr_coroutine_t p;
            list_for_each_entry(p, &co->stack.doc->owner_list, doc_node) {
                struct list_head *conns = &p->conns;
                struct pcintr_coroutine_rdr_conn *p;
                struct pcintr_coroutine_rdr_conn *q;
                list_for_each_entry_safe(p, q, conns, ln) {
                    p->dom_handle = 0;
                }
            }
        }
    }
    else {
        purc_log_warn("Not a loaded coroutine: %p\n", co);
        return false;
    }

    return true;
}

bool
pcintr_suppress_crtn_doc(struct pcinst *inst, pcintr_coroutine_t co_loaded,
        uint64_t ctrn_handle)
{
    pcintr_coroutine_t co = (pcintr_coroutine_t)(uintptr_t)ctrn_handle;
    purc_document_t doc;

    pcintr_heap_t heap = inst->intr_heap;
    if (pcutils_sorted_array_find(heap->loaded_crtn_handles,
                co, (void **)&doc, NULL)) {
        assert(co->stack.doc->ldc != 0);
        co->stack.doc->ldc--;

        if ((co_loaded == NULL || co_loaded->stack.doc != doc) &&
                co->stack.doc->ldc == 0) {
            /* fire rdrState:pageSuppressed event */
            pcintr_coroutine_t p;
            list_for_each_entry(p, &doc->owner_list, doc_node) {
                purc_variant_t hvml = purc_variant_make_ulongint(p->cid);
                pcintr_coroutine_post_event(p->cid,
                        PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                        hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_PAGE_SUPPRESSED,
                        PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
                purc_variant_unref(hvml);
            }
        }
    }
    else {
        purc_log_warn("Not a loaded coroutine: %p\n", co);
        return false;
    }

    return true;
}

bool
pcintr_reload_crtn_doc(struct pcinst *inst, pcintr_coroutine_t co_revoked,
        uint64_t ctrn_handle)
{
    pcintr_coroutine_t co = (pcintr_coroutine_t)(uintptr_t)ctrn_handle;
    purc_document_t doc;
    pcintr_heap_t heap = inst->intr_heap;
    if (pcutils_sorted_array_find(heap->loaded_crtn_handles,
                co, (void **)&doc, NULL)) {
        co->stack.doc->ldc++;

         if (co_revoked == NULL || co_revoked->stack.doc != doc) {
            pcintr_rdr_page_control_load(inst, &co->stack);

            /* fire rdrState:pageReloaded event */
            pcintr_coroutine_t p;
            list_for_each_entry(p, &doc->owner_list, doc_node) {
                purc_variant_t hvml = purc_variant_make_ulongint(p->cid);
                pcintr_coroutine_post_event(p->cid,
                        PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                        hvml, MSG_TYPE_RDR_STATE, MSG_SUB_TYPE_PAGE_RELOADED,
                        PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
                purc_variant_unref(hvml);
            }
        }
    }
    else {
        purc_log_warn("Not a loaded coroutine: %p\n", co);
        return false;
    }

    return true;
}

