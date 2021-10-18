/*
 * @file ejson.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of EJSON dynamic variant object.
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
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "helper.h"

#include <assert.h>

static purc_variant_t
number_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var;
    size_t number;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    assert (argv[0] != PURC_VARIANT_INVALID);

    switch (purc_variant_get_type (argv[0])) {
        case PURC_VARIANT_TYPE_UNDEFINED:
            number = 0;
            break;

        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            number = 1;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            number = purc_variant_object_get_size (argv[0]);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            number = purc_variant_array_get_size (argv[0]);
            break;

        case PURC_VARIANT_TYPE_SET:
            number = purc_variant_set_get_size (argv[0]);
            break;
    }

    ret_var = purc_variant_make_ulongint (number);
    return ret_var;
}

static purc_variant_t
type_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    static const char *type_names[] = {
        "undefined",
        "null",
        "boolean",
        "number",
        "longint",
        "ulongint",
        "longdouble",
        "atomstring",
        "string",
        "bsequence",
        "dynamic",
        "native",
        "object",
        "array",
        "set",
    };

    /* make sure that the last one is `set` */
    assert (sizeof(type_names[PURC_VARIANT_TYPE_FIRST]) == 10);
    assert (sizeof(type_names[PURC_VARIANT_TYPE_LAST]) == 4);

    UNUSED_PARAM(root);

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    assert (argv[0] != PURC_VARIANT_INVALID);

    return purc_variant_make_string_static (
            type_names [purc_variant_get_type (argv[0])], false);
}

// only for test now.
purc_variant_t pcdvojbs_get_ejson (void)
{
    static struct pcdvojbs_dvobjs method [] = {
        {"type",     type_getter, NULL},
        {"number",   number_getter, NULL}
    };

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}
