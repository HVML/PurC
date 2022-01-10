/*
 * @file stringbuilder.c
 * @author Xu Xiaohong
 * @date 2021/11/06
 * @brief The implementation for stringbuilder.
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

#include "config.h"
#include "private/stringbuilder.h"

#include <stdio.h>
#include <stdlib.h>

struct pcutils_buf
{
    struct list_head    node;

    size_t              sz;
    size_t              curr;

    char                buf[0];
};

void
pcutils_stringbuilder_reset(struct pcutils_stringbuilder *sb)
{
    struct list_head *p, *n;
    list_for_each_safe(p, n, &sb->list) {
        struct pcutils_buf *buf;
        buf = container_of(p, struct pcutils_buf, node);
        list_del(p);
        free(buf);
    }

    memset(sb, 0, sizeof(*sb));
}

int
pcutils_stringbuilder_keep(struct pcutils_stringbuilder *sb, size_t sz)
{
    if (sb->curr) {
        if (sb->curr->curr + 1 < sb->curr->sz)
            return 0;
        if (sz == (size_t)-1) {
            sz = sb->curr->sz;
        }
        sb->curr = NULL;
    } else {
        if (sz == (size_t)-1) {
            sz = sb->chunk;
        }
    }

    struct pcutils_buf *buf;
    buf = (struct pcutils_buf*)malloc(sizeof(*buf) + sz);
    if (!buf)
        return -1;

    buf->sz = sz;
    buf->curr = 0;
    buf->buf[0] = '\0';

    list_add_tail(&buf->node, &sb->list);
    sb->curr = buf;

    return 0;
}

int
pcutils_stringbuilder_snprintf(struct pcutils_stringbuilder *sb,
        const char *fmt, ...)
{
    const size_t chunk = sb->chunk;
    char *p;
    size_t len;
    size_t sz;
    int n;
    struct pcutils_buf *buf;

    va_list ap, cp;
    va_start(ap, fmt);
    va_copy(cp, ap);

    do {
        if (sb->oom)
            break;

        if (pcutils_stringbuilder_keep(sb, chunk)) {
            sb->oom = 1;
            break;
        }

        buf = sb->curr;
        p   = buf->buf + buf->curr;
        len = buf->sz - buf->curr;
        n = vsnprintf(p, len, fmt, ap);
        if (n < 0) {
            sb->oom = 1; // FIXME: actually `fmt` is bad-format
            break;
        }
        if ((size_t)n < len)
            break;

        *p = '\0';

        sb->curr = NULL;
        sz = n + 1;
        // sz = pcutils_get_next_fibonacci_number(sz);
        if (sz < chunk)
            sz = chunk;
        if (pcutils_stringbuilder_keep(sb, sz)) {
            sb->curr -= 1;
            sb->oom = 1;
            break;
        }

        buf = sb->curr;
        p   = buf->buf + buf->curr;
        len = buf->sz - buf->curr;
        n = vsnprintf(p, len, fmt, cp);
        if (n < 0) {
            sb->oom = 1; // FIXME: actually `fmt` is bad-format
            break;
        }
    } while (0);

    if (sb->oom)
        return -1;

    buf->curr += n;
    sb->total += n;
    return n;
}

char*
pcutils_stringbuilder_build(struct pcutils_stringbuilder *sb)
{
    size_t sz = sb->total + 1;
    char *buf = (char*)malloc(sz);
    if (!buf)
        return NULL;

    char *s = buf;
    struct list_head *p, *n;
    list_for_each_safe(p, n, &sb->list) {
        struct pcutils_buf *buf;
        buf = container_of(p, struct pcutils_buf, node);
        int n = snprintf(s, sz, "%s", buf->buf);
        sz -= n;
        s  += n;
    }

    return buf;
}

void
pcutils_string_init(struct pcutils_string *string, size_t chunk_size)
{
    string->chunk_size = chunk_size;

    string->buf[0]     = '\0';
    string->abuf       = string->buf;
    string->end        = string->buf + sizeof(string->buf);
    string->curr       = string->buf;
}

void
pcutils_string_reset(struct pcutils_string *string)
{
    if (string->abuf != string->buf) {
        free(string->abuf);
    }

    string->abuf       = string->buf;
    string->end        = string->buf + sizeof(string->buf);
    string->curr       = string->buf;
}

int
string_check_size(struct pcutils_string *string, size_t size)
{
    if (size < (size_t)(string->end - string->abuf))
        return 0;

    size_t chunk_size = string->chunk_size;
    size_t align = (size + chunk_size - 1) / chunk_size * chunk_size;
    char *p;
    if (string->abuf == string->buf) {
        p = (char*)malloc(align);
        if (!p)
            return -1;

        snprintf(p, align, "%s", string->abuf);
    }
    else {
        p = (char*)realloc(string->abuf, align);
        if (!p)
            return -1;
    }

    string->curr = p + (string->curr - string->abuf);
    string->abuf = p;
    string->end  = p + align;

    return 0;
}

__attribute__ ((format (printf, 2, 3)))
int
pcutils_string_append(struct pcutils_string *string, const char *fmt, ...)
{
    int n;
    va_list ap, ap_dup;
    va_start(ap, fmt);
    va_copy(ap_dup, ap);
    size_t len = string->end - string->curr;
    n = vsnprintf(string->curr, len, fmt, ap);
    va_end(ap);

    if (n<0)
        return -1;

    if ((size_t)n < len)
        return 0;

    int r = string_check_size(string, (string->end - string->abuf) + n);
    if (r)
        return -1;

    n = vsnprintf(string->curr, len, fmt, ap_dup);
    va_end(ap_dup);

    if (n<0)
        return -1;

    if ((size_t)n < len)
        return 0;

    return -1;
}

int
pcutils_string_length(struct pcutils_string *string, size_t *len)
{
    if (len) {
        *len = string->curr - string->abuf;
    }

    return 0;
}

int
pcutils_string_is_empty(struct pcutils_string *string, int *empty)
{
    if (empty) {
        *empty = (string->curr == string->buf) ? true : false;
    }

    return 0;
}

int
pcutils_token_by_delim(const char *start, const char *end, const char c,
        void *ud, pcutils_token_found_f cb)
{
    int r;
    const char *p = start;
    for (; p<end; ++p) {
        if (*p != c)
            continue;
        r = cb(start, p, ud);
        if (r)
            return r;
        start = p + 1;
    }

    r = cb(start, end, ud);
    return r;
}

