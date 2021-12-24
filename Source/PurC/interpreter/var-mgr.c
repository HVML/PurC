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

#include <stdlib.h>
#include <string.h>

static void* pcvarmgr_list_copy_val(const void* val)
{
    purc_variant_t var = (purc_variant_t)val;
    purc_variant_ref(var);
    return var;
}

static void pcvarmgr_list_free_val(void* val)
{
    purc_variant_t var = (purc_variant_t)val;
    purc_variant_unref(var);
}

pcvarmgr_list_t pcvarmgr_list_create(void)
{
    return pcutils_map_create (copy_key_string, free_key_string,
            pcvarmgr_list_copy_val, pcvarmgr_list_free_val,
            comp_key_string, false);
}

int pcvarmgr_list_destroy(pcvarmgr_list_t list)
{
    return pcutils_map_destroy(list);
}

bool pcvarmgr_list_add(pcvarmgr_list_t list, const char* name,
        purc_variant_t variant)
{
    if (pcutils_map_find_replace_or_insert(list, name,
                (void *)variant, NULL)) {
        return false;
    }

    return true;
}

purc_variant_t pcvarmgr_list_get(pcvarmgr_list_t list, const char* name)
{
    const pcutils_map_entry* entry = NULL;

    if (name == NULL) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    if ((entry = pcutils_map_find(list, name))) {
        return (purc_variant_t) entry->val;
    }

    purc_set_error_exinfo(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

bool pcvarmgr_list_remove(pcvarmgr_list_t list, const char* name)
{
    if (name) {
        if (pcutils_map_erase (list, (void*)name))
            return true;
    }
    return false;
}

static purc_variant_t
_find_named_scope_var(pcvdom_element_t elem, const char* name)
{
    if (!elem || !name) {
        PC_ASSERT(name); // FIXME: still recoverable???
        purc_set_error_exinfo(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcintr_get_scope_variable(elem, name);
    if (v) {
        return v;
    }

    pcvdom_element_t parent = pcvdom_element_parent(elem);
    if (parent) {
        return _find_named_scope_var(parent, name);
    }
    purc_set_error_exinfo(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
_find_doc_buildin_var(purc_vdom_t vdom, const char* name)
{
    PC_ASSERT(name);
    if (!vdom) {
        purc_set_error_exinfo(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcvdom_document_get_variable(vdom, name);
    if (v) {
        return v;
    }
    purc_set_error_exinfo(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t _find_inst_var(const char* name)
{
    if (!name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcvarmgr_list_get(inst->variables, name);
    if (v) {
        return v;
    }
    purc_set_error_exinfo(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
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

    purc_variant_t v = _find_named_scope_var(frame->pos, name);
    if (v) {
        return v;
    }

    v = _find_doc_buildin_var(stack->vdom, name);
    if (v) {
        return v;
    }

    v = _find_inst_var(name);
    if (v) {
        return v;
    }

    return purc_variant_make_undefined();
}

enum purc_symbol_var _to_symbol(char symbol)
{
    switch (symbol) {
    case '?':
        return PURC_SYMBOL_VAR_QUESTION_MARK;
    case '@':
        return PURC_SYMBOL_VAR_AT_SIGN;
    case '#':
        return PURC_SYMBOL_VAR_NUMBER_SIGN;
    case '*':
        return PURC_SYMBOL_VAR_ASTERISK;
    case ':':
        return PURC_SYMBOL_VAR_COLON;
    case '&':
        return PURC_SYMBOL_VAR_AMPERSAND;
    case '%':
        return PURC_SYMBOL_VAR_PERCENT_SIGN;
    }
    // FIXME: NotFound???
    purc_set_error_exinfo(PCVARIANT_ERROR_NOT_FOUND, "symbol:%c", symbol);
    return PURC_SYMBOL_VAR_MAX;
}

purc_variant_t
pcintr_get_symbolized_var (pcintr_stack_t stack, unsigned int number,
        char symbol)
{
    enum purc_symbol_var symbol_var = _to_symbol(symbol);
    if (symbol_var == PURC_SYMBOL_VAR_MAX) {
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    for (unsigned int i = 0; i < number; i++) {
        frame = pcintr_stack_frame_get_parent(frame);
    }

    if (!frame) {
        return purc_variant_make_undefined();
    }

    purc_variant_t v = frame->symbol_vars[symbol_var];
    return v ? v : purc_variant_make_undefined();
}

purc_variant_t
pcintr_get_numbered_var (pcintr_stack_t stack, unsigned int number)
{
    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    for (unsigned int i = 0; i < number; i++) {
        frame = pcintr_stack_frame_get_parent(frame);
    }

    if (!frame) {
        return purc_variant_make_undefined();
    }

    purc_variant_t v = frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK];
    return v ? v : purc_variant_make_undefined();
}
