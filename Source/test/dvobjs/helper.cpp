/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc/purc.h"
#include "private/avl.h"
#include "private/hashtable.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"

#include "../helpers.h"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <gtest/gtest.h>

void get_variant_total_info (size_t *mem, size_t *value, size_t *resv)
{
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    *mem = stat->sz_total_mem;
    *value = stat->nr_total_values;
    *resv = stat->nr_reserved;
}

static purc_variant_t getter(
        purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    purc_variant_t value = purc_variant_make_number (3.1415926);
    return value;
}

static purc_variant_t setter(
        purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    purc_variant_t value = purc_variant_make_number (2.71828828);
    return value;
}

static void rws_releaser (void* entity)
{
    UNUSED_PARAM(entity);
}

static struct purc_native_ops rws_ops = {
    .property_getter       = NULL,
    .property_setter       = NULL,
    .property_cleaner      = NULL,
    .property_eraser       = NULL,

    .updater               = NULL,
    .cleaner               = NULL,
    .eraser                = NULL,
    .match_observe         = NULL,

    .on_observe           = NULL,
    .on_forget            = NULL,
    .on_release           = rws_releaser,
};

static void replace_for_bsequence(char *buf, size_t *length_sub)
{
    size_t tail = 0;
    size_t head = 0;
    char chr = 0;
    unsigned char number = 0;
    unsigned char temp = 0;

    for (tail = 0; tail < *length_sub; tail++)  {
        if (*(buf + tail) == '\\')  {
            tail++;
            chr = *(buf + tail);
            if ((chr >= '0') && (chr <= '9'))
                number = chr - '0';
            else if ((chr >= 'a') && (chr <= 'z'))
                number = chr - 'a';
            else if ((chr >= 'A') && (chr <= 'Z'))
                number = chr - 'A';
            number = number << 4;

            tail++;
            chr = *(buf + tail);
            if ((chr >= '0') && (chr <= '9'))
                temp = chr - '0';
            else if ((chr >= 'a') && (chr <= 'z'))
                temp = chr - 'a';
            else if ((chr >= 'A') && (chr <= 'Z'))
                temp = chr - 'A';
            number |= temp;

            *(buf + head) = number;
            head++;
        } else {
            *(buf + head) = *(buf + tail);
            head++;
        }
    }

    *length_sub = head;

    return;
}

purc_variant_t get_variant (char *buf, size_t *length)
{
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;
    char *temp = NULL;
    char *temp_end = NULL;
    char tag[64];
    double d = 0.0;
    int64_t i64;
    uint64_t u64;
    long double ld = 0.0L;
    int number = 0;
    int i = 0;
    size_t length_sub = 0;

    *length = 0;

    temp = strchr (buf, ':');
    snprintf (tag, (temp - buf + 1), "%s", buf);

    switch (*tag) {
        case 'a':
        case 'A':
            switch (*(tag + 1))  {
                case 'r':       // array
                case 'R':
                    temp_end = strchr (temp + 1, ':');
                    snprintf (tag, (temp_end - temp), "%s", temp + 1);
                    number = atoi (tag);
                    temp = temp_end + 1;

                    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
                    for (i = 0; i < number; i++) {
                        val = get_variant (temp, &length_sub);
                        purc_variant_array_append (ret_var, val);
                        purc_variant_unref (val);
                        if (i < number - 1)
                            temp += (length_sub + 1);
                    }
                    *length = temp - buf + length_sub;
                    break;
                case 't':       // atomstring
                case 'T':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_atom_string (temp + 1, false);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'b':
        case 'B':
            switch (*(tag + 1))  {
                case 'o':       // boolean
                case 'O':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    if (strncasecmp (temp + 1, "true", 4) == 0)
                        ret_var = purc_variant_make_boolean (true);
                    else
                        ret_var = purc_variant_make_boolean (false);
                    *length = temp_end - buf;
                    break;
                case 's':       // byte sequence
                case 'S':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    length_sub = temp_end - temp - 1;
                    replace_for_bsequence(temp + 1, &length_sub);
                    ret_var = purc_variant_make_byte_sequence (temp + 1,
                            length_sub);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'd':               // dynamic
        case 'D':
            temp_end = strchr (buf, ';');
            *temp_end = 0x00;
            ret_var = purc_variant_make_dynamic (getter, setter);
            *length = temp_end - buf;
            break;
        case 'i':
        case 'I':
            temp_end = strchr (buf, ';');
            *length = temp_end - buf;
            ret_var = PURC_VARIANT_INVALID;
            break;
        case 'l':
        case 'L':
            switch (*(tag + 4))  {
                case 'd':       // long double
                case 'D':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ld = atof (temp + 1);
                    ret_var = purc_variant_make_longdouble (ld);
                    *length = temp_end - buf;
                    break;
                case 'i':       // long int
                case 'I':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    i64 = atoll (temp + 1);
                    ret_var = purc_variant_make_longint (i64);
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'n':
        case 'N':
            switch (*(tag + 2))  {
                case 't':       // native;
                case 'T':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_native ((void *)"hello world",
                            &rws_ops);
                    *length = temp_end - buf;
                    break;
                case 'l':       // null;
                case 'L':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_null ();
                    *length = temp_end - buf;
                    break;
                case 'm':       // number
                case 'M':
                    temp_end = strchr (temp + 1, ';');
                    *temp_end = 0x00;
                    d = atof (temp + 1);
                    ret_var = purc_variant_make_number (d);
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'o':               // object
        case 'O':
            temp_end = strchr (temp + 1, ':');
            snprintf (tag, (temp_end - temp), "%s", temp + 1);
            number = atoi (tag);
            temp = temp_end + 1;

            ret_var = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                                                    PURC_VARIANT_INVALID);
            for (i = 0; i < number; i++) {
                // get key
                purc_variant_t key = PURC_VARIANT_INVALID;
                temp = strchr (temp, '\"');
                temp_end = strchr (temp + 1, '\"');
                snprintf (tag, temp_end - temp, "%s", temp + 1);
                key = purc_variant_make_string(tag, true);

                // get value
                temp = temp_end + 2;
                *length = temp - buf;
                val = get_variant (temp, &length_sub);
                purc_variant_object_set (ret_var, key, val);

                purc_variant_unref (key);
                purc_variant_unref (val);
                if (i < number - 1)
                    temp += (length_sub + 1);
            }
            *length = temp - buf + length_sub;
            break;
        case 's':
        case 'S':
            switch (*(tag + 1))  {
                case 'e':       // set
                case 'E':
                    temp_end = strchr (temp + 1, ':');
                    snprintf (tag, (temp_end - temp), "%s", temp + 1);
                    number = atoi (tag);
                    temp = temp_end + 1;

                    ret_var = purc_variant_make_set_by_ckey(0, NULL, NULL);
                    for (i = 0; i < number; i++) {
                        val = get_variant (temp, &length_sub);
                        PC_ASSERT(purc_variant_is_object(val));
                        purc_variant_set_add (ret_var, val, true);
                        purc_variant_unref (val);
                        if (i < number - 1)
                            temp += (length_sub + 1);
                    }
                    *length = temp - buf + length_sub;
                    break;
                case 't':       // sting
                case 'T':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_string (temp + 1, false);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'u':
        case 'U':
            switch (*(tag + 1))  {
                case 'l':       // unsigned long int
                case 'L':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    u64 = atoll (temp + 1);
                    ret_var = purc_variant_make_ulongint (u64);
                    *length = temp_end - buf;
                    break;
                case 'n':       // undefined
                case 'N':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_undefined ();
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        default:
            temp_end = strchr (buf, ';');
            *length = temp_end - buf;
            ret_var = PURC_VARIANT_INVALID;
            break;
    }

    return ret_var;
}

