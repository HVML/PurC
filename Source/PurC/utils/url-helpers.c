/*
 * @file url-helpers.c
 * @author Vincent Wei
 * @date 2025/06/21
 * @brief The helpers encoding or decoding the URL components.
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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

#include "config.h"

#include "purc-utils.h"

#include <string.h>
#include <assert.h>

// Implementation of Punycode encoding and decoding according to RFC 3492

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
int pcutils_punycode_encode(struct pcutils_mystring *output, const char* hostname)
{
    if (hostname == NULL)
        goto failed;

    const char *p = hostname;
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

    return 0;

failed:
    return -1;
}

// Decode a Punycode string back to UTF-8
static int punycode_decode(struct pcutils_mystring *output,
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
            size_t total_len = output->nr_bytes;
            unsigned char utf8ch[10];

            unsigned uclen = pcutils_unichar_to_utf8(n, utf8ch);
            utf8ch[uclen] = 0;

            // expand the buffer
            if (pcutils_mystring_append_mchar(output, utf8ch, uclen))
                goto failed;

            // find the position to insert
            char *move_start = output->buff;
            int j = 0;
            while (j < i) {
                move_start = pcutils_utf8_next_char(move_start);
                j++;
            }

            // move memory and insert the new character.
            size_t offset = move_start - output->buff;
            memmove(move_start + uclen, move_start,
                    total_len - offset);
            memcpy(move_start, utf8ch, uclen);
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
        goto failed;

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

    return 0;

failed:
    return -1;
}

// Encode URL path components according to RFC 3986
int pcutils_url_path_encode(struct pcutils_mystring *output, const char* path)
{
    if (path == NULL)
        return -1;

    const char *p = path;
    const char *comp_start = NULL;

    // Iterate through the path string
    while (true) {
        char ch = *p;
        
        // Handle component separator or end of string
        if (ch == '/' || ch == '\0') {
            if (comp_start) {
                size_t comp_len = p - comp_start;
                
                // Encode each character in the path component
                for (size_t i = 0; i < comp_len; i++) {
                    unsigned char c = (unsigned char)comp_start[i];
                    
                    // According to RFC 3986, the following characters don't need encoding:
                    // alphanumeric, '-', '.', '_', '~'
                    if ((c >= 'a' && c <= 'z') ||
                        (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') ||
                        c == '-' || c == '.' || 
                        c == '_' || c == '~') {
                        if (pcutils_mystring_append_char(output, c))
                            return -1;
                    }
                    // Other characters need percent-encoding
                    else {
                        char hex[4];
                        snprintf(hex, sizeof(hex), "%%%02X", c);
                        if (pcutils_mystring_append_mchar(output, 
                                (const unsigned char*)hex, 3))
                            return -1;
                    }
                }
                comp_start = NULL;
            }

            if (ch == '\0')
                break;

            // Preserve path separator
            if (pcutils_mystring_append_char(output, '/'))
                return -1;
        }
        else if (comp_start == NULL) {
            comp_start = p;
        }

        p++;
    }

    return 0;
}

/** Decode URL path components according to RFC 3986 and append to output. */
int pcutils_url_path_decode(struct pcutils_mystring *output, const char* encoded)
{
    if (encoded == NULL)
        return -1;

    const char *p = encoded;
    const char *comp_start = NULL;

    // Iterate through the encoded path string
    while (true) {
        char ch = *p;

        // Handle component separator or end of string
        if (ch == '/' || ch == '\0') {
            if (comp_start) {
                const char *curr = comp_start;

                // Decode each character in the path component
                while (curr < p) {
                    // Handle percent-encoded characters
                    if (*curr == '%') {
                        // Need at least 2 more characters for hex value
                        if (curr + 2 >= p) {
                            return -1;
                        }

                        // Convert hex to decimal
                        char hex[3] = {curr[1], curr[2], '\0'};
                        char *end;
                        long value = strtol(hex, &end, 16);
                        
                        // Check for valid hex conversion
                        if (*end != '\0' || value < 0 || value > 255) {
                            return -1;
                        }

                        if (pcutils_mystring_append_char(output, (char)value))
                            return -1;

                        curr += 3;
                    }
                    // Copy unencoded character
                    else {
                        if (pcutils_mystring_append_char(output, *curr))
                            return -1;
                        curr++;
                    }
                }
                comp_start = NULL;
            }

            if (ch == '\0')
                break;

            // Preserve path separator
            if (pcutils_mystring_append_char(output, '/'))
                return -1;
        }
        else if (comp_start == NULL) {
            comp_start = p;
        }

        p++;
    }

    return 0;
}
