/*
 * @file helper.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of tools for all files in this directory.
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

#include <math.h>

#include "private/instance.h"
#include "private/errors.h"
#include "private/dvobjs.h"
#include "private/atom-buckets.h"
#include "purc-utils.h"
#include "purc-variant.h"
#include "helper.h"

#include <regex.h>

#if USE(GLIB)
#include <glib.h>
#endif

char *pcdvobjs_remove_space(char *buffer)
{
    int i = 0;
    int j = 0;
    while (*(buffer + i) != 0x00) {
        if (*(buffer + i) != ' ') {
            *(buffer + j) = *(buffer + i);
            j++;
        }
        i++;
    }
    *(buffer + j) = 0x00;

    return buffer;
}

#if USE(GLIB)
bool pcdvobjs_wildcard_cmp(const char *pattern, const char *str)
{
    GPatternSpec *glib_pattern = g_pattern_spec_new(pattern);

#if GLIB_CHECK_VERSION(2, 70, 0)
    gboolean result = g_pattern_spec_match_string(glib_pattern, str);
#else
    gboolean result = g_pattern_match_string(glib_pattern, str);
#endif

    g_pattern_spec_free(glib_pattern);

    return (bool)result;
}

int pcdvobjs_wildcard_cmp_ex(const char *pattern,
        const char *strs[], int nr_strs)
{
    int matched = 0;

    GPatternSpec *glib_pattern = g_pattern_spec_new(pattern);

    for (int i = 0; i < MIN(nr_strs, 31) && strs[i] != NULL; i++) {
        gboolean result;
#if GLIB_CHECK_VERSION(2, 70, 0)
        result = g_pattern_spec_match_string(glib_pattern, strs[i]);
#else
        result = g_pattern_match_string(glib_pattern, strs[i]);
#endif
        if (result) {
            matched |= (0x01 << i);
        }
    }

    g_pattern_spec_free(glib_pattern);

    return matched;
}

#else
bool pcdvobjs_wildcard_cmp(const char *pattern, const char *str)
{
    if (str == NULL)
        return false;
    if (pattern == NULL)
        return false;

    size_t len1 = strlen(str);
    size_t len2 = strlen(pattern);
    size_t mark = 0;
    size_t p1 = 0;
    size_t p2 = 0;

    while ((p1 < len1) && (p2 < len2)) {
        if (pattern[p2] == '?') {
            p1++;
            p2++;
            continue;
        }
        if (pattern[p2] == '*') {
            p2++;
            mark = p2;
            continue;
        }
        if (str[p1] != pattern[p2]) {
            if (p1 == 0 && p2 == 0)
                return false;
            p1 -= p2 - mark - 1;
            p2 = mark;
            continue;
        }
        p1++;
        p2++;
    }
    if (p2 == len2) {
        if (p1 == len1)
            return true;
        if (pattern[p2 - 1] == '*')
            return true;
    }
    while (p2 < len2) {
        if (pattern[p2] != '*')
            return false;
        p2++;
    }
    return true;
}

int pcdvobjs_wildcard_cmp_ex(const char *pattern,
        const char *strs[], int nr_strs)
{
    int matched = 0;

    for (int i = 0; i < MIN(nr_strs, 31) && strs[i] != NULL; i++) {
        if (pcdvobjs_wildcard_cmp(pattern, strs[i])) {
            matched |= (0x01 << i);
        }
    }

    return matched;
}

#endif

static bool
init_regex(regex_t *regex, const char *pattern, int *eflags)
{
    assert(pattern);

    int cflags = REG_EXTENDED | REG_NOSUB;
    const char *_pattern = pattern;

    *eflags = REG_NOTBOL | REG_NOTEOL;
    if (pattern[0] == '/') {
        _pattern = strdup(pattern + 1);

        char *last_slash = strrchr(_pattern, '/');
        if (last_slash != NULL) {
            /* check the flags: */
            const char *flags = last_slash + 1;
            while (*flags) {

                switch (*flags) {
                    case 'i':
                        cflags |= REG_ICASE;
                        break;
                    case 's':
                        cflags |= REG_NEWLINE;
                        break;
                    case 'm':
                        *eflags &= ~(REG_NOTBOL | REG_NOTEOL);
                }

                flags++;
            }

            /* remove the last slash */
            *last_slash = 0;
        }
    }

    bool result = true;
    if (regcomp(regex, _pattern, cflags) != 0) {
        result = false;
    }

    if (_pattern != pattern)
        free((void *)_pattern);

    return result;
}

bool pcdvobjs_regex_cmp(const char *pattern, const char *str)
{
    regex_t regex;
    int eflags;

    assert(str);

    if (!init_regex(&regex, pattern, &eflags))
        goto error;

    bool result = false;
    if (regexec(&regex, str, 0, NULL, eflags) == 0) {
        result = true;
    }

    regfree(&regex);
    return result;

error:
    return false;
}

int pcdvobjs_regex_cmp_ex(const char *pattern, const char *strs[], int nr_strs)
{
    regex_t regex;
    int eflags;
    int matched = 0;

    if (!init_regex(&regex, pattern, &eflags)) {
        goto error;
    }

    assert(strs);
    for (int i = 0; i < MIN(nr_strs, 31) && strs[i] != NULL; i++) {
        if (regexec(&regex, strs[i], 0, NULL, eflags) == 0) {
            matched |= (0x01 << i);
        }
    }

    regfree(&regex);
    return matched;

error:
    return -1;
}

int pcdvobjs_match_events(const char *main_pattern, const char *sub_pattern,
        const char *events[], int nr_events)
{
    int matched = 0;

    assert(main_pattern);

    const char *pattern;
    if (sub_pattern == NULL) {
        pattern = main_pattern;
    }
    else {
        char *p = malloc(strlen(main_pattern) + strlen(sub_pattern) + 2);
        pattern = p;
        p = stpcpy(p, main_pattern);
        *p = ':';
        p++;
        strcpy(p, sub_pattern);
    }

    if (pattern[0] == '/') { /* regexp */
        matched = pcdvobjs_regex_cmp_ex(pattern, events, nr_events);
    }
    else if (strchr(pattern, '*') || strchr(pattern, '/')) { /* wildcard */
        matched = pcdvobjs_wildcard_cmp_ex(pattern, events, nr_events);
    }
    else { /* plain */
        for (int i = 0; i < MIN(nr_events, 31) && events[i] != NULL; i++) {
            if (strcmp(events[i], pattern) == 0) {
                matched |= (0x01 << i);
            }
        }
    }

    if (pattern != main_pattern)
        free((void *)pattern);

    return matched;
}

purc_variant_t
purc_dvobj_make_from_methods(const struct purc_dvobj_method *methods,
        size_t size)
{
    size_t i = 0;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t ret_var = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    if (ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (i = 0; i < size; i++) {
        val = purc_variant_make_dynamic(methods[i].getter, methods[i].setter);
        if (val == PURC_VARIANT_INVALID) {
            goto error;
        }

        if (!purc_variant_object_set_by_static_ckey(ret_var,
                    methods[i].name, val)) {
            goto error;
        }

        purc_variant_unref(val);
    }

    return ret_var;

error:
    purc_variant_unref(ret_var);
    return PURC_VARIANT_INVALID;
}

bool pcdvobjs_cast_to_timeval(struct timeval *timeval, purc_variant_t t)
{
    switch (purc_variant_get_type(t)) {
    case PURC_VARIANT_TYPE_NUMBER:
    {
        double time_d, sec_d, usec_d;

        purc_variant_cast_to_number(t, &time_d, false);
        if (isinf(time_d) || isnan(time_d)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        usec_d = modf(time_d, &sec_d);
        timeval->tv_sec = (time_t)sec_d;
        timeval->tv_usec = (suseconds_t)(usec_d * 1000000.0);
        break;
    }

    case PURC_VARIANT_TYPE_LONGINT:
    case PURC_VARIANT_TYPE_ULONGINT:
    {
        int64_t sec;
        if (!purc_variant_cast_to_longint(t, &sec, false)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        timeval->tv_usec = (time_t)sec;
        timeval->tv_usec = 0;
        break;
    }

    case PURC_VARIANT_TYPE_LONGDOUBLE:
    {
        long double time_d, sec_d, usec_d;
        purc_variant_cast_to_longdouble(t, &time_d, false);

        if (isinf(time_d) || isnan(time_d)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        usec_d = modfl(time_d, &sec_d);
        timeval->tv_sec = (time_t)sec_d;
        timeval->tv_usec = (suseconds_t)(usec_d * 1000000.0);
        break;
    }

    default:
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    return true;

failed:
    return false;
}

int pcdvobjs_parse_options(purc_variant_t vrt,
        const struct pcdvobjs_option_to_atom *single_options, size_t nr_sopt,
        const struct pcdvobjs_option_to_atom *composite_options, size_t nr_copt,
        int flags4null, int flags4failed)
{
    int flags = 0;

    if (vrt == PURC_VARIANT_INVALID) {
        flags = flags4null;
        goto done;
    }

    purc_atom_t atom = 0;
    const char *opts;
    size_t opts_len;
    opts = purc_variant_get_string_const_ex(vrt, &opts_len);
    if (!opts) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    opts = pcutils_trim_spaces(opts, &opts_len);
    if (opts_len == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }
    else {
        char tmp[opts_len + 1];
        strncpy(tmp, opts, opts_len);
        tmp[opts_len]= '\0';
        atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
    }

    /* try single options first */
    if (single_options) {
        for (size_t i = 0; i < nr_sopt; i++) {
            if (atom == single_options[i].atom) {
                flags = single_options[i].flag;
                goto done;
            }
        }

        if (composite_options == NULL) {
            /* not matched single option. */
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    /* try composite options then */
    if (composite_options) {
        const char *opt;
        size_t opt_len;
        foreach_keyword(opts, opts_len, opt, opt_len) {
            char tmp[opt_len + 1];
            strncpy(tmp, opt, opt_len);
            tmp[opt_len]= '\0';
            atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);

            size_t i = 0;
            for (; i < nr_copt; i++) {
                if (atom == composite_options[i].atom) {
                    flags |= composite_options[i].flag;
                    break;
                }
            }

            if (i == nr_copt) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
        }
    }

done:
    return flags;

failed:
    return flags4failed;
}

// Implementation of Punycode encoding according to RFC 3492

#define BASE 36
#define TMIN 1
#define TMAX 26
#define SKEW 38
#define DAMP 700
#define INITIAL_BIAS 72
#define INITIAL_N 128

static const char encoding_digits[] = "abcdefghijklmnopqrstuvwxyz0123456789";

// Adapt bias for the next delta
static int adapt(int delta, int numpoints, int firsttime)
{
    delta = firsttime ? delta / DAMP : delta >> 1;
    delta += delta / numpoints;

    int k = 0;
    while (delta > ((BASE - TMIN) * TMAX) / 2) {
        delta /= BASE - TMIN;
        k += BASE;
    }

    return k + (((BASE - TMIN + 1) * delta) / (delta + SKEW));
}

// Encode a UTF-8 string to Punycode
static int punycode_encode(struct pcutils_mystring *output,
        const char* orig, size_t input_len)
{
    if (input_len == 0)
        return 0;

    // Convert input UTF-8 to code points
    size_t nr_ucs;
    uint32_t *input;
    input = pcutils_string_decode_utf8_alloc(orig, input_len, &nr_ucs);
    if (input == NULL) {
        return -1;
    }

    size_t basic_len = 0;
    for (size_t i = 0; i < nr_ucs; i++) {
        if (input[i] < 0x80) {
            if (pcutils_mystring_append_char(output, input[i]))
                goto failed;
            basic_len++;
        }
    }

    // add a delimeter
    if (basic_len > 0) {
        if (pcutils_mystring_append_char(output, '-'))
            goto failed;
    }

    // Main encoding loop
    uint32_t n = INITIAL_N;
    size_t h = basic_len;
    int bias = INITIAL_BIAS;
    int delta = 0;

    while (h < nr_ucs) {
        // Find the smallest code point >= n
        uint32_t min_cp = 0x10FFFF;
        for (size_t j = 0; j < nr_ucs; j++) {
            if (input[j] >= n && input[j] < min_cp) {
                min_cp = input[j];
            }
        }

        delta += (min_cp - n) * (h + 1);
        n = min_cp;

        // Encode all code points < min_cp
        for (size_t j = 0; j < nr_ucs; j++) {
            if (input[j] < min_cp) {
                delta++;
            }
            else if (input[j] == min_cp) {
                // Encode delta
                int q = delta;
                for (int k = BASE;; k += BASE) {
                    int t = k <= bias ? TMIN : k >= bias +
                        TMAX ? TMAX : k - bias;
                    if (q < t)
                        break;

                    if (pcutils_mystring_append_char(output,
                            encoding_digits[t + ((q - t) % (BASE - t))]))
                        goto failed;
                    q = (q - t) / (BASE - t);
                }

                if (pcutils_mystring_append_char(output, encoding_digits[q]))
                    goto failed;

                bias = adapt(delta, h + 1, h == basic_len);
                delta = 0;
                h++;
            }
        }

        delta++;
        n++;
    }

    if (input)
        free(input);
    return 0;

failed:
    if (input)
        free(input);
    return -1;
}

// Encode a UTF-8 domain name to Punycode
int pcutils_punycode_encode(struct pcutils_mystring *output, const char* orig)
{
    if (orig == NULL)
        goto done;

    const char *p = orig;
    const char *comp_start = NULL;
    while (true) {
        const char *next = pcutils_utf8_next_char(p);
        size_t len = next - p;
        if (len == 1 && (*p == '.' || *p == '\0')) {
            if (comp_start) {
                size_t comp_len = p - comp_start;
                size_t nr_ucs;
                if (!pcutils_string_check_utf8(comp_start, comp_len, &nr_ucs,
                            NULL))
                    goto failed;

                /* no any non-ASCII character */
                if (comp_len == nr_ucs) {
                    pcutils_mystring_append_mchar(output,
                            (const unsigned char*)comp_start, comp_len);
                }
                else {
                    pcutils_mystring_append_string(output, "xn--");
                    if (punycode_encode(output, comp_start, comp_len))
                        goto failed;
                }

                comp_start = NULL;
            }

            if (*p == '\0')
                break;

            if (pcutils_mystring_append_char(output, '.'))
                goto failed;
        }
        else if (comp_start == NULL) {
            comp_start = p;
        }

        p = next;
    }

done:
    return 0;

failed:
    return -1;
}

// Decode a Punycode string back to UTF-8
int punycode_decode(struct pcutils_mystring *output,
        const char *punycode, size_t input_len)
{
    if (input_len == 0)
        goto failed;

    // Find delimiter position
    const char* end = punycode + input_len;
    const char* delimiter = strchr(punycode, '-');
    size_t basic_len;
    if (delimiter != NULL && delimiter < end) {
        basic_len = delimiter - punycode;
    }
    else
        basic_len = 0;

    // Copy basic code points
    if (basic_len > 0 && pcutils_mystring_append_mchar(output,
                (const unsigned char *)punycode, basic_len)) {
        goto failed;
    }

    size_t nr_output = basic_len;

    // Main decoding loop
    int i = 0;
    int n = INITIAL_N;
    int bias = INITIAL_BIAS;
    size_t pos = basic_len > 0 ? basic_len + 1 : 0;

    while (pos < input_len) {
        int org_i = i;
        int w = 1;
        int k = BASE;

        while (pos < input_len) {
            char c = punycode[pos++];
            int digit;

            if (c >= '0' && c <= '9') {
                digit = c - '0' + 26;
            }
            else if (c >= 'a' && c <= 'z') {
                digit = c - 'a';
            }
            else {
                // invalid character
                goto failed;
            }

            i += digit * w;

            int t;
            if (k <= bias) {
                t = TMIN;
            }
            else if (k >= bias + TMAX) {
                t = TMAX;
            }
            else {
                t = k - bias;
            }

            if (digit < t)
                break;

            w *= BASE - t;
            k += BASE;
        }

        bias = adapt(i - org_i, nr_output + 1, org_i == 0);

        n += i / (nr_output + 1);
        // check Unicode range
        if (n > 0x10FFFF)
            goto failed;

        i %= (nr_output + 1);
        if (nr_output > (size_t)i) {
            if (pcutils_mystring_append_uchar(output, n, 1))
                goto failed;

            printf("call memmove() here\n");
            // memmove(dst + i + 1, dst + i, (di - i) * sizeof(uint32_t));
        }
        else {
            if (pcutils_mystring_append_uchar(output, n, 1))
                goto failed;
        }

        i++;
        nr_output++;
    }

    return 0;

failed:
    return -1;
}

// Decode a Punycode domain name back to UTF-8
int
pcutils_punycode_decode(struct pcutils_mystring *output, const char* punycode)
{
    if (punycode == NULL)
        goto done;

    const char *p = punycode;
    const char *comp_start = NULL;
    while (true) {
        const char *next = pcutils_utf8_next_char(p);
        size_t len = next - p;
        if (len == 1 && (*p == '.' || *p == '\0')) {
            if (comp_start) {
                size_t comp_len = p - comp_start;
                if (strncmp(comp_start, "xn--", 4) == 0) {
                    if (punycode_decode(output, comp_start + 4, comp_len - 4))
                        goto failed;
                }
                else {
                    // ASCII characters
                    pcutils_mystring_append_mchar(output,
                            (const unsigned char*)comp_start, comp_len);
                }

                comp_start = NULL;
            }

            if (*p == '\0')
                break;

            if (pcutils_mystring_append_char(output, '.'))
                goto failed;
        }
        else if (comp_start == NULL) {
            comp_start = p;
        }

        p = next;
    }

done:
    return 0;

failed:
    return -1;
}

