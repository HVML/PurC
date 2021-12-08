/*
 * @file helper.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of tools for all files in this directory.
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

# include <math.h>

#include "private/instance.h"
#include "private/errors.h"
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "helper.h"

const char *pcdvobjs_get_next_option (const char *data,
        const char *delims, size_t *length)
{
    const char *head = data;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00))
        return NULL;

    *length = 0;

    while (*data != 0x00) {
        temp = strchr (delims, *data);
        if (temp) {
            if (head == data) {
                head = data + 1;
            } else
                break;
        }
        data++;
    }

    *length = data - head;
    if (*length == 0)
        head = NULL;

    return head;
}

const char *pcdvobjs_get_prev_option (const char *data,
        size_t str_len, const char *delims, size_t *length)
{
    const char *head = NULL;
    size_t tail = *length;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00) ||
            (str_len == 0))
        return NULL;

    *length = 0;

    while (str_len) {
        temp = strchr (delims, *(data + str_len - 1));
        if (temp) {
            if (tail == str_len) {
                str_len--;
                tail = str_len;
            } else 
                break;
        }
        str_len--;
    }

    *length = tail - str_len;
    if (*length == 0)
        head = NULL;
    else
        head = data + str_len;

    return head;
}

const char *pcdvobjs_remove_space (char *buffer)
{
    int i = 0;
    int j = 0;
    while (*(buffer + i) != 0x00) {
        if (*(buffer + i) != ' ') {
            *(buffer + j) = *(buffer + i);
            j++;
        }
        i++;
    }
    *(buffer + j) = 0x00;

    return buffer;
}

bool pcdvobjs_wildcard_cmp (const char *str1, const char *pattern)
{
    if (str1 == NULL)
        return false;
    if (pattern == NULL)
        return false;

    int len1 = strlen (str1);
    int len2 = strlen (pattern);
    int mark = 0;
    int p1 = 0;
    int p2 = 0;

    while ((p1 < len1) && (p2<len2)) {
        if (pattern[p2] == '?') {
            p1++;
            p2++;
            continue;
        }
        if (pattern[p2] == '*') {
            p2++;
            mark = p2;
            continue;
        }
        if (str1[p1] != pattern[p2]) {
            if (p1 == 0 && p2 == 0)
                return false;
            p1 -= p2 - mark - 1;
            p2 = mark;
            continue;
        }
        p1++;
        p2++;
    }
    if (p2 == len2) {
        if (p1 == len1)
            return true;
        if (pattern[p2 - 1] == '*')
            return true;
    }
    while (p2 < len2) {
        if (pattern[p2] != '*')
            return false;
        p2++;
    }
    return true;
}

purc_variant_t pcdvobjs_make_dvobjs (const struct pcdvobjs_dvobjs *method,
                                    size_t size)
{
    size_t i = 0;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t ret_var= purc_variant_make_object (0, PURC_VARIANT_INVALID,
                                                    PURC_VARIANT_INVALID);

    if (ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (i = 0; i < size; i++) {
        val = purc_variant_make_dynamic (method[i].getter, method[i].setter);
        if (val == PURC_VARIANT_INVALID) {
            goto error;
        }

        if (!purc_variant_object_set_by_static_ckey (ret_var,
                    method[i].name, val)) {
            goto error;
        }

        purc_variant_unref (val);
    }

    return ret_var;

error:
    purc_variant_unref (ret_var);
    return PURC_VARIANT_INVALID;
}

long double pcdvobjs_get_variant_value (purc_variant_t var)
{
    if (var == PURC_VARIANT_INVALID)
        return 0.0;

    size_t i = 0;
    size_t length = 0;
    long double number = 0.0;
    long int templongint = 0;
    struct purc_variant_object_iterator *it_obj = NULL;
    struct purc_variant_set_iterator *it_set = NULL;
    purc_variant_t val = PURC_VARIANT_INVALID;
    bool having = false;
    enum purc_variant_type type = purc_variant_get_type (var);
    purc_dvariant_method dynamic_func = NULL;
    purc_nvariant_method native_func = NULL;
    struct purc_native_ops *ops = NULL;

    switch ((int)type) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            purc_variant_cast_to_long_double (var, &number, false);
            if (number)
                number = 1.0;
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
            dynamic_func = purc_variant_dynamic_get_getter (var);
            if (dynamic_func) {
                val = dynamic_func (NULL, 0, NULL);
                number = pcdvobjs_get_variant_value (val);
            }
            break;

        case PURC_VARIANT_TYPE_NATIVE:
            ops = purc_variant_native_get_ops (var);
            if (ops) {
                native_func = ops->property_getter("__number");
                if (native_func) {
                    val = native_func (purc_variant_native_get_entity (var),
                            0, NULL);
                    number = pcdvobjs_get_variant_value (val);
                }
            }
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            it_obj = purc_variant_object_make_iterator_begin(var);
            while (it_obj) {
                val = purc_variant_object_iterator_get_value(it_obj);
                number += pcdvobjs_get_variant_value (val);

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

                number += pcdvobjs_get_variant_value (val);
            }

            break;

        case PURC_VARIANT_TYPE_SET:
            it_set = purc_variant_set_make_iterator_begin(var);
            while (it_set) {
                val = purc_variant_set_iterator_get_value(it_set);

                number += pcdvobjs_get_variant_value (val);

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

bool pcdvobjs_test_variant (purc_variant_t var)
{
    if (var == PURC_VARIANT_INVALID)
        return false;

    long double number = 0.0L;
    bool ret = false;
    enum purc_variant_type type = purc_variant_get_type (var);

    // in this step, ret = true, means: need numberify
    switch ((int)type) {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
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

    if (ret) {
        ret = false;
        number = pcdvobjs_get_variant_value (var);
        if (fabsl (number) > 1.0E-10)
            ret = true;
    }

    return ret;
}

