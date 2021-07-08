/*
 * @file safelist.h
 * @date 2021/07/05
 * @brief A linked list protected against recursive iteration with deletes.
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
 * The code is derived from uBox, which is licensed under MIT licence.
 *
 * Copyright (C) 2013 Felix Fietkau <nbd@openwrt.org>
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

#ifndef PURC_PRIVATE_SAFELIST_H
#define PURC_PRIVATE_SAFELIST_H

#include <stdbool.h>

#include "private/list.h"

struct safe_list;
struct safe_list_iterator;

struct safe_list {
    struct list_head list;
    struct safe_list_iterator *i;
};

#ifdef __cplusplus
extern "C" {
#endif

int pcutils_safelist_for_each(struct safe_list *list,
        int (*cb)(void *ctx, struct safe_list *list),
        void *ctx);

void pcutils_safelist_add(struct safe_list *list, struct safe_list *head);
void pcutils_safelist_add_first(struct safe_list *list, struct safe_list *head);
void pcutils_safelist_del(struct safe_list *list);

#ifdef __cplusplus
}
#endif

#define INIT_SAFE_LIST(_head)           \
    do {                                \
        INIT_LIST_HEAD(_head.list);     \
        (_head)->i = NULL;              \
    } while (0)

#define SAFE_LIST_INIT(_name) { LIST_HEAD_INIT(_name.list), NULL }
#define SAFE_LIST(_name)    struct safe_list _name = SAFE_LIST_INIT(_name)

static inline bool pcutils_safelist_empty(struct safe_list *head)
{
    return list_empty(&head->list);
}

#endif  /* PURC_PRIVATE_SAFELIST_H */
