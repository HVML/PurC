/*
 * @file string.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of string dynamic variant object.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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

#define _GNU_SOURCE
#include "config.h"

#include "private/errors.h"
#include "private/dvobjs.h"
#include "private/utils.h"
#include "private/variant.h"
#include "private/atom-buckets.h"
#include "private/ports.h"
#include "purc-variant.h"
#include "helper.h"

#include "purc-dvobjs.h"

static bool is_all_ascii(const char *str)
{
    while (*str) {
        if (*str & 0x80)
            return false;
        str++;
    }

    return true;
}

static const char *get_next_segment(const char *data,
        const char *delim, size_t *length)
{
    const char *head = NULL;
    char *temp = NULL;

    *length = 0;
    if ((*data == 0x00) || (*delim == 0x00))
        return NULL;

    head = data;
    temp = strstr(head, delim);

    if (temp) {
        *length =  temp - head;
    }
    else {
        *length = strlen(head);
    }

    return head;
}

static purc_variant_t
nr_bytes_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    size_t nr_bytes;
    if (!purc_variant_get_bytes_const(argv[0], &nr_bytes)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    /* override nr_bytes with string length if argv[1] is false
       and argv[0] is a string. */
    if (nr_args > 1 && !purc_variant_booleanize(argv[1])) {
        size_t len;
        if (purc_variant_get_string_const_ex(argv[0], &len)) {
            nr_bytes = len;
        }
    }

    return purc_variant_make_ulongint(nr_bytes);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_ulongint(0);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
nr_chars_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    size_t nr_chars;
    if (!purc_variant_string_chars(argv[0], &nr_chars)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    return purc_variant_make_ulongint(nr_chars);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
contains_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char* haystack, *needle;
    size_t len_haystack, len_needle;
    haystack = purc_variant_get_string_const_ex(argv[0], &len_haystack);
    if (haystack == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    needle = purc_variant_get_string_const_ex(argv[1], &len_needle);
    if (needle == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    bool ignore_case = false;
    if (nr_args > 2) {
        ignore_case = purc_variant_booleanize(argv[2]);
    }

    bool result;
    if (len_needle == 0) {
        result = true;
    }
    else if (!ignore_case && len_needle > len_haystack) {
        result = false;
    }
    else if (ignore_case) {
        result  = pcutils_strcasestr(haystack, needle) != NULL;
    }
    else {
        result = strstr(haystack, needle) != NULL;
    }

    return purc_variant_make_boolean(result);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
starts_with_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char* haystack, *needle;
    size_t len_haystack, len_needle;
    haystack = purc_variant_get_string_const_ex(argv[0], &len_haystack);
    if (haystack == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    needle = purc_variant_get_string_const_ex(argv[1], &len_needle);
    if (needle == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    bool ignore_case = false;
    if (nr_args > 2) {
        ignore_case = purc_variant_booleanize(argv[2]);
    }

    bool result;
    if (len_needle == 0) {
        result = true;
    }
    else if (!ignore_case && len_needle > len_haystack) {
        result = false;
    }
    else if (ignore_case) {
        result = pcutils_strncasecmp(haystack, needle, len_needle) == 0;
    }
    else {
        result = strncmp(haystack, needle, len_needle) == 0;
    }

    return purc_variant_make_boolean(result);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
ends_with_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char* haystack, *needle;
    size_t len_haystack, len_needle;
    haystack = purc_variant_get_string_const_ex(argv[0], &len_haystack);
    if (haystack == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    needle = purc_variant_get_string_const_ex(argv[1], &len_needle);
    if (needle == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    bool ignore_case = false;
    if (nr_args > 2) {
        ignore_case = purc_variant_booleanize(argv[2]);
    }

    bool result;
    if (len_needle == 0) {
        result = true;
    }
    else if (!ignore_case && len_needle > len_haystack) {
        result = false;
    }
    else if (ignore_case) {
        result = pcutils_strncasecmp(haystack + len_haystack - len_needle,
                needle, len_needle) == 0;
    }
    else {
        result = strncmp(haystack + len_haystack - len_needle,
                needle, len_needle) == 0;
    }

    return purc_variant_make_boolean(result);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
join_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    purc_rwstream_t rwstream;
    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    for (size_t i = 0; i < nr_args; i++) {
#if 0
        const char *str;
        size_t len_str;

        str = purc_variant_get_string_const_ex(argv[i], &len_str);
        if (str) {
            ssize_t nr_wrotten = purc_rwstream_write(rwstream, str, len_str);
            if (nr_wrotten < 0 || (size_t)nr_wrotten < len_str) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto fatal;
            }
        }
        else if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
            // do nothing
        }
        else {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto fatal;
        }
#else
        if (purc_variant_stringify(rwstream, argv[i], 0, NULL) < 0) {
            goto fatal;
        }
#endif
    }

    if (purc_rwstream_write(rwstream, "", 1) < 1) {
        goto fatal;
    }

    size_t sz_buffer = 0;
    size_t sz_content = 0;
    char *content = NULL;
    content = purc_rwstream_get_mem_buffer_ex(rwstream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(rwstream);

    return purc_variant_make_string_reuse_buff(content, sz_buffer, false);

fatal:
    if (rwstream)
        purc_rwstream_destroy(rwstream);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
tolower_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *str;
    size_t length;
    str = purc_variant_get_string_const_ex(argv[0], &length);
    if (str == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (length == 0)
        return purc_variant_make_string_static("", false);

    char *new_str = NULL;
    size_t len_new;

    new_str = pcutils_strtolower(str, length, &len_new);
    if (new_str == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    return purc_variant_make_string_reuse_buff(new_str, len_new + 1, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);
fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
toupper_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *str;
    size_t length;
    str = purc_variant_get_string_const_ex(argv[0], &length);
    if (str == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (length == 0)
        return purc_variant_make_string_static("", false);

    char *new_str = NULL;
    size_t len_new;

    new_str = pcutils_strtoupper(str, length, &len_new);
    if (new_str == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    return purc_variant_make_string_reuse_buff(new_str, len_new + 1, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);
fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
shuffle_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *str;
    size_t len;
    size_t nr_chars;

    str = purc_variant_get_string_const_ex(argv[0], &len);
    if (str == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    purc_variant_string_chars(argv[0], &nr_chars);
    if (nr_chars < 2) {
        // just return the value itself, but reference it.
        return purc_variant_ref(argv[0]);
    }

    char *new_str = NULL;
    if (nr_chars == len) {
        // ASCII string
        new_str = strndup(str, len);
        if (new_str == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fatal;
        }

        for (size_t i =  0; i < len; i++) {
            size_t new_idx = (size_t)pcdvobjs_get_random() % (size_t)len;

            if (new_idx != i) {
                char tmp = new_str[new_idx];
                new_str[new_idx] = new_str[i];
                new_str[i] = tmp;
            }
        }
    }
    else {
        uint32_t *ucs = malloc(sizeof(uint32_t) * nr_chars);
        if (ucs == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fatal;
        }

        size_t n = pcutils_string_decode_utf8(ucs, nr_chars, str);
        assert(n == nr_chars);

        for (size_t i =  0; i < n; i++) {
            size_t new_idx;
            if (n < RAND_MAX) {
                new_idx = (size_t)pcdvobjs_get_random() % n;
            }
            else {
                new_idx = (size_t)pcdvobjs_get_random();
                new_idx = new_idx * n / RAND_MAX;
            }

            if (new_idx != i) {
                uint32_t tmp = ucs[new_idx];
                ucs[new_idx] = ucs[i];
                ucs[i] = tmp;
            }
        }

        new_str = pcutils_string_encode_utf8(ucs, n, &len);
        free(ucs);

        if (new_str == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fatal;
        }
    }

    return purc_variant_make_string_reuse_buff(new_str, len + 1, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);
fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
repeat_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_rwstream_t rwstream = NULL;
    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *str;
    size_t len_str;

    str = purc_variant_get_string_const_ex(argv[0], &len_str);
    if (str == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    int64_t times;
    if (!purc_variant_cast_to_longint(argv[1], &times, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (times < 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (len_str == 0 || times == 0) {
        return purc_variant_make_string_static("", false);
    }

    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    for (int64_t i=0; i < times; i++) {
        ssize_t nr_wrotten = purc_rwstream_write(rwstream, str, len_str);
        if (nr_wrotten < 0 || (size_t)nr_wrotten < len_str) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fatal;
        }
    }

    if (purc_rwstream_write(rwstream, "", 1) < 1) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    size_t sz_buffer = 0;
    size_t sz_content = 0;
    char *content = NULL;
    content = purc_rwstream_get_mem_buffer_ex(rwstream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(rwstream);

    return purc_variant_make_string_reuse_buff(content, sz_buffer, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);

fatal:
    if (rwstream)
        purc_rwstream_destroy(rwstream);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
reverse_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *str;
    size_t length;
    str = purc_variant_get_string_const_ex(argv[0], &length);
    if (str == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (length == 0)
        return purc_variant_make_string_static("", false);

    size_t nr_chars;
    purc_variant_string_chars(argv[0], &nr_chars);
    if (nr_chars < 2) {
        // just return the value itself, but reference it.
        return purc_variant_ref(argv[0]);
    }

    char *new_str = NULL;
    new_str = pcutils_strreverse(str, length, nr_chars);
    if (new_str == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    return purc_variant_make_string_reuse_buff(new_str, length + 1, false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);
fatal:
    return PURC_VARIANT_INVALID;
}

static size_t
count_seperated_segements(const char *haystack, const char *needle)
{
    size_t n = 0;
    size_t len = strlen(needle);

    while (*haystack) {
        haystack = strstr(haystack, needle);
        if (haystack == NULL) {
            n++;
            break;
        }
        else
            haystack += len;

        n++;
    }

    return n;
}

static purc_variant_t
explode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((nr_args < 1)) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    const char *seperator = "";
    size_t sep_len = 0;
    if (nr_args > 1 && !purc_variant_is_string (argv[1])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }
    else if (nr_args > 1) {
        seperator = purc_variant_get_string_const_ex(argv[1], &sep_len);
    }

    int64_t limit = 0;
    if (nr_args > 2 &&
            !purc_variant_cast_to_longint(argv[2], &limit, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    ret_var = purc_variant_make_array_0();

    const char *source = purc_variant_get_string_const(argv[0]);
    if (sep_len == 0) {
        const char *p = source;
        int64_t nr_max;
        if (limit == 0) {
            nr_max = INT64_MAX;
        }
        else if (limit > 0) {
            nr_max = limit - 1;
        }
        else {
            size_t nr_chars;
            pcutils_string_check_utf8(source, -1, &nr_chars, NULL);
            nr_max = nr_chars + limit;
        }

        int64_t n = 0;
        while (*p && n < nr_max) {
            const char *next = pcutils_utf8_next_char(p);
            size_t len = next - p;

            purc_variant_t tmp;
            tmp = purc_variant_make_string_ex(p, len, false);
            purc_variant_array_append(ret_var, tmp);
            purc_variant_unref(tmp);

            p = next;
            n++;
        }

        if (limit >= 0 && *p) {
            purc_variant_t tmp;
            tmp = purc_variant_make_string(p, false);
            purc_variant_array_append(ret_var, tmp);
            purc_variant_unref(tmp);
        }
    }
    else {
        const char *p = strstr(source, seperator);

        if (p == NULL && limit < 0) {
            purc_variant_array_append(ret_var, argv[0]);
            goto done;
        }

        int64_t nr_max;
        if (limit == 0) {
            nr_max = INT64_MAX;
        }
        else if (limit > 0) {
            nr_max = limit - 1;
        }
        else {
            size_t nr_segments;
            nr_segments = count_seperated_segements(source, seperator);
            nr_max = nr_segments + limit;
        }

        int64_t n = 0;
        p = source;
        while (*p && n < nr_max) {
            const char *found = strstr(p, seperator);
            if (found == NULL)
                break;

            size_t seg_len = found - p;

            if (seg_len == 0) {
                purc_variant_t tmp;
                tmp = purc_variant_make_string_static("", false);
                purc_variant_array_append(ret_var, tmp);
                purc_variant_unref(tmp);
            }
            else {
                purc_variant_t tmp;
                tmp = purc_variant_make_string_ex(p, seg_len, false);
                purc_variant_array_append(ret_var, tmp);
                purc_variant_unref(tmp);
            }

            p = found + sep_len;
            n++;
        }

        if (limit >= 0) {
            if (*p) {
                purc_variant_t tmp;
                tmp = purc_variant_make_string(p, false);
                purc_variant_array_append(ret_var, tmp);
                purc_variant_unref(tmp);
            }
            else {
                purc_variant_t tmp;
                tmp = purc_variant_make_string_static("", false);
                purc_variant_array_append(ret_var, tmp);
                purc_variant_unref(tmp);
            }
        }
    }

done:
    return ret_var;

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_array_0();
    return PURC_VARIANT_INVALID;
}


static purc_variant_t
implode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    size_t array_size = 0;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    if (!purc_variant_linear_container_size(argv[0], &array_size)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    // seperator can be null
    const char *seperator = "";
    size_t sep_len = 0;
    if (nr_args > 1 && !purc_variant_is_string(argv[1])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }
    else if (nr_args > 1) {
        seperator = purc_variant_get_string_const_ex(argv[1], &sep_len);
    }

    purc_rwstream_t rwstream = purc_rwstream_new_buffer(32, 0);
    for (size_t i = 0; i < array_size; i++) {
        purc_variant_t tmp;
        tmp = purc_variant_linear_container_get(argv[0], i);
        purc_variant_stringify(rwstream, tmp,
                PCVRNT_STRINGIFY_OPT_IGNORE_ERRORS, NULL);

        if (i != array_size - 1 && sep_len > 0) {
            purc_rwstream_write(rwstream, seperator, sep_len);
        }
    }
    purc_rwstream_write(rwstream, "\0", 1);

    size_t sz_content, sz_rwbuf;
    char *rw_string;
    rw_string = purc_rwstream_get_mem_buffer_ex(rwstream, &sz_content,
            &sz_rwbuf, true);
    purc_rwstream_destroy(rwstream);

    return purc_variant_make_string_reuse_buff(rw_string, sz_rwbuf, false);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);
    return PURC_VARIANT_INVALID;
}

static int do_replace_case(purc_rwstream_t rwstream, const char *subject,
        const char *search, size_t len_search,
        const char *replace, size_t len_replace)
{
    size_t length = 0;
    const char *head = get_next_segment(subject, search, &length);

    while (head) {
        purc_rwstream_write(rwstream, head, length);

        if (*(head + length) != 0x00) {
            purc_rwstream_write(rwstream, replace, len_replace);
            head = get_next_segment(head + length + len_search,
                    search, &length);
        }
        else
            break;
    }

    return 0;
}

/*
$STR.tokenize(
    <string $string: `The string to break.`>,
    <string $delimiters: `The delimiters to sperate the tokens.`>
) array
 */

static purc_variant_t
tokenize_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t item = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if ((nr_args < 1)) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *source;
    size_t src_len;
    source = purc_variant_get_string_const_ex(argv[0], &src_len);

    const char *delimiters;
    size_t del_len;
    if (nr_args > 1) {
        delimiters = purc_variant_get_string_const_ex(argv[1], &del_len);
    }
    else {
        delimiters = PURC_KW_DELIMITERS;
        del_len = sizeof(PURC_KW_DELIMITERS) - 1;
    }

    if (source == NULL || delimiters == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    if (del_len == 0) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto error;
    }

    retv = purc_variant_make_array_0();
    if (retv == PURC_VARIANT_INVALID) {
        goto failed;
    }

    if (is_all_ascii(delimiters)) {
        const char *kw;
        size_t kw_len;
        foreach_keyword_ex(source, src_len, delimiters, kw, kw_len) {
            if (kw_len > 0) {
                item = purc_variant_make_string_ex(kw, kw_len, false);
                if (item == PURC_VARIANT_INVALID) {
                    goto failed;
                }
                if (!purc_variant_array_append(retv, item)) {
                    goto failed;
                }

                purc_variant_unref(item);
                item = PURC_VARIANT_INVALID;
            }
        }
    }
    else {
        const char *kw = source;
        size_t kw_len = 0;
        while (*source) {
            const char *next = pcutils_utf8_next_char(source);
            size_t uchlen = next - source;
            assert(uchlen <= 6);

            char utf8ch[10];
            strncpy(utf8ch, source, uchlen);
            utf8ch[uchlen] = 0x00;

            if (strstr(delimiters, utf8ch)) {
                /* utf8ch is a delimiter */
                if (kw_len) {
                    item = purc_variant_make_string_ex(kw, kw_len, false);
                    if (item == PURC_VARIANT_INVALID) {
                        goto failed;
                    }
                    if (!purc_variant_array_append(retv, item)) {
                        goto failed;
                    }

                    purc_variant_unref(item);
                    item = PURC_VARIANT_INVALID;

                    kw_len = 0;
                }
                kw = next;
            }
            else {
                /* utf8ch is not a delimiter */
                kw_len += uchlen;
            }

            source = next;
        }
    }

    return retv;

failed:
    if (item)
        purc_variant_unref(item);
    if (retv)
        purc_variant_unref(retv);

error:
    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_array_0();
    return PURC_VARIANT_INVALID;
}


static int do_replace_caseless(purc_rwstream_t rwstream, const char *subject,
        const char *search, size_t len_search,
        const char *replace, size_t len_replace)
{
    const char *found;

    while ((found = pcutils_strcasestr(subject, search))) {
        size_t before = found - subject;
        if (before > 0) {
            purc_rwstream_write(rwstream, subject, before);
            subject += before;
        }

        purc_rwstream_write(rwstream, replace, len_replace);
        subject += len_search;
    }

    if (subject[0]) {
        purc_rwstream_write(rwstream, subject, strlen(subject));
    }

    return 0;
}

#define SZ_IN_STACK     16

static const char **
normalize_replace_arg(const char *strings_stack[], size_t sz_stack,
        purc_variant_t arg, size_t *size, int *ec)
{
    const char **strings = NULL;

    const char *tmp;
    if ((tmp = purc_variant_get_string_const(arg))) {
        *size = 0;  // mark the arg is a string.
        strings = strings_stack;
        strings[0] = tmp;
    }
    else if (purc_variant_linear_container_size(arg, size)) {
        if (*size <= sz_stack) {
            strings = strings_stack;
        }
        else {
            strings = calloc(*size, sizeof(strings[0]));
            if (strings == NULL) {
                *ec = PURC_ERROR_OUT_OF_MEMORY;
                goto error;
            }
        }

        for (size_t i = 0; i < *size; i++) {
            tmp = purc_variant_get_string_const(
                    purc_variant_linear_container_get(arg, i));
            if (tmp == NULL) {
                *ec = PURC_ERROR_WRONG_DATA_TYPE;
                goto error;
            }

            strings[i] = tmp;
        }
    }
    else {
        *ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    return strings;

error:
    if (strings != NULL && strings != strings_stack) {
        free(strings);
    }

    return NULL;
}

static purc_variant_t
replace_one_subject(const char *orig_subject,
        const char **searches, size_t nr_searches,
        const char **replaces, size_t nr_replaces,
        int (*do_replace)(purc_rwstream_t, const char *,
        const char *, size_t, const char *, size_t))
{
    purc_rwstream_t rwstream;
    rwstream = purc_rwstream_new_buffer(32, MAX_SIZE_BUFSTM);

    const char *subject = orig_subject;
    for (size_t i = 0; i < nr_searches; i++) {
        const char *search = searches[i];
        const char *replace = (i < nr_replaces) ? replaces[i] : "";

        do_replace(rwstream, subject, search, strlen(search),
                replace, strlen(replace));
        purc_rwstream_write(rwstream, "", 1);

        char *replaced = purc_rwstream_get_mem_buffer(rwstream, NULL);
        if (subject != orig_subject)
            free((char *)subject);

        subject = strdup(replaced);
        purc_rwstream_seek(rwstream, SEEK_SET, 0);
    }
    if (subject != orig_subject)
        free((char *)subject);

    size_t rw_size = 0;
    char *rw_string = purc_rwstream_get_mem_buffer_ex(rwstream,
            NULL, &rw_size, true);
    if (rw_string == NULL) {
        purc_rwstream_destroy(rwstream);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t retv = purc_variant_make_string_reuse_buff(rw_string,
            rw_size, false);
    purc_rwstream_destroy(rwstream);
    return retv;
}

static purc_variant_t
replace_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    const char *subjects_stack[SZ_IN_STACK];
    const char *searches_stack[SZ_IN_STACK], *replaces_stack[SZ_IN_STACK];
    const char **subjects = NULL, **searches = NULL, **replaces = NULL;
    size_t nr_subjects, nr_searches, nr_replaces;

    if (nr_args < 3) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    if ((subjects = normalize_replace_arg(subjects_stack, SZ_IN_STACK, argv[0],
                &nr_subjects, &ec)) == NULL)
        goto error;

    if ((searches = normalize_replace_arg(searches_stack, SZ_IN_STACK, argv[1],
                &nr_searches, &ec)) == NULL)
        goto error;
    if (nr_searches == 0) nr_searches = 1;

    for (size_t i = 0; i < nr_searches; i++) {
        if (searches[i][0] == 0) {
            PC_WARN("Got an empty search string\n");
            ec = PURC_ERROR_INVALID_VALUE;
            goto error;
        }
    }

    if ((replaces = normalize_replace_arg(replaces_stack, SZ_IN_STACK, argv[2],
                &nr_replaces, &ec)) == NULL)
        goto error;
    if (nr_replaces == 0) nr_replaces = 1;

    int (*do_replace)(purc_rwstream_t, const char *,
        const char *, size_t, const char *, size_t) = do_replace_case;
    if (nr_args > 3 && purc_variant_booleanize(argv[3])) {
        do_replace = do_replace_caseless;
    }

    if (nr_subjects == 0) {
        retv = replace_one_subject(subjects[0], searches, nr_searches,
                replaces, nr_replaces, do_replace);
        if (retv == PURC_VARIANT_INVALID) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto error;
        }
    }
    else {
        retv = purc_variant_make_array_0();
        if (retv == PURC_VARIANT_INVALID) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto error;
        }

        for (size_t i = 0; i < nr_subjects; i++) {
            purc_variant_t tmp;
            tmp = replace_one_subject(subjects[i], searches, nr_searches,
                    replaces, nr_replaces, do_replace);
            if (tmp == PURC_VARIANT_INVALID) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto error;
            }

            purc_variant_array_append(retv, tmp);
            purc_variant_unref(tmp);
        }
    }

    if (subjects != NULL && subjects != subjects_stack) {
        free(subjects);
    }
    if (searches != NULL && searches != searches_stack) {
        free(searches);
    }
    if (replaces != NULL && replaces != replaces_stack) {
        free(replaces);
    }

    return retv;

error:
    if (subjects != NULL && subjects != subjects_stack) {
        free(subjects);
    }
    if (searches != NULL && searches != searches_stack) {
        free(searches);
    }
    if (replaces != NULL && replaces != replaces_stack) {
        free(replaces);
    }

    if (retv)
        purc_variant_unref(retv);

    purc_set_error(ec);
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);
    return PURC_VARIANT_INVALID;
}

/*
$STR.translate(
    <string $string>,
    <string $from>,
    <string $to>
) string

$STR.translate(
    <string $string>,
    <object $from_to_pairs>,
) string
 */
static purc_variant_t
translate_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *searches_stack[SZ_IN_STACK], *replaces_stack[SZ_IN_STACK];
    const char **searches = NULL, **replaces = NULL;
    size_t nr_searches = 0;

    purc_variant_t retv = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 2) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *subject = purc_variant_get_string_const(argv[0]);
    if (subject == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    size_t nr_kvs;
    if (purc_variant_object_size(argv[1], &nr_kvs)) {

        if (nr_kvs == 0) {
            ec = PURC_ERROR_INVALID_VALUE;
            goto error;
        }

        if (nr_kvs <= SZ_IN_STACK) {
            searches = searches_stack;
            replaces = replaces_stack;
        }
        else {
            searches = calloc(nr_kvs, sizeof(searches[0]));
            if (searches == NULL) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto error;
            }

            replaces = calloc(nr_kvs, sizeof(replaces[0]));
            if (replaces == NULL) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto error;
            }
        }

        size_t n = 0;
        purc_variant_t kk, vv;
        foreach_key_value_in_variant_object(argv[1], kk, vv) {
            const char *from = purc_variant_get_string_const(kk);
            const char *to = purc_variant_get_string_const(vv);

            if (from == NULL || to == NULL) {
                if (!(call_flags & PCVRT_CALL_FLAG_SILENTLY)) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto error;
                }
            }
            else {
                searches[n] = from;
                replaces[n] = to;
                n++;
            }
        } end_foreach;

        nr_searches = n;
    }
    else {
        if (nr_args < 3) {
            ec = PURC_ERROR_ARGUMENT_MISSED;
            goto error;
        }

        searches = searches_stack;
        replaces = replaces_stack;

        const char *from, *to;
        from = purc_variant_get_string_const(argv[1]);
        to = purc_variant_get_string_const(argv[2]);
        if (from == NULL || to == NULL) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto error;
        }

        searches[0] = from;
        replaces[0] = to;
        nr_searches = 1;
    }

    if (nr_searches == 0) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto error;
    }

    retv = replace_one_subject(subject, searches, nr_searches,
            replaces, nr_searches, do_replace_case);
    if (retv == PURC_VARIANT_INVALID) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    if (searches != NULL && searches != searches_stack) {
        free(searches);
    }
    if (replaces != NULL && replaces != replaces_stack) {
        free(replaces);
    }
    return retv;

error:
    if (searches != NULL && searches != searches_stack) {
        free(searches);
    }
    if (replaces != NULL && replaces != replaces_stack) {
        free(replaces);
    }

    purc_set_error(ec);
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);
    return PURC_VARIANT_INVALID;
}

#define SZ_CONVERSION_SPECIFICATION     64
#define SZ_PRINT_BUFF                   128
#define C_PRINTF_CONVERSION_SPECIFIERS "dioupxXeEfFgGaAcsnm%"

/* The overall syntax of a conversion specification is:
   %[$][flags][width][.precision][length modifier]conversion */
static int
conversion_specification(char *conv_spec_buf, int sz_buf, const char *format)
{
    assert(format[0] == '%' && sz_buf > 1);

    conv_spec_buf[0] = format[0];
    int len = 1, len_limit = sz_buf - 1;
    while (format[len] && len < len_limit) {
        conv_spec_buf[len] = format[len];

        if (strchr(C_PRINTF_CONVERSION_SPECIFIERS, format[len])) {
            len++;
            goto done;
        }

        len++;
    }

done:
    conv_spec_buf[len] = '\0';
    return len;
}

static purc_variant_t
format_c_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_rwstream_t rwstream = NULL;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    const char *format;
    size_t format_len;

    if ((format = purc_variant_get_string_const_ex(argv[0], &format_len)) ==
            NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    bool is_array = false;
    size_t sz_array = 0;
    if (purc_variant_linear_container_size(argv[1], &sz_array)) {
        is_array = true;
    }

    rwstream = purc_rwstream_new_buffer(32, MAX_SIZE_BUFSTM);

    size_t start = 0, i = 0, j = 0;
    while (format[i]) {
        if (format[i] == '%') {
            purc_variant_t item = PURC_VARIANT_INVALID;
            if (is_array) {
                if (j < sz_array) {
                    item = purc_variant_linear_container_get(argv[1], j);
                }
            }
            else {
                if (nr_args > j + 1) {
                    item = argv[j + 1];
                }
            }

            char conv_spec[SZ_CONVERSION_SPECIFICATION];
            int cs_len = conversion_specification(conv_spec,
                    sizeof(conv_spec), format + i);

            if (conv_spec[cs_len - 1] == '\0') {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto error;
            }

            if (strchr("nm%", conv_spec[cs_len - 1]) == NULL
                    && item == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
                goto error;
            }

            purc_rwstream_write(rwstream, format + start, i - start);

            char buff[SZ_PRINT_BUFF], *buff_alloc;
            int64_t i64 = 0;
            uint64_t u64 = 0;
            const char *string = NULL;
            int len, arg_used = 1;

            switch (conv_spec[cs_len - 1]) {
            case 'i':
            case 'd':
            case 'c':
                if (!purc_variant_cast_to_longint(item, &i64, true)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                if (strstr(conv_spec, "ll")) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (long long)i64);
                }
                else if (strstr(conv_spec, "hh")) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (char)i64);
                }
                else if (strchr(conv_spec, 'j')) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (intmax_t)i64);
                }
                else if (strchr(conv_spec, 'z')) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (ssize_t)i64);
                }
                else if (strchr(conv_spec, 'l')) {
                    len = snprintf(buff, sizeof(buff), conv_spec, (long)i64);
                }
                else if (strchr(conv_spec, 'h')) {
                    len = snprintf(buff, sizeof(buff), conv_spec, (short)i64);
                }
                else {
                    len = snprintf(buff, sizeof(buff), conv_spec, (int)i64);
                }

                if (len < 0) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto error;
                }
                purc_rwstream_write(rwstream, buff, len);
                break;

            case 'o':
            case 'u':
            case 'p':
            case 'x':
            case 'X':
                if (!purc_variant_cast_to_ulongint(item, &u64, false)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }

                if (strstr(conv_spec, "ll")) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (unsigned long long)u64);
                }
                else if (strstr(conv_spec, "hh")) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (unsigned char)i64);
                }
                else if (strchr(conv_spec, 'j')) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (uintmax_t)i64);
                }
                else if (strchr(conv_spec, 'z')) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (size_t)i64);
                }
                else if (strchr(conv_spec, 'l')) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (unsigned long)u64);
                }
                else if (strchr(conv_spec, 'h')) {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (unsigned short)i64);
                }
                else {
                    len = snprintf(buff, sizeof(buff), conv_spec,
                            (unsigned)u64);
                }
                if (len < 0) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto error;
                }
                purc_rwstream_write(rwstream, buff, len);
                break;

            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
                if (strchr(conv_spec, 'L')) {
                    long double ld;
                    if (!purc_variant_cast_to_longdouble(item, &ld, false)) {
                        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                        goto error;
                    }
                    len = snprintf(buff, sizeof(buff), conv_spec, ld);
                }
                else {
                    double d;
                    if (!purc_variant_cast_to_number(item, &d, false)) {
                        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                        goto error;
                    }
                    len = snprintf(buff, sizeof(buff), conv_spec, d);
                }
                if (len < 0) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto error;
                }
                purc_rwstream_write(rwstream, buff, len);
                break;

            case 's':
                string = purc_variant_get_string_const(item);
                if (string == NULL) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                len = asprintf(&buff_alloc, conv_spec, string);
                if (len < 0) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto error;
                }

                if (!pcutils_string_check_utf8(buff_alloc, -1, NULL, NULL)) {
                    free(buff_alloc);
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto error;
                }

                purc_rwstream_write(rwstream, buff_alloc, len);
                free(buff_alloc);
                break;

            case 'n':
            case 'm':
            case '%':
                len = snprintf(buff, sizeof(buff), conv_spec, 0);
                if (len < 0) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto error;
                }
                purc_rwstream_write(rwstream, buff, len);
                arg_used = 0;
                break;

            default:
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto error;
                break;
            }

            i += cs_len;
            j += arg_used;
            start = i;
        }
        else
            i++;
    }

    if (i != start)
        purc_rwstream_write(rwstream, format + start, strlen(format + start));
    purc_rwstream_write(rwstream, "\0", 1);

    size_t rw_size = 0;
    size_t content_size = 0;
    char *rw_string = purc_rwstream_get_mem_buffer_ex(rwstream,
            &content_size, &rw_size, true);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    if ((rw_size == 0) || (rw_string == NULL)) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }
    else {
        retv = purc_variant_make_string_reuse_buff(rw_string, rw_size, false);
    }
    purc_rwstream_destroy(rwstream);
    return retv;

error:
    if (rwstream)
        purc_rwstream_destroy(rwstream);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
format_p_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    const char *format = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t tmp_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, MAX_SIZE_BUFSTM);
    int type = 0;
    char buffer[32];
    int index = 0;
    char *buf = NULL;
    size_t sz_stream = 0;
    size_t format_size = 0;
    purc_rwstream_t serialize = NULL;     // used for serialize

    if ((argv == NULL) || (nr_args == 0)) {
        purc_rwstream_destroy(rwstream);
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ( (!purc_variant_is_string (argv[0]))) {
        purc_rwstream_destroy(rwstream);
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    else {
        format = purc_variant_get_string_const (argv[0]);
        format_size = purc_variant_string_size (argv[0]);
    }

    if (argv[1] == PURC_VARIANT_INVALID) {
        purc_rwstream_destroy(rwstream);
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    enum purc_variant_type vt = purc_variant_get_type(argv[1]);
    if (vt == PURC_VARIANT_TYPE_ARRAY || vt == PURC_VARIANT_TYPE_SET
            || vt == PURC_VARIANT_TYPE_TUPLE) {
        type = 0;
    }
    else if (vt == PURC_VARIANT_TYPE_OBJECT) {
        type = 1;
    }
    else {
        purc_rwstream_destroy(rwstream);
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (type == 0) {
        const char *start = NULL;
        const char *end = NULL;
        size_t length = 0;
        const char *head = pcutils_get_next_token (format, "{", &length);
        while (head) {
            purc_rwstream_write (rwstream, head, length);

            start = head + length + 1;
            head = pcutils_get_next_token (head + length + 1, "}", &length);
            end = head + length;
            strncpy(buffer, start, end - start);
            *(buffer + (end - start)) = 0x00;
            pcdvobjs_remove_space (buffer);
            index = atoi (buffer);

            tmp_var = purc_variant_linear_container_get (argv[1], index);
            if (tmp_var == PURC_VARIANT_INVALID) {
                purc_rwstream_destroy (rwstream);
                return PURC_VARIANT_INVALID;
            }

            serialize = purc_rwstream_new_buffer (32, MAX_SIZE_BUFSTM);
            purc_variant_serialize (tmp_var, serialize, 3, 0, &sz_stream);
            buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
            purc_rwstream_write (rwstream, buf + 1, sz_stream - 2);
            purc_rwstream_destroy (serialize);

            head = pcutils_get_next_token (head + length + 1, "{", &length);
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
        const char *head = pcutils_get_next_token (format, "{", &length);
        while (head) {
            purc_rwstream_write (rwstream, head, length);

            start = head + length + 1;
            head = pcutils_get_next_token (head + length + 1, "}", &length);
            end = head + length;
            strncpy(buffer, start, end - start);
            *(buffer + (end - start)) = 0x00;
            pcdvobjs_remove_space (buffer);

            tmp_var = purc_variant_object_get_by_ckey_ex (argv[1], buffer, false);
            if (tmp_var == PURC_VARIANT_INVALID) {
                purc_rwstream_destroy (rwstream);
                return PURC_VARIANT_INVALID;
            }

            serialize = purc_rwstream_new_buffer (32, MAX_SIZE_BUFSTM);
            purc_variant_serialize (tmp_var, serialize, 3, 0, &sz_stream);
            buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
            purc_rwstream_write (rwstream, buf + 1, sz_stream - 2);
            purc_rwstream_destroy (serialize);

            head = pcutils_get_next_token (head + length + 1, "{", &length);
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
            purc_set_error (PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
}

static purc_variant_t
substr_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 2)) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    size_t str_len = 0;
    purc_variant_string_bytes (argv[0], &str_len);
    const char * src = purc_variant_get_string_const (argv[0]);

    if (!(purc_variant_is_longint (argv[1])
            || purc_variant_is_number (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    int64_t pos = 0;
    purc_variant_cast_to_longint (argv[1], &pos, false);

    // pos is valid
    if ((int64_t)(str_len - 1) < (pos >= 0? pos : -pos))
        return purc_variant_make_string("", false);

    // get the start position
    const char *start = NULL;
    const char *end = src + str_len - 1;
    if (pos >= 0)
        start = src + pos;
    else
        start = src + str_len - 1 + pos;

    // get the length
    int64_t length = 0;
    if (nr_args > 2) {
        if(argv[2] == NULL || !(purc_variant_is_longint (argv[2])
                    || purc_variant_is_number (argv[2]))) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            return PURC_VARIANT_INVALID;
        }
        purc_variant_cast_to_longint (argv[2], &length, false);

        if (length > 0) {
            if ((start + length) <= end)
                end = start + length;
        }
        else if (length < 0) {
            end = end + length;
            if (end <= start)
                return purc_variant_make_string("", false);
        }
        else        // 0
            return purc_variant_make_string("", false);
    }

    length = end - start;
    if (length == 0)
        return purc_variant_make_string("", false);
    else {
        char *buf = malloc (length + 1);
        if (buf == NULL) {
            purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        strncpy (buf, start, length);
        buf[length] = 0x00;
        ret_var = purc_variant_make_string_reuse_buff (buf, length + 1, false);
    }

    return ret_var;
}

/*
$STR.strstr(
        <string $haystack>,
        <string $needle>
        [, <bool $before_needle = false>
            [, <bool $case_insensitivity = false:
                false - `performs a case-sensitive check;`
                true - `performs a case-insensitive check.`>
            ]
        ]
) string | false
*/

static purc_variant_t
strstr_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_is_string(argv[0]) ||
            !purc_variant_is_string(argv[1])) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    const char *haystack = purc_variant_get_string_const(argv[0]);
    const char *needle = purc_variant_get_string_const(argv[1]);

    bool before_needle = false;
    if (nr_args > 2) {
        before_needle = purc_variant_booleanize(argv[2]);
    }

    bool caseless = false;
    if (nr_args > 3) {
        caseless = purc_variant_booleanize(argv[3]);
    }

    const char *found;
    if (caseless) {
        found = pcutils_strcasestr(haystack, needle);
    }
    else {
        found = strstr(haystack, needle);
    }

    if (found == NULL) {
        return purc_variant_make_boolean(false);
    }

    if (before_needle) {
        size_t len = (size_t)(found - haystack);
        if (len == 0) {
            return purc_variant_make_string_static("", false);
        }

        return purc_variant_make_string_ex(haystack, len, false);
    }
    else {
        return purc_variant_make_string(found, false);
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

/*
$STR.strpos(
    <string $haystack: `The string in which to find the substring $needle.`>,
    <string $needle: `The substring to find in $haystack.`>
    [, <real $offset = 0: `The offset starting to search. If $offset is
            less than 0, this method will return the last occurrence of
            $needle.`>
        [, <bool $case_insensitivity = false:
            false -  `Perform a case-sensitive check.`
            true -  `Perform a case-insensitive check.`>
        ]
    ]
) ulongint | false

 */
static purc_variant_t
strpos_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 2) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *haystack = purc_variant_get_string_const(argv[0]);
    size_t needle_len;
    const char *needle =
        purc_variant_get_string_const_ex(argv[1], &needle_len);

    if (!haystack || !needle) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    if (needle[0] == 0) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto error;
    }

    int64_t off = 0;
    if (nr_args > 2 && !purc_variant_cast_to_longint(argv[2], &off, false)) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    bool caseless = false;
    if (nr_args > 3) {
        caseless = purc_variant_booleanize(argv[3]);
    }

    const char *found = NULL;
    if (off >= 0) {
        const char *p = haystack;
        int64_t i = 0;
        while (*p && i < off) {
            const char *next = pcutils_utf8_next_char(p);
            i++;
            p = next;
        }

        if (*p == 0) {
            retv = purc_variant_make_boolean(false);
            goto done;
        }

        if (caseless) {
            found = pcutils_strcasestr(p, needle);
        }
        else {
            found = strstr(p, needle);
        }
    }
    else {
        /* find the last occurrence */
        const char *p = haystack;
        const char *occur;
        do {
            size_t matched_len = 0;
            if (caseless) {
                occur = pcutils_strcasestr_ex(p, needle, &matched_len);
            }
            else {
                occur = strstr(p, needle);
                matched_len = needle_len;
            }

            if (occur) {
                p = occur + matched_len;
                found = occur;
            }
        } while (occur);
    }

    size_t pos = 0;
    if (found == NULL) {
        retv = purc_variant_make_boolean(false);
        goto done;
    }
    else {
        const char *p = haystack;
        while (found > p) {
            const char *next = pcutils_utf8_next_char(p);
            pos++;
            p = next;
        }
    }

    retv = purc_variant_make_ulongint(pos);
done:
    return retv;

error:
    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

/*
$STR.strpbrk(
        <string $string: `The string.`>,
        <string $characters: `The characters to search in the string.`>
        [, <bool $case_insensitivity = false:
            - false:     `Perform a case-sensitive check.`
            - true:      `Perform a case-insensitive check.`>
        ]
) string | false
 */
static purc_variant_t
strpbrk_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 2) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *string = purc_variant_get_string_const(argv[0]);
    const char *characters = purc_variant_get_string_const(argv[1]);

    if (!string || !characters) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    if (characters[0] == 0) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto error;
    }

    bool caseless = false;
    if (nr_args > 2) {
        caseless = purc_variant_booleanize(argv[2]);
    }

    const char *sub = NULL;
    const char *p = string;
    while (*p) {
        const char *next = pcutils_utf8_next_char(p);
        size_t uchlen = next - p;
        assert(uchlen <= 6);

        char utf8ch[10];
        strncpy(utf8ch, p, uchlen);
        utf8ch[uchlen] = 0x00;

        const char *found;
        if (caseless) {
            found = pcutils_strcasestr(characters, utf8ch);
        }
        else {
            found = strstr(characters, utf8ch);
        }

        if (found) {
            sub = p;
            break;
        }

        p = next;
    }

    if (sub) {
        retv = purc_variant_make_string(sub, false);
    }
    else {
        retv = purc_variant_make_boolean(false);
    }

    return retv;

error:
    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

/*
$STR.trim(
    <string $string: `The orignal string to trim.`>
    [, <string $characters = " \n\r\t\v\f": `The characters to trim from
            the original string.`>
        [, < 'left | right | both' $position  = 'both': `The trimming
                position.`> ]
    ]
) string
*/

enum {
    POS_LEFT = 0x01,
    POS_RIGHT = 0x02,
    POS_BOTH = POS_LEFT | POS_RIGHT,
};

static struct pcdvobjs_option_to_atom position_ckws[] = {
    { "left",   0,  POS_LEFT },
    { "right",  0,  POS_RIGHT },
    { "both",   0,  POS_BOTH },
};

static int trim_ascii_chars(purc_rwstream_t rwstream,
        const char *str, size_t len, const char *chars, int pos)
{
    int ec = PURC_ERROR_OK;

    const char *left = str;
    const char *right = str + len - 1;

    if (pos & POS_LEFT) {
        while (len > 0) {
            if (strchr(chars, *left)) {
                left++;
                len--;
            }
            else {
                break;
            }
        }
    }

    if (pos & POS_RIGHT) {
        while (len > 0) {
            if (strchr(chars, *right)) {
                right--;
                len--;
            }
            else {
                break;
            }
        }
    }

    if (len > 0 && purc_rwstream_write(rwstream, left, len) < 1) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
    }

    return ec;
}

static const char *utf8_prev_char(const char *str)
{
    /* The maximum length of a UTF-8 character is 6 */
    for (int n = 6; n > 0; n--) {
        const char *p = str - n;
        if (pcutils_utf8_next_char(p) == str)
            return p;
    }

    return NULL;
}

static int trim_utf8_chars(purc_rwstream_t rwstream,
        const char *str, size_t len, const char *characters, int pos)
{
    int ec = PURC_ERROR_OK;

    const char *left = str;
    const char *right = str + len;

    if (pos & POS_LEFT) {
        while (len > 0) {
            char utf8[10] = { 0 };
            const char *p = pcutils_utf8_next_char(left);
            size_t utf8_len = p - left;
            assert(utf8_len < sizeof(utf8));

            strncpy(utf8, left, utf8_len);
            PC_NONE("characters: '%s', utf8: '%s', strstr:  %s\n",
                    characters, utf8,
                    strstr(characters, utf8) ? "true" : "false");
            /* work-around: strstr(" 中国", " ") may return NULL */
            if ((utf8_len == 1 && strchr(characters, utf8[0])) ||
                    strstr(characters, utf8)) {
                left = p;
                len -= utf8_len;
            }
            else {
                break;
            }
        }
    }

    if (pos & POS_RIGHT) {
        while (len > 0) {
            char utf8[10] = { 0 };
            const char *p = utf8_prev_char(right);
            size_t utf8_len = right - p;
            assert(utf8_len < sizeof(utf8));

            strncpy(utf8, p, utf8_len);
            PC_NONE("Right string: %s, Previouse char: %s\n", right, utf8);
            PC_NONE("characters: '%s', utf8: '%s', strstr:  %s\n",
                    characters, utf8,
                    strstr(characters, utf8) ? "true" : "false");
            if ((utf8_len == 1 && strchr(characters, utf8[0])) ||
                    strstr(characters, utf8)) {
                right = p;
                len -= utf8_len;
            }
            else {
                break;
            }
        }
    }

    if (len > 0 && purc_rwstream_write(rwstream, left, len) < 1) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
    }

    return ec;
}

static purc_variant_t
trim_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int errcode = PURC_ERROR_OK;
    purc_rwstream_t rwstream = NULL;

    if (nr_args < 1) {
        errcode = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *chars = " \n\r\t\v\f";
    bool all_ascci = true;
    const char *str;
    size_t len, chars_len = strlen(chars);

    if ((str = purc_variant_get_string_const_ex(argv[0], &len)) == NULL) {
        errcode = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    int position = POS_BOTH;
    if (nr_args > 1) {
        if (position_ckws[0].atom == 0) {
            for (size_t i = 0; i < PCA_TABLESIZE(position_ckws); i++) {
                position_ckws[i].atom = purc_atom_from_static_string_ex(
                        ATOM_BUCKET_DVOBJ, position_ckws[i].option);
            }
        }

        position = pcdvobjs_parse_options(argv[1], NULL, 0,
                position_ckws, PCA_TABLESIZE(position_ckws), 0, -1);
        if (position == -1) {
            goto error;
        }
    }

    if (nr_args > 2) {
        if ((chars = purc_variant_get_string_const_ex(argv[2], &chars_len)) ==
                NULL) {
            errcode = PURC_ERROR_WRONG_DATA_TYPE;
            goto error;
        }

        all_ascci = is_all_ascii(chars);
    }

    if (len == 0 || chars_len == 0)
        goto empty_done;

    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL) {
        errcode = PURC_ERROR_OUT_OF_MEMORY;
        goto failed;
    }

    // call worker
    int ret;
    if (all_ascci) {
        ret = trim_ascii_chars(rwstream, str, len, chars, position);
    }
    else {
        ret = trim_utf8_chars(rwstream, str, len, chars, position);
    }

    if (ret) {
        errcode = ret;
        goto failed;
    }

    size_t sz_buffer = 0;
    size_t sz_content = 0;
    char *content = NULL;
    content = purc_rwstream_get_mem_buffer_ex(rwstream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(rwstream);

    return purc_variant_make_string_reuse_buff(content, sz_buffer, false);

empty_done:
    return purc_variant_ref(argv[0]);

failed:
    if (rwstream)
        purc_rwstream_destroy(rwstream);
error:
    if (errcode)
        purc_set_error(errcode);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

/*
$STR.rot13(
        <string $string: `The string to convert.`>
) string
 */

static purc_variant_t
rot13_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t item = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *chars;
    if ((chars = purc_variant_get_string_const(argv[0])) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    struct pcutils_mystring mystr;
    pcutils_mystring_init(&mystr);

    const char *p = chars;
    while (*p) {
        const char *next = pcutils_utf8_next_char(p);
        size_t uchlen = next - p;
        assert(uchlen <= 6);

        char utf8ch[10];
        strncpy(utf8ch, p, uchlen);
        utf8ch[uchlen] = 0x00;

        if (uchlen == 1) {
            int ch = utf8ch[0];
            if (purc_islower(ch)) {
                ch += 13;
                if (ch > 'z')
                    ch = 'a' + ch - 'z' - 1;

                utf8ch[0] = ch;
            }
            else if (purc_isupper(ch)) {
                ch += 13;
                if (ch > 'Z')
                    ch = 'A' + ch - 'Z' - 1;

                utf8ch[0] = ch;
            }
        }

        if (pcutils_mystring_append_mchar(&mystr,
                    (unsigned char *)utf8ch, uchlen)) {
            pcutils_mystring_free(&mystr);
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto error;
        }

        p = next;
    }

    pcutils_mystring_done(&mystr);
    retv = purc_variant_make_string_reuse_buff(mystr.buff,
            mystr.sz_space, false);

    return retv;

error:
    if (item) {
        purc_variant_unref(item);
    }

    if (retv) {
        purc_variant_unref(retv);
    }

    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

/*
$STR.count_chars(
    < string $string: `The examined string.` >
    [,
        < 'object | string' $mode = 'object':
           - 'object': `Return an object with the character as key and
                the frequency of every character as value.`
           - 'string': `Return a string containing all unique characters. `
        >
    ]
) object | string
 */

enum {
    CCM_OBJECT,
    CCM_STRING,
};

static struct pcdvobjs_option_to_atom ccm_skws[] = {
    { "object",     0, CCM_OBJECT },
    { "string",     0, CCM_STRING },
};

static purc_variant_t
count_chars_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t item = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *chars;
    if ((chars = purc_variant_get_string_const(argv[0])) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    int ccm;
    if (ccm_skws[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(ccm_skws); i++) {
            ccm_skws[i].atom = purc_atom_from_static_string_ex(
                    ATOM_BUCKET_DVOBJ, ccm_skws[i].option);
        }
    }

    ccm = pcdvobjs_parse_options(
            nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID,
            ccm_skws, PCA_TABLESIZE(ccm_skws), NULL, 0, CCM_OBJECT, -1);
    if (ccm == -1) {
        goto error;
    }

    if (ccm == CCM_OBJECT) {
        retv = purc_variant_make_object_0();
        if (retv == PURC_VARIANT_INVALID)
            goto error;

        const char *p = chars;
        while (*p) {
            const char *next = pcutils_utf8_next_char(p);
            size_t uchlen = next - p;
            assert(uchlen <= 6);

            char utf8ch[10];
            strncpy(utf8ch, p, uchlen);
            utf8ch[uchlen] = 0x00;

            purc_variant_t v;
            v = purc_variant_object_get_by_ckey(retv, utf8ch);
            if (v) {
                /* XXX: change the value directly */
                v->u64++;
            }
            else {
                item = purc_variant_make_ulongint(1);
                if (!purc_variant_object_set_by_ckey(retv, utf8ch, item))
                    goto error;
                purc_variant_unref(item);
                item = PURC_VARIANT_INVALID;
            }

            p = next;
        }
    }
    else if (ccm == CCM_STRING) {
        struct pcutils_mystring mystr;
        pcutils_mystring_init(&mystr);

        const char *p = chars;
        while (*p) {
            const char *next = pcutils_utf8_next_char(p);
            size_t uchlen = next - p;
            assert(uchlen <= 6);

            char utf8ch[10];
            strncpy(utf8ch, p, uchlen);
            utf8ch[uchlen] = 0x00;

            if (mystr.buff == NULL ||
                    !strnstr(mystr.buff, utf8ch, mystr.nr_bytes)) {
                if (pcutils_mystring_append_mchar(&mystr,
                            (unsigned char *)utf8ch, uchlen)) {
                    pcutils_mystring_free(&mystr);
                    ec = PURC_ERROR_OUT_OF_MEMORY;
                    goto error;
                }
            }

            p = next;
        }

        pcutils_mystring_done(&mystr);
        retv = purc_variant_make_string_reuse_buff(mystr.buff,
                mystr.sz_space, false);
    }

    return retv;

error:
    if (item) {
        purc_variant_unref(item);
    }

    if (retv) {
        purc_variant_unref(retv);
    }

    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

/*
$STR.count_bytes(
    < string | bsequence $data: `The examined data.` >
    [, < 'tuple-all | object-all | object-appeared | object-not-appeared |
            bytes-appeared | bytes-not-appeared' $mode = 'tuple-all':
        - 'tuple-all':  `Return a tuple with the byte-value (0 ~ 255) as
            index and the frequency of every byte as value.`
        - 'object-all': `Return an object with the byte-value (decimal string)
            as key and the frequency of every byte as value.`
        - 'object-appeared': `Same as 'object-all' but  only byte-values with
            a frequency greater than zero are listed.`
        - 'object-not-appeared': `Same as 'object-all' but only byte-values
            with a frequency equal to zero are listed.`
        - 'bytes-appeared': `A binary sequence containing all unique bytes
            is returned.`
        - 'bytes-not-appeared': `A binary sequence containing all not used
            bytes is returned.`
        >
    ]
) tuple | object | bsequence
 */

enum {
    CBM_TUPLE_ALL,
    CBM_OBJECT_ALL,
    CBM_OBJECT_APPEARED,
    CBM_OBJECT_NOT_APPEARED,
    CBM_BYTES_APPEARED,
    CBM_BYTES_NOT_APPEARED,
};

static struct pcdvobjs_option_to_atom cbm_skws[] = {
    { "tuple-all",          0, CBM_TUPLE_ALL },
    { "object-all",         0, CBM_OBJECT_ALL },
    { "object-appeared",    0, CBM_OBJECT_APPEARED },
    { "object-not-appeared",0, CBM_OBJECT_NOT_APPEARED },
    { "bytes-appeared",     0, CBM_BYTES_APPEARED },
    { "bytes-not-appeared", 0, CBM_BYTES_NOT_APPEARED },
};

static purc_variant_t
count_bytes_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t item = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const unsigned char *bytes;
    size_t nr_bytes;

    if ((bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes)) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    int cbm;
    if (cbm_skws[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(cbm_skws); i++) {
            cbm_skws[i].atom = purc_atom_from_static_string_ex(
                    ATOM_BUCKET_DVOBJ, cbm_skws[i].option);
        }
    }

    cbm = pcdvobjs_parse_options(
            nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID,
            cbm_skws, PCA_TABLESIZE(cbm_skws), NULL, 0, CBM_TUPLE_ALL, -1);
    if (cbm == -1) {
        goto error;
    }

    size_t counted[256]= { 0 };
    for (size_t i = 0; i < nr_bytes; i++) {
        counted[(int)bytes[i]]++;
    }

    switch (cbm) {
    case CBM_TUPLE_ALL:
        retv = purc_variant_make_tuple(256, NULL);
        if (retv == PURC_VARIANT_INVALID)
            goto error;

        for (int i = 0; i < 256; i++) {
            item = purc_variant_make_ulongint(counted[i]);
            if (item && purc_variant_tuple_set(retv, i, item)) {
                purc_variant_unref(item);
                item = PURC_VARIANT_INVALID;
            }
            else {
                goto error;
            }
        }
        break;

    case CBM_OBJECT_ALL:
        retv = purc_variant_make_object_0();
        if (retv == PURC_VARIANT_INVALID)
            goto error;

        for (int i = 0; i < 256; i++) {
            char key[16];
            snprintf(key, sizeof(key), "%d", i);

            item = purc_variant_make_ulongint(counted[i]);
            if (item && purc_variant_object_set_by_ckey(retv, key, item)) {
                purc_variant_unref(item);
                item = PURC_VARIANT_INVALID;
            }
            else {
                goto error;
            }
        }
        break;

    case CBM_OBJECT_APPEARED:
        retv = purc_variant_make_object_0();
        if (retv == PURC_VARIANT_INVALID)
            goto error;

        for (int i = 0; i < 256; i++) {
            if (counted[i] == 0)
                continue;

            char key[16];
            snprintf(key, sizeof(key), "%d", i);

            item = purc_variant_make_ulongint(counted[i]);
            if (item && purc_variant_object_set_by_ckey(retv, key, item)) {
                purc_variant_unref(item);
                item = PURC_VARIANT_INVALID;
            }
            else {
                goto error;
            }
        }
        break;

    case CBM_OBJECT_NOT_APPEARED:
        retv = purc_variant_make_object_0();
        if (retv == PURC_VARIANT_INVALID)
            goto error;

        for (int i = 0; i < 256; i++) {
            if (counted[i] > 0)
                continue;

            char key[16];
            snprintf(key, sizeof(key), "%d", i);

            item = purc_variant_make_ulongint(0);
            if (item && purc_variant_object_set_by_ckey(retv, key, item)) {
                purc_variant_unref(item);
                item = PURC_VARIANT_INVALID;
            }
            else {
                goto error;
            }
        }
        break;

    case CBM_BYTES_APPEARED: {
        unsigned char bytes_appeared[256];
        size_t n = 0;

        for (int i = 0; i < 256; i++) {
            if (counted[i] == 0)
                continue;

            bytes_appeared[n] = (unsigned char)i;
            n++;
        }

        retv = purc_variant_make_byte_sequence(bytes_appeared, n);
        if (retv == PURC_VARIANT_INVALID)
            goto error;
        break;
    }

    case CBM_BYTES_NOT_APPEARED: {
        unsigned char bytes_not_appeared[256];
        size_t n = 0;

        for (int i = 0; i < 256; i++) {
            if (counted[i] > 0)
                continue;

            bytes_not_appeared[n] = (unsigned char)i;
            n++;
        }

        retv = purc_variant_make_byte_sequence(bytes_not_appeared, n);
        if (retv == PURC_VARIANT_INVALID)
            goto error;
        break;
    }

    default:
        ec = PURC_ERROR_INVALID_VALUE;
        goto error;
        break;
    }

    return retv;

error:
    if (item) {
        purc_variant_unref(item);
    }

    if (retv) {
        purc_variant_unref(retv);
    }

    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

/*
$STR.codepoints(
    < string $the_string: `The string to convert.` >
    [, < 'array | tuple' $return_type = 'array':
        - 'array': `Return an array of codepoints.`
        - 'tuple': `Return a tuple of codepoints.`
        >
    ]
) array | tuple: `The array or tuple contains all Unicode
        codepoints of the string.`
*/

enum {
    TYPE_ARRAY,
    TYPE_TUPLE,
};

static struct pcdvobjs_option_to_atom type_skws[] = {
    { "array",  0,  TYPE_ARRAY },
    { "tuple",  0,  TYPE_TUPLE },
};

static purc_variant_t
codepoints_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t item = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *str;
    size_t len;

    if ((str = purc_variant_get_string_const_ex(argv[0], &len)) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    int ret_type;
    if (type_skws[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(type_skws); i++) {
            type_skws[i].atom = purc_atom_from_static_string_ex(
                    ATOM_BUCKET_DVOBJ, type_skws[i].option);
        }
    }

    ret_type = pcdvobjs_parse_options(
            nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID,
            type_skws, PCA_TABLESIZE(type_skws), NULL, 0, TYPE_ARRAY, -1);
    if (ret_type == -1) {
        goto error;
    }

    if (ret_type == TYPE_ARRAY) {
        retv = purc_variant_make_array_0();
    }
    else if (ret_type == TYPE_TUPLE) {
        size_t nr_chars = pcutils_string_utf8_chars(str, len);
        retv = purc_variant_make_tuple(nr_chars, NULL);
    }
    else {
        ec = PURC_ERROR_INVALID_VALUE;
    }

    if (retv == PURC_VARIANT_INVALID)
        goto error;

    char *p = (char *)str;
    size_t i = 0;
    while (*p) {

        uint32_t uc = pcutils_utf8_to_unichar((unsigned char *)p);

        if (ret_type == TYPE_ARRAY) {
            item = purc_variant_make_number(uc);
            if (item && purc_variant_array_append(retv, item)) {
                purc_variant_unref(item);
            }
            else {
                goto error;
            }
        }
        else {  /* tuple */
            item = purc_variant_make_number(uc);
            if (item && purc_variant_tuple_set(retv, i, item)) {
                purc_variant_unref(item);
            }
            else {
                goto error;
            }
        }

        p = pcutils_utf8_next_char(p);
        i++;
    }

    return retv;

error:
    if (item) {
        purc_variant_unref(item);
    }

    if (retv) {
        purc_variant_unref(retv);
    }

    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

/*
$STR.htmlentities_encode(
  <string $string: `The input string.`>
  [,
   <'[single-quotes || double-quotes || convert-all || double-encode'
        $flags = 'single-quotes double-quotes double-encode':
    - 'single-quotes':  `Will convert single-quotes.`
    - 'double-quotes':  `Will convert double quotes.`
    - 'convert-all':    `All characters which have HTML character entity
                        equivalents are translated into these entities;
                        or only the certain characters have special significance
                        in HTML are translated into these entities.`
    - 'double-encode':  `Convert everything; or keep the existing HTML entities.`
   >
 ]
) string | false
*/

enum {
    HEF_SINGLE_QUOTES   = 0x1 << 0,
    HEF_DOUBLE_QUOTES   = 0x1 << 1,
    HEF_CONVERT_ALL     = 0x1 << 2,
    HEF_DOUBLE_ENCODE   = 0x1 << 3,
};

static struct pcdvobjs_option_to_atom htmlentities_en_ckws[] = {
    { "single-quotes",  0,  HEF_SINGLE_QUOTES },
    { "double-quotes",  0,  HEF_DOUBLE_QUOTES },
    { "convert-all",    0,  HEF_CONVERT_ALL },
    { "double-encode",  0,  HEF_DOUBLE_ENCODE },
};

static bool is_valid_entity(const char *p)
{
    if (p[1] == 0 || (p[1] != '#' && !isalpha(p[1])))
        return false;

    /* TODO: we do not check the validation of entity name or code point */
    p++;
    while (*p) {
        if (*p == ';')
            return true;
        p++;
    }

    return false;
}

static purc_variant_t
htmlentities_encode_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    int ec = PURC_ERROR_OK;
    purc_rwstream_t rwstream = NULL;

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *str;
    size_t len;

    if ((str = purc_variant_get_string_const_ex(argv[0], &len)) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    if (htmlentities_en_ckws[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(htmlentities_en_ckws); i++) {
            htmlentities_en_ckws[i].atom = purc_atom_from_static_string_ex(
                    ATOM_BUCKET_DVOBJ, htmlentities_en_ckws[i].option);
        }
    }

    int flags;
    flags = pcdvobjs_parse_options(
            nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID, NULL, 0,
            htmlentities_en_ckws, PCA_TABLESIZE(type_skws),
            HEF_SINGLE_QUOTES | HEF_DOUBLE_QUOTES | HEF_DOUBLE_ENCODE, -1);
    if (flags == -1) {
        goto error;
    }

    if (len == 0)
        goto empty_done;

    if (!(flags & HEF_CONVERT_ALL) && strpbrk(str, "<>&'\"") == NULL)
        goto empty_done;

    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto failed;
    }

    char *p = (char *)str;
    while (*p) {

        uint32_t uc = pcutils_utf8_to_unichar((unsigned char *)p);

        const char *entity = NULL;
        if (!(flags & HEF_CONVERT_ALL)) {
            switch (uc) {
                case '\'':
                    if (flags & HEF_SINGLE_QUOTES)
                        entity = "apos";
                    break;
                case '"':
                    if (flags & HEF_DOUBLE_QUOTES)
                        entity = "quot";
                    break;
                case '<':
                    entity = "lt";
                    break;
                case '>':
                    entity = "gt";
                    break;
                case '&':
                    if (flags & HEF_DOUBLE_ENCODE)
                        entity = "amp";
                    else if (!is_valid_entity(p)) {
                        entity = "amp";
                    }
                    break;
                default:
                    break;
            }
        }
        else {
            if (uc == '&') {
                if (flags & HEF_DOUBLE_ENCODE) {
                    entity = "amp";
                }
                else if (!is_valid_entity(p)) {
                    entity = "amp";
                }
            }
            else {
                // convert all possible entities.
            }
        }

        if (entity == NULL) {
            unsigned char utf8[10];
            unsigned utf8_len = pcutils_unichar_to_utf8(uc, utf8);

            if (purc_rwstream_write(rwstream, utf8, utf8_len) < utf8_len) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto failed;
            }
        }
        else {
            char buf[32];
            int mylen = snprintf(buf, sizeof(buf), "&%s;", entity);
            if (mylen > 0 &&
                    purc_rwstream_write(rwstream, buf, mylen) < mylen) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto failed;
            }
        }

        p = pcutils_utf8_next_char(p);
    }

    size_t sz_buffer = 0;
    size_t sz_content = 0;
    char *content = NULL;
    content = purc_rwstream_get_mem_buffer_ex(rwstream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(rwstream);

    return purc_variant_make_string_reuse_buff(content, sz_buffer, false);

empty_done:
    return purc_variant_ref(argv[0]);

failed:
    if (rwstream)
        purc_rwstream_destroy(rwstream);

error:
    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

/*
$STR.htmlentities_decode(
    <string $string: `The input string.`>
    [,
        <'keep-double-quotes || keep-single-quotes || substitute-invalid ]'
                $flags = 'substitute-invalid':
            - 'keep-single-quotes': `Keep single-quotes unconverted.`
            - 'keep-double-quotes': `Keep double-quotes unconverted.`
            - 'substitute-invalid': `Replace invalid HTML entity with a
                    Unicode Replacement Character U+FFFD; or ignore it.` >
    ]
) string
 */
enum {
    HEF_KEEP_SINGLE_QUOTES  = 0x1 << 0,
    HEF_KEEP_DOUBLE_QUOTES  = 0x1 << 1,
    HEF_SUBSTITUE_INVALID   = 0x1 << 2,
};

static struct pcdvobjs_option_to_atom htmlentities_de_ckws[] = {
    { "keep-single-quotes",  0,  HEF_KEEP_SINGLE_QUOTES },
    { "keep-double-quotes",  0,  HEF_KEEP_DOUBLE_QUOTES },
    { "substitue-invalid",   0,  HEF_SUBSTITUE_INVALID },
};

static purc_variant_t
htmlentities_decode_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    int ec = PURC_ERROR_OK;
    purc_rwstream_t rwstream = NULL;

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *str;
    size_t len;

    if ((str = purc_variant_get_string_const_ex(argv[0], &len)) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    if (htmlentities_de_ckws[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(htmlentities_de_ckws); i++) {
            htmlentities_de_ckws[i].atom = purc_atom_from_static_string_ex(
                    ATOM_BUCKET_DVOBJ, htmlentities_de_ckws[i].option);
        }
    }

    int flags;
    flags = pcdvobjs_parse_options(
            nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID, NULL, 0,
            htmlentities_de_ckws, PCA_TABLESIZE(type_skws),
            HEF_SUBSTITUE_INVALID, -1);
    if (flags == -1) {
        goto error;
    }

    if (len == 0)
        goto empty_done;

    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto failed;
    }

    char *p = (char *)str;
    while (*p) {

        uint32_t uc = pcutils_utf8_to_unichar((unsigned char *)p);

        /* TODO: decode HTML entities here */
        unsigned char utf8[10];
        unsigned utf8_len = pcutils_unichar_to_utf8(uc, utf8);

        if (purc_rwstream_write(rwstream, utf8, utf8_len) < utf8_len) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto failed;
        }

        p = pcutils_utf8_next_char(p);
    }

    size_t sz_buffer = 0;
    size_t sz_content = 0;
    char *content = NULL;
    content = purc_rwstream_get_mem_buffer_ex(rwstream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(rwstream);

    return purc_variant_make_string_reuse_buff(content, sz_buffer, false);

empty_done:
    return purc_variant_ref(argv[0]);

failed:
    if (rwstream)
        purc_rwstream_destroy(rwstream);

error:
    if (ec)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_string_new(void)
{
    static struct purc_dvobj_method method [] = {
        { "nr_bytes",   nr_bytes_getter,    NULL },
        { "nr_chars",   nr_chars_getter,    NULL },
        { "contains",   contains_getter,    NULL },
        { "starts_with",starts_with_getter, NULL },
        { "ends_with",  ends_with_getter,   NULL },
        { "join",       join_getter,        NULL },
        { "tolower",    tolower_getter,     NULL },
        { "toupper",    toupper_getter,     NULL },
        { "shuffle",    shuffle_getter,     NULL },
        { "repeat",     repeat_getter,      NULL },
        { "reverse",    reverse_getter,     NULL },
        { "explode",    explode_getter,     NULL },
        { "implode",    implode_getter,     NULL },
        { "tokenize",   tokenize_getter,    NULL },
        { "replace",    replace_getter,     NULL },
        { "translate",  translate_getter,   NULL },
        { "format_c",   format_c_getter,    NULL },
        { "format_p",   format_p_getter,    NULL },
        { "substr",     substr_getter,      NULL },
        { "strstr",     strstr_getter,      NULL },
        { "strpos",     strpos_getter,      NULL },
        { "strpbrk",    strpbrk_getter,     NULL },
        { "trim",       trim_getter,        NULL },
        { "rot13",      rot13_getter,       NULL },
        { "count_chars",count_chars_getter, NULL },
        { "count_bytes",count_bytes_getter, NULL },
        { "codepoints", codepoints_getter,  NULL },
        { "htmlentities_encode", htmlentities_encode_getter,  NULL },
        { "htmlentities_decode", htmlentities_decode_getter,  NULL },
    };

    return purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
}
