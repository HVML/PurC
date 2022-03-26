/**
 * @file stringbuilder.h
 * @author Xu Xiaohong
 * @date 2021/11/06
 * @brief The internal interfaces for stringbuilder.
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
 */

#ifndef PURC_PRIVATE_STRINGBUILDER_H
#define PURC_PRIVATE_STRINGBUILDER_H

#include "config.h"

#include "purc-macros.h"
#include "private/list.h"

#include <stdarg.h>
#include <string.h>

struct pcutils_string {
    char             buf[128];
    size_t           chunk_size;

    char            *abuf;
    char            *end;

    char            *curr;
};

struct pcutils_stringbuilder
{
    struct list_head           list;

    struct pcutils_buf        *curr;

    size_t                     total;

    size_t                     chunk;
    unsigned int               oom:1;
};

PCA_EXTERN_C_BEGIN

static inline void
pcutils_stringbuilder_init(struct pcutils_stringbuilder *sb, size_t chunk)
{
    memset(sb, 0, sizeof(*sb));
    if (chunk == (size_t)-1) {
        chunk = 64;
    }
    sb->chunk = chunk;
    INIT_LIST_HEAD(&sb->list);
}

void
pcutils_stringbuilder_reset(struct pcutils_stringbuilder *sb);

int
pcutils_stringbuilder_keep(struct pcutils_stringbuilder *sb, size_t sz);

WTF_ATTRIBUTE_PRINTF(2, 3)
int
pcutils_stringbuilder_snprintf(struct pcutils_stringbuilder *sb,
    const char *fmt, ...);

char*
pcutils_stringbuilder_build(struct pcutils_stringbuilder *sb);


void
pcutils_string_init(struct pcutils_string *string, size_t chunk_size);

void
pcutils_string_reset(struct pcutils_string *string);

void
pcutils_string_clear(struct pcutils_string *string);

int
pcutils_string_check_size(struct pcutils_string *string, size_t size);

int
pcutils_string_append_chunk(struct pcutils_string *string,
        const char *chunk, size_t len);

static inline int
pcutils_string_append_str(struct pcutils_string *string, const char *str)
{
    return pcutils_string_append_chunk(string, str, strlen(str));
}

WTF_ATTRIBUTE_PRINTF(2, 0)
int
pcutils_string_vappend(struct pcutils_string *string,
        const char *fmt, va_list ap);

WTF_ATTRIBUTE_PRINTF(2, 3)
int
pcutils_string_append(struct pcutils_string *string, const char *fmt, ...);

static inline char*
pcutils_string_get(struct pcutils_string *string)
{
    return string->abuf;
}

static inline size_t
pcutils_string_length(struct pcutils_string *string)
{
    return string->curr - string->abuf;
}


int
pcutils_string_is_empty(struct pcutils_string *string, int *empty);

typedef int (*pcutils_token_found_f)(const char *start, const char *end,
        void *ud);

int
pcutils_token_by_delim(const char *start, const char *end, const char c,
        void *ud, pcutils_token_found_f cb);

struct pcutils_token {
    const char                *start;
    const char                *end;
};

struct pcutils_token_iterator {
    struct pcutils_token           curr;
    const char                    *next;

    const char                    *str;
    const char                    *end;
    int (*is_delim)(const char c);
};

struct pcutils_token_iterator
pcutils_token_it_begin(const char *start, const char *end,
        int (*is_delim)(const char c));

struct pcutils_token*
pcutils_token_it_value(struct pcutils_token_iterator *it);

struct pcutils_token*
pcutils_token_it_next(struct pcutils_token_iterator *it);

void
pcutils_token_it_end(struct pcutils_token_iterator *it);

PCA_EXTERN_C_END

#endif  /* PURC_PRIVATE_STRINGBUILDER_H */


