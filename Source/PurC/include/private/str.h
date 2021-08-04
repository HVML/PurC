/**
 * @file str.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for string operation.
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

#ifndef PCHTML_STR_H
#define PCHTML_STR_H

#include "config.h"

#include "html/base.h"
#include "private/mraw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define pchtml_str_get(str, attr) str->attr
#define pchtml_str_set(str, attr) pchtml_str_get(str, attr)
#define pchtml_str_len(str) pchtml_str_get(str, length)

#define pchtml_str_check_size_arg_m(str, size, mraw, plus_len, return_fail)    \
    do {                                                                       \
        void *tmp;                                                             \
                                                                               \
        if (str->length > (SIZE_MAX - (plus_len)))                             \
            return (return_fail);                                              \
                                                                               \
        if ((str->length + (plus_len)) > (size)) {                             \
            tmp = pchtml_mraw_realloc(mraw, str->data,                         \
                                      (str->length + plus_len));               \
                                                                               \
            if (tmp == NULL) {                                                 \
                return (return_fail);                                          \
            }                                                                  \
                                                                               \
            str->data = (unsigned char *) tmp;                                    \
        }                                                                      \
    }                                                                          \
    while (0)


typedef struct {
    unsigned char *data;
    size_t     length;
}
pchtml_str_t;


pchtml_str_t *
pchtml_str_create(void) WTF_INTERNAL;

unsigned char *
pchtml_str_init(pchtml_str_t *str, pchtml_mraw_t *mraw, 
                size_t size) WTF_INTERNAL;

void
pchtml_str_clean(pchtml_str_t *str) WTF_INTERNAL;

void
pchtml_str_clean_all(pchtml_str_t *str) WTF_INTERNAL;

pchtml_str_t *
pchtml_str_destroy(pchtml_str_t *str, pchtml_mraw_t *mraw, 
                bool destroy_obj) WTF_INTERNAL;


unsigned char *
pchtml_str_realloc(pchtml_str_t *str, pchtml_mraw_t *mraw, 
                size_t new_size) WTF_INTERNAL;

unsigned char *
pchtml_str_check_size(pchtml_str_t *str, pchtml_mraw_t *mraw, 
                size_t plus_len) WTF_INTERNAL;

/* Append */
unsigned char *
pchtml_str_append(pchtml_str_t *str, pchtml_mraw_t *mraw,
                const unsigned char *data, size_t length) WTF_INTERNAL;

unsigned char *
pchtml_str_append_before(pchtml_str_t *str, pchtml_mraw_t *mraw,
                const unsigned char *buff, size_t length) WTF_INTERNAL;

unsigned char *
pchtml_str_append_one(pchtml_str_t *str, pchtml_mraw_t *mraw,
                const unsigned char data) WTF_INTERNAL;

unsigned char *
pchtml_str_append_lowercase(pchtml_str_t *str, pchtml_mraw_t *mraw,
                const unsigned char *data, size_t length) WTF_INTERNAL;

unsigned char *
pchtml_str_append_with_rep_null_chars(pchtml_str_t *str, pchtml_mraw_t *mraw,
                const unsigned char *buff, size_t length) WTF_INTERNAL;

/* Other functions */
unsigned char *
pchtml_str_copy(pchtml_str_t *dest, const pchtml_str_t *target,
                pchtml_mraw_t *mraw) WTF_INTERNAL;

void
pchtml_str_stay_only_whitespace(pchtml_str_t *target) WTF_INTERNAL;

void
pchtml_str_strip_collapse_whitespace(pchtml_str_t *target) WTF_INTERNAL;

size_t
pchtml_str_crop_whitespace_from_begin(pchtml_str_t *target) WTF_INTERNAL;

size_t
pchtml_str_whitespace_from_begin(pchtml_str_t *target) WTF_INTERNAL;

size_t
pchtml_str_whitespace_from_end(pchtml_str_t *target) WTF_INTERNAL;


/* Data utils */
/*
 * [in] first: must be null-terminated
 * [in] sec: no matter what data
 * [in] sec_size: size of the 'sec' buffer
 *
 * Function compare two unsigned char data until find '\0' in first arg.
 * Successfully if the function returned a pointer starting with '\0',
 * otherwise, if the data of the second buffer is insufficient function returned
 * position in first buffer.
 * If function returns NULL, the data are not equal.
 */
const unsigned char *
pchtml_str_data_ncasecmp_first(const unsigned char *first, const unsigned char *sec,
                size_t sec_size) WTF_INTERNAL;
bool
pchtml_str_data_ncasecmp_end(const unsigned char *first, const unsigned char *sec,
                size_t size) WTF_INTERNAL;
bool
pchtml_str_data_ncasecmp_contain(const unsigned char *where, size_t where_size,
                const unsigned char *what, size_t what_size) WTF_INTERNAL;
bool
pchtml_str_data_ncasecmp(const unsigned char *first, const unsigned char *sec,
                size_t size) WTF_INTERNAL;
bool
pchtml_str_data_nlocmp_right(const unsigned char *first, const unsigned char *sec,
                size_t size) WTF_INTERNAL;
bool
pchtml_str_data_nupcmp_right(const unsigned char *first, const unsigned char *sec,
                size_t size) WTF_INTERNAL;
bool
pchtml_str_data_casecmp(const unsigned char *first, 
                const unsigned char *sec) WTF_INTERNAL;

bool
pchtml_str_data_ncmp_end(const unsigned char *first, const unsigned char *sec,
                size_t size) WTF_INTERNAL;
bool
pchtml_str_data_ncmp_contain(const unsigned char *where, size_t where_size,
                const unsigned char *what, size_t what_size) WTF_INTERNAL;
bool
pchtml_str_data_ncmp(const unsigned char *first, const unsigned char *sec,
                size_t size) WTF_INTERNAL;

bool
pchtml_str_data_cmp(const unsigned char *first, 
                const unsigned char *sec) WTF_INTERNAL;

bool
pchtml_str_data_cmp_ws(const unsigned char *first, 
                const unsigned char *sec) WTF_INTERNAL;

void
pchtml_str_data_to_lowercase(unsigned char *to, const unsigned char *from, 
                size_t len) WTF_INTERNAL;

void
pchtml_str_data_to_uppercase(unsigned char *to, const unsigned char *from, 
                size_t len) WTF_INTERNAL;

const unsigned char *
pchtml_str_data_find_lowercase(const unsigned char *data, 
                size_t len) WTF_INTERNAL;

const unsigned char *
pchtml_str_data_find_uppercase(const unsigned char *data, 
                size_t len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline unsigned char *
pchtml_str_data(pchtml_str_t *str)
{
    return str->data;
}

static inline size_t
pchtml_str_length(pchtml_str_t *str)
{
    return str->length;
}

static inline size_t
pchtml_str_size(pchtml_str_t *str)
{
    return pchtml_mraw_data_size(str->data);
}

static inline void
pchtml_str_data_set(pchtml_str_t *str, unsigned char *data)
{
    str->data = data;
}

static inline unsigned char *
pchtml_str_length_set(pchtml_str_t *str, pchtml_mraw_t *mraw, size_t length)
{
    if (length >= pchtml_str_size(str)) {
        unsigned char *tmp;

        tmp = pchtml_str_realloc(str, mraw, length + 1);
        if (tmp == NULL) {
            return NULL;
        }
    }

    str->length = length;
    str->data[length] = 0x00;

    return str->data;
}

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_STR_H */
