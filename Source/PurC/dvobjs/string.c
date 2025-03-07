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
#include "purc-variant.h"
#include "helper.h"

#include "purc-dvobjs.h"

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

    purc_rwstream_t rwstream = purc_rwstream_new_buffer(32, STREAM_SIZE);
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

static purc_variant_t
replace_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 3)) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[2] == PURC_VARIANT_INVALID) ||
            (!purc_variant_is_string (argv[2]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    size_t len_delim = purc_variant_string_size (argv[0]) - 1;
    if (len_delim == 0) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    len_delim = purc_variant_string_size (argv[1]) - 1;
    if (len_delim == 0) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    const char *source = purc_variant_get_string_const (argv[0]);
    const char *delim = purc_variant_get_string_const (argv[1]);
    const char *replace = purc_variant_get_string_const (argv[2]);
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);

    size_t len_replace = purc_variant_string_size (argv[2]) - 1;
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

    rwstream = purc_rwstream_new_buffer(32, STREAM_SIZE);

    size_t start = 0, i = 0, j = 0;
    while (*(format + i) != 0x00) {
        if (*(format + i) == '%') {
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

            char fc = format[i + 1];
            if (strchr("diouxXeEfFgGaAs", fc) && item == PURC_VARIANT_INVALID) {
                purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
                goto error;
            }

            char buff[64];
            int64_t i64 = 0;
            uint64_t u64 = 0;
            double number = 0;
            const char *string = NULL;
            size_t len;

            switch (fc) {
            case 0x00:
                break;

            case '%':
                purc_rwstream_write(rwstream, format + start, i - start);
                purc_rwstream_write(rwstream, "%", 1);
                i++;
                start = i + 1;
                break;

            case 'i':
            case 'd':
                purc_rwstream_write(rwstream, format + start, i - start);
                if (!purc_variant_cast_to_longint(item, &i64, true)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                len = snprintf(buff, sizeof(buff), "%lld", (long long int)i64);
                purc_rwstream_write(rwstream, buff, len);
                i++;
                j++;
                start = i + 1;
                break;

            case 'o':
            case 'u':
                purc_rwstream_write(rwstream, format + start, i - start);
                if (!purc_variant_cast_to_ulongint(item, &u64, false)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                len = snprintf(buff, sizeof(buff), fc == 'o' ? "%llo" : "%llu",
                        (long long unsigned)u64);
                purc_rwstream_write(rwstream, buff, len);
                i++;
                j++;
                start = i + 1;
                break;

            case 'x':
            case 'X':
                purc_rwstream_write(rwstream, format + start, i - start);
                if (!purc_variant_cast_to_ulongint(item, &u64, false)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                len = snprintf(buff, sizeof(buff), fc == 'x' ? "%llx" : "%llX",
                        (long long unsigned)u64);
                purc_rwstream_write(rwstream, buff, len);
                i++;
                j++;
                start = i + 1;
                break;

            case 'e':
            case 'E':
                purc_rwstream_write(rwstream, format + start, i - start);
                if (!purc_variant_cast_to_number(item, &number, false)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                len = snprintf(buff, sizeof(buff), fc == 'e' ? "%e" : "%E",
                        number);
                purc_rwstream_write(rwstream, buff, len);
                i++;
                j++;
                start = i + 1;
                break;

            case 'f':
            case 'F':
                purc_rwstream_write(rwstream, format + start, i - start);
                if (!purc_variant_cast_to_number(item, &number, false)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                len = snprintf(buff, sizeof(buff), fc == 'f' ? "%f" : "%F",
                        number);
                purc_rwstream_write(rwstream, buff, len);
                i++;
                j++;
                start = i + 1;
                break;

            case 'g':
            case 'G':
                purc_rwstream_write(rwstream, format + start, i - start);
                if (!purc_variant_cast_to_number(item, &number, false)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                len = snprintf(buff, sizeof(buff), fc == 'g' ? "%g" : "%G",
                        number);
                purc_rwstream_write(rwstream, buff, len);
                i++;
                j++;
                start = i + 1;
                break;

            case 'a':
            case 'A':
                purc_rwstream_write(rwstream, format + start, i - start);
                if (!purc_variant_cast_to_number(item, &number, false)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                len = snprintf(buff, sizeof(buff), fc == 'a' ? "%a" : "%A",
                        number);
                purc_rwstream_write(rwstream, buff, len);
                i++;
                j++;
                start = i + 1;
                break;

            case 's':
                purc_rwstream_write (rwstream, format + start, i - start);
                string = purc_variant_get_string_const_ex(item, &len);
                if (string == NULL) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto error;
                }
                purc_rwstream_write(rwstream, string, len);
                i++;
                j++;
                start = i + 1;
                break;
            }
        }
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

            serialize = purc_rwstream_new_buffer (32, STREAM_SIZE);
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
$STR.trim(
        <string $string: `The orignal string to trim.`>
        [, <string $characters = " \n\r\t\v\f": `The characters to trim from the original string.`>
            [, < 'left | right | both' $position  = 'both': `The trimming position.`> ]
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

static bool is_all_ascii(const char *str)
{
    while (*str) {
        if (*str & 0x80)
            return false;
        str++;
    }

    return true;
}

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
            char utf8[10];
            const char *p = pcutils_utf8_next_char(left);
            size_t utf8_len = p - left;
            assert(utf8_len < sizeof(utf8));

            strncpy(utf8, left, utf8_len);
            PC_DEBUG("characters: '%s', utf8: '%s', strstr:  %s\n",
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
            char utf8[10];
            const char *p = utf8_prev_char(right);
            size_t utf8_len = right - p;
            assert(utf8_len < sizeof(utf8));

            strncpy(utf8, p, utf8_len);
            PC_DEBUG("Right string: %s, Previouse char: %s\n", right, utf8);
            PC_DEBUG("characters: '%s', utf8: '%s', strstr:  %s\n",
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
    size_t len, chars_len;

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
        { "replace",    replace_getter,     NULL },
        { "format_c",   format_c_getter,    NULL },
        { "format_p",   format_p_getter,    NULL },
        { "substr",     substr_getter,      NULL },
        { "strstr",     strstr_getter,      NULL },
        { "trim",       trim_getter,        NULL },
    };

    return purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
}
