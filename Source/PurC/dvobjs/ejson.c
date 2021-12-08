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
count_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
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

static const char *type_names[] = {
    VARIANT_TYPE_NAME_UNDEFINED,
    VARIANT_TYPE_NAME_NULL,
    VARIANT_TYPE_NAME_BOOLEAN,
    VARIANT_TYPE_NAME_NUMBER,
    VARIANT_TYPE_NAME_LONGINT,
    VARIANT_TYPE_NAME_ULONGINT,
    VARIANT_TYPE_NAME_LONGDOUBLE,
    VARIANT_TYPE_NAME_ATOMSTRING,
    VARIANT_TYPE_NAME_STRING,
    VARIANT_TYPE_NAME_BYTESEQUENCE,
    VARIANT_TYPE_NAME_DYNAMIC,
    VARIANT_TYPE_NAME_NATIVE,
    VARIANT_TYPE_NAME_OBJECT,
    VARIANT_TYPE_NAME_ARRAY,
    VARIANT_TYPE_NAME_SET,
};

/* Make sure the number of variant types matches the size of `type_names` */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(types, PCA_TABLESIZE(type_names) == PURC_VARIANT_TYPE_NR);

#undef _COMPILE_TIME_ASSERT

static purc_variant_t
type_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    assert (argv[0] != PURC_VARIANT_INVALID);

    /* make sure that the first one is `undefined` */
    assert (strcmp (type_names[PURC_VARIANT_TYPE_FIRST], "undefined") == 0);
    /* make sure that the last one is `set` */
    assert (strcmp (type_names[PURC_VARIANT_TYPE_LAST], "set") == 0);

    return purc_variant_make_string_static (
            type_names [purc_variant_get_type (argv[0])], false);
}

static purc_variant_t
numberify_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var;
    long double number = 0.0;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    number = pcdvobjs_get_variant_value (argv[0]);
    ret_var = purc_variant_make_longdouble (number);

    return ret_var;
}

static purc_variant_t
booleanize_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if (pcdvobjs_test_variant (argv[0]))
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
stringify_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var;
    char *buffer = NULL;
    int total = 0;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    total = purc_variant_stringify_alloc (&buffer, argv[0]);
    ret_var = purc_variant_make_string_reuse_buff (buffer, total + 1, false);

    return ret_var;
}

static purc_variant_t
serialize_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
sort_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
compare_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

// only for test now.
purc_variant_t pcdvobjs_get_ejson (void)
{
    static struct pcdvobjs_dvobjs method [] = {
        {"type",        type_getter, NULL},
        {"count",       count_getter, NULL},
        {"numberify",   numberify_getter, NULL},
        {"booleanize",  booleanize_getter, NULL},
        {"stringify",   stringify_getter, NULL},
        {"serialize",   serialize_getter, NULL},
        {"sort",        sort_getter, NULL},
        {"compare",     compare_getter, NULL}
    };

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}
