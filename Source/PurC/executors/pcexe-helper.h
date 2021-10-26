/*
 * @file pcexe-helper.h
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


#ifndef PURC_EXECUTOR_PCEXE_HELPER_H
#define PURC_EXECUTOR_PCEXE_HELPER_H

#include "config.h"

#include "purc-macros.h"

#include <stddef.h>
#include <string.h>

PCA_EXTERN_C_BEGIN

int pcexe_unitoutf8(char *utf, const char *uni, size_t n);

struct pcexe_strlist {
    char         **strings;
    size_t         size;
    size_t         capacity;
};

struct pcexe_strlist* pcexe_strlist_create(void);
void pcexe_strlist_destroy(struct pcexe_strlist *list);

static inline void
pcexe_strlist_init(struct pcexe_strlist *list)
{
    list->strings  = NULL;
    list->size     = 0;
    list->capacity = 0;
}

void pcexe_strlist_reset(struct pcexe_strlist *list);
int pcexe_strlist_append_buf(struct pcexe_strlist *list,
        const char *buf, size_t n);

static inline int
pcexe_strlist_append_str(struct pcexe_strlist *list, const char *str)
{
    return pcexe_strlist_append_buf(list, str, strlen(str));
}

static inline int
pcexe_strlist_append_chr(struct pcexe_strlist *list, const char c)
{
    return pcexe_strlist_append_buf(list, &c, 1);
}

static inline int
pcexe_strlist_append_uni(struct pcexe_strlist *list,
        const char *uni, size_t n)
{
    char utf8[7];
    int r = pcexe_unitoutf8(utf8, uni, n);
    if (r)
        return r;

    return pcexe_strlist_append_str(list, utf8);
}

PCA_EXTERN_C_END

#endif // PURC_EXECUTOR_PCEXE_HELPER_H

