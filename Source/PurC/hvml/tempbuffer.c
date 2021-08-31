/*
 * @file tempbuffer.c
 * @author XueShuming
 * @date 2021/08/27
 * @brief The impl of tempbuffer.
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
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/errors.h"
#include "tempbuffer.h"

#define MIN_BUFFER_CAPACITY 32

static size_t get_buffer_size (size_t sz)
{
    size_t sz_buf = pcutils_get_next_fibonacci_number(sz);
    return sz_buf < MIN_BUFFER_CAPACITY ? MIN_BUFFER_CAPACITY : sz_buf;
}

struct pchvml_temp_buffer* pchvml_temp_buffer_new ()
{
    struct pchvml_temp_buffer* buffer = (struct pchvml_temp_buffer*) calloc(
            1, sizeof(struct pchvml_temp_buffer));
    size_t sz_init = get_buffer_size(MIN_BUFFER_CAPACITY);
    buffer->base = (uint8_t*) calloc (1, sz_init + 1);
    buffer->here = buffer->base;
    buffer->stop = buffer->base + sz_init;
    buffer->sz_char = 0;
    return buffer;
}

bool pchvml_temp_buffer_is_empty (struct pchvml_temp_buffer* buffer)
{
    return buffer->here == buffer->base;
}

size_t pchvml_temp_buffer_get_size_in_bytes (struct pchvml_temp_buffer* buffer)
{
    return buffer->here - buffer->base;
}

size_t pchvml_temp_buffer_get_size_in_chars (struct pchvml_temp_buffer* buffer)
{
    return buffer->sz_char;
}

static bool is_utf8_leading_byte (char c)
{
    return (c & 0xC0) != 0x80;
}

static wchar_t utf8_to_wchar_t (const unsigned char* utf8_char,
        int utf8_char_len)
{
    wchar_t wc = *((unsigned char *)(utf8_char++));
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

static void pchvml_temp_buffer_append_inner (struct pchvml_temp_buffer* buffer,
        const char* bytes, size_t nr_bytes)
{
    uint8_t* newpos = buffer->here + nr_bytes;
    if ( newpos > buffer->stop ) {
        size_t new_size = get_buffer_size(buffer->stop - buffer->base);
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

void pchvml_temp_buffer_append (struct pchvml_temp_buffer* buffer,
        const char* bytes, size_t nr_bytes)
{
    pchvml_temp_buffer_append_inner (buffer, bytes, nr_bytes);
    const uint8_t* p = (const uint8_t*)bytes;
    const uint8_t* end = p + nr_bytes;
    while (p != end) {
        if (is_utf8_leading_byte (*p)) {
            buffer->sz_char++;
        }
        p++;
    }
}

void pchvml_temp_buffer_append_temp_buffer (struct pchvml_temp_buffer* buffer,
        struct pchvml_temp_buffer* append)
{
    const char* bytes = pchvml_temp_buffer_get_buffer(append);
    size_t nr_bytes = pchvml_temp_buffer_get_size_in_bytes(append);
    if (bytes && nr_bytes) {
        pchvml_temp_buffer_append_inner (buffer, bytes, nr_bytes);
        buffer->sz_char += append->sz_char;
    }
}

const char* pchvml_temp_buffer_get_buffer (
        struct pchvml_temp_buffer* buffer)
{
    return (const char*)buffer->base;
}

bool pchvml_temp_buffer_end_with (struct pchvml_temp_buffer* buffer,
        const char* bytes, size_t nr_bytes)
{
    size_t sz = pchvml_temp_buffer_get_size_in_bytes(buffer);
    return (sz >= nr_bytes 
            && memcmp(buffer->here - nr_bytes, bytes, nr_bytes) == 0);
}

bool pchvml_temp_buffer_equal_to (struct pchvml_temp_buffer* buffer,
        const char* bytes, size_t nr_bytes)
{
    size_t sz = pchvml_temp_buffer_get_size_in_bytes(buffer);
    return (sz == nr_bytes && memcmp(buffer->base, bytes, sz) == 0);
}

wchar_t pchvml_temp_buffer_get_last_char (struct pchvml_temp_buffer* buffer)
{
    if (pchvml_temp_buffer_is_empty(buffer)) {
        return 0;
    }

    uint8_t* p = buffer->here - 1;
    while (p >= buffer->base) {
        if (is_utf8_leading_byte(*p)) {
            break;
        }
        p = p - 1;
    }
    return utf8_to_wchar_t(p, buffer->here - p);
}

void pchvml_temp_buffer_reset (struct pchvml_temp_buffer* buffer)
{
    buffer->here = buffer->base;
    buffer->sz_char = 0;
}

void pchvml_temp_buffer_destroy (struct pchvml_temp_buffer* buffer)
{
    if (buffer) {
        free(buffer->base);
        free(buffer);
    }
}
