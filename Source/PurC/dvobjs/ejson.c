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
#include <stdlib.h>

typedef struct __dvobjs_ejson_arg {
    bool asc;
    bool caseless;
    pcutils_map *map;
} dvobjs_ejson_arg;

static purc_variant_t
count_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    size_t number;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    assert (argv[0] != PURC_VARIANT_INVALID);

    switch (purc_variant_get_type (argv[0])) {
        case PURC_VARIANT_TYPE_UNDEFINED:
            number = 0;
            break;

        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_EXCEPTION:
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
    VARIANT_TYPE_NAME_EXCEPTION,
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
type_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    assert (argv[0] != PURC_VARIANT_INVALID);

    /* make sure that the first one is `undefined` */
    assert (strcmp (type_names[PURC_VARIANT_TYPE_FIRST],
                VARIANT_TYPE_NAME_UNDEFINED) == 0);
    /* make sure that the last one is `set` */
    assert (strcmp (type_names[PURC_VARIANT_TYPE_LAST],
                VARIANT_TYPE_NAME_SET) == 0);

    return purc_variant_make_string_static (
            type_names [purc_variant_get_type (argv[0])], false);
}

static purc_variant_t
numberify_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    number = purc_variant_numberify (argv[0]);
    ret_var = purc_variant_make_number (number);

    return ret_var;
}

static purc_variant_t
booleanize_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (purc_variant_booleanize (argv[0]))
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
stringify_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    char *buffer = NULL;
    int total = 0;
    char stackbuf[64] = {0,};

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    int type = purc_variant_get_type (argv[0]);

    switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
            total = purc_variant_stringify_alloc (&buffer, argv[0]);
            if (total == -1) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                ret_var = PURC_VARIANT_INVALID;
            }
            else
                ret_var = purc_variant_make_string_reuse_buff (
                        buffer, total, false);
            break;
        case PURC_VARIANT_TYPE_EXCEPTION:
        case PURC_VARIANT_TYPE_ATOMSTRING:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (type == PURC_VARIANT_TYPE_STRING) {
                total = purc_variant_string_size (argv[0]);
                buffer = malloc (total);
            }
            else if (type == PURC_VARIANT_TYPE_BSEQUENCE) {
                total = purc_variant_sequence_length (argv[0]);
                buffer = malloc (total * 2 + 1);
            }
            else if (type == PURC_VARIANT_TYPE_ATOMSTRING) {
                total = strlen (purc_variant_get_atom_string_const (argv[0])) + 1;
                buffer = malloc (total);
            }
            else {
                total = strlen (
                        purc_variant_get_exception_string_const (argv[0])) + 1;
                buffer = malloc (total);
            }

            if (buffer == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                ret_var = PURC_VARIANT_INVALID;
            }
            else {
                purc_variant_stringify (buffer, total, argv[0]);
                ret_var = purc_variant_make_string_reuse_buff (
                        buffer, total, false);
            }
            break;
        default:
            buffer = stackbuf;
            purc_variant_stringify (buffer, 64, argv[0]);
            ret_var = purc_variant_make_string (buffer, false);
            break;
    }

    return ret_var;
}

static purc_variant_t
serialize_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t serialize = NULL;
    char *buf = NULL;
    size_t sz_stream = 0;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    serialize = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_variant_serialize (argv[0], serialize, 3, 0, &sz_stream);
    buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
    purc_rwstream_destroy (serialize);

    ret_var = purc_variant_make_string_reuse_buff (buf, sz_stream + 1, false);

    return ret_var;
}

static int my_array_sort (purc_variant_t v1, purc_variant_t v2, void *ud)
{
    int ret = 0;
    char *p1 = NULL;
    char *p2 = NULL;
    pcutils_map_entry *entry = NULL;

    dvobjs_ejson_arg *sort_arg = (dvobjs_ejson_arg *)ud;
    entry = pcutils_map_find (sort_arg->map, v1);
    p1 = (char *)entry->val;
    entry = NULL;
    entry = pcutils_map_find (sort_arg->map, v2);
    p2 = (char *)entry->val;

    if (sort_arg->caseless)
        ret = strcasecmp (p1, p2);
    else
        ret = strcmp (p1, p2);

    if (!sort_arg->asc)
        ret = -1 * ret;

    if (ret != 0)
        ret = ret > 0? 1: -1;
    return ret;
}

static void * map_copy_key(const void *key)
{
    return (void *)key;
}

static void map_free_key(void *key)
{
    UNUSED_PARAM(key);
}

static void *map_copy_val(const void *val)
{
    return (void *)val;
}

static int map_comp_key(const void *key1, const void *key2)
{
    int ret = 0;
    if (key1 != key2)
        ret = key1 > key2? 1: -1;
    return ret;
}

static void map_free_val(void *val)
{
    if (val)
        free (val);
}

static purc_variant_t
sort_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    size_t i = 0;
    size_t totalsize = 0;
    const char *option = NULL;
    const char *order = NULL;
    dvobjs_ejson_arg sort_arg;
    char *buf = NULL;

    sort_arg.asc = true;
    sort_arg.caseless = false;
    sort_arg.map = NULL;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!(purc_variant_is_array (argv[0]) || purc_variant_is_set (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    // get sort order: asc, desc
    if ((argv[1] == PURC_VARIANT_INVALID) ||
                (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    order = purc_variant_get_string_const (argv[1]);
    if (strcasecmp (order, STRING_COMP_MODE_DESC) == 0)
        sort_arg.asc = false;

    // get sort option: case, caseless
    if ((nr_args == 3) && (argv[2] != PURC_VARIANT_INVALID) &&
                (purc_variant_is_string (argv[2]))) {
        option = purc_variant_get_string_const (argv[2]);
        if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0)
            sort_arg.caseless = true;
    }

    sort_arg.map = pcutils_map_create (map_copy_key, map_free_key,
            map_copy_val, map_free_val, map_comp_key, false);

    // it is the array
    if (purc_variant_is_array (argv[0])) {
        totalsize = purc_variant_array_get_size (argv[0]);

        for (i = 0; i < totalsize; ++i) {
            val = purc_variant_array_get(argv[0], i);
            purc_variant_stringify_alloc (&buf, val);
            pcutils_map_insert (sort_arg.map, val, buf);
        }
        pcvariant_array_sort (argv[0], (void *)&sort_arg, my_array_sort);
    }
    else {    // it is the set
        pcvariant_set_sort (argv[0]);
    }

    pcutils_map_destroy (sort_arg.map);

    ret_var = argv[0];

    return ret_var;
}

static purc_variant_t
compare_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    const char *option = NULL;
    double compare = 0.0L;
    unsigned int flag = PCVARIANT_COMPARE_OPT_AUTO;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[2] != PURC_VARIANT_INVALID) &&
         (!purc_variant_is_string (argv[2]))) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    option = purc_variant_get_string_const (argv[2]);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0)
        flag = PCVARIANT_COMPARE_OPT_CASELESS;
    else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0)
        flag = PCVARIANT_COMPARE_OPT_CASE;
    else if (strcasecmp (option, STRING_COMP_MODE_NUMBER) == 0)
        flag = PCVARIANT_COMPARE_OPT_NUMBER;
    else
        flag = PCVARIANT_COMPARE_OPT_AUTO;

    compare = purc_variant_compare_ex (argv[0], argv[1], flag);

    ret_var = purc_variant_make_number (compare);

    return ret_var;
}

// only for test now.
purc_variant_t purc_dvobj_ejson_new (void)
{
    static struct purc_dvobj_method method [] = {
        {"type",        type_getter, NULL},
        {"count",       count_getter, NULL},
        {"numberify",   numberify_getter, NULL},
        {"booleanize",  booleanize_getter, NULL},
        {"stringify",   stringify_getter, NULL},
        {"serialize",   serialize_getter, NULL},
        {"sort",        sort_getter, NULL},
        {"compare",     compare_getter, NULL}
    };

    return purc_dvobj_make_from_methods (method, PCA_TABLESIZE(method));
}
