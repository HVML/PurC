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
 *
 * Note that some code come from GLIB (<https://github.com/GNOME/glib>),
 * which is governed by LGPLv2. The copyright owners:
 *
 * Copyright (C) 1999 Tom Tromey
 * Copyright (C) 2000 Red Hat, Inc.
 */

// #undef NDEBUG

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

typedef enum {
  LOCALE_NORMAL,
  LOCALE_TURKIC,
  LOCALE_LITHUANIAN
} locale_type;

static locale_type get_locale_type(void)
{
    const char *locale = setlocale(LC_CTYPE, NULL);
    if (locale == NULL)
        return LOCALE_NORMAL;

    switch (locale[0]) {
    case 'a':
        if (locale[1] == 'z')
            return LOCALE_TURKIC;
        break;
    case 'l':
        if (locale[1] == 't')
            return LOCALE_LITHUANIAN;
        break;
    case 't':
        if (locale[1] == 'r')
            return LOCALE_TURKIC;
        break;
    }

    return LOCALE_NORMAL;
}

#define G_UNICHAR_FULLWIDTH_A 0xff21
#define G_UNICHAR_FULLWIDTH_I 0xff29
#define G_UNICHAR_FULLWIDTH_J 0xff2a
#define G_UNICHAR_FULLWIDTH_F 0xff26
#define G_UNICHAR_FULLWIDTH_a 0xff41
#define G_UNICHAR_FULLWIDTH_f 0xff46

/* traverses the string checking for characters with combining class == 230
 * until a base character is found */
static bool
has_more_above(const char *str)
{
    const char *p = str;
    int combining_class;

    while (*p) {
        combining_class = g_unichar_combining_class(g_utf8_get_char(p));
        if (combining_class == 230)
            return true;
        else if (combining_class == 0)
            break;

        p = g_utf8_next_char(p);
    }

    return false;
}

#define MAX_LOWER_CHARS     3

static size_t
utf8_char_to_lower(locale_type lt, const char *p, gunichar *ucs)
{
    gunichar c = g_utf8_get_char(p);
    size_t len = 0;

    // lower characters;
    memset(ucs, 0, sizeof(uint32_t) * MAX_LOWER_CHARS);

    const char *last = p;
    p = g_utf8_next_char(p);

    if (lt == LOCALE_TURKIC && (c == 'I' || c == 0x130 ||
                c == G_UNICHAR_FULLWIDTH_I)) {

        bool combining_dot = (c == 'I' || c == G_UNICHAR_FULLWIDTH_I) &&
            g_utf8_get_char(p) == 0x0307;

        if (combining_dot || c == 0x130) {
            /* I + COMBINING DOT ABOVE => i (U+0069)
             * LATIN CAPITAL LETTER I WITH DOT ABOVE => i (U+0069) */
            ucs[0] = 0x0069;
            if (combining_dot)
                len += _pcutils_utf8_skip[*(unsigned char *)p];
        }
        else {
            /* I => LATIN SMALL LETTER DOTLESS I */
            ucs[0] = 0x131;
        }
    }
    /* Introduce an explicit dot above when lowercasing capital I's and J's
     * whenever there are more accents above. [SpecialCasing.txt] */
    else if (lt == LOCALE_LITHUANIAN &&
            (c == 0x00cc || c == 0x00cd || c == 0x0128))
    {
        ucs[0] = 0x0069;
        ucs[1] = 0x0307;

        switch (c) {
        case 0x00cc:
            ucs[2] = 0x0300;
            break;
        case 0x00cd:
            ucs[2] = 0x0301;
            break;
        case 0x0128:
            ucs[2] = 0x0303;
            break;
        }
    }
    else if (lt == LOCALE_LITHUANIAN &&
            (c == 'I' || c == G_UNICHAR_FULLWIDTH_I ||
             c == 'J' || c == G_UNICHAR_FULLWIDTH_J || c == 0x012e) &&
            has_more_above(p))
    {
        ucs[0] = g_unichar_tolower(c);
        ucs[1] = 0x0307;
    }
    else if (c == 0x03A3) {
        /* GREEK CAPITAL LETTER SIGMA */
        if (*p) {
            gunichar next_c = g_utf8_get_char(p);

            /* SIGMA mapps differently depending on whether it is
             * final or not. The following simplified test would
             * fail in the case of combining marks following the
             * sigma, but I don't think that occurs in real text.
             * The test here matches that in ICU.
             */
            if (g_unichar_isalpha(next_c)) /* Lu,Ll,Lt,Lm,Lo */
                ucs[0] = 0x3c3;     /* GREEK SMALL SIGMA */
            else
                ucs[0] = 0x3c2;     /* GREEK SMALL FINAL SIGMA */
        }
        else
            ucs[0] = 0x3c2;         /* GREEK SMALL FINAL SIGMA */
    }
    else {
        ucs[0] = g_unichar_tolower(c);
    }

    len += _pcutils_utf8_skip[*(unsigned char *)last];

    return len;
}

int pcutils_strncasecmp(const char *s1, const char *s2, size_t n)
{
    gunichar ucs1[MAX_LOWER_CHARS];
    gunichar ucs2[MAX_LOWER_CHARS];

    locale_type lt = get_locale_type();

    while (n > 0) {
        size_t len1 = utf8_char_to_lower(lt, s1, ucs1);
        size_t len2 = utf8_char_to_lower(lt, s2, ucs2);

        int diff = memcmp(ucs1, ucs2, sizeof(ucs1));
        if (diff) {
            return diff;
        }

        if (n > MAX(len1, len2)) {
            n -= MAX(len1, len2);
        }
        else
            n = 0;

        s1 += len1;
        s2 += len2;
    }

    return 0;
}

char *pcutils_strcasestr(const char *haystack, const char *needle)
{
    locale_type lt = get_locale_type();
    gunichar ucs1[MAX_LOWER_CHARS];
    gunichar ucs2[MAX_LOWER_CHARS];

    char* p = (char *)haystack;
    while (*p) {

        size_t len1 = utf8_char_to_lower(lt, haystack, ucs1);
        size_t len2 = utf8_char_to_lower(lt, needle, ucs2);

        int diff = memcmp(ucs1, ucs2, sizeof(ucs1));
        if (diff == 0) {
            const char *p1 = p + len1;
            const char *p2 = needle + len2;
            while (*p1 && *p2) {
                len1 = utf8_char_to_lower(lt, p1, ucs1);
                len2 = utf8_char_to_lower(lt, p2, ucs2);

                if (memcmp(ucs1, ucs2, sizeof(ucs1)))
                    goto not_matched;

                p1 += len1;
                p2 += len2;
            }

            if (*p1 == 0 && *p2)    // end of haystack
                goto done;

            /* matched */
            return p;
        }

not_matched:
        p += len1;
    }

done:
    return NULL;
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
            const char *p1 = p + 1, *p2 = needle + 1;
            while (*p1 && *p2) {
                if (purc_tolower(*p1) != purc_tolower(*p2))
                    goto not_matched;

                p1++;
                p2++;
            }

            if (*p1 == 0 && *p2)    // end of haystack
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

#endif  /* !USE(GLIB) */

