/*
 * @file serializer.c
 * @author Vincent Wei
 * @date 2021/07/12
 * @brief The serializer of variant.
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
 *
 * Note that some codes in this file is derived from json-c which is licensed
 * under MIT Licence.
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
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

#include "purc-variant.h"
#include "purc-rwstream.h"
#include "private/variant.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"

#include "variant/variant-internals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <assert.h>

static const char *hex_chars = "0123456789abcdefABCDEF";

#define MY_WRITE(rws, buff, count)                                      \
    do {                                                                \
        const char* _buff = buff;                                       \
        ssize_t n;                                                      \
        size_t nr_left = (count);                                       \
        if (len_expected)                                               \
            *len_expected += (count);                                   \
        while (1) {                                                     \
            n = purc_rwstream_write((rws), _buff, nr_left);             \
            if (n <= 0) {                                               \
                if (flags & PCVRNT_SERIALIZE_OPT_IGNORE_ERRORS)      \
                    break;                                              \
                else                                                    \
                    goto failed;                                        \
            }                                                           \
            else if ((size_t)n < nr_left) {                             \
                nr_written += n;                                        \
                _buff += n;                                             \
                continue;                                               \
            }                                                           \
            else {                                                      \
                nr_written += n;                                        \
                break;                                                  \
            }                                                           \
        }                                                               \
    } while (0)

#define MY_CHECK(n)                                                     \
    do {                                                                \
        if ((n) < 0 &&                                                  \
                !(flags & PCVRNT_SERIALIZE_OPT_IGNORE_ERRORS)) {     \
            goto failed;                                                \
        }                                                               \
        else {                                                          \
            nr_written += (n);                                          \
        }                                                               \
    } while (0)

static ssize_t
serialize_string(purc_rwstream_t rws, const char* str,
        size_t len, unsigned int flags, size_t *len_expected)
{
    int nr_written = 0;
    size_t pos = 0, start_offset = 0;
    unsigned char c;
    char buff[3];

    while (len--) {
        c = str[pos];
        switch (c) {
        case '\b':
        case '\n':
        case '\r':
        case '\t':
        case '\f':
        case '"':
        case '\\':
        case '/':
            if ((flags & PCVRNT_SERIALIZE_OPT_NOSLASHESCAPE) && c == '/') {
                pos++;
                break;
            }

            if (pos - start_offset > 0)
                MY_WRITE(rws, str + start_offset, pos - start_offset);

            buff[0] = 0;
            if (c == '\b') {
                buff[0] = '\\';
                buff[1] = 'b';
            }
            else if (c == '\n') {
                buff[0] = '\\';
                buff[1] = 'n';
            }
            else if (c == '\r') {
                buff[0] = '\\';
                buff[1] = 'r';
            }
            else if (c == '\t') {
                buff[0] = '\\';
                buff[1] = 't';
            }
            else if (c == '\f') {
                buff[0] = '\\';
                buff[1] = 'f';
            }
            else if (c == '"') {
                buff[0] = '\\';
                buff[1] = '\"';
            }
            else if (c == '\\') {
                buff[0] = '\\';
                buff[1] = '\\';
            }
            else if (c == '/') {
                buff[0] = '\\';
                buff[1] = '/';
            }

            if (buff[0])
                MY_WRITE(rws, buff, 2);

            start_offset = ++pos;
            break;

        default:
            if (c < ' ') {
                char sbuf[7];
                if (pos - start_offset > 0)
                    MY_WRITE(rws, str + start_offset, pos - start_offset);

                if (snprintf(sbuf, sizeof(sbuf), "\\u00%c%c",
                            hex_chars[c >> 4], hex_chars[c & 0xf]) < 0)
                    goto failed;

                MY_WRITE(rws, sbuf, sizeof(sbuf) - 1);
                start_offset = ++pos;
            }
            else
                pos++;
        }
    }
    if (pos - start_offset > 0)
        MY_WRITE(rws, str + start_offset, pos - start_offset);

    return nr_written;

failed:
    return -1;
}

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64_pad = '=';

/* (From RFC1521 and draft-ietf-dnssec-secext-03.txt)
   The following encoding technique is taken from RFC 1521 by Borenstein
   and Freed.  It is reproduced here in a slightly edited form for
   convenience.

   A 65-character subset of US-ASCII is used, enabling 6 bits to be
   represented per printable character. (The extra 65th character, "=",
   is used to signify a special processing function.)

   The encoding process represents 24-bit groups of input bits as output
   strings of 4 encoded characters. Proceeding from left to right, a
   24-bit input group is formed by concatenating 3 8-bit input groups.
   These 24 bits are then treated as 4 concatenated 6-bit groups, each
   of which is translated into a single digit in the base64 alphabet.

   Each 6-bit group is used as an index into an array of 64 printable
   characters. The character referenced by the index is placed in the
   output string.

                         Table 1: The Base64 Alphabet

      Value Encoding  Value Encoding  Value Encoding  Value Encoding
          0 A            17 R            34 i            51 z
          1 B            18 S            35 j            52 0
          2 C            19 T            36 k            53 1
          3 D            20 U            37 l            54 2
          4 E            21 V            38 m            55 3
          5 F            22 W            39 n            56 4
          6 G            23 X            40 o            57 5
          7 H            24 Y            41 p            58 6
          8 I            25 Z            42 q            59 7
          9 J            26 a            43 r            60 8
         10 K            27 b            44 s            61 9
         11 L            28 c            45 t            62 +
         12 M            29 d            46 u            63 /
         13 N            30 e            47 v
         14 O            31 f            48 w         (pad) =
         15 P            32 g            49 x
         16 Q            33 h            50 y

   Special processing is performed if fewer than 24 bits are available
   at the end of the data being encoded.  A full encoding quantum is
   always completed at the end of a quantity.  When fewer than 24 input
   bits are available in an input group, zero bits are added (on the
   right) to form an integral number of 6-bit groups.  Padding at the
   end of the data is performed using the '=' character.

   Since all base64 input is an integral number of octets, only the
         -------------------------------------------------
   following cases can arise:

       (1) the final quantum of encoding input is an integral
           multiple of 24 bits; here, the final unit of encoded
       output will be an integral multiple of 4 characters
       with no "=" padding,
       (2) the final quantum of encoding input is exactly 8 bits;
           here, the final unit of encoded output will be two
       characters followed by two "=" padding characters, or
       (3) the final quantum of encoding input is exactly 16 bits;
           here, the final unit of encoded output will be three
       characters followed by one "=" padding character.
   */

static ssize_t serialize_bsequence_base64(purc_rwstream_t rws,
        const void *_src, size_t srclength,
        unsigned int flags, size_t *len_expected)
{
    const unsigned char *src = _src;
    ssize_t nr_written = 0;
    uint8_t input[3] = {0};
    uint8_t output[4];
    char buff[4];
    size_t i;

    while (2 < srclength) {
        input[0] = *src++;
        input[1] = *src++;
        input[2] = *src++;
        srclength -= 3;

        output[0] = input[0] >> 2;
        output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
        output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
        output[3] = input[2] & 0x3f;

        buff[0] = base64_chars[output[0]];
        buff[1] = base64_chars[output[1]];
        buff[2] = base64_chars[output[2]];
        buff[3] = base64_chars[output[3]];

        MY_WRITE(rws, buff, 4);
    }

    /* Now we worry about padding. */
    if (0 != srclength) {
        /* Get what's left. */
        input[0] = input[1] = input[2] = '\0';
        for (i = 0; i < srclength; i++)
            input[i] = *src++;

        output[0] = input[0] >> 2;
        output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
        output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

        buff[0] = base64_chars[output[0]];
        buff[1] = base64_chars[output[1]];
        if (srclength == 1)
            buff[2] = base64_pad;
        else
            buff[2] = base64_chars[output[2]];
        buff[3] = base64_pad;

        MY_WRITE(rws, buff, 4);
    }

    return nr_written;

failed:
    return -1;
}

static ssize_t
serialize_bsequence(purc_rwstream_t rws, const char* content,
        size_t sz_content, unsigned int flags, size_t *len_expected)
{
    ssize_t nr_written = 0, n;
    size_t i;

    switch (flags & PCVRNT_SERIALIZE_OPT_BSEQUENCE_MASK) {
        case PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX_STRING:
            MY_WRITE(rws, "\"", 1);
            for (i = 0; i < sz_content; i++) {
                unsigned char byte = (unsigned char)content[i];
                char buff[2];
                buff [0] = hex_chars[(byte >> 4) & 0x0f];
                buff [1] = hex_chars[byte & 0x0f];
                MY_WRITE(rws, buff, 2);
            }
            MY_WRITE(rws, "\"", 1);
            break;

        case PCVRNT_SERIALIZE_OPT_BSEQUENCE_HEX:
            MY_WRITE(rws, "bx", 2);
            for (i = 0; i < sz_content; i++) {
                unsigned char byte = (unsigned char)content[i];
                char buff[2];
                buff [0] = hex_chars[(byte >> 4) & 0x0f];
                buff [1] = hex_chars[byte & 0x0f];
                MY_WRITE(rws, buff, 2);
            }
            break;

        case PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN:
        case PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT:
            MY_WRITE(rws, "bb", 2);
            for (i = 0; i < sz_content; i++) {
                unsigned char byte = (unsigned char)content[i];
                char buff[10];
                size_t k = 0;
                for (int j = 0; j < 8; j++) {
                    if (byte & 0x80 >> j)
                        buff[k] = '1';
                    else
                        buff[k] = '0';
                    k++;

                    if ((flags & PCVRNT_SERIALIZE_OPT_BSEQUENCE_MASK) ==
                            PCVRNT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT &&
                            (j == 3 || (j == 7 && i != sz_content - 1))) {
                        buff[k] = '.';
                        k++;
                    }
                }

                MY_WRITE(rws, buff, k);
            }
            break;

        case PCVRNT_SERIALIZE_OPT_BSEQUENCE_BASE64:
        default:
            MY_WRITE(rws, "b64", 3);
            n = serialize_bsequence_base64(rws, content, sz_content,
                    flags, len_expected);
            MY_CHECK(n);
            break;
    }

    return nr_written;

failed:
    return -1;
}

/* securely comparison of floating-point variables */
static inline UNUSED_FUNCTION bool equal_doubles(double a, double b)
{
    double max_val = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= max_val * DBL_EPSILON);
}

/* securely comparison of floating-point variables */
static inline UNUSED_FUNCTION bool equal_long_doubles(long double a,
        long double b)
{
    long double max_val = fabsl(a) > fabsl(b) ? fabsl(a) : fabsl(b);
    return (fabsl(a - b) <= max_val * LDBL_EPSILON);
}

/* strlen of character literals resolved at compile time */
#define static_strlen(string_literal) (sizeof(string_literal) - sizeof(""))

static ssize_t
serialize_number(purc_rwstream_t rws, double d, size_t *len_expected)
{
    char buf[128];
    int size;

    /* Although JSON RFC does not support
     * NaN or Infinity as numeric values
     * ECMA 262 section 9.8.1 defines
     * how to handle these cases as strings
     */
    if (isnan(d)) {
        strcpy(buf, "NaN");
        size = static_strlen("NaN");
    }
    else if (isinf(d)) {
        if (d > 0) {
            strcpy(buf, "Infinity");
            size = static_strlen("Infinity");
        }
        else {
            strcpy(buf, "-Infinity");
            size = static_strlen("-Infinity");
        }
    }
    else {
        double test;

        /* try to format the double without decimals */
        size = snprintf(buf, sizeof(buf), "%.0f", d);
        if (size < 0 || size >= (int)sizeof(buf)) {
            pcinst_set_error(PURC_ERROR_TOO_SMALL_BUFF);
            return -1;
        }

        /* Check whether the original double can be recovered */
        if ((sscanf(buf, "%lg", &test) != 1) || !equal_doubles(test, d)) {
            /* If not, return 0 and call serialize_double */
            return 0;
        }
    }

    if (len_expected)
        *len_expected += size;
    return purc_rwstream_write(rws, buf, size);
}

static ssize_t
serialize_double(purc_rwstream_t rws, double d, int flags,
        const char *format, size_t *len_expected)
{
    char buf[128], *p, *q;
    int size;

    static const char *std_format = "%.17g";
    int format_drops_decimals = 0;
    int looks_numeric = 0;

    if (!format) {
        format = std_format;
    }

    size = snprintf(buf, sizeof(buf), format, d);
    // although unlikely, snprintf might fail
    if (UNLIKELY(size < 0)) {
        pcinst_set_error(PURC_ERROR_OUTPUT);
        return -1;
    }

    p = strchr(buf, ',');
    if (p)
        *p = '.';
    else
        p = strchr(buf, '.');

    if (format == std_format || strstr(format, ".0f") == NULL)
        format_drops_decimals = 1;

    looks_numeric = /* Looks like *some* kind of number */
        purc_isdigit((unsigned char)buf[0]) ||
        (size > 1 && buf[0] == '-' && purc_isdigit((unsigned char)buf[1]));

    if (size < (int)sizeof(buf) - 2 &&
            looks_numeric && !p && /* Has no decimal point */
            strchr(buf, 'e') == NULL && /* Not scientific notation */
            format_drops_decimals) {
        // Ensure it looks like a float, even if snprintf didn't,
        // unless a custom format is set to omit the decimal.
        strcat(buf, ".0");
        size += 2;
    }

    if (p && (flags & PCVRNT_SERIALIZE_OPT_NOZERO)) {
        /* last useful digit, always keep 1 zero */
        p++;
        for (q = p; *q; q++) {
            if (*q != '0')
                p = q;
        }
        /* drop trailing zeroes */
        if (*p != 0)
            *(++p) = 0;
        size = p - buf;
    }

    if (size >= (int)sizeof(buf))
        // The standard formats are guaranteed not to overrun the buffer,
        // but if a custom one happens to do so, just silently truncate.
        size = sizeof(buf) - 1;

    if (len_expected)
        *len_expected += size;
    return purc_rwstream_write(rws, buf, size);
}

static ssize_t
serialize_long_double(purc_rwstream_t rws, long double ld, int flags,
        const char *format, size_t *len_expected)
{
    char buf[256], *p, *q;
    int size;

    if (isnan(ld)) {
        strcpy(buf, "NaN");
        size = static_strlen("NaN");
    }
    else if (isinf(ld)) {
        if (ld > 0) {
            strcpy(buf, "Infinity");
            size = static_strlen("Infinity");
        }
        else {
            strcpy(buf, "-Infinity");
            size = static_strlen("-Infinity");
        }
    }
    else {
        static const char *std_format = "%.17Lg";
        if (!format) {
            format = std_format;
        }

        size = snprintf(buf, sizeof(buf) - 2, format, ld);
        if (UNLIKELY(size < 0)) {
            pcinst_set_error(PURC_ERROR_OUTPUT);
            return -1;
        }

        if (size >= (int)sizeof(buf) - 2) {
            // The standard formats are guaranteed not to overrun the buffer,
            // but if a custom one happens to do so, just silently truncate.
            size = sizeof(buf) - 3;
            buf[size] = 0;
        }

        p = strchr(buf, ',');
        if (p)
            *p = '.';
        else
            p = strchr(buf, '.');

        if (p && (flags & PCVRNT_SERIALIZE_OPT_NOZERO)) {
            /* last useful digit, always keep 1 zero */
            p++;
            for (q = p; *q; q++) {
                if (*q != '0')
                    p = q;
            }
            /* drop trailing zeroes */
            if (*p != 0)
                *(++p) = 0;
            size = p - buf;
        }

        // append FL postfix
        if (flags & PCVRNT_SERIALIZE_OPT_REAL_EJSON) {
            strcat(buf, "FL");
            size += 2;
        }
    }

    if (len_expected)
        *len_expected += size;
    return purc_rwstream_write(rws, buf, size);
}

static ssize_t
print_newline(purc_rwstream_t rws, unsigned int flags, size_t *len_expected)
{
    ssize_t nr_written = 0;

    if (flags & PCVRNT_SERIALIZE_OPT_PRETTY) {
        if (len_expected)
            *len_expected += 1;
        nr_written = purc_rwstream_write(rws, "\n", 1);
    }

    return nr_written;
}

static ssize_t
print_indent(purc_rwstream_t rws, int level, unsigned int flags,
        size_t *len_expected)
{
    size_t n;
    char buff[MAX_EMBEDDED_LEVELS * 2];

    if (level <= 0 || level > MAX_EMBEDDED_LEVELS)
        return 0;

    if (flags & PCVRNT_SERIALIZE_OPT_PRETTY) {
        if (flags & PCVRNT_SERIALIZE_OPT_PRETTY_TAB) {
            n = level;
            memset(buff, '\t', n);
        }
        else {
            n = level * 2;
            memset(buff, ' ', n);
        }

        if (len_expected)
            *len_expected += n;
        return purc_rwstream_write(rws, buff, n);
    }

    return 0;
}

static inline ssize_t
print_space(purc_rwstream_t rws, unsigned int flags, size_t* len_expected)
{
    ssize_t nr_written = 0;

    if (flags & PCVRNT_SERIALIZE_OPT_SPACED) {
        if (len_expected)
            *len_expected += 1;
        nr_written = purc_rwstream_write(rws, " ", 1);
    }

    return nr_written;
}

static inline ssize_t print_space_no_pretty(purc_rwstream_t rws,
        unsigned int flags, size_t *len_expected)
{
    ssize_t nr_written = 0;

    if (flags & PCVRNT_SERIALIZE_OPT_SPACED &&
            !(flags & PCVRNT_SERIALIZE_OPT_PRETTY)) {
        if (len_expected)
            *len_expected += 1;
        nr_written = purc_rwstream_write(rws, " ", 1);
    }

    return nr_written;
}

static int stringify_cb_bigint(const void *s, size_t len, void *ctxt)
{
    purc_rwstream_t rws = ctxt;

    ssize_t written = purc_rwstream_write(rws, s, len);
    if (written < 0 || (size_t)written < len)
        return -1;
    if (purc_rwstream_write(rws, "n", 1) < 1)
        return -1;

    return 0;
}

ssize_t purc_variant_serialize(purc_variant_t value, purc_rwstream_t rws,
        int level, unsigned int flags, size_t *len_expected)
{
    ssize_t nr_written = 0, n;
    const char* content = NULL;
    size_t sz_content = 0;
    size_t i, idx;
    char buff [256];
    purc_variant_t member = NULL;
    purc_variant_t key;
    char* format_double = NULL;
    char* format_long_double = NULL;
    variant_set_t data;

    purc_get_local_data(PURC_LDNAME_FORMAT_DOUBLE,
            (uintptr_t *)&format_double, NULL);
    purc_get_local_data(PURC_LDNAME_FORMAT_LDOUBLE,
            (uintptr_t *)&format_long_double, NULL);

    PC_ASSERT(value);

    switch (value->type) {
        case PURC_VARIANT_TYPE_UNDEFINED:
            if (flags & PCVRNT_SERIALIZE_OPT_RUNTIME_STRING)
                content = "\"<undefined>\"";
            else
                content = "null";
            break;

        case PURC_VARIANT_TYPE_NULL:
            content = "null";
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (value->b) {
                content = "true";
            }
            else {
                content = "false";
            }
            break;

        case PURC_VARIANT_TYPE_EXCEPTION:
            content = purc_atom_to_string(value->atom);
            sz_content = strlen(content);
            MY_WRITE(rws, "\"", 1);
            n = serialize_string(rws, content, sz_content,
                        flags, len_expected);
            MY_CHECK(n);
            MY_WRITE(rws, "\"", 1);

            content = NULL;
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            /* try to serialize the number as an integer first */
            n = serialize_number(rws, value->d, len_expected);
            if (n < 0)
                goto failed;
            if (n == 0) {
                n = serialize_double(rws, value->d, flags,
                        format_double, len_expected);
                if (n < 0)
                    goto failed;
            }
            nr_written += n;

            content = NULL;
            break;

        case PURC_VARIANT_TYPE_LONGINT:
        {
            const char *format;
            if (flags & PCVRNT_SERIALIZE_OPT_REAL_EJSON)
                format = "%lldL";
            else
                format = "%lld";

            if (snprintf(buff, sizeof(buff), format,
                        (long long int)value->i64) < 0)
                goto failed;
            content = buff;
            // sz_content = strlen(buff);
            break;
        }

        case PURC_VARIANT_TYPE_ULONGINT:
        {
            const char *format;
            if (flags & PCVRNT_SERIALIZE_OPT_REAL_EJSON)
                format = "%lluUL";
            else
                format = "%llu";

            if (snprintf(buff, sizeof(buff), format,
                        (long long unsigned)value->u64) < 0)
                goto failed;
            content = buff;
            // sz_content = strlen(buff);
            break;
        }

        case PURC_VARIANT_TYPE_BIGINT:
            if (!(flags & PCVRNT_SERIALIZE_OPT_REAL_EJSON))
                MY_WRITE(rws, "\"", 1);
            n = bigint_stringify(value, 10, rws, stringify_cb_bigint);
            MY_WRITE(rws, "n", 1);  // postfix
            if (!(flags & PCVRNT_SERIALIZE_OPT_REAL_EJSON))
                MY_WRITE(rws, "\"", 1);
            MY_CHECK(n);

            content = NULL;
            break;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            n = serialize_long_double(rws, value->ld, flags,
                    format_long_double, len_expected);
            MY_CHECK(n);

            content = NULL;
            break;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            content = purc_atom_to_string(value->atom);
            sz_content = strlen(content);
            MY_WRITE(rws, "\"", 1);
            n = serialize_string(rws, content, sz_content,
                        flags, len_expected);
            MY_CHECK(n);
            MY_WRITE(rws, "\"", 1);

            content = NULL;
            break;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (value->flags & PCVRNT_FLAG_STATIC_DATA) {
                content = (const char*)value->ptr2;
                sz_content = (size_t)value->len;
            }
            else if (value->flags & PCVRNT_FLAG_EXTRA_SIZE) {
                content = (const char*)value->ptr2;
                sz_content = (size_t)value->len;
            }
            else {
                content = (const char*)value->bytes;
                sz_content = value->size;
            }
            if (value->type == PURC_VARIANT_TYPE_STRING) {
                MY_WRITE(rws, "\"", 1);
                n = serialize_string(rws, content, sz_content - 1,
                            flags, len_expected);
                MY_WRITE(rws, "\"", 1);
            }
            else
                n = serialize_bsequence(rws, content, sz_content,
                            flags, len_expected);
            MY_CHECK(n);

            content = NULL;
            break;

        case PURC_VARIANT_TYPE_DYNAMIC:
            if (flags & PCVRNT_SERIALIZE_OPT_RUNTIME_STRING)
                content = "\"<dynamic>\"";
            else
                content = "null";
            break;

        case PURC_VARIANT_TYPE_NATIVE:
            if (flags & PCVRNT_SERIALIZE_OPT_RUNTIME_STRING)
                content = "\"<native>\"";
            else
                content = "null";
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            content = NULL;

            n = print_indent(rws, level, flags, len_expected);
            MY_CHECK(n);

            MY_WRITE(rws, "{", 1);
            n = print_newline(rws, flags, len_expected);
            MY_CHECK(n);

            i = 0;
            foreach_key_value_in_variant_object(value, key, member)
                if (i > 0) {
                    MY_WRITE(rws, ",", 1);
                    n = print_newline(rws, flags, len_expected);
                    MY_CHECK(n);
                }

                n = print_space_no_pretty(rws, flags, len_expected);
                MY_CHECK(n);

                n = print_indent(rws, level + 1, flags, len_expected);
                MY_CHECK(n);

                // key
                MY_WRITE(rws, "\"", 1);
                size_t len;
                const char *ks = purc_variant_get_string_const_ex(key, &len);
                assert(ks != NULL);
                n = serialize_string(rws, ks, len, flags, len_expected);
                MY_CHECK(n);
                MY_WRITE(rws, "\"", 1);

                MY_WRITE(rws, ":", 1);
                n = print_space(rws, flags, len_expected);
                MY_CHECK(n);

                // value
                n = purc_variant_serialize(member,
                        rws, level + 1, flags, len_expected);
                MY_CHECK(n);

                i++;
            end_foreach;

            if (i > 0) {
                n = print_newline(rws, flags, len_expected);
                MY_CHECK(n);
            }

            n = print_indent(rws, level, flags, len_expected);
            MY_CHECK(n);

            n = print_space_no_pretty(rws, flags, len_expected);
            MY_CHECK(n);

            MY_WRITE(rws, "}", 1);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            content = NULL;

            n = print_indent(rws, level, flags, len_expected);
            MY_CHECK(n);

            MY_WRITE(rws, "[", 1);
            n = print_newline(rws, flags, len_expected);
            MY_CHECK(n);

            i = 0;
            foreach_value_in_variant_array(value, member, idx)
                (void)idx;
                if (i > 0) {
                    MY_WRITE(rws, ",", 1);
                    n = print_newline(rws, flags, len_expected);
                    MY_CHECK(n);
                }

                n = print_space_no_pretty(rws, flags, len_expected);
                MY_CHECK(n);

                n = print_indent(rws, level + 1, flags, len_expected);
                MY_CHECK(n);

                // member
                n = purc_variant_serialize(member,
                        rws, level + 1, flags, len_expected);
                MY_CHECK(n);

                i++;
            end_foreach;

            if (i > 0) {
                n = print_newline(rws, flags, len_expected);
                MY_CHECK(n);
            }

            n = print_indent(rws, level, flags, len_expected);
            MY_CHECK(n);

            n = print_space_no_pretty(rws, flags, len_expected);
            MY_CHECK(n);

            MY_WRITE(rws, "]", 1);
            break;

        case PURC_VARIANT_TYPE_SET:
            content = NULL;

            n = print_indent(rws, level, flags, len_expected);
            MY_CHECK(n);

            if (flags & PCVRNT_SERIALIZE_OPT_UNIQKEYS)
                MY_WRITE(rws, "[!", 2);
            else
                MY_WRITE(rws, "[", 1);

            n = print_newline(rws, flags, len_expected);
            MY_CHECK(n);

            if (flags & PCVRNT_SERIALIZE_OPT_UNIQKEYS) {
                data = pcvar_set_get_data(value);
                if (data->keynames) {
                    for (size_t i=0; i<data->nr_keynames; ++i) {
                        const char *sk = data->keynames[i];
                        if (i>0)
                            MY_WRITE(rws, " ", 1);
                        MY_WRITE(rws, sk, strlen(sk));
                    }
                }
            }

            i = 0;
            foreach_value_in_variant_set_order(value, member)
                if (i > 0 || flags & PCVRNT_SERIALIZE_OPT_UNIQKEYS) {
                    MY_WRITE(rws, ",", 1);
                    n = print_newline(rws, flags, len_expected);
                    MY_CHECK(n);
                }

                n = print_space_no_pretty(rws, flags, len_expected);
                MY_CHECK(n);

                n = print_indent(rws, level + 1, flags, len_expected);
                MY_CHECK(n);

                // member
                n = purc_variant_serialize(member,
                        rws, level + 1, flags, len_expected);
                MY_CHECK(n);

                i++;
            end_foreach;

            if (i > 0) {
                n = print_newline(rws, flags, len_expected);
                MY_CHECK(n);
            }

            n = print_indent(rws, level, flags, len_expected);
            MY_CHECK(n);

            n = print_space_no_pretty(rws, flags, len_expected);
            MY_CHECK(n);

            MY_WRITE(rws, "]", 1);
            break;

        case PURC_VARIANT_TYPE_TUPLE:
        {
            content = NULL;

            n = print_indent(rws, level, flags, len_expected);
            MY_CHECK(n);

            /* TODO: might use '(' in the future. */
            if (flags & PCVRNT_SERIALIZE_OPT_TUPLE_EJSON)
                MY_WRITE(rws, "[!", 2);
            else
                MY_WRITE(rws, "[", 1);

            n = print_newline(rws, flags, len_expected);
            MY_CHECK(n);

            i = 0;

            purc_variant_t *members;
            size_t sz;
            members = tuple_members(value, &sz);
            assert(members);

            for (idx = 0; idx < sz; idx++) {

                if (i > 0) {
                    MY_WRITE(rws, ",", 1);
                    n = print_newline(rws, flags, len_expected);
                    MY_CHECK(n);
                }

                n = print_space_no_pretty(rws, flags, len_expected);
                MY_CHECK(n);

                n = print_indent(rws, level + 1, flags, len_expected);
                MY_CHECK(n);

                // member
                n = purc_variant_serialize(members[idx],
                        rws, level + 1, flags, len_expected);
                MY_CHECK(n);

                i++;
            }

            if (i > 0) {
                n = print_newline(rws, flags, len_expected);
                MY_CHECK(n);
            }

            n = print_indent(rws, level, flags, len_expected);
            MY_CHECK(n);

            n = print_space_no_pretty(rws, flags, len_expected);
            MY_CHECK(n);

            /* TODO: might use ']' in the future. */
            MY_WRITE(rws, "]", 1);
            break;
        }

        default:
            break;
    }

    if (content) {
        // for simple types
        MY_WRITE(rws, content, strlen (content));
    }

    return nr_written;

failed:
    return -1;
}

