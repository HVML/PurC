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
#include "helper.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <memory.h>
#include <stdlib.h>

static bool reg_cmp (const char *buf1, const char *buf2)
{
    regex_t reg;
    int err = 0;
    int number = 10;
    regmatch_t pmatch[number];

    if (regcomp (&reg, buf1, REG_EXTENDED) < 0) {
        return false;
    }

    err = regexec(&reg, buf2, number, pmatch, 0);

    if (err == REG_NOMATCH) {
        regfree (&reg);
        return false;
    }
    else if (err) {
        regfree (&reg);
        return false;
    }

    if (pmatch[0].rm_so == -1) {
        regfree (&reg);
        return false;
    }

    regfree (&reg);

    return true;
}

static bool test_variant (purc_variant_t var)
{
    if (var == PURC_VARIANT_INVALID)
        return false;

    double number = 0.0d;
    bool ret = false;
    enum purc_variant_type type = purc_variant_get_type (var);

    switch ((int)type) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
            break;
        case PURC_VARIANT_TYPE_BOOLEAN:
            purc_variant_cast_to_number (var, &number, false);
            if (number)
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


static long double get_variant_value (purc_variant_t var)
{
    if (var == PURC_VARIANT_INVALID)
        return false;

    long double number = 0.0d;
    size_t length = 0;
    long int templongint = 0;
    uintptr_t temppointer = 0;
    struct purc_variant_object_iterator* it_obj = NULL;
    struct purc_variant_set_iterator* it_set = NULL;
    purc_variant_t val = PURC_VARIANT_INVALID;
    size_t i = 0;
    bool having = false;
    enum purc_variant_type type = purc_variant_get_type (var);

    switch ((int)type) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
            break;
        case PURC_VARIANT_TYPE_BOOLEAN:
            purc_variant_cast_to_long_double (var, &number, false);
            if (number)
                number = 1.0d;
            break;
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            purc_variant_cast_to_long_double (var, &number, false);
            break;
        case PURC_VARIANT_TYPE_ATOMSTRING:
            number = strtold (purc_variant_get_atom_string_const (var), NULL);
            break;
        case PURC_VARIANT_TYPE_STRING:
            number = strtold (purc_variant_get_string_const (var), NULL);
            break;
        case PURC_VARIANT_TYPE_BSEQUENCE:
            length = purc_variant_sequence_length (var);
            if (length > 8)
                memcpy (&templongint, purc_variant_get_bytes_const (var,
                                            &length) + length - 8, 8);
            else
                memcpy (&templongint, purc_variant_get_bytes_const (var,
                                            &length), length);
            number = (long double) templongint;
            break;
        case PURC_VARIANT_TYPE_DYNAMIC:
            temppointer = (uintptr_t)purc_variant_dynamic_get_getter (var);
            temppointer += (uintptr_t)purc_variant_dynamic_get_setter (var);
            number = (long double) temppointer;
            break;
        case PURC_VARIANT_TYPE_NATIVE:
            temppointer = (uintptr_t)purc_variant_native_get_entity (var);
            number = (long double) temppointer;
            break;
        case PURC_VARIANT_TYPE_OBJECT:
            it_obj = purc_variant_object_make_iterator_begin(var);
            while (it_obj) {
                val = purc_variant_object_iterator_get_value(it_obj);
                number += get_variant_value (val);

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

                number += get_variant_value (val);
            }

            break;

        case PURC_VARIANT_TYPE_SET:
            it_set = purc_variant_set_make_iterator_begin(var);
            while (it_set) {
                val = purc_variant_set_iterator_get_value(it_set);

                number += get_variant_value (val);

                having = purc_variant_set_iterator_next(it_set);
                if (!having)
                    break;
            }
            if (it_set)
                purc_variant_set_release_iterator(it_set);

            break;

        default:
            break;
    }

    return number;
}

static purc_variant_t
logical_not (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((nr_args == 1) && argv[0] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if (test_variant (argv[0]))
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
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    while (argv[i]) {
        if (!test_variant (argv[i])) {
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
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    while (argv[i]) {
        if (test_variant (argv[i])) {
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    unsigned judge1 = 0x00;
    unsigned judge2 = 0x00;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (argv[1] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if (test_variant (argv[0]))
        judge1 = 0x01;

    if (test_variant (argv[1]))
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double value1 = 0.0d;
    long double value2 = 0.0d;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (argv[1] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    value1 = get_variant_value (argv[0]);
    value2 = get_variant_value (argv[1]);

    if (fabsl (value1 - value2) < 1.0E-10)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
logical_ne (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double value1 = 0.0d;
    long double value2 = 0.0d;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (argv[1] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    value1 = get_variant_value (argv[0]);
    value2 = get_variant_value (argv[1]);

    if (fabsl (value1 - value2) >= 1.0E-10)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
logical_gt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double value1 = 0.0d;
    long double value2 = 0.0d;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (argv[1] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    value1 = get_variant_value (argv[0]);
    value2 = get_variant_value (argv[1]);

    if (value1 > value2)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
logical_ge (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double value1 = 0.0d;
    long double value2 = 0.0d;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (argv[1] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    value1 = get_variant_value (argv[0]);
    value2 = get_variant_value (argv[1]);

    if (value1 >= value2)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
logical_lt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double value1 = 0.0d;
    long double value2 = 0.0d;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (argv[1] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    value1 = get_variant_value (argv[0]);
    value2 = get_variant_value (argv[1]);

    if (value1 < value2)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
logical_le (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double value1 = 0.0d;
    long double value2 = 0.0d;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (argv[1] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    value1 = get_variant_value (argv[0]);
    value2 = get_variant_value (argv[1]);

    if (value1 <= value2)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
logical_streq (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 3)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char * option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;

    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, "caseless") == 0) {
        if (strcasecmp (buf1, buf2) == 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, "case") == 0) {
        if (strcmp (buf1, buf2) == 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, "wildcard") == 0) {
        if (wildcard_cmp (buf2, buf1))
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, "reg") == 0) {
        if (reg_cmp (buf1, buf2))
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
logical_strne (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 3)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char * option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;


    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, "caseless") == 0) {
        if (strcasecmp (buf1, buf2) == 0)
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }
    else if (strcasecmp (option, "case") == 0) {
        if (strcmp (buf1, buf2) == 0)
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }
    else if (strcasecmp (option, "wildcard") == 0) {
        if (wildcard_cmp (buf2, buf1))
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }
    else if (strcasecmp (option, "reg") == 0) {
        if (reg_cmp (buf1, buf2))
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
logical_strgt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 3)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char * option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;

    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, "caseless") == 0) {
        if (strcasecmp (buf1, buf2) > 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, "case") == 0) {
        if (strcmp (buf1, buf2) > 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
logical_strge (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 3)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char * option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;


    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, "caseless") == 0) {
        if (strcasecmp (buf1, buf2) >= 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, "case") == 0) {
        if (strcmp (buf1, buf2) >= 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
logical_strlt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 3)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char * option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;


    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, "caseless") == 0) {
        if (strcasecmp (buf1, buf2) < 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, "case") == 0) {
        if (strcmp (buf1, buf2) < 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
logical_strle (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 3)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char * option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;


    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, "caseless") == 0) {
        if (strcasecmp (buf1, buf2) <= 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, "case") == 0) {
        if (strcmp (buf1, buf2) <= 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
logical_eval (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_object (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

#if 0
    size_t length = purc_variant_string_length (argv[0]);
    struct pcdvobjs_logical_param myparam = {0, argv[1]}; /* my instance data */
    yyscan_t lexer;                 /* flex instance data */

    if(logicallex_init_extra(&myparam, &lexer)) {
        return PURC_VARIANT_INVALID;
    }

    YY_BUFFER_STATE buffer = logical_scan_bytes (
                purc_variant_get_string_const (argv[0]), length, lexer);
    logical_switch_to_buffer (buffer, lexer);
    logicalparse(&myparam, lexer);
    logical_delete_buffer(buffer, lexer);
    logicallex_destroy (lexer);
#else // ! 0
    struct pcdvobjs_logical_param myparam = {0.0d, argv[1]};
    logical_parse(purc_variant_get_string_const(argv[0]), &myparam);
#endif // 0

    if (myparam.result)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);
    
    return ret_var;
}

// only for test now.
purc_variant_t pcdvojbs_get_logical (void)
{
    static struct pcdvojbs_dvobjs method [] = {
        {"not",   logical_not, NULL},
        {"and",   logical_and, NULL},
        {"or",    logical_or, NULL},
        {"xor",   logical_xor, NULL},
        {"eq",    logical_eq, NULL},
        {"ne",    logical_ne, NULL},
        {"gt",    logical_gt, NULL},
        {"ge",    logical_ge, NULL},
        {"lt",    logical_lt, NULL},
        {"le",    logical_le, NULL},
        {"streq", logical_streq, NULL},
        {"strne", logical_strne, NULL},
        {"strgt", logical_strgt, NULL},
        {"strge", logical_strge, NULL},
        {"strlt", logical_strlt, NULL},
        {"strle", logical_strle, NULL},
        {"eval",  logical_eval, NULL} };

    size_t size = sizeof (method) / sizeof (struct pcdvojbs_dvobjs);
    return pcdvobjs_make_dvobjs (method, size);
}
