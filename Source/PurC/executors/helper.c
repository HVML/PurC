/*
 * @file helper.c
 * @author Xu Xiaohong
 * @date 2021/10/23
 * @brief
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

#include "helper.h"

#include <stdio.h>
#include <stdlib.h>

int pcexe_unitoutf8(char *utf8, const char *uni, size_t n)
{
    if (n != 6)
        return -1;

    uni += 2;
    n -= 2;

    char buf[7];
    strncpy(buf, uni, n);
    buf[4] = '\0';

    char *end;
    unsigned long int r = strtoul(buf, &end, 16);
    if (end && *end)
        return -1;

    // FIXME: big-endian?
    if (r < 0x80) {
        utf8[0] = (char)r;
        utf8[1] = '\0';
        return 0;
    }

    if (r < 0x800) {
        utf8[0] = ((r >> 6) & 0b11111) | 0b11000000;
        utf8[1] = (r & 0b111111) | 0b10000000;
        utf8[2] = '\0';
        return 0;
    }

    if (r < 0xD800 || ((r >= 0xE000) && (r <= 0xFFFF))) {
        utf8[0] = ((r >> 12) & 0b1111) | 0b11100000;
        utf8[1] = ((r >> 6) & 0b111111) | 0b10000000;
        utf8[2] = (r & 0b111111) | 0b10000000;
        utf8[3] = '\0';
        return 0;
    }

    if (r >= 0x10000 && r <= 0x10FFFF) {
        utf8[0] = ((r >> 18) & 0b111) | 0b11110000;
        utf8[1] = ((r >> 12) & 0b111111) | 0b10000000;
        utf8[2] = ((r >> 6) & 0b111111) | 0b10000000;
        utf8[3] = (r & 0b111111) | 0b10000000;
        utf8[4] = '\0';
        return 0;
    }

    return -1;
}

struct pcexe_strlist* pcexe_strlist_create(void)
{
    struct pcexe_strlist *list = (struct pcexe_strlist*)calloc(1, sizeof(*list));
    if (!list)
        return NULL;
    return list;
}

void pcexe_strlist_destroy(struct pcexe_strlist *list)
{
    if (!list)
        return;
    pcexe_strlist_reset(list);
    free(list);
}

void pcexe_strlist_reset(struct pcexe_strlist *list)
{
    if (!list)
        return;

    for (size_t i=0; i<list->size; ++i) {
        char *str = list->strings[i];
        free(str);
    }

    free(list->strings);
    list->strings = NULL;
    list->size = 0;
    list->capacity = 0;
}

int
pcexe_strlist_append_buf(struct pcexe_strlist *list, const char *buf, size_t len)
{
    if (list->size >= list->capacity) {
        size_t n = (list->size + 15) / 8 * 8;
        char **p = (char**)realloc(list->strings, n * sizeof(*p));
        if (!p)
            return -1;
        list->capacity = n;
        list->strings = p;
    }

    char *p = malloc(len+1);
    if (!p)
        return -1;
    memcpy(p, buf, len);
    p[len] = '\0';

    list->strings[list->size++] = p;

    return 0;
}

