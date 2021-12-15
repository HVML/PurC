/*
 * @file errors.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The get/set error code of PurC.
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

#include "purc-errors.h"

#include "private/errors.h"
#include "private/instance.h"

purc_atom_t purc_except_bad_name;
purc_atom_t purc_except_no_data;
purc_atom_t purc_except_not_ready;
purc_atom_t purc_except_unauthorized;
purc_atom_t purc_except_timeout;
purc_atom_t purc_except_syntax_error;
purc_atom_t purc_except_not_iterable;
purc_atom_t purc_except_index_error;
purc_atom_t purc_except_key_error;
purc_atom_t purc_except_zero_division;
purc_atom_t purc_except_overflow;
purc_atom_t purc_except_floating_point;
purc_atom_t purc_except_not_implemented;
purc_atom_t purc_except_max_recursion_depth;
purc_atom_t purc_except_bad_encoding;
purc_atom_t purc_except_bad_value;
purc_atom_t purc_except_wrong_data_type;
purc_atom_t purc_except_wrong_domain;
purc_atom_t purc_except_os_error;
purc_atom_t purc_except_access_denied;
purc_atom_t purc_except_io_error;
purc_atom_t purc_except_too_many;
purc_atom_t purc_except_too_long;
purc_atom_t purc_except_not_desired_entity;
purc_atom_t purc_except_entity_not_found;
purc_atom_t purc_except_entity_exists;
purc_atom_t purc_except_broken_pipe;
purc_atom_t purc_except_connection_aborted;
purc_atom_t purc_except_connection_refused;
purc_atom_t purc_except_connection_reset;

static pcutils_map* purc_error_except_map;
static pcutils_map* purc_except_exinfo_required_map;

#define DEFINE_EXCEPT(name, value, requied_exinfo)          \
    name =  purc_atom_from_static_string(value);            \
    pcutils_map_insert(purc_except_exinfo_required_map,     \
            (void*)name, (void*)requied_exinfo);

#define MAP_EE(error, except)                 \
    pcutils_map_insert(purc_error_except_map, (void*)error, (void*)except);

void init_except_exinfo_map(void)
{
    DEFINE_EXCEPT(purc_except_bad_name, "BadName", false);
    DEFINE_EXCEPT(purc_except_no_data, "NoData", false);
    DEFINE_EXCEPT(purc_except_not_ready, "NotReady", false);
    DEFINE_EXCEPT(purc_except_unauthorized, "Unauthorized", false);
    DEFINE_EXCEPT(purc_except_timeout, "Timeout", false);
    DEFINE_EXCEPT(purc_except_syntax_error, "SyntaxError", false);
    DEFINE_EXCEPT(purc_except_not_iterable, "NotIterable", false);
    DEFINE_EXCEPT(purc_except_index_error, "IndexError", false);
    DEFINE_EXCEPT(purc_except_key_error, "KeyError", false);
    DEFINE_EXCEPT(purc_except_zero_division, "ZeroDivision", false);
    DEFINE_EXCEPT(purc_except_overflow, "Overflow", false);
    DEFINE_EXCEPT(purc_except_floating_point, "FloatingPoint", false);
    DEFINE_EXCEPT(purc_except_not_implemented, "NotImplemented", false);
    DEFINE_EXCEPT(purc_except_max_recursion_depth, "MaxRecursionDepth", false);
    DEFINE_EXCEPT(purc_except_bad_encoding, "BadEncoding", false);
    DEFINE_EXCEPT(purc_except_bad_value, "BadValue", false);
    DEFINE_EXCEPT(purc_except_wrong_data_type, "WrongDataType", false);
    DEFINE_EXCEPT(purc_except_wrong_domain, "WrongDomain", false);
    DEFINE_EXCEPT(purc_except_os_error, "OSError", false);
    DEFINE_EXCEPT(purc_except_access_denied, "AccessDenied", false);
    DEFINE_EXCEPT(purc_except_io_error, "IOError", false);
    DEFINE_EXCEPT(purc_except_too_many, "TooMany", false);
    DEFINE_EXCEPT(purc_except_too_long, "TooLong", false);
    DEFINE_EXCEPT(purc_except_not_desired_entity, "NotDesiredEntity", false);
    DEFINE_EXCEPT(purc_except_entity_not_found, "EntityNotFound", false);
    DEFINE_EXCEPT(purc_except_entity_exists, "EntityExists", false);
    DEFINE_EXCEPT(purc_except_broken_pipe, "BrokenPipe", false);
    DEFINE_EXCEPT(purc_except_connection_aborted, "ConnectionAborted", false);
    DEFINE_EXCEPT(purc_except_connection_refused, "ConnectionRefused", false);
    DEFINE_EXCEPT(purc_except_connection_reset, "ConnectionReset", false);
}

void init_error_except_map(void)
{
    MAP_EE(PURC_ERROR_OK, 0);
    MAP_EE(PURC_ERROR_BAD_SYSTEM_CALL,
            purc_except_os_error);
}

void purc_error_init_once(void)
{
    purc_error_except_map = pcutils_map_create(NULL, NULL, NULL, NULL, NULL,
            false);

    purc_except_exinfo_required_map = pcutils_map_create(NULL, NULL, NULL,
            NULL, NULL, false);

    init_except_exinfo_map();
    init_error_except_map();
}

bool is_except_exinfo_requited(purc_atom_t except)
{
    const pcutils_map_entry* entry = NULL;
    if ((entry = pcutils_map_find(purc_except_exinfo_required_map,
                    (const void*)except)))
    {
        return (bool) entry->val;
    }
    return false;
}

int purc_get_last_error(void)
{
    const struct pcinst* inst = pcinst_current();
    if (inst) {
        return inst->errcode;
    }

    return PURC_ERROR_NO_INSTANCE;
}

purc_variant_t purc_get_last_error_ex(void)
{
    const struct pcinst* inst = pcinst_current();
    if (inst) {
        return inst->err_exinfo;
    }

    return PURC_VARIANT_INVALID; // FIXME: or make_undefined?
}

int purc_set_error(int errcode)
{
    return purc_set_error_ex(errcode, PURC_VARIANT_INVALID);
}

int purc_set_error_ex(int errcode, purc_variant_t exinfo)
{
    UNUSED_PARAM(exinfo);
    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        return PURC_ERROR_NO_INSTANCE;
    }

    inst->errcode = errcode;
    inst->err_exinfo = exinfo;

    // TODO:
    // set the exception info into stack
#if 0
    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        purc_atom_t except = purc_get_error_exception(errcode);
        if (is_except_exinfo_requited(except) && !exinfo) {
            // error
        }
    }
#endif
    return PURC_ERROR_OK;
}

static LIST_HEAD(_err_msg_seg_list);

/* Error Messages */
#define UNKNOWN_ERR_CODE    "Unknown Error Code"

const char* purc_get_error_message(int errcode)
{
    struct list_head *p;

    list_for_each(p, &_err_msg_seg_list) {
        struct err_msg_seg *seg = container_of (p, struct err_msg_seg, list);
        if (errcode >= seg->first_errcode && errcode <= seg->last_errcode) {
            return seg->msgs[errcode - seg->first_errcode];
        }
    }

    return UNKNOWN_ERR_CODE;
}

purc_atom_t purc_get_error_exception(int errcode)
{
    const pcutils_map_entry* entry = NULL;
    if ((entry = pcutils_map_find(purc_error_except_map,
                    (const void*)(uintptr_t)errcode))) {
        return (purc_atom_t) entry->val;
    }
    return 0;
}

void pcinst_register_error_message_segment(struct err_msg_seg* seg)
{
    list_add(&seg->list, &_err_msg_seg_list);
}

