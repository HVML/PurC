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
#include "purc-variant.h"
#include "helper.h"

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

    return purc_variant_make_ulongint(nr_bytes);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

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
            nr_max = limit;
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

        if (limit == 0 && *p) {
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
            nr_max = limit;
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

    if (array_size == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
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
format_c_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    const char *format = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);

    if ((argv == NULL) || (nr_args == 0)) {
        purc_rwstream_destroy (rwstream);
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        purc_rwstream_destroy (rwstream);
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    else
        format = purc_variant_get_string_const (argv[0]);

    char buff[32];
    int start = 0, len;
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
                len = snprintf(buff, sizeof(buff), "%lld", (long long int)i64);
                purc_rwstream_write (rwstream, buff, len);
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
                len = snprintf(buff, sizeof(buff), "%llo", (long long unsigned)u64);
                purc_rwstream_write(rwstream, buff, len);
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
                len = snprintf(buff, sizeof(buff), "%llu", (long long unsigned)u64);
                purc_rwstream_write (rwstream, buff, len);
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
                len = snprintf(buff, sizeof(buff), "%llx", (long long unsigned)u64);
                purc_rwstream_write (rwstream, buff, len);
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
                len = snprintf(buff, sizeof(buff), "%lf", number);
                purc_rwstream_write(rwstream, buff, len);
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
            purc_set_error (PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
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
    UNUSED_PARAM(call_flags);

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

purc_variant_t purc_dvobj_string_new(void)
{
    static struct purc_dvobj_method method [] = {
        { "nr_bytes",   nr_bytes_getter,    NULL },
        { "nr_chars",   nr_chars_getter,    NULL },
        { "contains",   contains_getter,    NULL },
        { "starts_with",starts_with_getter,   NULL },
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
    };

    return purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
}
