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

static int inflate_unistr(foil_unistr *unistr, size_t nr_inc_chars)
{
    if (unistr->len + nr_inc_chars > unistr->sz) {
        size_t new_sz;
        new_sz = pcutils_get_next_fibonacci_number(unistr->len + nr_inc_chars);
        unistr->ucs = realloc(unistr->ucs, sizeof(uint32_t) * new_sz);
        if (unistr->ucs == NULL)
            return -1;

        unistr->sz = new_sz;
    }

    return 0;
}

static int deflate_unistr(foil_unistr *unistr, size_t nr_dec_chars)
{
    assert(unistr->len >= nr_dec_chars);

    size_t new_sz = pcutils_get_prev_fibonacci_number(unistr->sz);
    if (unistr->len - nr_dec_chars <= new_sz) {
        unistr->ucs = realloc(unistr->ucs, sizeof(uint32_t) * new_sz);
        if (unistr->ucs == NULL)
            return -1;

        unistr->sz = new_sz;
    }

    return 0;
}

foil_unistr *foil_unistr_new_len(const char *str_utf8, ssize_t len)
{
    size_t nr_chars = pcutils_string_utf8_chars_with_nulls(str_utf8, len);

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
    if (inflate_unistr(unistr, dfl_size)) {
        free(unistr);
        unistr = NULL;
    }
    else {
        unistr->len = dfl_size;
    }

    return unistr;
}

foil_unistr *foil_unistr_clone(foil_unistr *unistr)
{
    foil_unistr *new_unistr = calloc(1, sizeof(*unistr));
    if (inflate_unistr(new_unistr, unistr->len)) {
        free(new_unistr);
        new_unistr = NULL;
    }
    else {
        memcpy(new_unistr->ucs, unistr->ucs, sizeof(uint32_t) * unistr->len);
        new_unistr->len = unistr->len;
    }

    return new_unistr;
}

foil_unistr *foil_unistr_new_ucs(const char *ucs, size_t len)
{
    foil_unistr *unistr = calloc(1, sizeof(*unistr));
    if (inflate_unistr(unistr, len)) {
        free(unistr);
        unistr = NULL;
    }
    else {
        memcpy(unistr->ucs, ucs, sizeof(uint32_t) * len);
        unistr->len = len;
    }

    return unistr;
}

foil_unistr *foil_unistr_new_moving_in(uint32_t *ucs, size_t len)
{
    assert(ucs != NULL && len > 0);

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

static size_t shift_ucs_right(foil_unistr *unistr, ssize_t pos, size_t nr_chars)
{
    assert(nr_chars > 0);

    size_t real_pos = 0;
    if (pos < 0) {
        if ((size_t)-pos > unistr->len)
            real_pos = 0;
        else
            real_pos = unistr->len + pos;
    }
    else {
        if ((size_t)pos >= unistr->len)
            real_pos = unistr->len - 1;
        else
            real_pos = unistr->len + pos;
    }

    real_pos += nr_chars;
    size_t n = nr_chars;
    while (n > 0) {
        unistr->ucs[real_pos + nr_chars] = unistr->ucs[real_pos];
        real_pos--;
        n--;
    }

    return real_pos;
}

foil_unistr *foil_unistr_insert_len(foil_unistr *unistr, ssize_t pos,
        const char *str_utf8, ssize_t len)
{
    size_t nr_chars = pcutils_string_utf8_chars_with_nulls(str_utf8, len);

    if (nr_chars > 0) {
        if (inflate_unistr(unistr, nr_chars)) {
            free(unistr);
            unistr = NULL;
        }
        else {
            size_t real_pos = shift_ucs_right(unistr, pos, nr_chars);
            const char *next = str_utf8;
            while (nr_chars > 0) {
                unistr->ucs[real_pos++] =
                    pcutils_utf8_to_unichar((const unsigned char *)next);

                next = pcutils_utf8_next_char(next);
                nr_chars--;
            }

            unistr->len += nr_chars;
        }
    }

    return unistr;
}

foil_unistr *foil_unistr_insert_unichar(foil_unistr *unistr, ssize_t pos,
        uint32_t unichar)
{
    if (inflate_unistr(unistr, 1)) {
        free(unistr);
        unistr = NULL;
    }
    else {
        shift_ucs_right(unistr, pos, 1);
        unistr->ucs[unistr->len] = unichar;
        unistr->len += 1;
    }

    return unistr;
}

foil_unistr *foil_unistr_erase(foil_unistr *unistr,
        ssize_t pos, ssize_t nr_chars)
{
    if (nr_chars == 0)
        return unistr;

    size_t real_pos = 0;
    if (pos < 0) {
        if ((size_t)-pos > unistr->len)
            real_pos = 0;
        else
            real_pos = unistr->len + pos;
    }
    else {
        if ((size_t)pos >= unistr->len)
            real_pos = unistr->len - 1;
        else
            real_pos = unistr->len + pos;
    }

    if (nr_chars < 0 || (real_pos + (size_t)nr_chars) > unistr->len) {
        return foil_unistr_truncate(unistr, real_pos);
    }

    size_t n = nr_chars;
    while (n > 0) {
        unistr->ucs[real_pos + nr_chars] = unistr->ucs[real_pos];
        real_pos++;
        n--;
    }

    if (deflate_unistr(unistr, nr_chars)) {
        free(unistr);
        unistr = NULL;
    }
    else {
        unistr->len = unistr->len - nr_chars;
    }

    return unistr;
}

foil_unistr *foil_unistr_truncate(foil_unistr *unistr, size_t len)
{
    if (len < unistr->len) {
        if (deflate_unistr(unistr, unistr->len - len)) {
            free(unistr);
            unistr = NULL;
        }
        else {
            unistr->len = len;
        }
    }

    return unistr;
}

foil_unistr *foil_unistr_assign_len(foil_unistr *unistr,
        const char *str_utf8, ssize_t len)
{
    size_t nr_chars = pcutils_string_utf8_chars_with_nulls(str_utf8, len);

    if (nr_chars > 0) {
        int ret = 0;
        if (nr_chars > unistr->len) {
            ret = inflate_unistr(unistr, nr_chars - unistr->len);
        }
        else if (nr_chars < unistr->len) {
            ret = deflate_unistr(unistr, unistr->len - nr_chars);
        }

        if (ret) {
            free(unistr);
            unistr = NULL;
        }
        else {
            size_t n = 0;
            const char *next = str_utf8;
            while (n < nr_chars) {
                unistr->ucs[n++] =
                    pcutils_utf8_to_unichar((const unsigned char *)next);

                next = pcutils_utf8_next_char(next);
            }

            unistr->len = nr_chars;
        }
    }

    return unistr;
}

