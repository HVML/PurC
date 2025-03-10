/*
 * utils.c - misc utility and helper functions
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
 * Copyright (C) 2012 Felix Fietkau <nbd@openwrt.org>
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

#include "config.h"

#include "purc-errors.h"
#include "private/utils.h"
#include "private/errors.h"
#include "private/printbuf.h"
#include "private/ports.h"
#include "private/debug.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#if HAVE(GLIB)
#include <glib.h>
#endif // HAVE(GLIB)

#define foreach_arg(_arg, _addr, _len, _first_addr, _first_len) \
    for (_addr = (_first_addr), _len = (_first_len); \
        _addr; \
        _addr = va_arg(_arg, void **), _len = _addr ? va_arg(_arg, size_t) : 0)

#define C_PTR_ALIGN    (sizeof(size_t))
#define C_PTR_MASK    (-C_PTR_ALIGN)

void *pcutils_calloc_a(size_t len, ...)
{
    va_list ap, ap1;
    void *ret;
    void **cur_addr;
    size_t cur_len;
    int alloc_len = 0;
    char *ptr;

    va_start(ap, len);

    va_copy(ap1, ap);
    foreach_arg(ap1, cur_addr, cur_len, &ret, len)
        alloc_len += (cur_len + C_PTR_ALIGN - 1 ) & C_PTR_MASK;
    va_end(ap1);

    ptr = calloc(1, alloc_len);
    if (!ptr) {
        va_end(ap);
        return NULL;
    }

    alloc_len = 0;
    foreach_arg(ap, cur_addr, cur_len, &ret, len) {
        *cur_addr = &ptr[alloc_len];
        alloc_len += (cur_len + C_PTR_ALIGN - 1) & C_PTR_MASK;
    }
    va_end(ap);

    return ret;
}

static const char *hex_digits_lower = "0123456789abcdef";
static const char *hex_digits_upper = "0123456789ABCDEF";

void pcutils_bin2hex (const unsigned char *bin, size_t len, char *hex,
        bool uppercase)
{
    const char *hex_digits;

    if (uppercase)
        hex_digits = hex_digits_upper;
    else
        hex_digits = hex_digits_lower;

    for (size_t i = 0; i < len; i++) {
        unsigned char byte = bin [i];
        hex [i*2] = hex_digits [(byte >> 4) & 0x0f];
        hex [i*2+1] = hex_digits [byte & 0x0f];
    }
    hex [len * 2] = '\0';
}

int pcutils_hex2bin (const char *hex, unsigned char *bin, size_t *converted)
{
    size_t pos = 0;
    size_t sz = 0;

    while (*hex) {
        unsigned char half;

        if (*hex >= '0' && *hex <= '9') {
            half = (*hex - '0') & 0x0f;
        }
        else {
            int c = purc_tolower (*hex);
            if (c >= 'a' && c <= 'f') {
                half = (*hex - 'a' + 0x0a) & 0x0f;
            }
            else {
                goto failed;
            }
        }

        if (pos % 2 == 0) {
            *bin = half << 4;
        }
        else {
            *bin |= half;
            bin++;
            sz++;
        }

        pos++;
        hex++;
    }

    if (converted)
        *converted = sz;
    return 0;

failed:
    if (converted)
        *converted = sz;
    return -1;
}

int pcutils_hex2byte (const char *hex, unsigned char *byte)
{
    size_t pos;

    for (pos = 0; pos < 2; pos++) {
        unsigned char half;

        if (*hex >= '0' && *hex <= '9') {
            half = (*hex - '0') & 0x0f;
        }
        else {
            int c = purc_tolower (*hex);
            if (c >= 'a' && c <= 'f') {
                half = (*hex - 'a' + 0x0a) & 0x0f;
            }
            else {
                goto failed;
            }
        }

        if (pos % 2 == 0) {
            *byte = half << 4;
        }
        else {
            *byte |= half;
        }

        hex++;
    }

    return 0;

failed:
    return -1;
}

size_t pcutils_get_prev_fibonacci_number(size_t n)
{
    size_t fib_0 = 0;
    size_t fib_1 = 1;
    size_t fib_n = 0;

    if (n < 2) {
        return 0;
    }

    while (fib_n < n) {
        fib_n = fib_1 + fib_0;
        fib_0 = fib_1;
        fib_1 = fib_n;
    }

    return fib_0;
}

size_t pcutils_get_next_fibonacci_number(size_t n)
{
    size_t fib_0 = 0;
    size_t fib_1 = 1;
    size_t fib_n = 0;

    if (n < 2) {
        return n + 1;
    }

    while (fib_n <= n) {
        fib_n = fib_1 + fib_0;
        fib_0 = fib_1;
        fib_1 = fib_n;
    }

    return fib_n;
}

#ifndef MIN
#define MIN(x, y) (((x) > (y)) ? (y) : (x))
#endif

int pcutils_parse_int32(const char *buf, size_t len, int32_t *retval)
{
    (void)len;
    char *end = NULL;
    int32_t val;

    errno = 0;
    val = strtol(buf, &end, 10);
    if (end != buf)
        *retval = val;
    return ((val == 0 && errno != 0) || (end == buf)) ? 1 : 0;
}

int pcutils_parse_uint32(const char *buf, size_t len, uint32_t *retval)
{
    (void)len;
    char *end = NULL;
    uint32_t val;

    errno = 0;
    while (*buf == ' ')
        buf++;
    if (*buf == '-')
        return 1; /* error: uint cannot be negative */

    val = strtoul(buf, &end, 10);
    if (end != buf)
        *retval = val;
    return ((val == 0 && errno != 0) || (end == buf)) ? 1 : 0;
}

int pcutils_parse_int64(const char *buf, size_t len, int64_t *retval)
{
    (void)len;
    char *end = NULL;
    int64_t val;

    errno = 0;
    val = strtoll(buf, &end, 10);
    if (end != buf)
        *retval = val;
    return ((val == 0 && errno != 0) || (end == buf)) ? 1 : 0;
}

int pcutils_parse_uint64(const char *buf, size_t len, uint64_t *retval)
{
    (void)len;
    char *end = NULL;
    uint64_t val;

    errno = 0;
    while (*buf == ' ')
        buf++;
    if (*buf == '-')
        return 1; /* error: uint cannot be negative */

    val = strtoull(buf, &end, 10);
    if (end != buf)
        *retval = val;
    return ((val == 0 && errno != 0) || (end == buf)) ? 1 : 0;
}

int pcutils_parse_double(const char *buf, size_t len, double *retval)
{
    (void)len;
    char *end;

    *retval = strtod(buf, &end);
    if (buf + len == end)
        return 0; // It worked
    return 1;
}

int pcutils_parse_long_double(const char *buf, size_t len, long double *retval)
{
    (void)len;
    char *end;

    *retval = strtold(buf, &end);
    if (buf + len == end)
        return 0; // It worked
    return 1;
}

WTF_ATTRIBUTE_PRINTF(3, 0)
char*
pcutils_vsnprintf(char *buf, size_t *sz_io, const char *fmt, va_list ap)
{
    va_list dp;
    va_copy(dp, ap);

    size_t nr = *sz_io;
    size_t sz = nr;

    char *p = NULL;
    int n = vsnprintf(buf, sz, fmt, dp);
    va_end(dp);

    do {
        nr = n;

        PC_ASSERT(n>=0);

        p = buf;
        if ((size_t)n < sz)
            break;

        sz = n+1;
        p = (char*)malloc(sz);
        if (!p)
            break;

        *p = '\0';
        n = vsnprintf(p, sz, fmt, ap);

        nr = n;
        if (n<0 || (size_t)n >= sz) {
            free(p);
            p = NULL;
            PC_ASSERT(0);
            break;
        }
    } while (0);

    *sz_io = nr;
    return p;
}

char*
pcutils_snprintf(char *buf, size_t *sz_io, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char *p = pcutils_vsnprintf(buf, sz_io, fmt, ap);

    va_end(ap);

    return p;
}

const char*
pcutils_trim_blanks(const char *str, size_t *sz_io)
{
    const char *head = str;
    const char *tail = str + *sz_io;

    const char *start = head;
    while ((start < tail) && purc_isblank(*start))
        ++start;
    const char *end = tail;
    while (end > start) {
        if (!purc_isblank(*(end-1)))
            break;
        --end;
    }

    *sz_io = end - start;
    return start;
}

const char*
pcutils_trim_spaces(const char *str, size_t *sz_io)
{
    const char *head = str;
    const char *tail = str + *sz_io;

    const char *start = head;
    while ((start < tail) && purc_isspace(*start))
        ++start;
    const char *end = tail;
    while (end > start) {
        if (!purc_isspace(*(end-1)))
            break;
        --end;
    }

    *sz_io = end - start;
    return start;
}

bool
pcutils_contains_graph(const char *str)
{
    while (*str) {
        if (purc_isgraph(*str))
            return true;

        str++;
    }

    return false;
}

const char *
pcutils_get_next_token(const char *data, const char *delims, size_t *length)
{
    const char *head = data;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00))
        return NULL;

    *length = 0;
    while (*data) {
        temp = strchr(delims, *data);
        if (temp) {
            if (head == data) {
                head = data + 1;
            }
            else
                break;
        }
        data++;
    }

    *length = data - head;
    if (*length == 0)
        head = NULL;

    return head;
}

const char *
pcutils_get_next_token_len(const char *data, size_t str_len,
        const char *delims, size_t *length)
{
    const char *head = data;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00) ||
            (str_len == 0))
        return NULL;

    *length = 0;
    while (str_len && *data) {
        temp = strchr(delims, *data);
        if (temp) {
            if (head == data) {
                head = data + 1;
            }
            else
                break;
        }
        data++;
        str_len--;
    }

    *length = data - head;
    if (*length == 0)
        head = NULL;

    return head;
}

const char *
pcutils_get_prev_token(const char *data, size_t str_len,
        const char *delims, size_t *length)
{
    const char *head = NULL;
    size_t tail = *length;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00) ||
            (str_len == 0))
        return NULL;

    *length = 0;

    while (str_len) {
        temp = strchr(delims, *(data + str_len - 1));
        if (temp) {
            if (tail == str_len) {
                str_len--;
                tail = str_len;
            }
            else
                break;
        }
        str_len--;
    }

    *length = tail - str_len;
    if (*length == 0)
        head = NULL;
    else
        head = data + str_len;

    return head;
}

const char *
pcutils_get_next_line_len(const char *str, size_t str_len,
        const char *sep, size_t *length)
{
    if ((*str == 0) || (str_len == 0))
        return NULL;

    /* Skip the seperator if the string starts with the seperator */
    size_t sep_len = strlen(sep);
    if (sep_len <= str_len && strncmp(str, sep, sep_len) == 0) {
        str += sep_len;
        str_len -= sep_len;

        if (str_len == 0)
            return NULL;
    }

    /* Find the line seprator in the string. */
    const char *p = strnstr(str, sep, str_len);
    if (p == NULL) {
        *length = str_len;
        return str;
    }

    *length = p - str;
    if (*length == 0)
        return NULL;

    return str;
}

static const char *json_hex_chars = "0123456789abcdefABCDEF";

char* pcutils_escape_string_for_json (const char* str)
{
    struct pcutils_printbuf my_buff, *pb = &my_buff;
    size_t pos = 0, start_offset = 0;
    unsigned char c;

    if (pcutils_printbuf_init (pb)) {
        PC_ERROR ("Failed to initialize buffer for escape string for JSON.\n");
        return NULL;
    }

    while (str [pos]) {
        const char* escaped;

        c = str[pos];
        switch (c) {
        case '\b':
            escaped = "\\b";
            break;
        case '\n':
            escaped = "\\n";
            break;
        case '\r':
            escaped = "\\n";
            break;
        case '\t':
            escaped = "\\t";
            break;
        case '\f':
            escaped = "\\f";
            break;
        case '"':
            escaped = "\\\"";
            break;
        case '\\':
            escaped = "\\\\";
            break;
        default:
            escaped = NULL;
            if (c < ' ') {
                char sbuf[7];
                if (pos - start_offset > 0)
                    pcutils_printbuf_memappend (pb,
                            str + start_offset, pos - start_offset);
                snprintf (sbuf, sizeof (sbuf), "\\u00%c%c",
                        json_hex_chars[c >> 4], json_hex_chars[c & 0xf]);
                pcutils_printbuf_memappend_fast (pb, sbuf,
                        (int)(sizeof(sbuf) - 1));
                start_offset = ++pos;
            }
            else
                pos++;
            break;
        }

        if (escaped) {
            if (pos - start_offset > 0)
                pcutils_printbuf_memappend (pb, str + start_offset,
                        pos - start_offset);

            pcutils_printbuf_memappend (pb, escaped, strlen (escaped));
            start_offset = ++pos;
        }
    }

    if (pos - start_offset > 0)
        pcutils_printbuf_memappend (pb, str + start_offset, pos - start_offset);

    return pb->buf;
}

struct pcutils_wildcard
{
#if USE(GLIB)            /* { */
    GPatternSpec *pattern;
#else                    /* }{ */
    char         *pattern;
#endif                   /* } */
};

struct pcutils_wildcard*
pcutils_wildcard_create(const char *pattern, size_t nr)
{
    struct pcutils_wildcard *wildcard;
    wildcard = (struct pcutils_wildcard*)calloc(1, sizeof(*wildcard));
    if (!wildcard) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    char *s = NULL;
    const char *p = NULL;
    if (pattern[nr] == '\0') {
        p = pattern;
    }
    else {
        s = strndup(pattern, nr);
        if (!s) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }
        p = s;
    }

    if (pattern[nr] == '\0') {
#if USE(GLIB)            /* { */
        wildcard->pattern = g_pattern_spec_new(p);
#else                    /* }{ */
        wildcard->pattern = s;
        s = NULL;
#endif                   /* } */
    }

    if (s)
        free(s);

    if (!wildcard->pattern) {
        pcutils_wildcard_destroy(wildcard);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    return wildcard;
}

void
pcutils_wildcard_destroy(struct pcutils_wildcard *wildcard)
{
    if (!wildcard)
        return;

    if (wildcard->pattern) {
#if USE(GLIB)            /* { */
        g_pattern_spec_free(wildcard->pattern);
#else                    /* }{ */
        free(wildcard->pattern);
#endif                   /* } */
        wildcard->pattern = NULL;
    }

    free(wildcard);
}

#if USE(GLIB)            /* { */
static bool
wildcard_match(GPatternSpec *pattern, const char *str)
{
#if GLIB_CHECK_VERSION(2, 70, 0)
    return g_pattern_spec_match_string(pattern, str);
#else
    return g_pattern_match_string(pattern, str);
#endif
}

#else                    /* }{ */

static bool
wildcard_match(const char *pattern, const char *str)
{
    // TODO: better match in unicode

    int len1 = strlen (str);
    int len2 = strlen (pattern);
    int mark = 0;
    int p1 = 0;
    int p2 = 0;

    while ((p1 < len1) && (p2<len2)) {
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
#endif                   /* } */

int
pcutils_wildcard_match(struct pcutils_wildcard *wildcard,
        const char *str, size_t nr, bool *matched)
{
    if (!wildcard || !wildcard->pattern) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (str[nr] == '\0') {
        *matched = wildcard_match(wildcard->pattern, str);
        return 0;
    }

    char *s = strndup(str, nr);
    if (!s) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    *matched = wildcard_match(wildcard->pattern, s);
    free(s);
    return 0;
}

