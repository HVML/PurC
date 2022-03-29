/**
 * @file strcase.c
 * @author Vincent Wei
 * @date 2021/03/29
 * @brief The implementation of some string case-insensitive operations.
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
 */

#undef NDEBUG

#include "purc-utils.h"

#include <string.h>
#include <assert.h>

#include "config.h"
#include "private/utf8.h"

#if USE(GLIB)
#include <glib.h>

char *pcutils_strtoupper(const char *str, ssize_t len, size_t *len_new)
{
    char *new_str = g_utf8_strup(str, len);
    if (len_new)
        *len_new = strlen(new_str);
    return new_str;
}

char *pcutils_strtolower(const char *str, ssize_t len, size_t *len_new)
{
    char *new_str = g_utf8_strdown(str, len);
    if (len_new)
        *len_new = strlen(new_str);
    return new_str;
}

int pcutils_strncasecmp(const char *s1, const char *s2, size_t n)
{
#if 0
    int ret;
    gchar *g_haystack = g_utf8_strdown(haystack, - 1);
    gchar *g_needle =  g_utf8_strdown(needle, - 1);
    /* the length may change after calling g_utf8_strdown */
    len_haystack = strlen(g_haystack);
    len_needle = strlen(g_needle);
    ret = strncmp(g_haystack, g_needle, strlen(g_needle));
    g_free(g_haystack);
    g_free(g_needle);
    return ret;
#else
    (void)s1;
    (void)s2;
    (void)n;
    return 0;
#endif
}

char *pcutils_strcasestr(const char *haystack, const char *needle)
{
#if 0
    int result;

    gchar *g_haystack = g_utf8_strdown(haystack, -1);
    if (g_haystack == NULL)
        return -1;
    gchar *g_needle =  g_utf8_strdown(needle, -1);
    if (g_needle == NULL) {
        free(g_haystack);
        return -1;
    }

    result = strstr(g_haystack, g_needle) ? 0 : 1;

    g_free(g_haystack);
    g_free(g_needle);

    return result;
#else
    (void)haystack;
    (void)needle;
    return NULL;
#endif
}

char *pcutils_strreverse(const char *str, ssize_t len, size_t nr_chars)
{
    UNUSED_PARAM(nr_chars);
    return g_utf8_strreverse(str, len);
}

#else /* USE(GLIB) */

char *pcutils_strtoupper(const char *str, ssize_t len, size_t *len_new)
{
    size_t length;

    if (len < 0)
        length = strlen(str);
    else
        length = (size_t)len;

    char *new_str = strndup(str, length);

    if (new_str) {
        size_t n;
        for (n = 0; n < length && new_str[n] != '\0'; n++) {
            if (purc_islower(new_str[n]))
                new_str[n] = purc_toupper(new_str[n]);
        }

        if (len_new)
            *len_new = n;
    }

    return new_str;
}

char *pcutils_strtolower(const char *str, ssize_t len, size_t *len_new)
{
    size_t length;

    if (len < 0)
        length = strlen(str);
    else
        length = (size_t)len;

    char *new_str = strndup(str, length);

    if (new_str) {
        size_t n;
        for (n = 0; n < length && new_str[n] != '\0'; n++) {
            if (purc_isupper(new_str[n]))
                new_str[n] = purc_tolower(new_str[n]);
        }

        if (len_new)
            *len_new = n;
    }

    return new_str;
}

int pcutils_strncasecmp(const char *s1, const char *s2, size_t n)
{
    return strncasecmp(s1, s2, n);
}

char *pcutils_strcasestr(const char *haystack, const char *needle)
{
    char* p = (char *)haystack;

    while (*p) {

        int lower1, lower2;

        lower1 = purc_tolower(*p);
        lower2 = purc_tolower(*needle);

        if (lower1 == lower2) {
            const char *p1 = p, *p2 = needle;
            while (*p1 && *p2) {
                if (purc_tolower(*p1) != purc_tolower(*p2))
                    goto not_matched;

                p1++;
                p2++;
            }

            if (*p2)    // end of haystack
                goto done;

            /* matched */
            return p;
        }

not_matched:
        p++;
    }

done:
    return NULL;
}

char *pcutils_strreverse(const char *str, ssize_t len, size_t nr_chars)
{
    char *new_str;
    size_t length;

    if (len >= 0) {
        length = (size_t)len;
    }
    else {
        const char *p = str;
        const char *start = str;

        nr_chars = 0;
        while (*p) {
            p = pcutils_utf8_next_char(p);
            ++nr_chars;
        }
        length = p - start;
    }

    if (nr_chars == 0) {
        new_str = strdup("");
        if (new_str == NULL) {
            goto fatal;
        }
    }
    else if (nr_chars == length) {
        // ASCII string
        new_str = strndup(str, length);
        if (new_str == NULL) {
            goto fatal;
        }

        for (size_t i =  0; i < length >> 1; i++) {
            char tmp = new_str[length - i - 1];
            new_str[length - i - 1] = new_str[i];
            new_str[i] = tmp;
        }
    }
    else {
        uint32_t *ucs = malloc(sizeof(uint32_t) * nr_chars);
        if (ucs == NULL) {
            goto fatal;
        }

        size_t n = pcutils_string_decode_utf8(ucs, nr_chars, str);
        assert(n == nr_chars);

        for (size_t i =  0; i < n >> 1; i++) {
            uint32_t tmp = ucs[length - i - 1];
            ucs[length - i - 1] = ucs[i];
            ucs[i] = tmp;
        }

        new_str = pcutils_string_encode_utf8(ucs, n, &length);
        free(ucs);

        if (new_str == NULL) {
            goto fatal;
        }
    }

    return new_str;

fatal:
    return NULL;
}

#endif  /* !USE(GLIB) */

