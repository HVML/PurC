/**
 * @file str.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of string operation.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "config.h"
#include "private/utils.h"

#include "private/str.h"

#define PCHTML_STR_RES_ANSI_REPLACEMENT_CHARACTER
#define PCHTML_STR_RES_MAP_LOWERCASE
#define PCHTML_STR_RES_MAP_UPPERCASE
#include "str_res.h"


pcutils_str_t *
pcutils_str_create(void)
{
    return pcutils_calloc(1, sizeof(pcutils_str_t));
}

unsigned char *
pcutils_str_init(pcutils_str_t *str, pcutils_mraw_t *mraw, size_t size)
{
    if (str == NULL) {
        return NULL;
    }

    str->data = pcutils_mraw_alloc(mraw, (size + 1));
    str->length = 0;

    if (str->data != NULL) {
        *str->data = '\0';
    }

    return str->data;
}

void
pcutils_str_clean(pcutils_str_t *str)
{
    str->length = 0;
}

void
pcutils_str_clean_all(pcutils_str_t *str)
{
    memset(str, 0, sizeof(pcutils_str_t));
}

pcutils_str_t *
pcutils_str_destroy(pcutils_str_t *str, pcutils_mraw_t *mraw, bool destroy_obj)
{
    if (str == NULL) {
        return NULL;
    }

    if (str->data != NULL) {
        str->data = pcutils_mraw_free(mraw, str->data);
    }

    if (destroy_obj) {
        return pcutils_free(str);
    }

    return str;
}

unsigned char *
pcutils_str_realloc(pcutils_str_t *str, pcutils_mraw_t *mraw, size_t new_size)
{
    unsigned char *tmp = pcutils_mraw_realloc(mraw, str->data, new_size);
    if (tmp == NULL) {
        return NULL;
    }

    str->data = tmp;

    return tmp;
}

unsigned char *
pcutils_str_check_size(pcutils_str_t *str, pcutils_mraw_t *mraw, size_t plus_len)
{
    unsigned char *tmp;

    if (str->length > (SIZE_MAX - plus_len)) {
        return NULL;
    }

    if ((str->length + plus_len) <= pcutils_str_size(str)) {
        return str->data;
    }

    tmp = pcutils_mraw_realloc(mraw, str->data, (str->length + plus_len));
    if (tmp == NULL) {
        return NULL;
    }

    str->data = tmp;

    return tmp;
}

/* Append API */
unsigned char *
pcutils_str_append(pcutils_str_t *str, pcutils_mraw_t *mraw,
                  const unsigned char *buff, size_t length)
{
    unsigned char *data_begin;

    pcutils_str_check_size_arg_m(str, pcutils_str_size(str),
                                mraw, (length + 1), NULL);

    data_begin = &str->data[str->length];
    memcpy(data_begin, buff, sizeof(unsigned char) * length);

    str->length += length;
    str->data[str->length] = '\0';

    return data_begin;
}

unsigned char *
pcutils_str_append_before(pcutils_str_t *str, pcutils_mraw_t *mraw,
                         const unsigned char *buff, size_t length)
{
    unsigned char *data_begin;

    pcutils_str_check_size_arg_m(str, pcutils_str_size(str),
                                mraw, (length + 1), NULL);

    data_begin = &str->data[str->length];

    memmove(&str->data[length], str->data, sizeof(unsigned char) * str->length);
    memcpy(str->data, buff, sizeof(unsigned char) * length);

    str->length += length;
    str->data[str->length] = '\0';

    return data_begin;
}

unsigned char *
pcutils_str_append_one(pcutils_str_t *str, pcutils_mraw_t *mraw,
                      const unsigned char data)
{
    pcutils_str_check_size_arg_m(str, pcutils_str_size(str), mraw, 2, NULL);

    str->data[str->length] = data;

    str->length += 1;
    str->data[str->length] = '\0';

    return &str->data[(str->length - 1)];
}

unsigned char *
pcutils_str_append_lowercase(pcutils_str_t *str, pcutils_mraw_t *mraw,
                            const unsigned char *data, size_t length)
{
    size_t i;
    unsigned char *data_begin;

    pcutils_str_check_size_arg_m(str, pcutils_str_size(str),
                                mraw, (length + 1), NULL);

    data_begin = &str->data[str->length];

    for (i = 0; i < length; i++) {
        data_begin[i] = pcutils_str_res_map_lowercase[ data[i] ];
    }

    data_begin[i] = '\0';
    str->length += length;

    return data_begin;
}

unsigned char *
pcutils_str_append_with_rep_null_chars(pcutils_str_t *str, pcutils_mraw_t *mraw,
                                      const unsigned char *buff, size_t length)
{
    const unsigned char *pos, *res, *end;
    size_t current_len = str->length;

    pcutils_str_check_size_arg_m(str, pcutils_str_size(str),
                                mraw, (length + 1), NULL);
    end = buff + length;

    while (buff != end) {
        pos = memchr(buff, '\0', sizeof(unsigned char) * (end - buff));
        if (pos == NULL) {
            break;
        }

        res = pcutils_str_append(str, mraw, buff, (pos - buff));
        if (res == NULL) {
            return NULL;
        }

        res = pcutils_str_append(str, mraw,
                         pcutils_str_res_ansi_replacement_character,
                         sizeof(pcutils_str_res_ansi_replacement_character) - 1);
        if (res == NULL) {
            return NULL;
        }

        buff = pos + 1;
    }

    if (buff != end) {
        res = pcutils_str_append(str, mraw, buff, (end - buff));
        if (res == NULL) {
            return NULL;
        }
    }

    return &str->data[current_len];
}

unsigned char *
pcutils_str_copy(pcutils_str_t *dest, const pcutils_str_t *target,
                pcutils_mraw_t *mraw)
{
    if (target->data == NULL) {
        return NULL;
    }

    if (dest->data == NULL) {
        pcutils_str_init(dest, mraw, target->length);

        if (dest->data == NULL) {
            return NULL;
        }
    }

    return pcutils_str_append(dest, mraw, target->data, target->length);
}

void
pcutils_str_stay_only_whitespace(pcutils_str_t *target)
{
    size_t i, pos = 0;
    unsigned char *data = target->data;

    for (i = 0; i < target->length; i++) {
        if (pcutils_html_whitespace(data[i], ==, ||)) {
            data[pos] = data[i];
            pos++;
        }
    }

    target->length = pos;
}

void
pcutils_str_strip_collapse_whitespace(pcutils_str_t *target)
{
    size_t i, offset, ws_i;
    unsigned char *data = target->data;

    if (target->length == 0) {
        return;
    }

    if (pcutils_html_whitespace(*data, ==, ||)) {
        *data = 0x20;
    }

    for (i = 0, offset = 0, ws_i = 0; i < target->length; i++)
    {
        if (pcutils_html_whitespace(data[i], ==, ||)) {
            if (data[ws_i] != 0x20) {
                data[offset] = 0x20;

                ws_i = offset;
                offset++;
            }
        }
        else {
            if (data[ws_i] == 0x20) {
                ws_i = offset;
            }

            data[offset] = data[i];
            offset++;
        }
    }

    if (offset != i) {
        if (offset != 0) {
            if (data[offset - 1] == 0x20) {
                offset--;
            }
        }

        data[offset] = 0x00;
        target->length = offset;
    }
}

size_t
pcutils_str_crop_whitespace_from_begin(pcutils_str_t *target)
{
    size_t i;
    unsigned char *data = target->data;

    for (i = 0; i < target->length; i++) {
        if (pcutils_html_whitespace(data[i], !=, &&)) {
            break;
        }
    }

    if (i != 0 && i != target->length) {
        memmove(target->data, &target->data[i], (target->length - i));
    }

    target->length -= i;
    return i;
}

size_t
pcutils_str_whitespace_from_begin(pcutils_str_t *target)
{
    size_t i;
    unsigned char *data = target->data;

    for (i = 0; i < target->length; i++) {
        if (pcutils_html_whitespace(data[i], !=, &&)) {
            break;
        }
    }

    return i;
}

size_t
pcutils_str_whitespace_from_end(pcutils_str_t *target)
{
    size_t i = target->length;
    unsigned char *data = target->data;

    while (i) {
        i--;

        if (pcutils_html_whitespace(data[i], !=, &&)) {
            return target->length - (i + 1);
        }
    }

    return 0;
}

/*
 * Data utils
 * TODO: All functions need optimization.
 */
const unsigned char *
pcutils_str_data_ncasecmp_first(const unsigned char *first, const unsigned char *sec,
                               size_t sec_size)
{
    size_t i;

    for (i = 0; i < sec_size; i++) {
        if (first[i] == '\0') {
            return &first[i];
        }

        if (pcutils_str_res_map_lowercase[ first[i] ]
            != pcutils_str_res_map_lowercase[ sec[i] ])
        {
            return NULL;
        }
    }

    return &first[i];
}

bool
pcutils_str_data_ncasecmp_end(const unsigned char *first, const unsigned char *sec,
                             size_t size)
{
    while (size != 0) {
        size--;

        if (pcutils_str_res_map_lowercase[ first[size] ]
            != pcutils_str_res_map_lowercase[ sec[size] ])
        {
            return false;
        }
    }

    return true;
}

bool
pcutils_str_data_ncasecmp_contain(const unsigned char *where, size_t where_size,
                                 const unsigned char *what, size_t what_size)
{
    for (size_t i = 0; what_size <= (where_size - i); i++) {
        if(pcutils_str_data_ncasecmp(&where[i], what, what_size)) {
            return true;
        }
    }

    return false;
}

bool
pcutils_str_data_ncasecmp(const unsigned char *first, const unsigned char *sec,
                         size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (pcutils_str_res_map_lowercase[ first[i] ]
            != pcutils_str_res_map_lowercase[ sec[i] ])
        {
            return false;
        }
    }

    return true;
}

bool
pcutils_str_data_nlocmp_right(const unsigned char *first, const unsigned char *sec,
                             size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (first[i] != pcutils_str_res_map_lowercase[ sec[i] ]) {
            return false;
        }
    }

    return true;
}

bool
pcutils_str_data_nupcmp_right(const unsigned char *first, const unsigned char *sec,
                             size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (first[i] != pcutils_str_res_map_uppercase[ sec[i] ]) {
            return false;
        }
    }

    return true;
}

bool
pcutils_str_data_casecmp(const unsigned char *first, const unsigned char *sec)
{
    for (;;) {
        if (pcutils_str_res_map_lowercase[*first]
            != pcutils_str_res_map_lowercase[*sec])
        {
            return false;
        }

        if (*first == '\0') {
            return true;
        }

        first++;
        sec++;
    }
}

bool
pcutils_str_data_ncmp_end(const unsigned char *first, const unsigned char *sec,
                         size_t size)
{
    while (size != 0) {
        size--;

        if (first[size] != sec[size]) {
            return false;
        }
    }

    return true;
}

bool
pcutils_str_data_ncmp_contain(const unsigned char *where, size_t where_size,
                             const unsigned char *what, size_t what_size)
{
    for (size_t i = 0; what_size <= (where_size - i); i++) {
        if(memcmp(&where[i], what, sizeof(unsigned char) * what_size) == 0) {
            return true;
        }
    }

    return false;
}

bool
pcutils_str_data_ncmp(const unsigned char *first, const unsigned char *sec,
                     size_t size)
{
    return memcmp(first, sec, sizeof(unsigned char) * size) == 0;
}

bool
pcutils_str_data_cmp(const unsigned char *first, const unsigned char *sec)
{
    for (;;) {
        if (*first != *sec) {
            return false;
        }

        if (*first == '\0') {
            return true;
        }

        first++;
        sec++;
    }
}

bool
pcutils_str_data_cmp_ws(const unsigned char *first, const unsigned char *sec)
{
    for (;;) {
        if (*first != *sec) {
            return false;
        }

        if (pcutils_html_whitespace(*first, ==, ||) || *first == '\0') {
            return true;
        }

        first++;
        sec++;
    }
}

void
pcutils_str_data_to_lowercase(unsigned char *to, const unsigned char *from, size_t len)
{
    while (len) {
        len--;

        to[len] = pcutils_str_res_map_lowercase[ from[len] ];
    }
}

void
pcutils_str_data_to_uppercase(unsigned char *to, const unsigned char *from, size_t len)
{
    while (len) {
        len--;

        to[len] = pcutils_str_res_map_uppercase[ from[len] ];
    }
}

unsigned char
pcutils_unsigned_char_to_uppercase(unsigned char from)
{
    return pcutils_str_res_map_uppercase[from];
}

unsigned char
pcutils_unsigned_char_to_lowercase(unsigned char from)
{
    return pcutils_str_res_map_lowercase[from];
}

const unsigned char *
pcutils_str_data_find_lowercase(const unsigned char *data, size_t len)
{
    while (len) {
        len--;

        if (data[len] == pcutils_str_res_map_lowercase[ data[len] ]) {
            return &data[len];
        }
    }

    return NULL;
}

const unsigned char *
pcutils_str_data_find_uppercase(const unsigned char *data, size_t len)
{
    while (len) {
        len--;

        if (data[len] == pcutils_str_res_map_uppercase[ data[len] ]) {
            return &data[len];
        }
    }

    return NULL;
}

