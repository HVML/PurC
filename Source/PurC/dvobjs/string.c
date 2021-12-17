/*
 * @file string.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of string dynamic variant object.
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


static const char * get_next_segment (const char *data,
        const char *delim, size_t *length)
{
    const char *head = NULL;
    char *temp = NULL;

    *length = 0;

    if ((*data == 0x00) || (*delim == 0x00))
        return NULL;

    head = data;

    temp = strstr (head, delim);

    if (temp) {
        *length =  temp - head;
    }
    else {
        *length = strlen (head);
    }

    return head;
}

static purc_variant_t
contains_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
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
            (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *source = purc_variant_get_string_const (argv[0]);
    const char *sub = purc_variant_get_string_const (argv[1]);

    if (strstr (source, sub))
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}


static purc_variant_t
ends_with_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
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
            (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *source = purc_variant_get_string_const (argv[0]);
    size_t len_source = purc_variant_string_length (argv[0]) - 1;
    const char *sub = purc_variant_get_string_const (argv[1]);
    size_t len_sub = purc_variant_string_length (argv[1]) -1;

    if ((len_source == 0) || (len_sub == 0) || (len_source < len_sub)) {
        ret_var = purc_variant_make_boolean (false);
    }
    else {
        if (strncmp (source + len_source - len_sub, sub, len_sub) != 0)
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }

    return ret_var;
}


static purc_variant_t
explode_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    char *buf = NULL;

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
            (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *source = purc_variant_get_string_const (argv[0]);
    const char *delim = purc_variant_get_string_const (argv[1]);
    size_t len_delim = purc_variant_string_length (argv[1]) - 1;
    size_t length = 0;
    const char *head = get_next_segment (source, delim, &length);

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    while (head) {
        buf = malloc (length + 1);
        if (buf == NULL) {
            pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            purc_variant_unref (ret_var);
            return PURC_VARIANT_INVALID;
        }

        strncpy (buf, head, length + 1);
        *(buf + length)= 0x00;
        val = purc_variant_make_string_reuse_buff (buf, length, true);
        purc_variant_array_append (ret_var, val);
        purc_variant_unref (val);

        if (*(head + length) != 0x00)
            head = get_next_segment (head + length + len_delim,
                    delim, &length);
        else
            break;
    }

    return ret_var;
}


static purc_variant_t
implode_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    const char *delimiter = NULL;
    size_t delimiter_len = 0;
    size_t array_size = 0;
    size_t i = 0;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // argv[1] must be array
    if ((argv[1] != PURC_VARIANT_INVALID) &&
            !purc_variant_is_array (argv[1])) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    purc_variant_array_size (argv[1], &array_size);
    if (array_size == 0) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // delimiter can be null
    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
    }
    else {
        delimiter = purc_variant_get_string_const (argv[0]);
        purc_variant_string_bytes (argv[0], &delimiter_len);
    }

    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t total_size = 0;
    size_t string_size = 0;

    for (i = 0; i < array_size; i++) {
        val = purc_variant_array_get (argv[1], i);
        if (!purc_variant_is_string (val))
            continue;

        purc_variant_string_bytes (val, &string_size);
        if (string_size == 1)
            continue;

        total_size += (string_size - 1);
        if (total_size > STREAM_SIZE - 1) {
            total_size -= (string_size - 1);
            break;
        }
        purc_rwstream_write (rwstream,
                purc_variant_get_string_const(val), string_size - 1);

        if (i != array_size - 1) {
            if (delimiter_len > 1) {
                total_size += (delimiter_len - 1);
                if (total_size > STREAM_SIZE - 1) {
                    total_size -= (string_size - 1);
                    break;
                }
                purc_rwstream_write (rwstream, delimiter, delimiter_len - 1);
            }
        }
    }
    purc_rwstream_write (rwstream, "\0", 1);

    size_t rw_size = 0;
    const char *rw_string = purc_rwstream_get_mem_buffer (rwstream, &rw_size);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        ret_var = purc_variant_make_string (rw_string, false);
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
}


static purc_variant_t
shuffle_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    size_t size = purc_variant_string_length (argv[0]);
    if (size < 2 || size == PURC_VARIANT_BADSIZE) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    char *src = malloc (size);
    if (src == NULL) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    strcpy (src, purc_variant_get_string_const (argv[0]));
    *(src + size - 1) = 0x00;

    size_t i = 0;
    int random = 0;
    for(i =  0; i < size - 1; i++) {
        random = (rand () % (size - 1));
        int tmp = *(src + random);
        *(src + random) = *(src + i);
        *(src + i) = tmp;
    }

    ret_var = purc_variant_make_string_reuse_buff (src, size-1, false);

    return ret_var;
}


static purc_variant_t
replace_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
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

    if ((argv[1] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[2] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[2]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    size_t len_delim = purc_variant_string_length (argv[0]) - 1;
    if (len_delim == 0) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    len_delim = purc_variant_string_length (argv[1]) - 1;
    if (len_delim == 0) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *source = purc_variant_get_string_const (argv[0]);
    const char *delim = purc_variant_get_string_const (argv[1]);
    const char *replace = purc_variant_get_string_const (argv[2]);
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);

    size_t len_replace = purc_variant_string_length (argv[2]) - 1;
    size_t length = 0;
    const char *head = get_next_segment (source, delim, &length);

    while (head) {
        purc_rwstream_write (rwstream, head, length);

        if (*(head + length) != 0x00) {
            purc_rwstream_write (rwstream, replace, len_replace);
            head = get_next_segment (head + length + len_delim,
                    delim, &length);
        } else
            break;
    }

    size_t rw_size = 0;
    const char *rw_string = purc_rwstream_get_mem_buffer (rwstream, &rw_size);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        ret_var = purc_variant_make_string (rw_string, false);
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
}

static purc_variant_t
format_c_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *format = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);

    if ((argv == NULL) || (nr_args == 0)) {
        purc_rwstream_destroy (rwstream);
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_rwstream_destroy (rwstream);
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    else
        format = purc_variant_get_string_const (argv[0]);

    char buffer[16];
    int start = 0;
    int i = 0;
    int j = 1;
    int64_t i64 = 0;
    uint64_t u64 = 0;
    double number = 0;
    const char *temp_str = NULL;

    while (*(format + i) != 0x00) {
        if (*(format + i) == '%') {
            switch (*(format + i + 1)) {
                case 0x00:
                    break;
                case '%':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    purc_rwstream_write (rwstream, "%", 1);
                    i++;
                    start = i + 1;
                    break;
                case 'd':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == PURC_VARIANT_INVALID) {
                        purc_rwstream_destroy (rwstream);
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_longint (argv[j], &i64, false);
                    sprintf (buffer, "%lld", (long long int)i64);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 'o':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == PURC_VARIANT_INVALID) {
                        purc_rwstream_destroy (rwstream);
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_ulongint (argv[j], &u64, false);
                    sprintf (buffer, "%llo", (long long unsigned)u64);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 'u':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == PURC_VARIANT_INVALID) {
                        purc_rwstream_destroy (rwstream);
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_ulongint (argv[j], &u64, false);
                    sprintf (buffer, "%llu", (long long unsigned)u64);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 'x':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == PURC_VARIANT_INVALID) {
                        purc_rwstream_destroy (rwstream);
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_ulongint (argv[j], &u64, false);
                    sprintf (buffer, "%llx", (long long unsigned)u64);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 'f':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == PURC_VARIANT_INVALID) {
                        purc_rwstream_destroy (rwstream);
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_number (argv[j], &number, false);
                    sprintf (buffer, "%lf", number);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 's':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == PURC_VARIANT_INVALID) {
                        purc_rwstream_destroy (rwstream);
                        return PURC_VARIANT_INVALID;
                    }
                    if (!purc_variant_is_string (argv[0])) {
                        purc_rwstream_destroy (rwstream);
                        return PURC_VARIANT_INVALID;
                    }
                    temp_str = purc_variant_get_string_const (argv[j]);
                    purc_rwstream_write (rwstream, temp_str, strlen (temp_str));
                    i++;
                    j++;
                    start = i + 1;
                    break;
            }
        }
        i++;
    }

    if (i != start)
        purc_rwstream_write (rwstream, format + start, strlen (format + start));
    purc_rwstream_write (rwstream, "\0", 1);

    size_t rw_size = 0;
    size_t content_size = 0;
    char *rw_string = purc_rwstream_get_mem_buffer_ex (rwstream,
            &content_size, &rw_size, true);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        ret_var = purc_variant_make_string_reuse_buff (rw_string,
                rw_size, false);
        if(ret_var == PURC_VARIANT_INVALID) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
}

static purc_variant_t
format_p_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *format = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t tmp_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);
    int type = 0;
    char buffer[32];
    int index = 0;
    char *buf = NULL;
    size_t sz_stream = 0;
    size_t format_size = 0;
    purc_rwstream_t serialize = NULL;     // used for serialize

    if ((argv == NULL) || (nr_args == 0)) {
        purc_rwstream_destroy(rwstream);
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_rwstream_destroy(rwstream);
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    else {
        format = purc_variant_get_string_const (argv[0]);
        format_size = purc_variant_string_length (argv[0]);
    }

    if ((argv[1] != PURC_VARIANT_INVALID) &&
            (purc_variant_is_array (argv[1])))
        type = 0;
    else if ((argv[1] != PURC_VARIANT_INVALID) &&
            (purc_variant_is_object (argv[1])))
        type = 1;
    else {
        purc_rwstream_destroy(rwstream);
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if (type == 0) {
        const char *start = NULL;
        const char *end = NULL;
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (format, "{", &length);
        while (head) {
            purc_rwstream_write (rwstream, head, length);

            start = head + length + 1;
            head = pcdvobjs_get_next_option (head + length + 1, "}", &length);
            end = head + length;
            strncpy(buffer, start, end - start);
            *(buffer + (end - start)) = 0x00;
            pcdvobjs_remove_space (buffer);
            index = atoi (buffer);

            tmp_var = purc_variant_array_get (argv[1], index);
            if (tmp_var == PURC_VARIANT_INVALID) {
                purc_rwstream_destroy (rwstream);
                return PURC_VARIANT_INVALID;
            }

            serialize = purc_rwstream_new_buffer (32, STREAM_SIZE);
            purc_variant_serialize (tmp_var, serialize, 3, 0, &sz_stream);
            buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
            purc_rwstream_write (rwstream, buf + 1, sz_stream - 2);
            purc_rwstream_destroy (serialize);

            head = pcdvobjs_get_next_option (head + length + 1, "{", &length);
            end++;

            if (length == format_size - (head - format) - 1)
                break;
        }

        if (end != NULL)
            purc_rwstream_write (rwstream, end, strlen (end));
    }
    else {
        const char *start = NULL;
        const char *end = NULL;
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (format, "{", &length);
        while (head) {
            purc_rwstream_write (rwstream, head, length);

            start = head + length + 1;
            head = pcdvobjs_get_next_option (head + length + 1, "}", &length);
            end = head + length;
            strncpy(buffer, start, end - start);
            *(buffer + (end - start)) = 0x00;
            pcdvobjs_remove_space (buffer);

            tmp_var = purc_variant_object_get_by_ckey (argv[1], buffer);
            if (tmp_var == PURC_VARIANT_INVALID) {
                purc_rwstream_destroy (rwstream);
                return PURC_VARIANT_INVALID;
            }

            serialize = purc_rwstream_new_buffer (32, STREAM_SIZE);
            purc_variant_serialize (tmp_var, serialize, 3, 0, &sz_stream);
            buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
            purc_rwstream_write (rwstream, buf + 1, sz_stream - 2);
            purc_rwstream_destroy (serialize);

            head = pcdvobjs_get_next_option (head + length + 1, "{", &length);
            end++;

            if (length == format_size - (head - format) - 1)
                break;
        }

        if (end != NULL)
            purc_rwstream_write (rwstream, end, strlen (end));
    }

    size_t rw_size = 0;
    size_t content_size = 0;
    char *rw_string = purc_rwstream_get_mem_buffer_ex (rwstream,
            &content_size, &rw_size, true);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        ret_var = purc_variant_make_string_reuse_buff (rw_string,
                content_size, false);
        if(ret_var == PURC_VARIANT_INVALID) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
}

static purc_variant_t
strcat_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    if ((argv[1] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    size_t size = purc_variant_string_length (argv[0]) +
        purc_variant_string_length (argv[1]);

    if (size <= 2 || size == PURC_VARIANT_BADSIZE) {
        ret_var = purc_variant_make_string("", false);
    }
    else {
        char *dest = malloc (size - 1);
        if (dest == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            ret_var = PURC_VARIANT_INVALID;
        }
        else {
            strcpy(dest, purc_variant_get_string_const (argv[0]));
            strcat(dest, purc_variant_get_string_const (argv[1]));
            ret_var = purc_variant_make_string_reuse_buff (dest, size - 1, false);
        }
    }

    return ret_var;
}

static purc_variant_t
strlen_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    ret_var = purc_variant_make_ulongint (purc_variant_string_length (argv[0]));

    return ret_var;
}

static purc_variant_t
lower_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    size_t length = 0;
    purc_variant_string_bytes (argv[0], &length);
    const char * src = purc_variant_get_string_const (argv[0]);
    char *buf = malloc (length);
    if (buf == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        ret_var = PURC_VARIANT_INVALID;
    }
    else {
        for (size_t i = 0; i < length - 1; i++) {
            if (*(src + i) >= 'A' && *(src + i) <= 'Z')
                *(buf + i) = *(src + i) + 32;
            else
                *(buf + i) = *(src + i);
        }
        *(buf + length - 1) = 0x00;
        ret_var = purc_variant_make_string_reuse_buff (buf, length, false);
    }

    return ret_var;
}


static purc_variant_t
upper_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    size_t length = 0;
    purc_variant_string_bytes (argv[0], &length);
    const char * src = purc_variant_get_string_const (argv[0]);
    char *buf = malloc (length);
    if (buf == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        ret_var = PURC_VARIANT_INVALID;
    }
    else {
        for (size_t i = 0; i < length - 1; i++) {
            if (*(src + i) >= 'a' && *(src + i) <= 'z')
                *(buf + i) = *(src + i) - 32;
            else
                *(buf + i) = *(src + i);
        }
        *(buf + length - 1) = 0x00;
        ret_var = purc_variant_make_string_reuse_buff (buf, length, false);
    }

    return ret_var;
}

// only for test now.
purc_variant_t pcdvobjs_get_string (void)
{
    static struct pcdvobjs_dvobjs method [] = {
        {"contains",  contains_getter,  NULL},
        {"ends_with", ends_with_getter, NULL},
        {"explode",   explode_getter,   NULL},
        {"implode",   implode_getter,   NULL},
        {"shuffle",   shuffle_getter,   NULL},
        {"replace",   replace_getter,   NULL},
        {"format_c",  format_c_getter,  NULL},
        {"format_p",  format_p_getter,  NULL},
        {"strcat",    strcat_getter,    NULL},
        {"strlen",    strlen_getter,    NULL},
        {"upper",     upper_getter,     NULL},
        {"lower",     lower_getter,     NULL},
    };

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}
