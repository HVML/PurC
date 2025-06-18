/*
 * @file string.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of string dynamic variant object.
 *
 * Copyright (C) 2021 ~ 2025 FMSoft <https://www.fmsoft.cn>
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

#undef NDEBUG

#include "purc-errors.h"
#define _GNU_SOURCE
#include "config.h"

#include "helper.h"

#include "private/errors.h"
#include "private/dvobjs.h"
#include "private/stream.h"
#include "private/utils.h"
#include "private/variant.h"
#include "private/atom-buckets.h"
#include "private/ports.h"

#include "purc-variant.h"
#include "purc-dvobjs.h"

#include <errno.h>

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

union string_or_utf8char {
    const char *string;
    char        utf8ch[8];
};

static int cmp_strlen_desc(const void *v1, const void *v2)
{
    const union string_or_utf8char *str1 = v1;
    const union string_or_utf8char *str2 = v2;

    size_t len1 = strlen(str1->string);
    size_t len2 = strlen(str2->string);

    if (len2 > len1)
        return 1;
    else if (len2 < len1)
        return -1;

    return strcmp(str2->string, str1->string);
}

static purc_variant_t
translate_characters(const char *subject, size_t nr_searches,
        union string_or_utf8char *searches, union string_or_utf8char *replaces)
{
    purc_rwstream_t rwstream;
    rwstream = purc_rwstream_new_buffer(32, MAX_SIZE_BUFSTM);

    while (*subject) {
        const char *found = NULL;
        for (size_t i = 0; i < nr_searches; i++) {
            const char *search, *replace;
            search = searches[i].utf8ch;
            replace = replaces[i].utf8ch;

            found = strstr(subject, search);
            if (found) {
                if (found > subject)
                    purc_rwstream_write(rwstream, subject, found - subject);
                purc_rwstream_write(rwstream, replace, strlen(replace));
                subject = found + strlen(search);
                break;
            }
        }

        // write left characters
        if (found == NULL) {
            purc_rwstream_write(rwstream, subject, strlen(subject));
            break;
        }
    }

    purc_rwstream_write(rwstream, "", 1);
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
translate_strings(const char *subject, size_t nr_searches,
        union string_or_utf8char *searches, union string_or_utf8char *replaces)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    union string_or_utf8char *medians_heap = NULL;
    const char *medians_stack[SZ_IN_STACK];
    const char **medians = NULL;

    if (nr_searches > (0x7FFFFFFF - 0x100000))
        goto done;

    medians_heap = calloc(nr_searches, sizeof(medians_heap[0]));
    if (medians_heap == NULL) {
        goto done;
    }

    if (nr_searches <= SZ_IN_STACK) {
        medians = medians_stack;
    }
    else {
        medians = calloc(nr_searches, sizeof(medians[0]));
        if (medians == NULL) {
            goto done;
        }
    }

    /* We use the not-used Unicode code points (>= 0x100000)
       as the median strings. */
    uint32_t nuc = 0x100000;
    for (size_t i = 0; i < nr_searches; i++) {
        unsigned n = pcutils_unichar_to_utf8(nuc,
                (unsigned char*)medians_heap[i].utf8ch);
        medians_heap[i].utf8ch[n] = 0;
        medians[i] = medians_heap[i].utf8ch;
        nuc++;
    }

    retv = replace_one_subject(subject, (const char **)searches, nr_searches,
            medians, nr_searches, do_replace_case);
    if (!retv)
        goto done;

    purc_variant_t tmp = retv;
    subject = purc_variant_get_string_const(tmp);
    retv = replace_one_subject(subject, medians, nr_searches,
            (const char **)replaces, nr_searches, do_replace_case);
    purc_variant_unref(tmp);

done:
    if (medians_heap != NULL)
        free((void *)medians_heap);

    if (medians != NULL && medians != medians_stack)
        free((void *)medians);

    return retv;
}

/*
$STR.translate(
    <string $string: `The string being translated.`>,
    <string $from: `The characters being translated to.`>,
    <string $to: `The characters replacing from.`>
) string

$STR.translate(
    <string $string: `The string being translated.`>,
    <object $from_to_pairs: `All the occurrences of the keys in $string will
            been replaced by the corresponding values.`>,
) string
 */
static purc_variant_t
translate_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    union string_or_utf8char searches_stack[SZ_IN_STACK];
    union string_or_utf8char replaces_stack[SZ_IN_STACK];
    union string_or_utf8char *searches = NULL;
    union string_or_utf8char *replaces = NULL;
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

    size_t nr_kvs = 0;
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
                searches[n].string = from;
                n++;
            }
        } end_foreach;

        nr_searches = n;
        qsort(searches, nr_searches, sizeof(searches[0]), cmp_strlen_desc);

        for (n = 0; n < nr_searches; n++) {
            vv = purc_variant_object_get_by_ckey_ex(argv[1],
                    searches[n].string, false);
            replaces[n].string = purc_variant_get_string_const(vv);
        }
    }
    else {
        if (nr_args < 3) {
            ec = PURC_ERROR_ARGUMENT_MISSED;
            goto error;
        }

        const char *from, *to;
        from = purc_variant_get_string_const(argv[1]);
        to = purc_variant_get_string_const(argv[2]);
        if (from == NULL || to == NULL) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto error;
        }

        size_t n1, n2;
        purc_variant_string_chars(argv[1], &n1);
        purc_variant_string_chars(argv[2], &n2);
        nr_searches = MIN(n1, n2);
        if (nr_searches <= SZ_IN_STACK) {
            searches = searches_stack;
            replaces = replaces_stack;
        }
        else {
            searches = calloc(nr_searches, sizeof(searches[0]));
            if (searches == NULL) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto error;
            }

            replaces = calloc(nr_searches, sizeof(replaces[0]));
            if (replaces == NULL) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto error;
            }
        }

        for (size_t i = 0; i < nr_searches; i++) {
            const char *n1 = pcutils_utf8_next_char(from);
            size_t chlen1 = n1 - from;
            memcpy(searches[i].utf8ch, from, chlen1);
            searches[i].utf8ch[chlen1] = 0;

            const char *n2 = pcutils_utf8_next_char(to);
            size_t chlen2 = n2 - to;
            memcpy(replaces[i].utf8ch, to, chlen2);
            replaces[i].utf8ch[chlen2] = 0;

            from = n1;
            to = n2;
        }
    }

    if (nr_searches == 0) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto error;
    }

    if (nr_kvs > 0) {
        retv = translate_strings(subject, nr_searches, searches, replaces);
    }
    else {
        retv = translate_characters(subject, nr_searches, searches, replaces);
    }

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
#define C_PRINTF_CONVERSION_SPECIFIERS  "dioupxXeEfFgGaAcsnm%"

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
printf_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int ec = PURC_ERROR_OK;
    pcdvobjs_stream *stream_ett = NULL;
    purc_rwstream_t output_stm = NULL;
    ssize_t sz_written;
    size_t sz_total = 0;

    if (nr_args > 0) {
        stream_ett = dvobjs_stream_check_entity(argv[0], NULL);
    }

    if (stream_ett) {
        if ((output_stm = stream_ett->stm4w) == NULL) {
            ec = PURC_ERROR_NOT_DESIRED_ENTITY;
            goto error;
        }

        nr_args--;
        argv++;
    }
    else {
        output_stm = purc_rwstream_new_buffer(32, MAX_SIZE_BUFSTM);
        if (output_stm == NULL) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto error;
        }
    }

    if (nr_args < 2) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *format;
    size_t format_len;

    if ((format = purc_variant_get_string_const_ex(argv[0], &format_len)) ==
            NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    bool is_array = false;
    size_t sz_array = 0;
    if (purc_variant_linear_container_size(argv[1], &sz_array)) {
        is_array = true;
    }

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
                ec = PURC_ERROR_INVALID_VALUE;
                goto error;
            }

            if (strchr("nm%", conv_spec[cs_len - 1]) == NULL
                    && item == PURC_VARIANT_INVALID) {
                ec = PURC_ERROR_ARGUMENT_MISSED;
                goto error;
            }

            if ((sz_written = purc_rwstream_write(output_stm,
                    format + start, i - start)) < 0) {
                goto error;
            }
            sz_total += sz_written;

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
                    ec = PURC_ERROR_WRONG_DATA_TYPE;
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
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto error;
                }
                if ((sz_written = purc_rwstream_write(output_stm,
                        buff, len)) < 0) {
                    goto error;
                }
                sz_total += sz_written;
                break;

            case 'o':
            case 'u':
            case 'p':
            case 'x':
            case 'X':
                if (!purc_variant_cast_to_ulongint(item, &u64, false)) {
                    ec = PURC_ERROR_WRONG_DATA_TYPE;
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
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto error;
                }
                if ((sz_written = purc_rwstream_write(output_stm,
                        buff, len)) < 0) {
                    goto error;
                }
                sz_total += sz_written;
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
                        ec = PURC_ERROR_WRONG_DATA_TYPE;
                        goto error;
                    }
                    len = snprintf(buff, sizeof(buff), conv_spec, ld);
                }
                else {
                    double d;
                    if (!purc_variant_cast_to_number(item, &d, false)) {
                        ec = PURC_ERROR_WRONG_DATA_TYPE;
                        goto error;
                    }
                    len = snprintf(buff, sizeof(buff), conv_spec, d);
                }
                if (len < 0) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto error;
                }
                if ((sz_written = purc_rwstream_write(output_stm,
                        buff, len)) < 0) {
                    goto error;
                }
                sz_total += sz_written;
                break;

            case 's':
                string = purc_variant_get_string_const(item);
                if (string == NULL) {
                    ec = PURC_ERROR_WRONG_DATA_TYPE;
                    goto error;
                }
                len = asprintf(&buff_alloc, conv_spec, string);
                if (len < 0) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto error;
                }

                if (!pcutils_string_check_utf8(buff_alloc, -1, NULL, NULL)) {
                    free(buff_alloc);
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto error;
                }

                if ((sz_written = purc_rwstream_write(output_stm, buff_alloc, len)) < 0) {
                    goto error;
                }
                sz_total += sz_written;
                free(buff_alloc);
                break;

            case 'n':
            case 'm':
            case '%':
                len = snprintf(buff, sizeof(buff), conv_spec, 0);
                if (len < 0) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto error;
                }
                if ((sz_written = purc_rwstream_write(output_stm,
                        buff, len)) < 0) {
                    goto error;
                }
                sz_total += sz_written;
                arg_used = 0;
                break;

            default:
                ec = PURC_ERROR_INVALID_VALUE;
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

    if (i != start) {
        if ((sz_written = purc_rwstream_write(output_stm,
                format + start, strlen(format + start))) < 0) {
            goto error;
        }
        sz_total += sz_written;
    }

    if (stream_ett == NULL) {
        purc_rwstream_write(output_stm, "\0", 1);

        size_t rw_size = 0;
        size_t content_size = 0;
        char *rw_string = purc_rwstream_get_mem_buffer_ex(output_stm,
                &content_size, &rw_size, true);

        purc_variant_t retv = PURC_VARIANT_INVALID;
        if ((rw_size == 0) || (rw_string == NULL)) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto error;
        }
        else {
            retv = purc_variant_make_string_reuse_buff(rw_string, rw_size, false);
        }
        purc_rwstream_destroy(output_stm);
        return retv;
    }

    return purc_variant_make_ulongint(sz_total);

error:
    if (stream_ett == NULL && output_stm != NULL)
        purc_rwstream_destroy(output_stm);

    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

#define C_SCANF_CONVERSION_SPECIFIERS   "dioupxXeEfFgGaAcs]"

#if OS(DARWIN)
static char *rebuild_directive(const char *directive, size_t width)
{
    // Scan directive to find the conversion specifier.
    // If the conversion specifier is 's', 'c', or '[', 
    // convert width to a decimal string and insert before the conversion specifier.
    // Or, just return a copy of direction.

    char *new_directive = malloc(strlen(directive) + 32 + 1);
    if (new_directive == NULL) {
        return NULL;
    }

    const char *src = directive;
    char *dst = new_directive;
    while (*src) {
        if (strchr("sc[", *src)) {
            // Insert width before the conversion specifier.
            int n = sprintf(dst, "%zu", width);
            assert(n <= 32);
            dst += n;
        }

        *dst = *src;
        src++;
        dst++;
    }

    *dst = '\0';
    return new_directive;
}
#endif

static purc_variant_t convert_one_directive(FILE *fp,
        char *directive, int *ec)
{
    purc_variant_t ans = PURC_VARIANT_INVALID;

    size_t dir_len = strlen(directive);
    assert(directive[0] == '%');

    int ret = -1;
    switch (directive[dir_len - 1]) {
    case 'i':
    case 'd':
        if (strstr(directive, "ll")) {
            int64_t i64;
            if ((ret = fscanf(fp, directive, &i64)) == 1) {
                ans = purc_variant_make_longint(i64);
            }
        }
        else if (strstr(directive, "hh")) {
            int8_t  i8;
            if ((ret = fscanf(fp, directive, &i8)) == 1) {
                ans = purc_variant_make_longint(i8);
            }
        }
        else if (strchr(directive, 'j')) {
            intmax_t imax;
            if ((ret = fscanf(fp, directive, &imax)) == 1) {
                ans = purc_variant_make_longint(imax);
            }
        }
        else if (strchr(directive, 'z')) {
            ssize_t ssz;
            if ((ret = fscanf(fp, directive, &ssz)) == 1) {
                ans = purc_variant_make_longint(ssz);
            }
        }
        else if (strchr(directive, 'l')) {
            long i32;
            if ((ret = fscanf(fp, directive, &i32)) == 1) {
                ans = purc_variant_make_longint(i32);
            }
        }
        else if (strchr(directive, 'h')) {
            short i16;
            if ((ret = fscanf(fp, directive, &i16)) == 1) {
                ans = purc_variant_make_longint(i16);
            }
        }
        else {
            int i32;
            if ((ret = fscanf(fp, directive, &i32)) == 1) {
                ans = purc_variant_make_longint(i32);
            }
        }
        break;

    case 'o':
    case 'u':
    case 'p':
    case 'x':
    case 'X':
        if (strstr(directive, "ll")) {
            uint64_t u64;
            if ((ret = fscanf(fp, directive, &u64)) == 1) {
                ans = purc_variant_make_ulongint(u64);
            }
        }
        else if (strstr(directive, "hh")) {
            uint8_t u8;
            if ((ret = fscanf(fp, directive, &u8)) == 1) {
                ans = purc_variant_make_ulongint(u8);
            }
        }
        else if (strchr(directive, 'j')) {
            uintmax_t umax;
            if ((ret = fscanf(fp, directive, &umax)) == 1) {
                ans = purc_variant_make_ulongint(umax);
            }
        }
        else if (strchr(directive, 'z')) {
            size_t sz;
            if ((ret = fscanf(fp, directive, &sz)) == 1) {
                ans = purc_variant_make_ulongint(sz);
            }
        }
        else if (strchr(directive, 'l')) {
            unsigned long u32;
            if ((ret = fscanf(fp, directive, &u32)) == 1) {
                ans = purc_variant_make_ulongint(u32);
            }
        }
        else if (strchr(directive, 'h')) {
            unsigned short u16;
            if ((ret = fscanf(fp, directive, &u16)) == 1) {
                ans = purc_variant_make_ulongint(u16);
            }
        }
        else {
            unsigned int u32;
            if ((ret = fscanf(fp, directive, &u32)) == 1) {
                ans = purc_variant_make_ulongint(u32);
            }
        }
        break;

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'a':
    case 'A':
        if (strchr(directive, 'L')) {
            long double ld;
            if ((ret = fscanf(fp, directive, &ld)) == 1) {
                ans = purc_variant_make_longdouble(ld);
            }
        }
        else if (strchr(directive, 'l')) {
            double d;
            if ((ret = fscanf(fp, directive, &d)) == 1) {
                ans = purc_variant_make_number(d);
            }
        }
        else {
            float f;
            if ((ret = fscanf(fp, directive, &f)) == 1) {
                ans = purc_variant_make_number(f);
            }
        }
        break;

    case 'c': {
        if (strchr(directive, 'l') != NULL) {
            *ec = PURC_ERROR_INVALID_VALUE;
            break;
        }

        char *buff = NULL;
        size_t sz_buff = 0;
        if (strchr(directive, '*') == NULL) {
            char *end;
            long width = strtol(directive + 1, &end, 10);
            if (width < 0) {
                *ec = PURC_ERROR_INVALID_VALUE;
                break;
            }

            if (width == 0 || end == directive + 1) {
                width = 1;
            }

            sz_buff = width + 1;
            buff = malloc(sz_buff);
            if (buff == NULL) {
                *ec = PURC_ERROR_OUT_OF_MEMORY;
                break;
            }
        }

        ret = fscanf(fp, directive, buff);
        if (ret != 1) {
            free(buff);
            break;
        }

        assert(buff);
        buff[sz_buff - 1] = '\0';   // null-terminated.
        PC_DEBUG("Directive: `%s`, converted: `%s`\n", directive, buff);
        if (!pcutils_string_check_utf8(buff, -1, NULL, NULL)) {
            ans = purc_variant_make_byte_sequence_reuse_buff(
                    buff, sz_buff, sz_buff);
        }
        else {
            ans = purc_variant_make_string_reuse_buff(
                    buff, sz_buff, true);
        }
        break;
    }

    case 's':
    case ']': {
        if (strchr(directive, 'l') != NULL) {
            *ec = PURC_ERROR_INVALID_VALUE;
            break;
        }

        char *buff = NULL;
        size_t sz_buff = 0;
        if (strchr(directive, '*') == NULL) {
            char *end;
            long width = strtol(directive + 1, &end, 10);
            if (width < 0) {
                *ec = PURC_ERROR_INVALID_VALUE;
                break;
            }

            if (width > 0) {
                sz_buff = width + 1;
                buff = malloc(sz_buff);
                if (buff == NULL) {
                    *ec = PURC_ERROR_OUT_OF_MEMORY;
                    break;
                }
            }
        }

#if OS(DARWIN)
#define MAX_SIZE_SCANF_STR  1023
        if (strchr(directive, '*') == NULL && buff == NULL) {
            // rebuild the directive to include the field width.
            char *new_dir = rebuild_directive(directive, MAX_SIZE_SCANF_STR);
            if (new_dir == NULL) {
                *ec = PURC_ERROR_OUT_OF_MEMORY;
                break;
            }

            sz_buff = MAX_SIZE_SCANF_STR + 1;
            buff = malloc(sz_buff);
            if (new_dir == NULL || buff == NULL) {
                *ec = PURC_ERROR_OUT_OF_MEMORY;
                break;
            }

            ret = fscanf(fp, new_dir, buff);
            PC_DEBUG("Directive: %s, converted: `%s`; ret: %d\n",
                    new_dir, buff, ret);
            free(new_dir);

            if (ret != 1) {
                free(buff);
                break;
            }
        }
        else {
            ret = fscanf(fp, directive, buff);
            if (ret != 1) {
                free(buff);
                break;
            }
        }

#else
        if (strchr(directive, '*') == NULL &&
                strchr(directive, 'm') == NULL && buff == NULL) {
            // we have appended an extra null-terminating character.
            memmove(directive + 2, directive + 1, dir_len - 1);
            directive[1] = 'm';
        }

        if (buff == NULL) {
            ret = fscanf(fp, directive, &buff);
        }
        else {
            ret = fscanf(fp, directive, buff);
        }

        if (ret != 1)
            break;

        if (sz_buff != 0) {
            buff[sz_buff - 1] = '\0';   // null-terminated
        }
        else
            sz_buff = strlen(buff) + 1;
#endif

        PC_DEBUG("Directive: %s, converted: `%s`; ret: %d\n",
                directive, buff, ret);

        assert(buff);
        if (!pcutils_string_check_utf8(buff, -1, NULL, NULL)) {
            ans = purc_variant_make_byte_sequence_reuse_buff(
                    buff, sz_buff, sz_buff);
        }
        else {
            ans = purc_variant_make_string_reuse_buff(
                    buff, sz_buff, true);
        }
        break;
    }

    default:
        *ec = PURC_ERROR_INVALID_VALUE;
        return ans;
    }

    if (ret < 0) {
        perror("Failed fscanf(): ");
        *ec = purc_error_from_errno(errno);
    }
    else if (ret == 1 && ans == PURC_VARIANT_INVALID) {
        *ec = purc_get_last_error();
    }
    else {
        *ec = PURC_ERROR_OK;
    }

    return ans;
}

static int fread_utf8_char(FILE *fp, uint8_t *buf_utf8, int *len_utf8)
{
    int c = fgetc(fp);

    if (c == EOF)
        return PURC_ERROR_NO_DATA;

    buf_utf8[0] = (uint8_t)c;
    if (buf_utf8[0] > 0xFD) {
        return PURC_ERROR_BAD_ENCODING;
    }

    int ch_len;
    if (c & 0x80) {
        int n = 1;
        while (c & (0x80 >> n))
            n++;

        if (n < 2 || n > 4) {
            return PURC_ERROR_BAD_ENCODING;
        }

        ch_len = n;
    }
    else {
        ch_len = 1;
    }

    /* read left bytes */
    int left = ch_len - 1;
    uint8_t* p = buf_utf8 + 1;
    while (left > 0) {
        c = fgetc(fp);
        if (c == EOF) {
            return PURC_ERROR_NO_DATA;
        }

        *p = (uint8_t)c;
        if ((*p & 0xC0) != 0x80) {
            return PURC_ERROR_BAD_ENCODING;
        }

        p++;
        left--;
    }

    if (len_utf8)
        *len_utf8 = ch_len;

    return PURC_ERROR_OK;
}

/*
$STR.scanf(
        < string | bsequence | stream $input: `The input data: a string, a byte sequence, or a readable stream.` >,
        < string $format: `The format string.` >
) array | false
 */
static purc_variant_t
scanf_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    pcdvobjs_stream *stream_ett = NULL;
    FILE *input_fp = NULL;

    purc_variant_t result = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 2) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto failed;
    }

    stream_ett = dvobjs_stream_check_entity(argv[0], NULL);
    if (stream_ett == NULL) {
        const unsigned char *bytes;
        size_t nr_bytes;
        bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);
        if (!bytes) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto failed;
        }

        input_fp = fmemopen((void *)bytes, nr_bytes, "r");
        if (input_fp == NULL) {
            ec = purc_error_from_errno(errno);
            goto failed;
        }
    }
    else if (stream_ett->fd4r < 0) {
        ec = PURC_ERROR_NOT_DESIRED_ENTITY;
        goto failed;
    }
    else {
        input_fp = fdopen(stream_ett->fd4r, "r");
        if (input_fp == NULL) {
            ec = purc_error_from_errno(errno);
            goto failed;
        }
    }

    // Get format string
    const char* format_str;
    size_t format_len;
    format_str = purc_variant_get_string_const_ex(argv[1], &format_len);
    if (!format_str) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    enum {
        STATE_UNKNOWN = 0,
        STATE_ORDINARY,
        STATE_SPACE,
        STATE_DIRECTIVE,
    };

    struct _scanner {
        int state;
        struct pcutils_mystring directive;
    } scanner = { STATE_UNKNOWN, { NULL, 0, 0 } };

    result = purc_variant_make_array_0();
    if (result == PURC_VARIANT_INVALID)
        goto failed;

    pcutils_mystring_init(&scanner.directive);

    const char *p = format_str;
    while (*p) {
        const char *next = pcutils_utf8_next_char(p);

        switch (scanner.state) {
        case STATE_UNKNOWN:
            if (*p == '%') {
                scanner.state = STATE_DIRECTIVE;
                pcutils_mystring_append_mchar(&scanner.directive,
                        (const unsigned char *)p, 1);
            }
            else if (purc_isspace(*p)) {
                // Consume all continuous spaces.
                while (purc_isspace(*next)) {
                    next = pcutils_utf8_next_char(next);
                }

                int c;
                // Ignore all continuous spaces in input stream.
                do {
                    if ((c = fgetc(input_fp)) == EOF) {
                        goto failed;
                    }
                } while (purc_isspace(c));
                ungetc(c, input_fp);
                scanner.state = STATE_UNKNOWN;

            }
            else {  // other character
                scanner.state = STATE_ORDINARY;
            }
            break;

        case STATE_DIRECTIVE:
            if (scanner.directive.nr_bytes == 1 && *p == '%') {
                scanner.state = STATE_ORDINARY;
            }
            else if (strchr(C_SCANF_CONVERSION_SPECIFIERS, *p)) {
                pcutils_mystring_append_mchar(&scanner.directive,
                        (const unsigned char *)p, 1);

                // append an extra null-terminating byte for possible 'm'.
                pcutils_mystring_append_mchar(&scanner.directive,
                        (const unsigned char *)"", 1);

                // Do convert here.
                pcutils_mystring_done(&scanner.directive);
                purc_variant_t tmp = convert_one_directive(input_fp,
                        (char *)scanner.directive.buff, &ec);
                if (ec != PURC_ERROR_OK) {
                    goto failed;
                }

                if (tmp != PURC_VARIANT_INVALID) {
                    if (!purc_variant_array_append(result, tmp)) {
                        purc_variant_unref(tmp);
                        goto failed;
                    }
                    purc_variant_unref(tmp);
                }

                pcutils_mystring_free(&scanner.directive);
                scanner.state = STATE_UNKNOWN;
            }
            else if (next > p + 1) {
                // Non-ASCII character.
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }
            else {
                pcutils_mystring_append_mchar(&scanner.directive,
                        (const unsigned char*)p, 1);
            }
            break;

        default:
            assert(0); // never be here.
            break;
        }

        if (scanner.state == STATE_ORDINARY) {
            char utf8ch[10];
            int uclen = 0;
            ec = fread_utf8_char(input_fp, (uint8_t *)utf8ch, &uclen);
            if (ec != PURC_ERROR_OK) {
                goto failed;
            }

            if (memcmp(p, utf8ch, uclen) != 0) {
                utf8ch[uclen] = 0;
                PC_DEBUG("Format string does not matche input data: "
                        "'%s' vs '%s'\n", p, utf8ch);
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }

            scanner.state = STATE_UNKNOWN;
        }

        p = next;
    }

    pcutils_mystring_free(&scanner.directive);

    if (scanner.state == STATE_DIRECTIVE) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

    if (input_fp != NULL)
        fclose(input_fp);

    if (result == PURC_VARIANT_INVALID)
        result = purc_variant_make_null();

    return result;

failed:
    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (result != PURC_VARIANT_INVALID)
        purc_variant_unref(result);

    if (input_fp != NULL) {
        fclose(input_fp);
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

/*
$STR.printp(
    < string $format: `The format string contains placeholders.` >,
    < array | object | any $data0: `The data to serialize.` >
    [,
        <any $data1: `The data to serialize.` >, ...
    ]
) string | true | false
 */
static purc_variant_t
printp_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    pcdvobjs_stream *stream_ett = NULL;
    purc_rwstream_t output_stm = NULL;
    int ec = PURC_ERROR_OK;
    ssize_t sz_written;
    size_t sz_total = 0;

    if (nr_args > 0) {
        stream_ett = dvobjs_stream_check_entity(argv[0], NULL);
    }

    if (stream_ett) {
        if ((output_stm = stream_ett->stm4w) == NULL) {
            ec = PURC_ERROR_NOT_DESIRED_ENTITY;
            goto failed;
        }

        nr_args--;
        argv++;
    }
    else {
        output_stm = purc_rwstream_new_buffer(32, MAX_SIZE_BUFSTM);
        if (output_stm == NULL) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto failed;
        }
    }

    if (nr_args < 2) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto failed;
    }

    const char *fmt_str = NULL;
    size_t fmt_len;
    fmt_str = purc_variant_get_string_const_ex(argv[0], &fmt_len);
    if (fmt_str == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    enum {
        TYPE_UNKNOWN = -1,
        TYPE_LICNTR = 0,
        TYPE_OBJECT,
    };

    int type = TYPE_UNKNOWN;
    size_t data0_len = 0;
    if (purc_variant_linear_container_size(argv[1], &data0_len)) {
        type = TYPE_LICNTR;
    }
    else if (purc_variant_is_object(argv[1])) {
        type = TYPE_OBJECT;
        data0_len = -1;
    }

    enum {
        STATE_CHAR = 0,
        STATE_ESCAPED,
        STATE_INDEX,
        STATE_KEY,
        STATE_ARG,
    };

    const char *p = fmt_str;
    int state = STATE_CHAR;
    const char *name_str;
    size_t name_len = 0;
    while (true) {
        if (*p == '\\') {
            if (state == STATE_ESCAPED) {
                if ((sz_written = purc_rwstream_write(output_stm,
                        p, 1)) < 0) {
                    goto failed;
                }
                sz_total += sz_written;
                state = STATE_CHAR;
            }
            else {
                state = STATE_ESCAPED;
            }
        }
        else if (*p == '[') {
            if (state == STATE_ESCAPED) {
                if ((sz_written = purc_rwstream_write(output_stm,
                        p, 1)) < 0) {
                    goto failed;
                }
                sz_total += sz_written;
                state = STATE_CHAR;
            }
            else if (type != TYPE_LICNTR) {
                ec = PURC_ERROR_WRONG_DATA_TYPE;
                goto failed;
            }
            else {
                state = STATE_INDEX;
                name_str = p + 1;
                name_len = 0;
            }
        }
        else if (*p == '{') {
            if (state == STATE_ESCAPED) {
                if ((sz_written = purc_rwstream_write(output_stm,
                        p, 1)) < 0) {
                    goto failed;
                }
                sz_total += sz_written;
                state = STATE_CHAR;
            }
            else if (type != TYPE_OBJECT) {
                ec = PURC_ERROR_WRONG_DATA_TYPE;
                goto failed;
            }
            else {
                state = STATE_KEY;
                name_str = p + 1;
                name_len = 0;
            }
        }
        else if (*p == '#') {
            if (state == STATE_ESCAPED) {
                if ((sz_written = purc_rwstream_write(output_stm,
                        p, 1)) < 0) {
                    goto failed;
                }
                sz_total += sz_written;
                state = STATE_CHAR;
            }
            else {
                state = STATE_ARG;
                name_str = p + 1;
                name_len = 0;
            }
        }
        else {
            if (state == STATE_ESCAPED) {
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }

            purc_variant_t tmp;
            long index;

            switch (state) {
            case STATE_INDEX:
                if (*p == ']') {
                    if (name_len > 0) {
                        index = strtol(name_str, NULL, 10);
                    }
                    else {
                        ec = PURC_ERROR_INVALID_VALUE;
                        goto failed;
                    }

                    if (index < 0 || (size_t)index >= data0_len) {
                        ec = PURC_ERROR_BAD_INDEX;
                        goto failed;
                    }

                    tmp = purc_variant_linear_container_get(argv[1], index);
                    if ((sz_written = purc_variant_serialize(tmp, output_stm, 0,
                            PCVRNT_SERIALIZE_OPT_REAL_EJSON |
                            PCVRNT_SERIALIZE_OPT_RUNTIME_NULL |
                            PCVRNT_SERIALIZE_OPT_NOSLASHESCAPE |
                            PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX |
                            PCVRNT_SERIALIZE_OPT_TUPLE_EJSON, NULL)) < 0) {
                        goto failed;
                    }
                    sz_total += sz_written;

                    state = STATE_CHAR;
                }
                else if (!purc_isdigit(*p)) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto failed;
                }
                else if (*p) {
                    name_len++;
                }
                break;

            case STATE_ARG:
                if (purc_isdigit(*p)) {
                    name_len++;
                }
                else {
                    if (name_len > 0) {
                        index = strtol(name_str, NULL, 10);
                    }
                    else {
                        ec = PURC_ERROR_INVALID_VALUE;
                        goto failed;
                    }

                    if (index < 0 || (size_t)index >= (nr_args - 1)) {
                        ec = PURC_ERROR_ARGUMENT_MISSED;
                        goto failed;
                    }

                    tmp = argv[index + 1];
                    if ((sz_written = purc_variant_serialize(tmp, output_stm,
                            0,
                            PCVRNT_SERIALIZE_OPT_REAL_EJSON |
                            PCVRNT_SERIALIZE_OPT_RUNTIME_NULL |
                            PCVRNT_SERIALIZE_OPT_NOSLASHESCAPE |
                            PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX |
                            PCVRNT_SERIALIZE_OPT_TUPLE_EJSON, NULL)) < 0) {
                        goto failed;
                    }
                    sz_total += sz_written;

                    if (*p) {
                        if ((sz_written = purc_rwstream_write(output_stm,
                                p, 1)) < 0) {
                            goto failed;
                        }
                        sz_total += sz_written;
                    }
                    state = STATE_CHAR;
                }
                break;

            case STATE_KEY:
                if (*p == '}') {
                    char *key = strndup(name_str, name_len);
                    if (key == NULL) {
                        ec = PURC_ERROR_OUT_OF_MEMORY;
                        goto failed;
                    }

                    tmp = purc_variant_object_get_by_ckey_ex(argv[1], key,
                            false);
                    free(key);

                    if (tmp == PURC_VARIANT_INVALID) {
                        goto failed;
                    }

                    if ((sz_written = purc_variant_serialize(tmp, output_stm, 0,
                            PCVRNT_SERIALIZE_OPT_REAL_EJSON |
                            PCVRNT_SERIALIZE_OPT_RUNTIME_NULL |
                            PCVRNT_SERIALIZE_OPT_NOSLASHESCAPE |
                            PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX |
                            PCVRNT_SERIALIZE_OPT_TUPLE_EJSON, NULL)) < 0) {
                        goto failed;
                    }
                    sz_total += sz_written;

                    state = STATE_CHAR;
                }
                else if (*p) {
                    name_len++;
                }
                break;

            case STATE_CHAR:
                if (*p) {
                    if ((sz_written = purc_rwstream_write(output_stm,
                            p, 1)) < 0) {
                        goto failed;
                    }
                    sz_total += sz_written;
                }
                break;

            default:
                PC_ERROR("Never here\n");
                assert(0);
                break;
            }
        }

        if (*p == 0) {
            break;
        }

        p++;
    }

    if (stream_ett == NULL) {
        if (purc_rwstream_write(output_stm, "", 1) < 1) {
            goto failed;
        }

        size_t content_len = 0;
        size_t sz_buff = 0;
        char *buff = purc_rwstream_get_mem_buffer_ex(output_stm,
                &content_len, &sz_buff, true);

        if (sz_buff == 0 || buff == NULL) {
            goto failed;
        }

        purc_rwstream_destroy(output_stm);
        return purc_variant_make_string_reuse_buff(buff, sz_buff, false);
    }

    return purc_variant_make_ulongint(sz_total);

failed:
    if (stream_ett == NULL && output_stm)
        purc_rwstream_destroy(output_stm);

    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static bool
set_array_element_with_padding_null(purc_variant_t array,
        size_t idx, purc_variant_t val)
{
    size_t sz;

    if (!purc_variant_array_size(array, &sz))
        goto failed;

    if (idx >= sz) {
        // expand the array first.
        for (size_t i = sz; i <= idx; i++) {
            purc_variant_t tmp = purc_variant_make_null();
            if (!purc_variant_array_append(array, tmp)) {
                purc_variant_unref(tmp);
                goto failed;
            }

            purc_variant_unref(tmp);
        }
    }

    return purc_variant_array_set(array, idx, val);

failed:
    return false;
}

/*
$STR.scanp(
        < string | bsequence | native/stream $input: `The input data: a string, a byte sequence, or a readable stream.` >,
        < string $format: `The string contains placeholders.` >,
) array | object | any | false
*/
static purc_variant_t
scanp_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    pcdvobjs_stream *stream_ett = NULL;
    purc_rwstream_t input_stm = NULL;

    purc_variant_t result = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 2) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto failed;
    }

    stream_ett = dvobjs_stream_check_entity(argv[0], NULL);
    if (stream_ett == NULL) {
        const unsigned char *bytes;
        size_t nr_bytes;
        bytes = purc_variant_get_bytes_const(argv[0], &nr_bytes);
        if (!bytes) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto failed;
        }

        input_stm = purc_rwstream_new_from_mem((void *)bytes, nr_bytes);
        if (input_stm == NULL) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto failed;
        }
    }
    else if ((input_stm = stream_ett->stm4r) == NULL) {
        ec = PURC_ERROR_NOT_DESIRED_ENTITY;
        goto failed;
    }

    // Get format string
    const char* format_str;
    size_t format_len;
    format_str = purc_variant_get_string_const_ex(argv[1], &format_len);
    if (!format_str) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    enum {
        STATE_UNKNOWN = 0,
        STATE_ORDINARY,
        STATE_SPACE,
        STATE_ESCAPED,
        STATE_INDEX,
        STATE_KEY,
        STATE_ARG,
    };

    enum {
        RETT_UNKNOWN = 0,
        RETT_ARRAY,
        RETT_OBJECT,
        RETT_ONEANY
    };

    struct _scanner {
        int state, last;
        int ret_type;
        struct pcutils_mystring idx_buf;
    } scanner = { STATE_UNKNOWN, STATE_UNKNOWN, RETT_UNKNOWN, { NULL, 0, 0 } };

    pcutils_mystring_init(&scanner.idx_buf);

    const char *p = format_str;
    while (*p) {
        const char *next = pcutils_utf8_next_char(p);

        switch (scanner.state) {
        case STATE_UNKNOWN:
            if (*p == '\\') {
                scanner.state = STATE_ESCAPED;
            }
            else if (*p == '[') {
                if (scanner.ret_type != RETT_UNKNOWN &&
                        scanner.ret_type != RETT_ARRAY) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto failed;
                }
                else if (scanner.ret_type == RETT_UNKNOWN) {
                    result = purc_variant_make_array_0();
                    if (result == PURC_VARIANT_INVALID) {
                        goto failed;
                    }
                    scanner.ret_type = RETT_ARRAY;
                }

                scanner.state = STATE_INDEX;
            }
            else if (*p == '{') {
                if (scanner.ret_type != RETT_UNKNOWN &&
                        scanner.ret_type != RETT_OBJECT) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto failed;
                }
                else if (scanner.ret_type == RETT_UNKNOWN) {
                    result = purc_variant_make_object_0();
                    if (result == PURC_VARIANT_INVALID) {
                        goto failed;
                    }
                    scanner.ret_type = RETT_OBJECT;
                }

                scanner.state = STATE_KEY;
                pcutils_mystring_free(&scanner.idx_buf);
            }
            else if (*p == '#') {
                if (scanner.ret_type != RETT_UNKNOWN) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto failed;
                }

                scanner.ret_type = RETT_ONEANY;
                scanner.state = STATE_ARG;
            }
            else if (purc_isspace(*p)) {
                // Consume all continuous spaces.
                while (purc_isspace(*next)) {
                    next = pcutils_utf8_next_char(next);
                }

                scanner.state = STATE_SPACE;
            }
            else {  // Non-ASCII character
                scanner.state = STATE_ORDINARY;
            }
            break;

        case STATE_ESCAPED:
            if (strchr("[{#\\", *p)) {
                scanner.state = STATE_ORDINARY;
            }
            else {
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }
            break;

        case STATE_INDEX:
            if (*p == ']') {
                // It's time to generate a new array element.

                purc_variant_t tmp;
                tmp = purc_variant_load_from_json_stream(input_stm);
                if (tmp == PURC_VARIANT_INVALID) {
                    PC_DEBUG("Failed purc_variant_load_from_json_stream()\n");
                    goto failed;
                }

                if (scanner.idx_buf.nr_bytes == 0) {
                    if (!purc_variant_array_append(result, tmp)) {
                        purc_variant_unref(tmp);
                        goto failed;
                    }
                }
                else {
                    pcutils_mystring_done(&scanner.idx_buf);
                    long idx = strtol(scanner.idx_buf.buff, NULL, 10);

                    if (idx < 0) {
                        ec = PURC_ERROR_INVALID_VALUE;
                        goto failed;
                    }

                    set_array_element_with_padding_null(result, idx, tmp);
                }

                pcutils_mystring_free(&scanner.idx_buf);
                purc_variant_unref(tmp);
                scanner.state = STATE_UNKNOWN;
            }
            else if (purc_isdigit(*p)) {
                pcutils_mystring_append_mchar(&scanner.idx_buf,
                        (const unsigned char*)p, 1);
            }
            else {
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }
            break;

        case STATE_KEY:
            if (*p == '}') {
                // It's time to generate a new object property.

                if (scanner.idx_buf.nr_bytes == 0) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto failed;
                }

                purc_variant_t key;
                key = purc_variant_make_string_ex(scanner.idx_buf.buff,
                        scanner.idx_buf.nr_bytes, false);
                pcutils_mystring_free(&scanner.idx_buf);

                if (key == PURC_VARIANT_INVALID) {
                    goto failed;
                }

                purc_variant_t val;
                val = purc_variant_load_from_json_stream(input_stm);
                if (val == PURC_VARIANT_INVALID) {
                    goto failed;
                }

                if (!purc_variant_object_set(result, key, val)) {
                    purc_variant_unref(key);
                    purc_variant_unref(val);
                    goto failed;
                }

                purc_variant_unref(key);
                purc_variant_unref(val);
                scanner.state = STATE_UNKNOWN;
            }
            else {
                pcutils_mystring_append_mchar(&scanner.idx_buf,
                        (const unsigned char*)p, next - p);
            }
            break;

        case STATE_ARG:
            if (*p == '?') {
                PC_DEBUG("It's time to generate the one value.\n");
                result = purc_variant_load_from_json_stream(input_stm);
                if (result == PURC_VARIANT_INVALID) {
                    PC_DEBUG("Failed purc_variant_load_from_json_stream()\n");
                    goto failed;
                }

                goto done;
            }
            else {
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }
            break;

        default:
            assert(0); // Never be here.
            break;
        }

        // PC_DEBUG("State: %d; format string: %s\n", scanner.state, p);
        if (scanner.state == STATE_ORDINARY) {
            char utf8ch[10];
            int len = 0;
            if (scanner.last == STATE_SPACE) {
                // Ignore all continuous spaces in input stream.
                do {
                    len = purc_rwstream_read_utf8_char(input_stm, utf8ch, NULL);
                    if (len <= 0) {
                        goto failed;
                    }
                } while (purc_isspace(utf8ch[0]));
            }
            else {
                len = purc_rwstream_read_utf8_char(input_stm, utf8ch, NULL);
                if (len <= 0) {
                    goto failed;
                }
            }

            utf8ch[len] = 0;
            if (memcmp(p, utf8ch, len) != 0) {
                utf8ch[len] = 0;
                PC_DEBUG("Format string does not matche input data: "
                        "'%s' vs '%s'\n", p, utf8ch);
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }

            scanner.last = STATE_UNKNOWN;
            scanner.state = STATE_UNKNOWN;
        }
        else if (scanner.state == STATE_SPACE) {
            scanner.last = scanner.state;
            scanner.state = STATE_UNKNOWN;
        }
        else {
            scanner.last = STATE_UNKNOWN;
            // do nothing
        }

        p = next;
    }

done:
    pcutils_mystring_free(&scanner.idx_buf);

    if (stream_ett == NULL && input_stm != NULL)
        purc_rwstream_destroy(input_stm);

    if (result == PURC_VARIANT_INVALID)
        result = purc_variant_make_null();

    return result;

failed:
    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (result != PURC_VARIANT_INVALID)
        purc_variant_unref(result);

    if (stream_ett == NULL && input_stm != NULL)
        purc_rwstream_destroy(input_stm);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
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
$STR.substr_compare(
    <string $haystack>,
    <string $needle>,
    <real $offset>,
    [, <real $length = null>
        [, <boolean $case_insensitivity = false:
            false -  `Perform a case-sensitive comparison;`
            true -  `Perform a case-insensitive comparison.`>
        ]
    ]
) number | false
*/

// This function is implemented with aid from AI.
static purc_variant_t
substr_compare_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // Get the haystack string
    const char *haystack;
    size_t haystack_len;
    haystack = purc_variant_get_string_const_ex(argv[0], &haystack_len);
    if (!haystack) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // Get the needle string to compare
    const char *needle;
    size_t needle_len;
    needle = purc_variant_get_string_const_ex(argv[1], &needle_len);
    if (!needle) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // Get the offset
    int64_t offset;
    if (!purc_variant_cast_to_longint(argv[2], &offset, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // Get comparison length (optional)
    int64_t length = -1;
    if (nr_args > 3 && !purc_variant_is_null(argv[3])) {
        if (!purc_variant_cast_to_longint(argv[3], &length, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
        if (length < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    // Get case sensitivity flag (optional)
    bool case_insensitive = false;
    if (nr_args > 4) {
        case_insensitive = purc_variant_booleanize(argv[4]);
    }

    // Calculate character count of haystack string
    size_t haystack_chars;
    if (!pcutils_string_check_utf8(haystack, haystack_len, &haystack_chars,
                NULL)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    // Calculate character count of needle string
    size_t needle_chars;
    if (!pcutils_string_check_utf8(needle, needle_len, &needle_chars,
                NULL)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    // Handle negative offset
    if (offset < 0) {
        offset = haystack_chars + offset;
    }

    // Check if offset is valid
    if (offset < 0 || (size_t)offset > haystack_chars) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    // Position to offset
    const char *start = haystack;
    for (int64_t i = 0; i < offset; i++) {
        start = pcutils_utf8_next_char(start);
    }

    // Limit comparison length if specified
    size_t compare_len;
    if (length >= 0) {
        // Calculate byte length for specified character count
        const char *p = needle;
        size_t bytes = 0;
        size_t chars = 0;
        while (*p && chars < (size_t)length) {
            const char *next = pcutils_utf8_next_char(p);
            bytes += (next - p);
            chars++;
            p = next;
        }
        compare_len = bytes;
    }
    else {
        // Use entire needle string byte length
        compare_len = strlen(needle);
    }

    // Perform string comparison
    int result;
    if (case_insensitive) {
        result = pcutils_strncasecmp(start, needle, compare_len);
    }
    else {
        /* XXX: strncmp("abc", "def", 3) returns -96 (not -3) on Linux */
        result = strncmp(start, needle, compare_len);
    }

    /* Normalize the result to avoid different result values among 
       various platforms.*/
    if (result > 0) result = 1;
    else if (result < 0) result = -1;

    return purc_variant_make_number((double)result);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

/*
$STR.substr_count(
    <string $haystack: `The input string.`>,
    <string $needle: `The substring to search.`>
    [, <real $offset = 0: `The offset to starting search.`>
        [, <real $length = 0: `The length of searching.` >
        ]
    ]
) ulongint | false
*/
static purc_variant_t
substr_count_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // Get input string
    const char *haystack;
    size_t haystack_len;
    haystack = purc_variant_get_string_const_ex(argv[0], &haystack_len);
    if (!haystack) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // Get substring to search for
    const char *needle;
    size_t needle_len;
    needle = purc_variant_get_string_const_ex(argv[1], &needle_len);
    if (!needle) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // Get offset
    int64_t offset = 0;
    if (nr_args > 2) {
        if (!purc_variant_cast_to_longint(argv[2], &offset, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
        if (offset < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    // Get search length
    int64_t length = 0;
    if (nr_args > 3) {
        if (!purc_variant_cast_to_longint(argv[3], &length, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
        if (length < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    // Return 0 for empty substring
    if (needle_len == 0) {
        return purc_variant_make_ulongint(0);
    }

    // Calculate number of characters in string
    size_t haystack_chars;
    if (!pcutils_string_check_utf8(haystack, haystack_len, &haystack_chars,
                NULL)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    // Return 0 if offset exceeds string length
    if ((size_t)offset >= haystack_chars) {
        return purc_variant_make_ulongint(0);
    }

    // Position to offset
    const char *p = haystack;
    for (size_t i = 0; i < (size_t)offset; i++) {
        p = pcutils_utf8_next_char(p);
    }

    // Calculate actual search length
    size_t remaining_chars = haystack_chars - (size_t)offset;
    size_t search_chars;
    if (length > 0) {
        search_chars =
            (size_t)length < remaining_chars ? (size_t)length : remaining_chars;
    }
    else {
        search_chars = remaining_chars;
    }

    // Calculate end position of search range
    const char *end = p;
    for (size_t i = 0; i < search_chars; i++) {
        end = pcutils_utf8_next_char(end);
    }

    // Count substring occurrences
    size_t count = 0;
    while (p < end) {
        if (strncmp(p, needle, needle_len) == 0) {
            count++;
            p += needle_len;
        }
        else {
            p = pcutils_utf8_next_char(p);
        }
    }

    return purc_variant_make_ulongint(count);
    
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
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
$STR.split(
        <string $string: `The original string to split.`>
        [, <real $length = 1: `The length of one substring.`> ]
) array | false
 */
 // This function is written with aid from AI.
 static purc_variant_t
 split_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
         unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    int ec = PURC_ERROR_OK;

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    const char *str;
    size_t len;
    str = purc_variant_get_string_const_ex(argv[0], &len);
    if (str == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    // Get substring length
    int64_t substr_len;
    if (nr_args > 1 &&
            !purc_variant_cast_to_longint(argv[1], &substr_len, false)) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }
    else if (nr_args == 1)
        substr_len = 1; // Default length is 1

    // Check length validity
    if (substr_len <= 0) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto error;
    }

    // Create return array
    retv = purc_variant_make_array_0();
    if (retv == PURC_VARIANT_INVALID) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    // Handle empty string
    if (len == 0) {
        return retv;
    }

    // Split string by UTF-8 characters
    const char *curr = str;
    size_t remaining_chars = pcutils_string_utf8_chars(str, len);
    size_t remaining_bytes = len;

    while (remaining_chars > 0) {
        // Calculate number of characters for current substring
        size_t chunk_chars = (remaining_chars < (size_t)substr_len) ? 
                            remaining_chars : (size_t)substr_len;

        // Calculate byte length for these characters
        const char *chunk_end = curr;
        size_t actual_chars = 0;
        size_t chunk_bytes = 0;

        while (actual_chars < chunk_chars && chunk_bytes < remaining_bytes) {
            const char *next = pcutils_utf8_next_char(chunk_end);
            chunk_bytes = next - curr;
            chunk_end = next;
            actual_chars++;
        }

        // Create substring and append to return array
        purc_variant_t item = purc_variant_make_string_ex(curr, chunk_bytes, false);
        if (!purc_variant_array_append(retv, item)) {
            purc_variant_unref(item);
            goto error;
        }
        purc_variant_unref(item);

        curr = chunk_end;
        remaining_chars -= actual_chars;
        remaining_bytes -= chunk_bytes;
    }

    return retv;

error:
    if (retv) {
        purc_variant_unref(retv);
    }

    if (ec) {
        purc_set_error(ec);
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

/*
$STR.chunk_split(
        <string $string: `The original string to split.`>
        [, <real $length = 76: `The length of a chunk.`>
            [, <string $separator = '\r\n': `The seperator between two chunks.`>
            ]
        ]
) string
*/
static purc_variant_t
chunk_split_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_rwstream_t rwstream = NULL;
    int ec = PURC_ERROR_OK;

    // Check number of arguments
    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    // Get original string
    const char* str;
    size_t str_len;
    if ((str = purc_variant_get_string_const_ex(argv[0], &str_len)) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    // Get chunk length, default is 76
    int64_t chunk_len = 76;
    if (nr_args > 1) {
        if (!purc_variant_cast_to_longint(argv[1], &chunk_len, false)) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto error;
        }
        if (chunk_len <= 0) {
            ec = PURC_ERROR_INVALID_VALUE;
            goto error;
        }
    }

    // Get separator, default is \r\n
    const char* separator = "\r\n";
    size_t sep_len = 2;
    if (nr_args > 2) {
        if ((separator = purc_variant_get_string_const_ex(
                        argv[2], &sep_len)) == NULL) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto error;
        }
        if (sep_len == 0) {
            ec = PURC_ERROR_INVALID_VALUE;
            goto error; 
        }
    }

    // If string is empty, return empty string directly
    if (str_len == 0) {
        return purc_variant_make_string_static("", false);
    }

    // Create output stream
    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    // Traverse characters and write in chunks
    const char* p = str;
    size_t curr_chars = 0;  // Number of characters in current chunk

    while (*p) {
        const char* next = pcutils_utf8_next_char(p);
        size_t char_len = next - p;

        // Write current character
        if (purc_rwstream_write(rwstream, p, char_len) < (ssize_t)char_len) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto error;
        }

        curr_chars++;

        // If chunk length reached, add separator
        if (curr_chars == (size_t)chunk_len && *next) {
            if (purc_rwstream_write(rwstream, separator, sep_len) <
                    (ssize_t)sep_len) {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto error;
            }
            curr_chars = 0;
        }

        p = next;
    }

    // Write terminator
    if (purc_rwstream_write(rwstream, "", 1) < 1) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    // Get result
    size_t sz_buffer = 0;
    size_t sz_content = 0;
    char* content = NULL;
    content = purc_rwstream_get_mem_buffer_ex(rwstream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(rwstream);

    return purc_variant_make_string_reuse_buff(content, sz_buffer, false);

error:
    if (rwstream)
        purc_rwstream_destroy(rwstream);

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
$STR.pad(
    <string $string: `The original string.`>,
    <real $length: `The length of the new string after padded.`>,
    [, <string $pad_string = " ": `The string use to pad.`>,
        [, <'left | right | both' $pad_type = 'right': `The padding position.`> ]
    ]
) string | false
*/
enum {
    PAD_RIGHT,
    PAD_LEFT, 
    PAD_BOTH
};

static struct pcdvobjs_option_to_atom pad_type_skws[] = {
    { "right",  0,  PAD_RIGHT },
    { "left",   0,  PAD_LEFT },
    { "both",   0,  PAD_BOTH },
};

static purc_variant_t
pad_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int ec = PURC_ERROR_OK;
    purc_rwstream_t rwstream = NULL;

    // Check number of arguments
    if (nr_args < 2) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    // Get original string
    const char* str;
    size_t str_len;
    if ((str = purc_variant_get_string_const_ex(argv[0], &str_len)) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }
    // Get the number of characters in the original string
    size_t str_chars;
    if (!purc_variant_string_chars(argv[0], &str_chars)) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    // Get the target number of characters
    int64_t tmp_i64;
    if (!purc_variant_cast_to_longint(argv[1], &tmp_i64, false)) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    if (tmp_i64 < 0) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto error;
    }

    size_t target_chars = (size_t)tmp_i64;
    // If target length is less than or equal to original length,
    /// return original string
    if (target_chars <= str_chars) {
        return purc_variant_ref(argv[0]);
    }

    // Get padding string, default is space
    const char* pad_str = " ";
    size_t pad_str_len = 1;
    size_t pad_str_chars = 1;
    if (nr_args > 2) {
        if ((pad_str = purc_variant_get_string_const_ex(argv[2],
                        &pad_str_len)) == NULL) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto error;
        }
        if (pad_str_len == 0) {
            pad_str = " ";
            pad_str_len = 1;
            pad_str_chars = 1;
        }
        else {
            // Calculate number of characters in padding string
            if (!pcutils_string_check_utf8(pad_str, pad_str_len,
                        &pad_str_chars, NULL)) {
                ec = PURC_ERROR_INVALID_VALUE;
                goto error;
            }
            if (pad_str_chars == 0) {
                pad_str = " ";
                pad_str_len = 1;
                pad_str_chars = 1;
            }
        }
    }

    // Get padding type
    if (pad_type_skws[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(pad_type_skws); i++) {
            pad_type_skws[i].atom = purc_atom_from_static_string_ex(
                    ATOM_BUCKET_DVOBJ, pad_type_skws[i].option);
        }
    }

    int pad_type = pcdvobjs_parse_options(
            nr_args > 3 ? argv[3] : PURC_VARIANT_INVALID, pad_type_skws,
            PCA_TABLESIZE(pad_type_skws), NULL, 0, PAD_RIGHT, -1);
    if (pad_type == -1) {
        goto error;
    }

    // Create output stream
    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    // Calculate number of characters to pad
    size_t pad_chars = target_chars - str_chars;
    size_t left_pad = 0, right_pad = 0;

    // Calculate left and right padding based on padding type
    switch (pad_type) {
        case PAD_LEFT:
            left_pad = pad_chars;
            break;
        case PAD_RIGHT:
            right_pad = pad_chars;
            break;
        case PAD_BOTH:
            left_pad = pad_chars / 2;
            right_pad = pad_chars - left_pad;
            break;
    }

    // Write left padding characters
    const char *p = pad_str;
    for (size_t i = 0; i < left_pad; i += pad_str_chars) {
        if (purc_rwstream_write(rwstream, p, pad_str_len) <
                (ssize_t)pad_str_len) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto error;
        }
    }

    // Write original string
    ssize_t written = purc_rwstream_write(rwstream, str, str_len);
    if (written < 0 || (size_t)written < str_len) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    // Write right padding characters
    for (size_t i = 0; i < right_pad; i += pad_str_chars) {
        if (purc_rwstream_write(rwstream, p, pad_str_len) <
                (ssize_t)pad_str_len) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto error;
        }
    }

    // Write terminator
    if (purc_rwstream_write(rwstream, "", 1) < 1) {
        ec = PURC_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    // Get result
    size_t sz_buffer = 0;
    size_t sz_content = 0;
    char *content = NULL;
    content = purc_rwstream_get_mem_buffer_ex(rwstream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(rwstream);

    return purc_variant_make_string_reuse_buff(content, sz_buffer, false);

error:
    if (rwstream)
        purc_rwstream_destroy(rwstream);

    if (ec)
        purc_set_error(ec);

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
$STR.htmlentities(
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
htmlentities_getter(purc_variant_t root, size_t nr_args,
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
$STR.htmlentities(!
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
htmlentities_setter(purc_variant_t root, size_t nr_args,
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

static purc_variant_t substr_replace_helper(const char *org, size_t org_len,
        const char *rep, size_t rep_len, int64_t sub_off, int64_t sub_chars)
{
    struct pcutils_mystring mystr;
    pcutils_mystring_init(&mystr);

    const char *sub_start = org;
    const char *sub_end = org + org_len;

    // Handle sub_off
    if (sub_off < 0) {
        const char *end = sub_end;
        while (end > org && sub_off < 0) {
            end = utf8_prev_char(end);
            sub_off++;
        }

        sub_start = end;
    }
    else if (sub_off > 0) {
        const char *next = sub_start;
        while (*next && sub_off > 0) {
            next = pcutils_utf8_next_char(next);
            sub_off--;
        }

        sub_start = next;
    }

    // Handle sub_chars
    if (sub_chars < 0) {
        const char *end = sub_end;
        while (end > org && sub_chars < 0) {
            end = utf8_prev_char(end);
            sub_chars++;
        }

        sub_end = end;
    }
    else if (sub_chars > 0) {
        const char *next = sub_start;
        while (*next && sub_chars > 0) {
            next = pcutils_utf8_next_char(next);
            sub_chars--;
        }

        sub_end = next;
    }
    else {
        sub_end = sub_start;
    }

    if (sub_end < sub_start) {
        sub_end = sub_start;
    }

    if (sub_start > org && pcutils_mystring_append_mchar(&mystr,
                (unsigned char *)org, sub_start - org)) {
        goto failed;
    }

    // Insert the replacement
    if (rep_len > 0 && pcutils_mystring_append_mchar(&mystr,
                (unsigned char *)rep, rep_len)) {
        goto failed;
    }

    // append the left bytes
    if (*sub_end) {
        if (pcutils_mystring_append_mchar(&mystr, (unsigned char *)sub_end,
                    strlen(sub_end))) {
            goto failed;
        }
    }

    pcutils_mystring_done(&mystr);
    return purc_variant_make_string_reuse_buff(mystr.buff,
            mystr.sz_space, false);

failed:
    pcutils_mystring_free(&mystr);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
substr_replace_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int ec = PURC_ERROR_OK;
    purc_variant_t result = PURC_VARIANT_INVALID;

    if (nr_args < 3) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto failed;
    }

    size_t ctnr_size = 0;
    if (purc_variant_is_string(argv[0])) {
        const char* org_str;
        size_t org_len;
        org_str = purc_variant_get_string_const_ex(argv[0], &org_len);

        const char* rep_str;
        size_t rep_len;
        rep_str = purc_variant_get_string_const_ex(argv[1], &rep_len);

        int64_t offset;
        if (!purc_variant_cast_to_longint(argv[2], &offset, false)) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto failed;
        }

        size_t sz;
        purc_variant_string_chars(argv[0], &sz);
        int64_t nchars = sz;
        if (nr_args > 3 &&
                !purc_variant_cast_to_longint(argv[3], &nchars, false)) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto failed;
        }

        result = substr_replace_helper(org_str, org_len, rep_str, rep_len,
            offset, nchars);
        if (result == PURC_VARIANT_INVALID) {
            ec = PURC_ERROR_OUT_OF_MEMORY;
            goto failed;
        }
    }
    else if (purc_variant_linear_container_size(argv[0], &ctnr_size)) {
        size_t sz_rep = 0;
        const char *scalar_rep_str;
        size_t scalar_rep_len;
        if (purc_variant_linear_container_size(argv[1], &sz_rep)) {
            if (sz_rep == 0) {
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }
        }
        else if (purc_variant_is_string(argv[1])) {
            scalar_rep_str = purc_variant_get_string_const_ex(argv[1],
                    &scalar_rep_len);
            if (scalar_rep_str == NULL) {
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }
        }
        else {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto failed;
        }

        size_t sz_off = 0;
        int64_t scalar_off;
        if (purc_variant_linear_container_size(argv[2], &sz_off)) {
            if (sz_off == 0) {
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }
        }
        else if (!purc_variant_cast_to_longint(argv[2], &scalar_off, false)) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto failed;
        }

        size_t sz_len = 0;
        int64_t scalar_len = 0;
        if (nr_args > 3) {
            if (purc_variant_linear_container_size(argv[3], &sz_len) &&
                    sz_len == 0) {
                ec = PURC_ERROR_INVALID_VALUE;
                goto failed;
            }
            else if (!purc_variant_cast_to_longint(argv[2], &scalar_len, false)) {
                ec = PURC_ERROR_WRONG_DATA_TYPE;
                goto failed;
            }
        }

        result = purc_variant_make_array_0();
        if (result == PURC_VARIANT_INVALID) {
            goto failed;
        }

        for (size_t i = 0; i < ctnr_size; i++) {
            purc_variant_t tmp;
            tmp = purc_variant_linear_container_get(argv[0], i);

            const char* org_str;
            size_t org_len;
            org_str = purc_variant_get_string_const_ex(tmp, &org_len);
            if (org_str == NULL) {
                ec = PURC_ERROR_WRONG_DATA_TYPE;
                goto failed;
            }

            size_t sz;
            purc_variant_string_chars(tmp, &sz);
            int64_t nchars = sz;

            const char* rep_str;
            size_t rep_len;
            if (sz_rep > 0) {
                tmp = purc_variant_linear_container_get(argv[1],
                        i < sz_rep ? i : (sz_rep - 1));

                rep_str = purc_variant_get_string_const_ex(tmp, &rep_len);
                if (rep_str == NULL) {
                    ec = PURC_ERROR_WRONG_DATA_TYPE;
                    goto failed;
                }
            }
            else {
                rep_str = scalar_rep_str;
                rep_len = scalar_rep_len;
            }

            int64_t offset;
            if (sz_off > 0) {
                tmp = purc_variant_linear_container_get(argv[2],
                        i < sz_off ? i : (sz_off - 1));
                if (!purc_variant_cast_to_longint(tmp, &offset, false)) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto failed;
                }
            }
            else {
                offset = scalar_off;
            }

            if (sz_len > 0) {
                /* argv[3] must be valid */
                tmp = purc_variant_linear_container_get(argv[3],
                        i < sz_len ? i : (sz_len - 1));
                if (!purc_variant_cast_to_longint(tmp, &nchars, false)) {
                    ec = PURC_ERROR_INVALID_VALUE;
                    goto failed;
                }
            }
            else if (nr_args > 3) {
                nchars = scalar_len;
            }
            else {
                nchars = org_len;
            }

            purc_variant_t replaced = substr_replace_helper(
                    org_str, org_len, rep_str, rep_len, offset, nchars);
            if (replaced) {
                if (!purc_variant_array_append(result, replaced)) {
                    purc_variant_unref(replaced);
                    goto failed;
                }
                purc_variant_unref(replaced);
            }
            else {
                ec = PURC_ERROR_OUT_OF_MEMORY;
                goto failed;
            }
        }
    }
    else {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    return result;

failed:
    if (result)
        purc_variant_unref(result);

    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_string_new(void)
{
    static struct purc_dvobj_method method [] = {
        { "contains",   contains_getter,    NULL },
        { "starts_with",starts_with_getter, NULL },
        { "ends_with",  ends_with_getter,   NULL },
        { "explode",    explode_getter,     NULL },
        { "implode",    implode_getter,     NULL },
        { "shuffle",    shuffle_getter,     NULL },
        { "replace",    replace_getter,     NULL },
        { "format_c",   printf_getter,      NULL },// for backward compability
        { "printf",     printf_getter,      NULL },
        { "scanf",      scanf_getter,       NULL },
        { "printp",     printp_getter,      NULL },
        { "scanp",      scanp_getter,       NULL },
        { "join",       join_getter,        NULL },
        { "nr_bytes",   nr_bytes_getter,    NULL },
        { "nr_chars",   nr_chars_getter,    NULL },
        { "tolower",    tolower_getter,     NULL },
        { "toupper",    toupper_getter,     NULL },
        { "substr",     substr_getter,      NULL },
        { "substr_compare", substr_compare_getter,  NULL },
        { "substr_count",   substr_count_getter,    NULL },
        { "substr_replace", substr_replace_getter,  NULL },
        { "strstr",     strstr_getter,      NULL },
        { "strpos",     strpos_getter,      NULL },
        { "strpbrk",    strpbrk_getter,     NULL },
        { "split",      split_getter,       NULL },
        { "chunk_split",    chunk_split_getter,     NULL },
        { "trim",       trim_getter,        NULL },
        { "pad",        pad_getter,         NULL },
        { "repeat",     repeat_getter,      NULL },
        { "reverse",    reverse_getter,     NULL },
        { "tokenize",   tokenize_getter,    NULL },
        { "translate",  translate_getter,   NULL },
        { "htmlentities", htmlentities_getter,  htmlentities_setter },
        { "rot13",      rot13_getter,       NULL },
        { "count_chars",count_chars_getter, NULL },
        { "count_bytes",count_bytes_getter, NULL },
        { "codepoints", codepoints_getter,  NULL },
    };

    return purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
}
