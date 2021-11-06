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

int pcutils_stringbuilder_vsnprintf(struct pcutils_stringbuilder *sb,
        const char *fmt, va_list ap)
{
    const size_t chunk = sb->chunk;
    char *p;
    size_t len;
    size_t sz;
    int n;
    struct pcutils_buf *buf;

    if (sb->oom)
        goto oom;

    if (pcutils_stringbuilder_keep(sb, chunk)) {
        sb->oom = 1;
        goto oom;
    }

    va_list cp;
    va_copy(cp, ap);

    buf = sb->curr;
    p   = buf->buf + buf->curr;
    len = buf->sz - buf->curr;
    n = vsnprintf(p, len, fmt, cp);
    if (n < 0) {
        sb->oom = 1;
        return -1;
    }
    if ((size_t)n < len)
        goto ok;

    *p = '\0';

    sb->curr = NULL;
    sz = n + 1;
    // sz = pcutils_get_next_fibonacci_number(sz);
    if (sz < chunk)
        sz = chunk;
    if (pcutils_stringbuilder_keep(sb, sz)) {
        sb->curr -= 1;
        sb->oom = 1;
        goto oom;
    }

    buf = sb->curr;
    p   = buf->buf + buf->curr;
    len = buf->sz - buf->curr;
    n = vsnprintf(p, len, fmt, ap);
    if (n < 0) {
        sb->oom = 1;
        return -1;
    }

    goto ok;

oom:
    n = vsnprintf(NULL, 0, fmt, ap);
    if (n > 0) {
        sb->total += n;
    }
    goto end;

ok:
    buf->curr += n;
    sb->total += n;

end:
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

