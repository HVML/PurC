/*
 * @file printbuf.c
 * @author gengyue
 * @date 2021/07/02
 * @brief The implementation of print buffer.
 *
 * Copyright (C) 2021 ~ 2023 FMSoft <https://www.fmsoft.cn>
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
 * Note that the original code come from json-c, which is licensed under
 * MIT License (<http://www.opensource.org/licenses/mit-license.php>).
 *
 * The copying annoucements are as follow:
 *
 * Copyright (c) 2008-2009 Yahoo! Inc.  All rights reserved.
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Author: Michael Clark <michael@metaparadigm.com>
 */

#define _GNU_SOURCE

#include "private/printbuf.h"
#include "purc-utils.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if !HAVE(VASPRINTF)
#include "private/ports.h"
#endif

/**
 * Extend the buffer p so it has a size of at least min_size.
 *
 * If the current size is large enough, nothing is changed.
 *
 * Note: this does not check the available space!  The caller
 *  is responsible for performing those calculations.
 */
static int printbuf_extend(struct pcutils_printbuf *p, size_t min_size)
{
    char *t;
    size_t new_size;

    if (p->size >= min_size)
        return 0;

    /* Prevent unsigned integer overflows with large buffers. */
    if (min_size > UINT_MAX - 8)
        return -1;

    if (p->size > UINT_MAX / 2)
        new_size = min_size + 8;
    else {
        new_size = pcutils_get_next_fibonacci_number(p->size);
        if (new_size < min_size + 8)
            new_size = min_size + 8;
    }

    if (!(t = (char *)realloc(p->buf, new_size))) {
        p->buf = NULL;
        p->size = 0;
        p->bpos = 0;
        return -1;
    }

    p->size = new_size;
    p->buf = t;
    return 0;
}

int pcutils_printbuf_init(struct pcutils_printbuf *p)
{
    p->size = 32;
    p->bpos = 0;

    if (!(p->buf = (char *)malloc(p->size))) {
        p->size = 0;
        return -1;
    }

    p->buf[0] = '\0';
    return 0;
}

struct pcutils_printbuf *pcutils_printbuf_new(void)
{
    struct pcutils_printbuf * p = NULL;

    p = (struct pcutils_printbuf *)calloc(1, sizeof(struct pcutils_printbuf));
    if (!p)
        return NULL;

    if (pcutils_printbuf_init(p)) {
        free(p);
        return NULL;
    }

    return p;
}

int pcutils_printbuf_memappend(struct pcutils_printbuf *p,
        const char *buf, size_t size)
{
    if (!p->buf)
        return -1;

    if (size == 0)
        size = strlen(buf);

    /* Prevent unsigned integer overflows with large buffers. */
    if (size > UINT_MAX - p->bpos - 1)
        return -1;
    if (p->size <= p->bpos + size + 1) {
        if (printbuf_extend(p, p->bpos + size + 1) < 0)
            return -1;
    }

    memcpy(p->buf + p->bpos, buf, size);
    p->bpos += size;
    p->buf[p->bpos] = '\0';
    return size;
}

int pcutils_printbuf_memset(struct pcutils_printbuf *pb, ssize_t offset,
        int charvalue, size_t len)
{
    size_t my_off;
    size_t size_needed;

    if (!pb->buf)
        return -1;

    if (offset < 0)
        my_off = pb->bpos;
    else
        my_off = offset;

    /* Prevent unsigned integer overflows with large buffers. */
    if (len > UINT_MAX - my_off)
        return -1;

    size_needed = my_off + len;
    if (pb->size < size_needed) {
        if (printbuf_extend(pb, size_needed + 1) < 0)
            return -1;
    }

    memset(pb->buf + my_off, charvalue, len);
    if (pb->bpos < size_needed)
        pb->bpos = size_needed;
    pb->buf[pb->bpos] = '\0';

    return 0;
}

int pcutils_printbuf_shrink(struct pcutils_printbuf *pb, size_t len)
{
    if (!pb->buf)
        return -1;

    if (len > pb->bpos)
        return -1;

    pb->bpos -= len;
    memset(pb->buf + pb->bpos, '\0', len);
    return 0;
}

#define SZ_STACK_BUFF   256

int pcutils_printbuf_format(struct pcutils_printbuf *p, const char *msg, ...)
{
    va_list ap;
    char *t;
    int size;
    char buf[SZ_STACK_BUFF];

    if (!p->buf)
        return -1;

    /* user stack buffer first */
    va_start(ap, msg);
    size = vsnprintf(buf, sizeof(buf), msg, ap);
    va_end(ap);
    /* if string is greater than stack buffer, then use dynamic string
     * with vasprintf.  Note: some implementation of vsnprintf return -1
     * if output is truncated whereas some return the number of bytes that
     * would have been written - this code handles both cases.
     */
    if (size == -1 || size >= SZ_STACK_BUFF) {
        va_start(ap, msg);
        if ((size = vasprintf(&t, msg, ap)) < 0) {
            va_end(ap);
            return -1;
        }
        va_end(ap);
        pcutils_printbuf_memappend(p, t, size);
        free(t);
        return size;
    }

    pcutils_printbuf_memappend(p, buf, size);
    return size;
}

void pcutils_printbuf_reset(struct pcutils_printbuf *p)
{
    if (!p->buf)
        return;

    p->buf[0] = '\0';
    p->bpos = 0;
}

char *pcutils_printbuf_delete(struct pcutils_printbuf *p, bool keep_buf)
{
    char *buf = NULL;
    if (p) {
        if (keep_buf)
            buf = p->buf;

        if (buf == NULL && p->buf)
            free(p->buf);
        free(p);
    }

    return buf;
}

