/*
 * @file utf8.c
 * @author Vincent Wei
 * @date 2022/02/28
 * @brief The implementation of utilities for UTF-8 enconding.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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
 * Note that some code come from GLIB (<https://github.com/GNOME/glib>),
 * which is governed by LGPLv2. The copyright owners:
 *
 * Copyright (C) 1999 Tom Tromey
 * Copyright (C) 2000 Red Hat, Inc.
 */

#include "config.h"

#include "private/utf8.h"

#include <assert.h>

#define VALIDATE_BYTE(mask, expect)                         \
do {                                                        \
    if (UNLIKELY((*(uint8_t *)p & (mask)) != (expect)))     \
    goto error;                                             \
} while (0)

/* see IETF RFC 3629 Section 4 */

static const char *
fast_validate(const char *str, size_t *nr_chars)
{
    size_t n = 0;
    const char *p;

    for (p = str; *p; p++) {
        if (*(uint8_t *)p < 128) {
            n++;
        }
        else {
            const char *last;

            last = p;
            if (*(uint8_t *)p < 0xe0) /* 110xxxxx */
            {
                if (UNLIKELY (*(uint8_t *)p < 0xc2))
                    goto error;
            }
            else {
                if (*(uint8_t *)p < 0xf0) /* 1110xxxx */
                {
                    switch (*(uint8_t *)p++ & 0x0f) {
                    case 0:
                        VALIDATE_BYTE(0xe0, 0xa0); /* 0xa0 ... 0xbf */
                        break;
                    case 0x0d:
                        VALIDATE_BYTE(0xe0, 0x80); /* 0x80 ... 0x9f */
                        break;
                    default:
                        VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
                    }
                }
                else if (*(uint8_t *)p < 0xf5) /* 11110xxx excluding out-of-range */
                {
                    switch (*(uint8_t *)p++ & 0x07) {
                    case 0:
                        VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
                        if (UNLIKELY((*(uint8_t *)p & 0x30) == 0))
                            goto error;
                        break;
                    case 4:
                        VALIDATE_BYTE(0xf0, 0x80); /* 0x80 ... 0x8f */
                        break;
                    default:
                        VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
                    }
                    p++;
                    VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
                }
                else
                    goto error;
            }

            p++;
            VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */

            n++;
            continue;
error:
            if (nr_chars)
                *nr_chars = n;
            return last;
        }
    }

    if (nr_chars)
        *nr_chars = n;
    return p;
}

static const char *
fast_validate_len (const char *str, ssize_t max_len, size_t *nr_chars)
{
    size_t n = 0;
    const char *p;

    assert(max_len >= 0);

    for (p = str; ((p - str) < max_len) && *p; p++) {
        if (*(uint8_t *)p < 128) {
            n++;
        }
        else {
            const char *last;

            last = p;
            if (*(uint8_t *)p < 0xe0) /* 110xxxxx */
            {
                if (UNLIKELY (max_len - (p - str) < 2))
                    goto error;

                if (UNLIKELY (*(uint8_t *)p < 0xc2))
                    goto error;
            }
            else {
                if (*(uint8_t *)p < 0xf0) /* 1110xxxx */
                {
                    if (UNLIKELY (max_len - (p - str) < 3))
                        goto error;

                    switch (*(uint8_t *)p++ & 0x0f) {
                    case 0:
                        VALIDATE_BYTE(0xe0, 0xa0); /* 0xa0 ... 0xbf */
                        break;
                    case 0x0d:
                        VALIDATE_BYTE(0xe0, 0x80); /* 0x80 ... 0x9f */
                        break;
                    default:
                        VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
                    }
                }
                else if (*(uint8_t *)p < 0xf5) /* 11110xxx excluding out-of-range */
                {
                    if (UNLIKELY (max_len - (p - str) < 4))
                        goto error;

                    switch (*(uint8_t *)p++ & 0x07) {
                    case 0:
                        VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
                        if (UNLIKELY((*(uint8_t *)p & 0x30) == 0))
                            goto error;
                        break;
                    case 4:
                        VALIDATE_BYTE(0xf0, 0x80); /* 0x80 ... 0x8f */
                        break;
                    default:
                        VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
                    }
                    p++;
                    VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */
                }
                else
                    goto error;
            }

            p++;
            VALIDATE_BYTE(0xc0, 0x80); /* 10xxxxxx */

            n++;
            continue;

error:
            if (nr_chars)
                *nr_chars = n;
            return last;
        }
    }

    if (nr_chars)
        *nr_chars = n;
    return p;
}

bool pcutils_string_check_utf8_len(const char* str, size_t max_len,
        size_t *nr_chars, const char **end)
{
    const char *p;

    p = fast_validate_len(str, max_len, nr_chars);

    if (end)
        *end = p;

    if (p != str + max_len)
        return false;
    else
        return true;
}

bool pcutils_string_check_utf8(const char *str, ssize_t max_len,
        size_t *nr_chars, const char **end)
{
    const char *p;

    if (max_len >= 0)
        return pcutils_string_check_utf8_len(str, max_len, nr_chars, end);

    p = fast_validate(str, nr_chars);

    if (end)
        *end = p;

    if (*p != '\0')
        return false;
    else
        return true;
}

static const char utf8_skip_data[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

const char * const _pcutils_utf8_skip = utf8_skip_data;

size_t
pcutils_string_utf8_chars(const char *p, ssize_t max)
{
    size_t nr_chars = 0;
    const char *start = p;

    if (p == NULL || max == 0)
        return 0;

    if (max < 0) {
        while (*p) {
            p = pcutils_utf8_next_char(p);
            ++nr_chars;
        }
    }
    else {
        p = pcutils_utf8_next_char(p);

        while (p - start < max && *p) {
            ++nr_chars;
            p = pcutils_utf8_next_char(p);
        }

        /* only do the last nr_chars increment if we got a complete
         * char (don't count partial chars)
         */
        if (p - start <= max)
            ++nr_chars;
    }

    return nr_chars;
}

char *
pcutils_string_decode_utf16(const unsigned char* bytes, size_t max_len,
        size_t *str_len, bool silently)
{
    (void)bytes;
    (void)max_len;
    (void)str_len;
    (void)silently;

    // TODO
    return NULL;
}

char *
pcutils_string_decode_utf32(const unsigned char* bytes, size_t max_len,
        size_t *str_len, bool silently)
{
    (void)bytes;
    (void)max_len;
    (void)str_len;
    (void)silently;

    // TODO
    return NULL;
}

