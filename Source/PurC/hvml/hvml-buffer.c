/*
 * @file hvml-buffer.c
 * @author XueShuming
 * @date 2021/08/27
 * @brief The impl of hvml buffer.
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
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/errors.h"
#include "hvml-buffer.h"

#define MIN_BUFFER_CAPACITY 32

static size_t get_buffer_size (size_t sz)
{
    size_t sz_buf = pcutils_get_next_fibonacci_number(sz);
    return sz_buf < MIN_BUFFER_CAPACITY ? MIN_BUFFER_CAPACITY : sz_buf;
}

struct pchvml_buffer* pchvml_buffer_new (void)
{
    struct pchvml_buffer* buffer = (struct pchvml_buffer*) calloc(
            1, sizeof(struct pchvml_buffer));
    size_t sz_init = get_buffer_size(MIN_BUFFER_CAPACITY);
    buffer->base = (uint8_t*) calloc (1, sz_init + 1);
    buffer->here = buffer->base;
    buffer->stop = buffer->base + sz_init;
    buffer->nr_chars = 0;
    return buffer;
}

static bool is_utf8_leading_byte (char c)
{
    return (c & 0xC0) != 0x80;
}

static uint32_t utf8_to_uint32_t (const unsigned char* utf8_char,
        int utf8_char_len)
{
    uint32_t wc = *((unsigned char *)(utf8_char++));
    int n = utf8_char_len;
    int t = 0;

    if (wc & 0x80) {
        wc &= (1 << (8-n)) - 1;
        while (--n > 0) {
            t = *((unsigned char *)(utf8_char++));
            wc = (wc << 6) | (t & 0x3F);
        }
    }

    return wc;
}

static void pchvml_buffer_append_inner (struct pchvml_buffer* buffer,
        const char* bytes, size_t nr_bytes)
{
    uint8_t* newpos = buffer->here + nr_bytes;
    if ( newpos > buffer->stop ) {
        size_t new_size = get_buffer_size(newpos - buffer->base);
        off_t here_offset = buffer->here - buffer->base;

        uint8_t* newbuf = (uint8_t*) realloc(buffer->base, new_size + 1);
        if (newbuf == NULL) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }

        buffer->base = newbuf;
        buffer->here = buffer->base + here_offset;
        buffer->stop = buffer->base + new_size;
    }

    memcpy(buffer->here, bytes, nr_bytes);
    buffer->here += nr_bytes;
    *buffer->here = 0;
}

void pchvml_buffer_append_bytes (struct pchvml_buffer* buffer,
        const char* bytes, size_t nr_bytes)
{
    pchvml_buffer_append_inner (buffer, bytes, nr_bytes);
    const uint8_t* p = (const uint8_t*)bytes;
    const uint8_t* end = p + nr_bytes;
    while (p != end) {
        if (is_utf8_leading_byte (*p)) {
            buffer->nr_chars++;
        }
        p++;
    }
}

static inline size_t uc_to_utf8(uint32_t c, char* outbuf)
{
    size_t len = 0;
    int first;
    int i;

    if (c < 0x80) {
        first = 0;
        len = 1;
    }
    else if (c < 0x800) {
        first = 0xc0;
        len = 2;
    }
    else if (c < 0x10000) {
        first = 0xe0;
        len = 3;
    }
    else if (c < 0x200000) {
        first = 0xf0;
        len = 4;
    }
    else if (c < 0x4000000) {
        first = 0xf8;
        len = 5;
    }
    else {
        first = 0xfc;
        len = 6;
    }

    if (outbuf) {
        for (i = len - 1; i > 0; --i) {
            outbuf[i] = (c & 0x3f) | 0x80;
            c >>= 6;
        }
        outbuf[0] = c | first;
    }

    return len;
}

void pchvml_buffer_append (struct pchvml_buffer* buffer,
        uint32_t uc)
{
    char buf[8] = {0};
    size_t len = uc_to_utf8(uc, buf);
    pchvml_buffer_append_bytes (buffer, buf, len);
}

void pchvml_buffer_append_chars (struct pchvml_buffer* buffer,
        const uint32_t* ucs, size_t nr_ucs)
{
    for (size_t i = 0; i < nr_ucs; i++) {
        pchvml_buffer_append (buffer, ucs[i]);
    }
}

void pchvml_buffer_delete_head_chars (
        struct pchvml_buffer* buffer, size_t sz)
{
    uint8_t* p = buffer->base;
    size_t nr = 0;
    while (p < buffer->here && nr <= sz) {
        if (is_utf8_leading_byte(*p)) {
            nr++;
        }
        p = p + 1;
    }
    p = p - 1;
    size_t n = buffer->here - p;
    memmove(buffer->base, p, n);
    buffer->here = buffer->base + n;
    memset(buffer->here, 0, buffer->stop - buffer->here);
}

void pchvml_buffer_delete_tail_chars (
        struct pchvml_buffer* buffer, size_t sz)
{
    uint8_t* p = buffer->here - 1;
    while (p >= buffer->base && sz > 0) {
        if (is_utf8_leading_byte(*p)) {
            sz--;
        }
        p = p - 1;
    }
    buffer->here = p + 1;
    memset(buffer->here, 0, buffer->stop - buffer->here);
}

bool pchvml_buffer_end_with (struct pchvml_buffer* buffer,
        const char* bytes, size_t nr_bytes)
{
    size_t sz = pchvml_buffer_get_size_in_bytes(buffer);
    return (sz >= nr_bytes
            && memcmp(buffer->here - nr_bytes, bytes, nr_bytes) == 0);
}

bool pchvml_buffer_equal_to (struct pchvml_buffer* buffer,
        const char* bytes, size_t nr_bytes)
{
    size_t sz = pchvml_buffer_get_size_in_bytes(buffer);
    return (sz == nr_bytes && memcmp(buffer->base, bytes, sz) == 0);
}

uint32_t pchvml_buffer_get_last_char (struct pchvml_buffer* buffer)
{
    if (pchvml_buffer_is_empty(buffer)) {
        return 0;
    }

    uint8_t* p = buffer->here - 1;
    while (p >= buffer->base) {
        if (is_utf8_leading_byte(*p)) {
            break;
        }
        p = p - 1;
    }
    return utf8_to_uint32_t(p, buffer->here - p);
}

void pchvml_buffer_reset (struct pchvml_buffer* buffer)
{
    memset(buffer->base, 0, buffer->stop - buffer->base);
    buffer->here = buffer->base;
    buffer->nr_chars = 0;
}

void pchvml_buffer_destroy (struct pchvml_buffer* buffer)
{
    if (buffer) {
        free(buffer->base);
        free(buffer);
    }
}

bool pchvml_buffer_is_int (struct pchvml_buffer* buffer)
{
    char* p = NULL;
    strtol((const char*)buffer->base, &p, 10);
    return (p == (char*)buffer->here);
}
