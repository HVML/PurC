/*
   Various utilities

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2009.

   This file is part of purc, an HVML interperter with CLI.

   Purc is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   Purc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file tty/util.c
 *  \brief Source: various utilities
 */

#include "config.h"

#include <ctype.h>

#include "screen.h"

static inline int
is_7bit_printable (unsigned char c)
{
    return (c > 31 && c < 127);
}

static inline int
is_iso_printable (unsigned char c)
{
    return ((c > 31 && c < 127) || c >= 160);
}

/* --------------------------------------------------------------------------------------------- */

static inline int
is_8bit_printable (unsigned char c)
{
    /* "Full 8 bits output" doesn't work on xterm */
    if (mc_global.tty.xterm_flag)
        return is_iso_printable (c);

    return (c > 31 && c != 127 && c != 155);
}

int
is_printable (int c)
{
    c &= 0xff;

    if (!mc_global.eight_bit_clean)
        return is_7bit_printable (c);

    if (mc_global.full_eight_bits)
        return is_8bit_printable (c);

    return is_iso_printable (c);
}

#define foreach_arg(_arg, _addr, _len, _first_addr, _first_len) \
    for (_addr = (_first_addr), _len = (_first_len); \
        _addr; \
        _addr = va_arg(_arg, void **), _len = _addr ? va_arg(_arg, size_t) : 0)

#define C_PTR_ALIGN    (sizeof(size_t))
#define C_PTR_MASK     (-C_PTR_ALIGN)

void *mc_calloc_a(size_t len, ...)
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

