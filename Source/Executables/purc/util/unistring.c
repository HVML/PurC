/*
 * unistring.c - Implementation of simple Unicode string.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Author: Vincent Wei <https://github.com/VincentWei>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "unistring.h"

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <purc/purc-utils.h>

foil_unistr *foil_unistr_new_len(const char *str_utf8, ssize_t len)
{
    size_t nr_chars = pcutils_string_utf8_chars(str_utf8, len);

    foil_unistr *unistr = foil_unistr_sized_new(nr_chars);
    if (unistr == NULL)
        goto done;

    size_t n = 0;
    const char *next = str_utf8;
    while (n < nr_chars) {
        unistr->ucs[n++] =
            pcutils_utf8_to_unichar((const unsigned char *)next);

        next = pcutils_utf8_next_char(next);
    }

done:
    return unistr;
}

foil_unistr *foil_unistr_sized_new(size_t dfl_size)
{
    foil_unistr *unistr = calloc(1, sizeof(*unistr));
    unistr->ucs = malloc(sizeof(uint32_t) * dfl_size);
    if (unistr->ucs) {
        unistr->len = dfl_size;
        unistr->sz = dfl_size;
    }
    else {
        free(unistr);
        unistr = NULL;
    }

    return unistr;
}

foil_unistr *foil_unistr_clone(foil_unistr *unistr)
{
    foil_unistr *new_unistr = calloc(1, sizeof(*unistr));
    new_unistr->ucs = malloc(sizeof(uint32_t) * unistr->len);
    if (new_unistr->ucs) {
        memcpy(new_unistr->ucs, unistr->ucs, sizeof(uint32_t) * unistr->len);
        new_unistr->len = unistr->len;
        new_unistr->sz = unistr->len;
    }
    else {
        free(new_unistr);
        new_unistr = NULL;
    }

    return new_unistr;
}

foil_unistr *foil_unistr_new_ucs(const char *ucs, size_t len)
{
    foil_unistr *unistr = calloc(1, sizeof(*unistr));
    unistr->ucs = malloc(sizeof(uint32_t) * len);
    if (unistr->ucs) {
        memcpy(unistr->ucs, ucs, sizeof(uint32_t) * len);
        unistr->len = len;
        unistr->sz = len;
    }
    else {
        free(unistr);
        unistr = NULL;
    }

    return unistr;
}

foil_unistr *foil_unistr_new_moving_in(uint32_t *ucs, size_t len)
{
    foil_unistr *unistr = calloc(1, sizeof(*unistr));

    unistr->ucs = ucs;
    unistr->len = len;
    unistr->sz = len;
    return unistr;
}

foil_unistr *foil_unistr_set_size(foil_unistr *unistr, size_t len)
{
    unistr->ucs = realloc(unistr->ucs, sizeof(uint32_t) * len);
    if (unistr->ucs) {
        unistr->len = len;
        unistr->sz = len;
    }
    else {
        free(unistr);
        unistr = NULL;
    }

    return unistr;
}

uint32_t *foil_unistr_free(foil_unistr *unistr, bool free_segment)
{
    uint32_t *ucs;

    if (free_segment) {
        free(unistr->ucs);
        ucs = NULL;
    }
    else {
        ucs = unistr->ucs;
    }

    free(unistr);
    return ucs;
}

static void shift_ucs_right(foil_unistr *unistr, ssize_t pos, size_t nr_chars)
{
    assert(nr_chars > 0);

    ssize_t start = unistr->len + pos;
    if (start < 0) {
        start = 0;
    }
    else if ((size_t)start >= unistr->len) {
        start = unistr->len - 1;
    }

    size_t n = nr_chars;
    while (n > 0) {
        unistr->ucs[start + nr_chars] = unistr->ucs[start];
        start++;
        n--;
    }
}

foil_unistr *foil_unistr_insert_len(foil_unistr *unistr, ssize_t pos,
        const char *str_utf8, ssize_t len)
{
    size_t nr_chars = pcutils_string_utf8_chars(str_utf8, len);

    if (nr_chars > 0) {
        unistr->ucs = realloc(unistr->ucs,
                sizeof(uint32_t) * (unistr->len + nr_chars));
        if (unistr->ucs) {
            shift_ucs_right(unistr, pos, nr_chars);

            size_t n = 0;
            const char *next = str_utf8;
            while (n < nr_chars) {
                unistr->ucs[n++] =
                    pcutils_utf8_to_unichar((const unsigned char *)next);

                next = pcutils_utf8_next_char(next);
            }

            unistr->len += nr_chars;
            unistr->sz = unistr->len;
        }
        else {
            free(unistr);
            unistr = NULL;
        }
    }

    return unistr;
}

foil_unistr *foil_unistr_insert_unichar(foil_unistr *unistr, ssize_t pos,
        uint32_t unichar)
{
    unistr->ucs = realloc(unistr->ucs,
            sizeof(uint32_t) * (unistr->len + 1));
    if (unistr->ucs) {
        shift_ucs_right(unistr, pos, 1);

        unistr->ucs[unistr->len] = unichar;
        unistr->len += 1;
        unistr->sz = unistr->len;
    }
    else {
        free(unistr);
        unistr = NULL;
    }

    return unistr;
}

foil_unistr *foil_unistr_erase(foil_unistr *unistr, ssize_t pos, ssize_t len)
{
    ssize_t start = unistr->len + pos;
    if (start < 0) {
        start = 0;
    }
    else if ((size_t)start >= unistr->len) {
        start = unistr->len - 1;
    }

    if (len < 0) {
        size_t new_len = start;
        unistr->ucs = realloc(unistr->ucs, sizeof(uint32_t) * new_len);
        if (unistr->ucs == NULL) {
            free(unistr);
            unistr = NULL;
            goto done;
        }
        unistr->len = new_len;
        unistr->sz = new_len;
    }
    else {
        size_t new_len;
        if ((size_t)start + (size_t)len > unistr->len)
            len = unistr->len - start;
        new_len = start + len;

        start += new_len;
        size_t n = (size_t)new_len;
        while (n > 0) {
            unistr->ucs[start - new_len] = unistr->ucs[start];
            start--;
            n--;
        }

        unistr->len = unistr->len - new_len;
        unistr->sz = unistr->len;
    }

done:
    return unistr;
}

foil_unistr *foil_unistr_truncate(foil_unistr *unistr, size_t len)
{
    if (len < unistr->len) {
        unistr->ucs = realloc(unistr->ucs, sizeof(uint32_t) * len);
        if (unistr->ucs == NULL) {
            free(unistr);
            unistr = NULL;
            goto done;
        }

        unistr->len = len;
        unistr->sz = len;
    }

done:
    return unistr;
}

foil_unistr *foil_unistr_assign_len(foil_unistr *unistr,
        const char *str_utf8, ssize_t len)
{
    size_t nr_chars = pcutils_string_utf8_chars(str_utf8, len);

    if (nr_chars > 0) {
        unistr->ucs = realloc(unistr->ucs, sizeof(uint32_t) * nr_chars);
        if (unistr->ucs) {
            size_t n = 0;
            const char *next = str_utf8;
            while (n < nr_chars) {
                unistr->ucs[n++] =
                    pcutils_utf8_to_unichar((const unsigned char *)next);

                next = pcutils_utf8_next_char(next);
            }

            unistr->len = nr_chars;
            unistr->sz = unistr->len;
        }
        else {
            free(unistr);
            unistr = NULL;
        }
    }

    return unistr;
}

