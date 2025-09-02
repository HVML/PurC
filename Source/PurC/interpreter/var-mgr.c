/*
 * @file var-mgr.c
 * @author XueShuming
 * @date 2021/12/06
 * @brief The impl of PurC variable manager.
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
 */

#include "purc.h"

#include "config.h"

#include "internal.h"

#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"
#include "private/regex.h"

#include <stdlib.h>
#include <string.h>

#define EVENT_ATTACHED          "change:attached"
#define EVENT_DETACHED          "change:detached"
#define EVENT_DISPLACED         "change:displaced"
#define EVENT_EXCEPT            "except:"

#define ATTR_KEY_ID             "id"
#define ATTR_KEY_IDD_BY         "idd-by"

#define KEY_FLAG                "__name_observe"
#define KEY_NAME                "name"
#define KEY_MGR                 "mgr"

#define ERROR_NOT_FOUND_FORMAT  "EntityNotFound: `%s`"

enum var_event_type {
    VAR_EVENT_TYPE_ATTACHED,
    VAR_EVENT_TYPE_DETACHED,
    VAR_EVENT_TYPE_DISPLACED,
    VAR_EVENT_TYPE_EXCEPT,
};

struct pcvarmgr_named_variables_observe {
    char *name;
    pcintr_stack_t stack;
    pcvdom_element_t elem;
};

static purc_variant_t
pcvarmgr_build_event_observed(const char *name, pcvarmgr_t mgr)
{
    purc_variant_t v = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t flag = purc_variant_make_boolean(true);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!purc_variant_object_set_by_static_ckey(v, KEY_FLAG, flag)) {
        purc_variant_unref(flag);
        goto failed;
    }
    purc_variant_unref(flag);

    purc_variant_t name_val = purc_variant_make_string(name, true);
    if (!name_val) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    if (!purc_variant_object_set_by_static_ckey(v, KEY_NAME, name_val)) {
        goto failed;
    }
    purc_variant_unref(name_val);

    purc_variant_t mgr_val = purc_variant_make_native(mgr, NULL);
    if (!mgr_val) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    if (!purc_variant_object_set_by_static_ckey(v, KEY_MGR, mgr_val)) {
        goto failed;
    }
    purc_variant_unref(mgr_val);

    return v;

failed:
    purc_variant_unref(v);

    return PURC_VARIANT_INVALID;
}

static int mgr_post_event(pcintr_stack_t stack,
        pcvarmgr_t mgr, const char *name, const char *type,
        const char *sub_type, purc_variant_t data)
{
    int ret = -1;
    purc_variant_t dest = pcvarmgr_build_event_observed(name, mgr);
    if (dest) {
        ret = pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                dest, type, sub_type, data, PURC_VARIANT_INVALID);
        purc_variant_unref(dest);
    }
    return ret;
}

#define DEF_ARRAY_SIZE 10
pcvarmgr_t pcvarmgr_create(void)
{
    pcvarmgr_t mgr = (pcvarmgr_t)calloc(1,
            sizeof(struct pcvarmgr));
    if (!mgr) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto err_ret;
    }

    mgr->object = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (mgr->object == PURC_VARIANT_INVALID) {
        goto err_free_mgr;
    }

    return mgr;

err_free_mgr:
    free(mgr);

err_ret:
    return NULL;
}

int pcvarmgr_destroy(pcvarmgr_t mgr)
{
    if (mgr) {
        PC_ASSERT(mgr->node.rb_parent == NULL);
        if (mgr->listener) {
            purc_variant_revoke_listener(mgr->object, mgr->listener);
        }
        purc_variant_unref(mgr->object);
        free(mgr);
    }
    return 0;
}

bool pcvarmgr_add(pcvarmgr_t mgr, const char* name,
        purc_variant_t variant)
{
    if (purc_variant_is_undefined(variant)) {
        return pcvarmgr_remove_ex(mgr, name, true);
    }

    if (mgr == NULL || mgr->object == PURC_VARIANT_INVALID
            || name == NULL || !variant) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    purc_variant_t k = purc_variant_make_string(name, true);
    if (k == PURC_VARIANT_INVALID) {
        return false;
    }
    bool ret = false;
    purc_variant_t v = purc_variant_object_get_ex(mgr->object, k, true);
    if (v == PURC_VARIANT_INVALID) {
        ret = purc_variant_object_set(mgr->object, k, variant);
        pcintr_stack_t stack = pcintr_get_stack();
        if (stack) {
            mgr_post_event(stack, mgr, name, MSG_TYPE_CHANGE,
                    MSG_SUB_TYPE_ATTACHED, PURC_VARIANT_INVALID);
        }
    }
    else {
        enum purc_variant_type type = purc_variant_get_type(v);
        switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
            // XXX: observe on=$name
            ret = pcvariant_container_displace(v, variant, false);
            break;

        default:
            // XXX: observe on=$name
            ret = purc_variant_object_set(mgr->object, k, variant);
            break;
        }
        pcintr_stack_t stack = pcintr_get_stack();
        if (stack) {
            mgr_post_event(stack, mgr, name, MSG_TYPE_CHANGE,
                    MSG_SUB_TYPE_DISPLACED, PURC_VARIANT_INVALID);
        }
    }

    purc_variant_unref(k);
    return ret;
}

purc_variant_t pcvarmgr_get(pcvarmgr_t mgr, const char* name)
{
    if (mgr == NULL || name == NULL) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;
    v = purc_variant_object_get_by_ckey_ex(mgr->object, name, true);
    if (v) {
        return v;
    }

    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
    return PURC_VARIANT_INVALID;
}

bool pcvarmgr_remove_ex(pcvarmgr_t mgr, const char* name, bool silently)
{
    if (name) {
        bool b =  purc_variant_object_remove_by_ckey(mgr->object,
                name, silently);
        pcintr_stack_t stack = pcintr_get_stack();
        if (stack) {
            mgr_post_event(stack, mgr, name, MSG_TYPE_CHANGE,
                    MSG_SUB_TYPE_DETACHED, PURC_VARIANT_INVALID);
        }
        return b;
    }
    return false;
}

bool pcvarmgr_dispatch_except(pcvarmgr_t mgr, const char* name,
        const char* except)
{
    pcintr_stack_t stack = pcintr_get_stack();
    if (!stack) {
        return true;
    }
    purc_variant_t dest = pcvarmgr_build_event_observed(name, mgr);
    if (dest) {
        pcintr_coroutine_post_event(stack->co->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
                dest, MSG_TYPE_CHANGE, except,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        purc_variant_unref(dest);
    }

    return true;
}

#if 0
static purc_variant_t
_find_named_scope_var_in_vdom(purc_coroutine_t cor,
        pcvdom_element_t elem, const char* name, pcvarmgr_t* mgr)
{
    if (!elem || !name) {
        PC_ASSERT(name); // FIXME: still recoverable???
        purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, "name:null");
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;

again:

    v = pcintr_get_scope_variable(cor, elem, name);
    if (v) {
        if (mgr) {
            *mgr = pcintr_get_scope_variables(cor, elem);
        }
        return v;
    }

    elem = pcvdom_element_parent(elem);
    if (elem)
        goto again;

    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
_find_named_scope_var(purc_coroutine_t cor,
        struct pcintr_stack_frame *frame, const char* name, pcvarmgr_t* mgr)
{
    pcvdom_element_t scope = NULL;
    purc_variant_t v;
    if (frame->scope) {
        v =  _find_named_scope_var_in_vdom(cor, frame->scope, name, mgr);
        if (v) {
            return v;
        }
        scope = frame->scope;
        goto find_parent_vdom;
    }

    pcvdom_element_t elem = frame->pos;

    if (!elem || !name) {
        PC_ASSERT(name); // FIXME: still recoverable???
        purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame *parent = frame;
    pcvdom_element_t last_elem = elem;

again:

    v = pcintr_get_scope_variable(cor, elem, name);
    if (v) {
        if (mgr) {
            *mgr = pcintr_get_scope_variables(cor, elem);
        }
        return v;
    }

    parent = pcintr_stack_frame_get_parent(parent);
    if (parent) {
        if (parent->scope) {
            v = _find_named_scope_var_in_vdom(cor, parent->scope, name, mgr);
            if (v) {
                return v;
            }
            scope = parent->scope;
            goto find_parent_vdom;
        }

        elem = parent->pos;
        if (elem) {
            goto again;
        }

        if (!parent->pos && !parent->scope) {
            goto out;
        }
    }

    scope = frame->pos;

find_parent_vdom:
    parent = pcintr_find_prev_include_frame(cor, frame, scope);
    if (parent == NULL || parent->pos == NULL) {
        goto out;
    }

    /* clear PCVRNT_ERROR_NOT_FOUND */
    purc_clr_error();

    elem = parent->pos;
    if (elem != last_elem) {
        last_elem = elem;
        goto again;
    }

out:
    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
    return PURC_VARIANT_INVALID;
}

#else

static purc_variant_t
_find_named_var_in_vdom(purc_coroutine_t cor,
        pcvdom_element_t elem, const char* name, pcvarmgr_t* mgr)
{
    purc_variant_t v = PURC_VARIANT_INVALID;
    struct pcvdom_document *vdom = pcvdom_document_from_node(&elem->node);
    bool is_stack_vdom = (vdom == cor->stack.vdom);

    while (elem) {
        v = pcintr_get_scope_variable(cor, elem, name);
        if (v) {
            if (mgr) {
                *mgr = pcintr_get_scope_variables(cor, elem);
            }
            goto out;
        }

        elem = pcvdom_element_parent(elem);
        if (!elem) {
            goto out_not_found;
        }

        if (elem->tag_id == PCHVML_TAG_DEFINE ||
                (!is_stack_vdom && (elem->node.type == PCVDOM_NODE_DOCUMENT ||
                                    elem->tag_id == PCHVML_TAG_HVML))
                ) {
            goto out_not_found;
        }
    }

out_not_found:
    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT,
            name);
out:
    return v;
}

static purc_variant_t
_find_named_scope_var(purc_coroutine_t cor,
        struct pcintr_stack_frame *frame, const char* name, pcvarmgr_t* mgr)
{
    purc_variant_t v = PURC_VARIANT_INVALID;
    pcvdom_element_t elem = frame->pos;
    v = _find_named_var_in_vdom(cor, elem, name, mgr);
    if (v) {
        goto out;
    }

    struct pcintr_stack_frame *parent;
    parent = pcintr_stack_frame_get_parent(frame);
    while (parent && parent->pos &&
            (parent->pos->tag_id != PCHVML_TAG_EXECUTE) &&
            (parent->pos->tag_id != PCHVML_TAG_CALL) &&
            (parent->pos->tag_id != PCHVML_TAG_OBSERVE)
            ) {
        parent = pcintr_stack_frame_get_parent(parent);
    }

    if (parent && parent->pos) {
        v = _find_named_scope_var(cor, parent, name, mgr);
        if (v) {
            goto out;
        }

        if (parent->pos->tag_id == PCHVML_TAG_OBSERVE) {
            parent = pcintr_stack_frame_get_parent(parent);
            if (parent && parent->pos) {
                if (parent->pos->tag_id == PCHVML_TAG_HVML) {
                    struct pcvdom_element *body = NULL;
                    struct pcvdom_element *child =
                        pcvdom_element_first_child_element(parent->pos);
                    while(child) {
                        if (child->tag_id == PCHVML_TAG_BODY) {
                            body = child;
                            break;
                        }
                        child = pcvdom_element_next_sibling_element(child);
                    }
                    if (body) {
                        v = _find_named_var_in_vdom(cor, body, name, mgr);
                        if (v) {
                            goto out;
                        }
                    }
                }
                v = _find_named_scope_var(cor, parent, name, mgr);
            }
        }
    }

out:
    return v;
}
#endif

static purc_variant_t
_find_named_root(purc_coroutine_t cor, struct pcintr_stack_frame *frame,
        const char* name)
{
    (void)frame;
    purc_variant_t v = PURC_VARIANT_INVALID;

    if (cor && cor->vdom) {
        struct pcvdom_element *p = pcvdom_document_get_root(cor->vdom);
        if (p) {
            v = pcintr_get_scope_variable(cor, p, name);
        }
    }


    if (v) {
        return v;
    }

    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
find_cor_level_var(purc_coroutine_t cor, const char* name)
{
    PC_ASSERT(name);
    if (!cor) {
        purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = purc_coroutine_get_variable(cor, name);
    if (v) {
        return v;
    }
    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
    return PURC_VARIANT_INVALID;
}

purc_variant_t
purc_get_runner_variable(const char* name)
{
    if (!name) {
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_t varmgr = pcinst_get_variables();
    if (varmgr == NULL) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcvarmgr_get(varmgr, name);
    if (v) {
        return v;
    }
    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
    return PURC_VARIANT_INVALID;
}

static inline purc_variant_t
find_inst_var(const char *name)
{
    return purc_get_runner_variable(name);
}

static purc_variant_t
_find_named_temp_var(struct pcintr_stack_frame *frame, const char *name)
{
    struct pcintr_stack_frame *p = frame;

again:

    if (p == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }

    do {
        purc_variant_t tmp;
        tmp = pcintr_get_user_var(p);
        if (tmp == PURC_VARIANT_INVALID)
            break;

        if (purc_variant_is_object(tmp) == false)
            break;

        purc_variant_t v;
        v = purc_variant_object_get_by_ckey_ex(tmp, name, false);
        if (v == PURC_VARIANT_INVALID)
            break;

        return v;
    } while (0);

    p = pcintr_stack_frame_get_parent(p);

    goto again;
}

purc_variant_t
pcintr_find_named_var(pcintr_stack_t stack, const char* name)
{
    if (!stack || !name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    purc_variant_t v;
    v = _find_named_temp_var(frame, name);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = _find_named_scope_var(stack->co, frame, name, NULL);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = _find_named_root(stack->co, frame, name);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = find_cor_level_var(stack->co, name);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = find_inst_var(name);
    if (v) {
        purc_clr_error();
        return v;
    }

    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
    return PURC_VARIANT_INVALID;
}

enum purc_symbol_var _to_symbol(char symbol)
{
    switch (symbol) {
    case '?':
        return PURC_SYMBOL_VAR_RES;
    case '<':
    case '~':
        return PURC_SYMBOL_VAR_IPT;
    case '@':
        return PURC_SYMBOL_VAR_POS;
    case '!':
        return PURC_SYMBOL_VAR_USR;
    case ':':
        return PURC_SYMBOL_VAR_KEY;
    case '=':
        return PURC_SYMBOL_VAR_VAL;
    case '%':
        return PURC_SYMBOL_VAR_IDX;
    case '^':
        return PURC_SYMBOL_VAR_CNT;
    default:
        // FIXME: NotFound???
        purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, "symbol:%c", symbol);
        return PURC_SYMBOL_VAR_MAX;
    }
}

bool
pcintr_is_symbolized_var(const char *name)
{
    return pcregex_is_match("^\\d*[@?!^:=%~<]$", name);
}

purc_variant_t
pcintr_get_symbolized_var (pcintr_stack_t stack, unsigned int number,
        char symbol)
{
    enum purc_symbol_var symbol_var = _to_symbol(symbol);
    if (symbol_var == PURC_SYMBOL_VAR_MAX) {
        purc_set_error(PURC_ERROR_BAD_NAME);
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    for (unsigned int i = 0; i < number; i++) {
        frame = pcintr_stack_frame_get_parent(frame);
    }

    if (!frame) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcintr_get_symbol_var(frame, symbol_var);
    PC_ASSERT(v != PURC_VARIANT_INVALID);
    if (v != PURC_VARIANT_INVALID) {
        purc_clr_error();
        return v;
    }
    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, "symbol:%c", symbol);
    return PURC_VARIANT_INVALID;
}

purc_variant_t
pcintr_find_anchor_symbolized_var(pcintr_stack_t stack, const char *anchor,
        char symbol)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    enum purc_symbol_var symbol_var = _to_symbol(symbol);
    if (symbol_var == PURC_SYMBOL_VAR_MAX) {
        purc_set_error(PURC_ERROR_BAD_NAME);
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);

    while (frame && frame->pos) {
        pcvdom_element_t elem = frame->pos;
        purc_variant_t elem_id;
        const char *name = elem->tag_name;
        const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
                strlen(name));
        if (entry &&
                (entry->cats & (PCHVML_TAGCAT_TEMPLATE | PCHVML_TAGCAT_VERB))) {
            elem_id = pcvdom_element_eval_attr_val(stack, elem, ATTR_KEY_IDD_BY);
        }
        else {
            elem_id = pcvdom_element_eval_attr_val(stack, elem, ATTR_KEY_ID);
        }
        if (!elem_id) {
            frame = pcintr_stack_frame_get_parent(frame);
            continue;
        }

        if (purc_variant_is_string(elem_id)) {
            const char *id = purc_variant_get_string_const(elem_id);
            if (id && strcmp(id, anchor) == 0) {
                ret = pcintr_get_symbol_var(frame, symbol_var);
                if (ret == PURC_VARIANT_INVALID) {
                    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND,
                            "symbol:%c", symbol);
                }
                else {
                    purc_clr_error();
                }
                purc_variant_unref(elem_id);
                return ret;
            }
        }
        purc_variant_unref(elem_id);
        frame = pcintr_stack_frame_get_parent(frame);
    }

    return PURC_VARIANT_INVALID;
}

static bool
_unbind_named_temp_var(struct pcintr_stack_frame *frame, const char *name)
{
    struct pcintr_stack_frame *p = frame;

again:

    if (p == NULL)
        return false;

    do {
        purc_variant_t tmp;
        tmp = pcintr_get_user_var(p);
        if (tmp == PURC_VARIANT_INVALID)
            break;

        if (purc_variant_is_object(tmp) == false)
            break;

        purc_variant_t v;
        v = purc_variant_object_get_by_ckey_ex(tmp, name, false);
        if (v == PURC_VARIANT_INVALID)
            break;

        return purc_variant_object_remove_by_ckey(tmp, name, false);
    } while (0);

    p = pcintr_stack_frame_get_parent(p);

    goto again;
}

static bool
_unbind_named_scope_var(purc_coroutine_t cor, pcvdom_element_t elem,
        const char* name)
{
    if (!elem) {
        return false;
    }

    purc_variant_t v = pcintr_get_scope_variable(cor, elem, name);
    if (v) {
        return pcintr_unbind_scope_variable(cor, elem, name);
    }

    pcvdom_element_t parent = pcvdom_element_parent(elem);
    if (parent) {
        return _unbind_named_scope_var(cor, parent, name);
    }
    else {
        // FIXME: vdom.c:563
        purc_clr_error();
    }
    return false;
}

static bool
_unbind_cor_level_var(purc_coroutine_t cor, const char* name)
{
    purc_variant_t v = purc_coroutine_get_variable(cor, name);
    if (v) {
        return purc_coroutine_unbind_variable(cor, name);
    }
    return false;
}

int
pcintr_unbind_named_var(pcintr_stack_t stack, const char *name)
{
    if (!stack || !name) {
        return PCVRNT_ERROR_NOT_FOUND;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    if (_unbind_named_temp_var(frame, name)) {
        return PURC_ERROR_OK;
    }

    if (_unbind_named_scope_var(stack->co, frame->pos, name)) {
        return PURC_ERROR_OK;
    }

    if (_unbind_cor_level_var(stack->co, name)) {
        return PURC_ERROR_OK;
    }
    purc_set_error_with_info(PCVRNT_ERROR_NOT_FOUND, ERROR_NOT_FOUND_FORMAT, name);
    return PCVRNT_ERROR_NOT_FOUND;
}

static bool
did_matched(void *native_entity, purc_variant_t val)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(val);
    if (!purc_variant_is_object(val)) {
        return false;
    }

    purc_variant_t flag = purc_variant_object_get_by_ckey_ex(val, KEY_FLAG, true);
    if (!flag) {
        return false;
    }

    struct pcvarmgr_named_variables_observe *obs =
        (struct pcvarmgr_named_variables_observe*)native_entity;

    purc_variant_t name_val = purc_variant_object_get_by_ckey_ex(val,
            KEY_NAME, true);
    if (!name_val) {
        return false;
    }

    if (strcmp(obs->name, purc_variant_get_string_const(name_val)) != 0) {
        return false;
    }

    purc_variant_t mgr_val = purc_variant_object_get_by_ckey_ex(val, KEY_MGR, true);
    if (!mgr_val || !purc_variant_is_native(mgr_val)) {
        return false;
    }

    void *comp = purc_variant_native_get_entity(mgr_val);

    pcvdom_element_t elem = obs->elem;
    while (elem) {
        pcvarmgr_t varmgr =
            pcintr_get_scope_variables(obs->stack->co, elem);
        if (varmgr == comp) {
            return true;
        }
        elem  = pcvdom_element_parent(elem);
        purc_clr_error();
    }

    pcvarmgr_t mgr = pcintr_get_coroutine_variables(obs->stack->co);
    if (mgr == comp) {
        return true;
    }

    mgr = pcinst_get_variables();
    if (mgr == comp) {
        return true;
    }

    return false;
}

static bool
on_observe(void *native_entity,
        const char *event_name, const char *event_subname)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    return true;
}


static void
named_destroy(struct pcvarmgr_named_variables_observe *named)
{
    if (named->name) {
        free(named->name);
    }
    free(named);
}

static void
on_release(void *native_entity)
{
    UNUSED_PARAM(native_entity);

    PC_ASSERT(native_entity);
    struct pcvarmgr_named_variables_observe *named =
        (struct pcvarmgr_named_variables_observe*)native_entity;
    named_destroy(named);
}

purc_variant_t
pcintr_get_named_var_for_observed(pcintr_stack_t stack, const char *name,
        pcvdom_element_t elem)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(name);
    UNUSED_PARAM(elem);
    static struct purc_native_ops ops = {
        .did_matched                = did_matched,

        .on_observe                 = on_observe,
        .on_release                 = on_release,
    };

    struct pcvarmgr_named_variables_observe *named =
        (struct pcvarmgr_named_variables_observe*)calloc(1, sizeof(*named));
    if (!named) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    named->name = strdup(name);
    if (!named->name) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    named->stack = stack;
    named->elem = elem;

    purc_variant_t v = purc_variant_make_native(named, &ops);
    if (v == PURC_VARIANT_INVALID) {
        named_destroy(named);
        return PURC_VARIANT_INVALID;
    }

    return v;
}

purc_variant_t
pcintr_get_named_var_for_event(pcintr_stack_t stack, const char *name,
        pcvarmgr_t mgr)
{
    if (!mgr) {
        mgr = pcintr_get_coroutine_variables(stack->co);
    }
    return pcvarmgr_build_event_observed(name, mgr);
}

bool
pcintr_is_named_var_for_event(purc_variant_t val)
{
    if (!purc_variant_is_object(val)) {
        return false;
    }

    purc_variant_t flag = purc_variant_object_get_by_ckey_ex(val, KEY_FLAG, true);
    if (!flag) {
        return false;
    }

    purc_variant_t name_val =
        purc_variant_object_get_by_ckey_ex(val, KEY_NAME, true);
    if (!name_val) {
        return false;
    }

    purc_variant_t mgr_val =
        purc_variant_object_get_by_ckey_ex(val, KEY_MGR, true);
    if (!mgr_val || !purc_variant_is_native(mgr_val)) {
        return false;
    }

    return true;
}

