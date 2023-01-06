/*
 * @file mystring.c
 * @author Vincent Wei
 * @date 2022/03/11
 * @brief The implementation of utilities for mystring.
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

#include "config.h"

#include "private/utils.h"

#include <string.h>
#include <assert.h>

int pcutils_mystring_append_mchar(struct pcutils_mystring *mystr,
        const unsigned char *mchar, size_t mchar_len)
{
    if (mchar_len == 0) {
        mchar_len = strlen((const char*)mchar);
    }

    if (mchar_len == 0)
        return 0;

    if (mystr->nr_bytes + mchar_len > mystr->sz_space) {
        size_t new_sz;
        new_sz = pcutils_get_next_fibonacci_number(mystr->nr_bytes + mchar_len);

        mystr->buff = realloc(mystr->buff, new_sz);
        if (mystr->buff == NULL)
            return -1;

        mystr->sz_space = new_sz;
    }

    memcpy(mystr->buff + mystr->nr_bytes, mchar, mchar_len);
    mystr->nr_bytes += mchar_len;
    return 0;
}

int pcutils_mystring_append_uchar(struct pcutils_mystring *mystr,
        uint32_t uchar, size_t n)
{
    unsigned char utf8[8];
    unsigned utf8_len = pcutils_unichar_to_utf8(uchar, utf8);
    size_t mchar_len = utf8_len * n;

    if (mchar_len == 0)
        return 0;

    if (mystr->nr_bytes + mchar_len > mystr->sz_space) {
        size_t new_sz;
        new_sz = pcutils_get_next_fibonacci_number(mystr->nr_bytes + mchar_len);

        mystr->buff = realloc(mystr->buff, new_sz);
        if (mystr->buff == NULL)
            return -1;

        mystr->sz_space = new_sz;
    }

    for (size_t i = 0; i < n; i++) {
        memcpy(mystr->buff + mystr->nr_bytes, utf8, utf8_len);
        mystr->nr_bytes += utf8_len;
    }

    return 0;
}

int pcutils_mystring_done(struct pcutils_mystring *mystr)
{
    if (mystr->nr_bytes + 1 > mystr->sz_space) {
        mystr->buff = realloc(mystr->buff, mystr->nr_bytes + 1);
        if (mystr->buff == NULL)
            return -1;
    }

    mystr->buff[mystr->nr_bytes] = '\0';  // null-terminated
    mystr->nr_bytes += 1;

    // shrink the buffer
    mystr->buff = realloc(mystr->buff, mystr->nr_bytes);
    mystr->sz_space = mystr->nr_bytes;
    return 0;
}

void pcutils_mystring_free(struct pcutils_mystring *mystr)
{
    if (mystr->buff)
        free(mystr->buff);
}

