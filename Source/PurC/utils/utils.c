/*
 * utils.c - misc libubox utility functions
 *
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define foreach_arg(_arg, _addr, _len, _first_addr, _first_len) \
	for (_addr = (_first_addr), _len = (_first_len); \
		_addr; \
		_addr = va_arg(_arg, void **), _len = _addr ? va_arg(_arg, size_t) : 0)

#define C_PTR_ALIGN	(sizeof(size_t))
#define C_PTR_MASK	(-C_PTR_ALIGN)

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

static char hex_digits [] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void pcutils_bin2hex (const unsigned char *bin, int len, char *hex)
{
    for (int i = 0; i < len; i++) {
        unsigned char byte = bin [i];
        hex [i*2] = hex_digits [(byte >> 4) & 0x0f];
        hex [i*2+1] = hex_digits [byte & 0x0f];
    }
    hex [len * 2] = '\0';
}

/* bin must be long enough to hold the bytes.
   return the number of bytes converted, <= 0 for error */
int pcutils_hex2bin (const char *hex, unsigned char *bin)
{
    int pos = 0;
    int sz = 0;

    while (*hex) {
        unsigned char half;

        if (*hex >= '0' && *hex <= '9') {
            half = (*hex - '0') & 0x0f;
        }
        else {
            int c = tolower (*hex);
            if (c >= 'a' && c <= 'f') {
                half = (*hex - 'a' + 0x10) & 0x0f;
            }
            else {
                return -1;
            }
        }

        if (pos % 2 == 0) {
            *bin = half;
        }
        else {
            *bin |= half << 4;
            bin++;
            sz++;
        }

        pos++;
    }

    return sz;
}

#if OS(LINUX)

size_t pcutils_get_cmdline_arg(int arg, char* buf, size_t sz_buf)
{
    size_t i, n = 0;
    FILE *fp = fopen("/proc/self/cmdline", "rb");

    if (fp == NULL) {
        pcinst_set_error (PURC_ERROR_BAD_STDC_CALL);
        return 0;
    }

    if (arg > 0) {
        while (1) {
            int ch = fgetc(fp);
            if (ch == '\0') {
                n++;
            }
            else if (ch == EOF) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return 0;
            }

            if (n == (size_t)arg)
                break;
        }
    }

    for (i = 0; i < sz_buf - 1; i++) {
        int ch = fgetc(fp);

        if (isalnum(ch))
            buf[n++] = ch;
    }

    buf[n] = '\0';
    fclose(fp);
    return n;
}

#else /* OS(LINUX) */

size_t pcutils_get_cmdline_arg(int arg, char* buf, size_t sz_buf)
{
    size_t i;
    const char* unknown = "unknown-cmdline";

    UNUSED_PARAM(arg);

    for (i = 0; i < sz_buf - 1; i++) {
        if (unknown[i])
            buf[i] = unknown[i];
        else
            break;
    }

    buf[i] = '\0';
    return i;
}
#endif /* not OS(LINUX) */
