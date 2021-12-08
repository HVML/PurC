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

typedef struct __dvobjs_ejson_element {
    int index;
    char *name;
} dvobjs_ejson_element;

typedef struct __dvobjs_ejson_sort {
    bool asc;
    bool caseless;
} dvobjs_ejson_sort;

static dvobjs_ejson_sort sort_type;

static purc_variant_t
count_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    number = purc_variant_numberify (argv[0]);
    ret_var = purc_variant_make_number (number);

    return ret_var;
}

static purc_variant_t
booleanize_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if (purc_variant_booleanize (argv[0]))
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
stringify_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t serialize = NULL;
    char *buf = NULL;
    size_t sz_stream = 0;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    serialize = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_variant_serialize (argv[0], serialize, 3, 0, &sz_stream);
    buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
    purc_rwstream_destroy (serialize);

    ret_var = purc_variant_make_string_reuse_buff (buf, sz_stream + 1, false);

    return ret_var;
}

static int mycompare (const void *elem1, const void *elem2)
{
    int ret = 0;
    dvobjs_ejson_element *p1 = (dvobjs_ejson_element *)elem1;
    dvobjs_ejson_element *p2 = (dvobjs_ejson_element *)elem2;

    if (sort_type.caseless)
        ret = strcasecmp (p1->name, p2->name);
    else
        ret = strcmp (p1->name, p2->name);

    if (!sort_type.asc)
        ret = -1 * ret;

    return ret;
}

static purc_variant_t
sort_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    size_t i = 0;
    size_t totalsize = 0;
    struct purc_variant_set_iterator *it_set = NULL;
    bool having = false;
    const char *option = NULL;
    const char *order = NULL;
    dvobjs_ejson_element *elements = NULL;

    sort_type.asc = true;
    sort_type.caseless = false;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) && !(purc_variant_is_array (argv[0])
                || purc_variant_is_set (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get sort order: asc, desc
    if ((argv[1] == PURC_VARIANT_INVALID) ||
                (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    order = purc_variant_get_string_const (argv[1]);
    if (strcasecmp (order, STRING_COMP_MODE_DESC) == 0)
        sort_type.asc = false;

    // get sort option: case, caseless
    if ((nr_args == 3) && (argv[2] != PURC_VARIANT_INVALID) &&
                (purc_variant_is_string (argv[2]))) {
        option = purc_variant_get_string_const (argv[2]);
        if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0)
            sort_type.caseless = true;
    }

    // it is the array
    if (purc_variant_is_array (argv[0])) {
        totalsize = purc_variant_array_get_size (argv[0]);
        elements = calloc ((unsigned)totalsize, sizeof(dvobjs_ejson_element));
        if (elements == NULL) {
            pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            return PURC_VARIANT_INVALID;
        }

        for (i = 0; i < totalsize; ++i) {
            val = purc_variant_array_get(argv[0], i);
            elements[i].index = i;
            purc_variant_stringify_alloc (&elements[i].name, val);
        }

        qsort (elements, totalsize, sizeof(dvobjs_ejson_element), mycompare);

        // get new sort
        // original: 0, 1, 2, 3, 4, 5
        // now:      3, 2, 0, 5, 4, 1


        for (i = 0; i < totalsize; ++i) {
            if (elements[i].name)
                free (elements[i].name);
        }
        free (elements);

    } else {    // it is the set
        totalsize = purc_variant_set_get_size (argv[0]);
        elements = calloc ((unsigned)totalsize, sizeof(dvobjs_ejson_element));
        if (elements == NULL) {
            pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            return PURC_VARIANT_INVALID;
        }

        it_set = purc_variant_set_make_iterator_begin(argv[0]);
        while (it_set) {
            val = purc_variant_set_iterator_get_value(it_set);

            elements[i].index = i;
            purc_variant_stringify_alloc (&elements[i].name, val);

            having = purc_variant_set_iterator_next(it_set);
            if (!having)
                break;
        }
        if (it_set)
            purc_variant_set_release_iterator(it_set);

        qsort (elements, totalsize, sizeof(dvobjs_ejson_element), mycompare);

        // get new sort
        // original: 0, 1, 2, 3, 4, 5
        // now:      3, 2, 0, 5, 4, 1

        for (i = 0; i < totalsize; ++i) {
            if (elements[i].name)
                free (elements[i].name);
        }
        free (elements);
    }

    return ret_var;
}

static purc_variant_t
compare_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    const char *option = NULL;
    double number1 = 0.0L;
    double number2 = 0.0L;
    char *buf1 = NULL;
    char *buf2 = NULL;
    int compare = 0;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    option = purc_variant_get_string_const (argv[2]);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0) {
        purc_variant_stringify_alloc (&buf1, argv[0]);
        purc_variant_stringify_alloc (&buf2, argv[1]);
        compare = strcasecmp (buf1, buf2);
        free (buf1);
        free (buf2);
    } else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0) {
        purc_variant_stringify_alloc (&buf1, argv[0]);
        purc_variant_stringify_alloc (&buf2, argv[1]);
        compare = strcmp (buf1, buf2);
        free (buf1);
        free (buf2);
    } else if (strcasecmp (option, STRING_COMP_MODE_NUMBER) == 0) {
        number1 = purc_variant_numberify (argv[0]);
        number2 = purc_variant_numberify (argv[1]);

        if (number1 == number2)
            compare = 0;
        else if (number1 < number2)
            compare = -1;
        else
            compare = 1;
    } else if (strcasecmp (option, STRING_COMP_MODE_AUTO) == 0) {
        int type1 = purc_variant_get_type (argv[0]);
        int type2 = purc_variant_get_type (argv[1]);
        if ((type1 > PURC_VARIANT_TYPE_BOOLEAN) &&
                (type1 < PURC_VARIANT_TYPE_ATOMSTRING) &&
                (type2 > PURC_VARIANT_TYPE_BOOLEAN) &&
                (type2 < PURC_VARIANT_TYPE_ATOMSTRING)) {
            number1 = purc_variant_numberify (argv[0]);
            number2 = purc_variant_numberify (argv[1]);

            if (number1 == number2)
                compare = 0;
            else if (number1 < number2)
                compare = -1;
            else
                compare = 1;
        } else {
            purc_variant_stringify_alloc (&buf1, argv[0]);
            purc_variant_stringify_alloc (&buf2, argv[1]);
            compare = strcmp (buf1, buf2);
            free (buf1);
            free (buf2);
        }
    }

    ret_var = purc_variant_make_longint (compare);

    return ret_var;
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
