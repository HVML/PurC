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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

static uint64_t get_variant_number (purc_variant_t var)
{
    if (var == PURC_VARIANT_INVALID)
        return 0;

    uint64_t number = 1;
    struct purc_variant_object_iterator *it_obj = NULL;
    struct purc_variant_set_iterator *it_set = NULL;
    purc_variant_t val = PURC_VARIANT_INVALID;
    size_t i = 0;
    bool having = false;
    enum purc_variant_type type = purc_variant_get_type (var);

    switch ((int)type) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
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
            break;
        case PURC_VARIANT_TYPE_OBJECT:
            it_obj = purc_variant_object_make_iterator_begin(var);
            while (it_obj) {
                val = purc_variant_object_iterator_get_value(it_obj);
                number += get_variant_number (val);

                having = purc_variant_object_iterator_next(it_obj);
                if (!having)
                    break;
            }
            if (it_obj)
                purc_variant_object_release_iterator(it_obj);

            break;

        case PURC_VARIANT_TYPE_ARRAY:
            for (i = 0; i < purc_variant_array_get_size (var); ++i) {
                val = purc_variant_array_get(var, i);
                number += get_variant_number (val);
            }

            break;

        case PURC_VARIANT_TYPE_SET:
            it_set = purc_variant_set_make_iterator_begin(var);
            while (it_set) {
                val = purc_variant_set_iterator_get_value(it_set);
                number += get_variant_number (val);
                having = purc_variant_set_iterator_next(it_set);
                if (!having)
                    break;
            }
            if (it_set)
                purc_variant_set_release_iterator(it_set);
            break;
        default:
            number = 0;
            break;
    }

    return number;
}

static purc_variant_t
number_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    uint64_t number = 0;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_array (argv[0])) &&
             (!purc_variant_is_object (argv[0])) &&
             (!purc_variant_is_set (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    number = get_variant_number (argv[0]);
    ret_var = purc_variant_make_ulongint (number);

    return ret_var;
}

static purc_variant_t
type_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if (argv[0] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    switch ((int)purc_variant_get_type (argv[0])) {
        case PURC_VARIANT_TYPE_NULL:
            ret_var = purc_variant_make_string (VARIANT_STRING_NULL, false);
            break;
        case PURC_VARIANT_TYPE_UNDEFINED:
            ret_var = purc_variant_make_string (VARIANT_STRING_UNDEFINED,
                    false);
            break;
        case PURC_VARIANT_TYPE_BOOLEAN:
            ret_var = purc_variant_make_string (VARIANT_STRING_BOOLEAN, false);
            break;
        case PURC_VARIANT_TYPE_NUMBER:
            ret_var = purc_variant_make_string (VARIANT_STRING_NUMBER, false);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            ret_var = purc_variant_make_string (VARIANT_STRING_LONGINT, false);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            ret_var = purc_variant_make_string (VARIANT_STRING_ULONGINT, false);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            ret_var = purc_variant_make_string (VARIANT_STRING_LONGDOUBLE,
                    false);
            break;
        case PURC_VARIANT_TYPE_ATOMSTRING:
            ret_var = purc_variant_make_string (VARIANT_STRING_ATOMSTRING,
                    false);
            break;
        case PURC_VARIANT_TYPE_STRING:
            ret_var = purc_variant_make_string (VARIANT_STRING_STRING, false);
            break;
        case PURC_VARIANT_TYPE_BSEQUENCE:
            ret_var = purc_variant_make_string (VARIANT_STRING_BYTESEQUENCE,
                    false);
            break;
        case PURC_VARIANT_TYPE_DYNAMIC:
            ret_var = purc_variant_make_string (VARIANT_STRING_DYNAMIC, false);
            break;
        case PURC_VARIANT_TYPE_NATIVE:
            ret_var = purc_variant_make_string (VARIANT_STRING_NATIVE, false);
            break;
        case PURC_VARIANT_TYPE_OBJECT:
            ret_var = purc_variant_make_string (VARIANT_STRING_OBJECT, false);
            break;
        case PURC_VARIANT_TYPE_ARRAY:
            ret_var = purc_variant_make_string (VARIANT_STRING_ARRAY, false);
            break;
        case PURC_VARIANT_TYPE_SET:
            ret_var = purc_variant_make_string (VARIANT_STRING_SET, false);
            break;
        default:
            break;
    }

    return ret_var;

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
