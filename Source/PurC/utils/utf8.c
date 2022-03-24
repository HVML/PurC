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

#undef NDEBUG

#include "config.h"

#include "private/utf8.h"

#include <string.h>
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

/* copy from MiniGUI */
#define MAKEWORD16(low, high)   \
    ((uint16_t)(((uint8_t)(low)) | (((uint16_t)((uint8_t)(high))) << 8)))

/* copy from MiniGUI */
static size_t
utf8_from_uc(uint32_t uc, unsigned char* mchar)
{
    int first;
    size_t len;

    if (uc < 0x80) {
        first = 0;
        len = 1;
    }
    else if (uc < 0x800) {
        first = 0xC0;
        len = 2;
    }
    else if (uc < 0x10000) {
        first = 0xE0;
        len = 3;
    }
    else if (uc < 0x200000) {
        first = 0xF0;
        len = 4;
    }
    else if (uc < 0x400000) {
        first = 0xF8;
        len = 5;
    }
    else {
        first = 0xFC;
        len = 6;
    }

    switch (len) {
        case 6:
            mchar [5] = (uc & 0x3f) | 0x80; uc >>= 6; /* Fall through */
        case 5:
            mchar [4] = (uc & 0x3f) | 0x80; uc >>= 6; /* Fall through */
        case 4:
            mchar [3] = (uc & 0x3f) | 0x80; uc >>= 6; /* Fall through */
        case 3:
            mchar [2] = (uc & 0x3f) | 0x80; uc >>= 6; /* Fall through */
        case 2:
            mchar [1] = (uc & 0x3f) | 0x80; uc >>= 6; /* Fall through */
        case 1:
            mchar [0] = uc | first;
    }

    return len;
}

struct my_string {
    char *buff;
    size_t nr_bytes;
    size_t sz_space;
};

static int mystring_append_mchar(struct my_string *mystr,
        const unsigned char *mchar, size_t mchar_len)
{
    if (mystr->nr_bytes + mchar_len < mystr->sz_space) {
        size_t new_sz;
        new_sz = pcutils_get_next_fibonacci_number(mystr->nr_bytes + mchar_len);

        mystr->buff = realloc(mystr->buff, new_sz);
        if (mystr->buff == NULL)
            return -1;

        mystr->sz_space = new_sz;
    }

    memcpy(mystr->buff + mystr->nr_bytes, mchar, mchar_len);
    mystr->nr_bytes += mchar_len;
    return 0;
}

static int mystring_done(struct my_string *mystr)
{
    if (mystr->nr_bytes + 1 > mystr->sz_space) {
        mystr->buff = realloc(mystr->buff, mystr->nr_bytes + 1);
        if (mystr->buff == NULL)
            return -1;
    }

    mystr->buff[mystr->nr_bytes] = '\0';  // null-terminated
    mystr->nr_bytes += 1;

    // shrink the buffer
    mystr->buff = realloc(mystr->buff, mystr->nr_bytes);
    mystr->sz_space = mystr->nr_bytes;
    return 0;
}

static void mystring_free(struct my_string *mystr)
{
    if (mystr->buff)
        free(mystr->buff);
}

static char *
string_decode_utf16(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, bool silently, bool le_or_be)
{
    size_t nr_consumed = 0, nr_left = max_len;
    uint32_t uc;

    struct my_string mystr = { NULL, 0, 0 };

    while (nr_left > 1) {
        uint16_t w1, w2;

        if (le_or_be)
            w1 = MAKEWORD16(bytes[nr_consumed], bytes[nr_consumed + 1]);
        else
            w1 = MAKEWORD16(bytes[nr_consumed + 1], bytes[nr_consumed]);

        if (w1 == 0)
            goto done;

        if (w1 < 0xD800 || w1 > 0xDFFF) {
            uc = w1;

            nr_consumed += 2;
            nr_left -= 2;
        }
        else {
            if (nr_left < 4)
                goto bad_encoding;

            if (le_or_be)
                w2 = MAKEWORD16(bytes[nr_consumed + 2], bytes[nr_consumed + 3]);
            else
                w2 = MAKEWORD16(bytes[nr_consumed + 3], bytes[nr_consumed + 2]);

            if (w2 < 0xDC00 || w2 > 0xDFFF)
                goto bad_encoding;

            uc = w1;
            uc <<= 10;
            uc |= (w2 & 0x03FF);
            uc += 0x10000;

            nr_consumed += 4;
            nr_left -= 4;
        }

        /* got a Unicode code point */
        unsigned char mchar[6];
        size_t mchar_len;
        mchar_len = utf8_from_uc(uc, mchar);
        if (mystring_append_mchar(&mystr, mchar, mchar_len)) {
            goto fatal;
        }
    }

bad_encoding:
    if (!silently) {
        mystring_free(&mystr);
        return (char *)-1;
    }

done:
    if (mystring_done(&mystr) == 0) {
        *sz_space = mystr.sz_space;
        return mystr.buff;
    }

fatal:
    return NULL;
}

char *
pcutils_string_decode_utf16le(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, bool silently)
{
    return string_decode_utf16(bytes, max_len, sz_space, silently, true);
}

char *
pcutils_string_decode_utf16be(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, bool silently)
{
    return string_decode_utf16(bytes, max_len, sz_space, silently, false);
}

#define MAKEDWORD32(first, second, third, fourth)       \
    ((uint32_t)(                                        \
        ((uint8_t)(first)) |                            \
        (((uint32_t)((uint8_t)(second))) << 8) |        \
        (((uint32_t)((uint8_t)(third))) << 16) |        \
        (((uint32_t)((uint8_t)(fourth))) << 24)         \
    ))

static char *
string_decode_utf32(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, bool silently, bool le_or_be)
{
    UNUSED_PARAM(silently);

    size_t nr_consumed = 0, nr_left = max_len;
    uint32_t uc;

    struct my_string mystr = { NULL, 0, 0 };

    while (nr_left > 3) {
        if (le_or_be)
            uc = MAKEDWORD32(bytes[nr_consumed], bytes[nr_consumed + 1],
                    bytes[nr_consumed + 2], bytes[nr_consumed + 3]);
        else
            uc = MAKEDWORD32(bytes[nr_consumed + 3], bytes[nr_consumed + 2],
                    bytes[nr_consumed + 1], bytes[nr_consumed]);

        if (uc == 0)
            goto done;

        nr_consumed += 4;
        nr_left -= 4;

        /* got a Unicode code point */
        unsigned char mchar[6];
        size_t mchar_len;
        mchar_len = utf8_from_uc(uc, mchar);
        if (mystring_append_mchar(&mystr, mchar, mchar_len)) {
            goto fatal;
        }
    }

done:
    if (mystring_done(&mystr) == 0) {
        *sz_space = mystr.sz_space;
        return mystr.buff;
    }

fatal:
    return NULL;
}

char *
pcutils_string_decode_utf32le(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, bool silently)
{
    return string_decode_utf32(bytes, max_len, sz_space, silently, true);
}

char *
pcutils_string_decode_utf32be(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, bool silently)
{
    return string_decode_utf32(bytes, max_len, sz_space, silently, false);
}

char *
pcutils_string_decode_utf16(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, bool silently)
{
    /* check BOM first */
    if (max_len > 1) {
        if (bytes[0] == 0xFF && bytes[1] == 0xFE)   /* LE */
            return string_decode_utf16(bytes + 2, max_len - 2,
                    sz_space, silently, true);
        else if (bytes[0] == 0xFE && bytes[1] == 0xFF)   /* BE */
            return string_decode_utf16(bytes + 2, max_len - 2,
                    sz_space, silently, false);
    }

#if CPU(LITTLE_ENDIAN)
    return string_decode_utf16(bytes, max_len, sz_space, silently, true);
#elif CPU(BIG_ENDIAN)
    return string_decode_utf16(bytes, max_len, sz_space, silently, false);
#else
#error "Unsupported endian"
#endif
}

char *
pcutils_string_decode_utf32(const unsigned char* bytes, size_t max_len,
        size_t *sz_space, bool silently)
{
    /* check BOM first */
    if (max_len > 3) {
        if (bytes[0] == 0xFF && bytes[1] == 0xFE &&
                bytes[2] == 0x00 && bytes[3] == 0x00)   /* LE */
            return string_decode_utf32(bytes + 4, max_len - 4,
                    sz_space, silently, true);
        else if (bytes[0] == 0x00 && bytes[1] == 0x00 &&
                bytes[2] == 0xFE && bytes[3] == 0xFF)   /* BE */
            return string_decode_utf32(bytes + 4, max_len - 4,
                    sz_space, silently, false);
    }

#if CPU(LITTLE_ENDIAN)
    return string_decode_utf32(bytes, max_len, sz_space, silently, true);
#elif CPU(BIG_ENDIAN)
    return string_decode_utf32(bytes, max_len, sz_space, silently, false);
#else
#error "Unsupported endian"
#endif
}

