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

void purc_error_init_once(void)
{
    purc_except_bad_name = purc_atom_from_static_string("BadName");
    purc_except_no_data = purc_atom_from_static_string("NoData");
    purc_except_not_ready = purc_atom_from_static_string("NotReady");
    purc_except_unauthorized = purc_atom_from_static_string("Unauthorized");
    purc_except_timeout = purc_atom_from_static_string("Timeout");
    purc_except_syntax_error = purc_atom_from_static_string("SyntaxError");
    purc_except_not_iterable = purc_atom_from_static_string("NotIterable");
    purc_except_index_error = purc_atom_from_static_string("IndexError");
    purc_except_key_error = purc_atom_from_static_string("KeyError");
    purc_except_zero_division = purc_atom_from_static_string("ZeroDivision");
    purc_except_overflow = purc_atom_from_static_string("Overflow");
    purc_except_floating_point = purc_atom_from_static_string("FloatingPoint");
    purc_except_not_implemented = purc_atom_from_static_string("NotImplemented");
    purc_except_max_recursion_depth = purc_atom_from_static_string(
            "MaxRecursionDepth");
    purc_except_bad_encoding = purc_atom_from_static_string("BadEncoding");
    purc_except_bad_value = purc_atom_from_static_string("BadValue");
    purc_except_wrong_data_type = purc_atom_from_static_string("WrongDataType");
    purc_except_wrong_domain = purc_atom_from_static_string("WrongDomain");
    purc_except_os_error = purc_atom_from_static_string("OSError");
    purc_except_access_denied = purc_atom_from_static_string("AccessDenied");
    purc_except_io_error = purc_atom_from_static_string("IOError");
    purc_except_too_many = purc_atom_from_static_string("TooMany");
    purc_except_too_long = purc_atom_from_static_string("TooLong");
    purc_except_not_desired_entity = purc_atom_from_static_string(
            "NotDesiredEntity");
    purc_except_entity_not_found = purc_atom_from_static_string(
            "EntityNotFound");
    purc_except_entity_exists = purc_atom_from_static_string("EntityExists");
    purc_except_broken_pipe = purc_atom_from_static_string("BrokenPipe");
    purc_except_connection_aborted = purc_atom_from_static_string(
            "ConnectionAborted");
    purc_except_connection_refused = purc_atom_from_static_string(
            "ConnectionRefused");
    purc_except_connection_reset = purc_atom_from_static_string(
            "ConnectionReset");
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
        purc_atom_t exception = purc_get_error_exception(errcode);
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
    struct list_head *p;

    list_for_each(p, &_err_msg_seg_list) {
        struct err_msg_seg *seg = container_of (p, struct err_msg_seg, list);
        if (seg->exceptions && errcode >= seg->first_errcode &&
                errcode <= seg->last_errcode) {
            return seg->exceptions[errcode - seg->first_errcode];
        }
    }

    return 0;
}

void pcinst_register_error_message_segment(struct err_msg_seg* seg)
{
    list_add(&seg->list, &_err_msg_seg_list);
}

