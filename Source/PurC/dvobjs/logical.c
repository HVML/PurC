/*
 * @file logical.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of logical dynamic variant object.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <math.h>

static bool judge_variant (purc_variant_t var) 
{
    if (var == NULL)
        return false;

    uint64_t u64 = 0;
    double number = 0.0d;
    bool ret = false;
    enum purc_variant_type type = purc_variant_get_type (var);

    switch ((int)type) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
            break;
        case PURC_VARIANT_TYPE_BOOLEAN:
            purc_variant_cast_to_ulongint (var, &u64, false);
            if (u64)
                ret = true;
            break;
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            purc_variant_cast_to_number (var, &number, false);
            if (fabs (number) > 1.0E-10)
                ret = true;
            break;
        case PURC_VARIANT_TYPE_ATOMSTRING:
            if (strlen (purc_variant_get_atom_string_const (var)) > 0)
                ret = true;
            break;
        case PURC_VARIANT_TYPE_STRING:
            if (purc_variant_string_length (var) > 1)
                ret = true;
            break;
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (purc_variant_sequence_length (var) > 0)
                ret = true;
            break;
        case PURC_VARIANT_TYPE_DYNAMIC:
            if ((purc_variant_dynamic_get_getter (var)) || 
                (purc_variant_dynamic_get_setter (var)))
                ret = true;
            break;
        case PURC_VARIANT_TYPE_NATIVE:
            if (purc_variant_native_get_entity (var))
                ret = true;
            break;
        case PURC_VARIANT_TYPE_OBJECT:
            if (purc_variant_object_get_size (var))
                ret = true;
            break;
        case PURC_VARIANT_TYPE_ARRAY:
            if (purc_variant_array_get_size (var))
                ret = true;
            break;
        case PURC_VARIANT_TYPE_SET:
            if (purc_variant_set_get_size (var))
                ret = true;
            break;
        default:
            break;
    }

    return ret;
}

static purc_variant_t
logical_not (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[0] == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (judge_variant (argv[0]))
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);
    
    return ret_var;
}

static purc_variant_t
logical_and (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    bool judge = true;
    int i = 0;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    while (argv[i]) {
        if (!judge_variant (argv[i])) {
            judge = false;
            break;
        }
        i++;
    }

    if (judge && (i > 0))
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);
    
    return ret_var;
}

static purc_variant_t
logical_or (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    bool judge = false;
    int i = 0;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    while (argv[i]) {
        if (judge_variant (argv[i])) {
            judge = true;
            break;
        }
        i++;
    }

    if (judge)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);
    
    return ret_var;
}

static purc_variant_t
logical_xor (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;
    unsigned judge1 = 0x00;
    unsigned judge2 = 0x00;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == NULL) || (argv[1] == NULL)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (judge_variant (argv[0]))
        judge1 = 0x01;

    if (judge_variant (argv[1]))
        judge2 = 0x01;

    judge1 ^= judge2;

    if (judge1)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);
    
    return ret_var;
}

static purc_variant_t
logical_eq (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_ne (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_gt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_ge (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_lt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_le (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_streq (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_strne (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_strgt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_strge (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_strlt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_strle (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
logical_eval (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

// only for test now.
purc_variant_t pcdvojbs_get_logical (void)
{
    purc_variant_t logical = purc_variant_make_object_c (7,
            "not",    purc_variant_make_dynamic (logical_not, NULL),
            "and",    purc_variant_make_dynamic (logical_and, NULL),
            "or",     purc_variant_make_dynamic (logical_or, NULL),
            "xor",    purc_variant_make_dynamic (logical_xor, NULL),
            "eq",     purc_variant_make_dynamic (logical_eq, NULL),
            "ne",     purc_variant_make_dynamic (logical_ne, NULL),
            "gt",     purc_variant_make_dynamic (logical_gt, NULL),
            "ge",     purc_variant_make_dynamic (logical_ge, NULL),
            "lt",     purc_variant_make_dynamic (logical_lt, NULL),
            "le",     purc_variant_make_dynamic (logical_le, NULL),
            "streq",  purc_variant_make_dynamic (logical_streq, NULL),
            "strne",  purc_variant_make_dynamic (logical_strne, NULL),
            "strgt",  purc_variant_make_dynamic (logical_strgt, NULL),
            "strge",  purc_variant_make_dynamic (logical_strge, NULL),
            "strlt",  purc_variant_make_dynamic (logical_strlt, NULL),
            "strle",  purc_variant_make_dynamic (logical_strle, NULL),
            "eval",   purc_variant_make_dynamic (logical_eval, NULL)
       );
    return logical;
}
