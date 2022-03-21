/*
 * @file hvml-rwswrap.c
 * @author XueShuming
 * @date 2021/09/05
 * @brief The impl of hvml rwswrap.
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/errors.h"
#include "hvml-rwswrap.h"

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define NR_CONSUMED_LIST_LIMIT   10

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

struct pchvml_rwswrap {
    purc_rwstream_t rws;
    struct list_head reconsume_list;
    struct list_head consumed_list;
    size_t nr_consumed_list;

    struct pchvml_uc curr_uc;
    int line;
    int column;
    int consumed;
};

struct pchvml_uc* pchvml_uc_new(void)
{
    return PCHVML_ALLOC(sizeof(struct pchvml_uc));
}

void pchvml_uc_destroy(struct pchvml_uc* uc)
{
    if (uc) {
        PCHVML_FREE(uc);
    }
}

struct pchvml_rwswrap* pchvml_rwswrap_new(void)
{
    struct pchvml_rwswrap* wrap = PCHVML_ALLOC(sizeof(struct pchvml_rwswrap));
    if (!wrap) {
        return NULL;
    }
    INIT_LIST_HEAD(&wrap->reconsume_list);
    INIT_LIST_HEAD(&wrap->consumed_list);
    wrap->line = 1;
    wrap->column = 0;
    wrap->consumed = 0;
    return wrap;
}

void pchvml_rwswrap_set_rwstream(struct pchvml_rwswrap* wrap,
        purc_rwstream_t rws)
{
    wrap->rws = rws;
}

static struct pchvml_uc*
pchvml_rwswrap_read_from_rwstream(struct pchvml_rwswrap* wrap)
{
    char c[8] = {0};
    uint32_t uc = 0;
    int nr_c = purc_rwstream_read_utf8_char(wrap->rws, c, &uc);
    if (nr_c < 0) {
        uc = PCHVML_INVALID_CHARACTER;
    }
    wrap->column++;
    wrap->consumed++;

    wrap->curr_uc.character = uc;
    wrap->curr_uc.line = wrap->line;
    wrap->curr_uc.column = wrap->column;
    wrap->curr_uc.position = wrap->consumed;
    if (uc == '\n') {
        wrap->line++;
        wrap->column = 0;
    }
    return &wrap->curr_uc;
}

static struct pchvml_uc*
pchvml_rwswrap_read_from_reconsume_list(struct pchvml_rwswrap* wrap)
{
    struct pchvml_uc* puc = list_entry(wrap->reconsume_list.next,
            struct pchvml_uc, list);
    wrap->curr_uc = *puc;
    list_del_init(&puc->list);
    pchvml_uc_destroy(puc);
    return &wrap->curr_uc;
}

#define print_uc_list(uc_list, tag)                                         \
    do {                                                                    \
        PC_DEBUG("begin print %s list\n|", tag);                            \
        struct list_head *p, *n;                                            \
        list_for_each_safe(p, n, uc_list) {                                 \
            struct pchvml_uc* puc = list_entry(p, struct pchvml_uc, list);  \
            PC_DEBUG("%c", puc->character);                                 \
        }                                                                   \
        PC_DEBUG("|\nend print %s list\n", tag);                            \
    } while(0)

#define PRINT_CONSUMED_LIST(wrap)    \
        print_uc_list(&wrap->consumed_list, "consumed")

#define PRINT_RECONSUM_LIST(wrap)    \
        print_uc_list(&wrap->reconsume_list, "reconsume")

static bool
pchvml_rwswrap_add_consumed(struct pchvml_rwswrap* wrap, struct pchvml_uc* uc)
{
    struct pchvml_uc* p = pchvml_uc_new();
    if (!p) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    *p = *uc;
    list_add_tail(&p->list, &wrap->consumed_list);
    wrap->nr_consumed_list++;

    if (wrap->nr_consumed_list > NR_CONSUMED_LIST_LIMIT) {
        struct pchvml_uc* first = list_first_entry(
                &wrap->consumed_list, struct pchvml_uc, list);
        list_del_init(&first->list);
        pchvml_uc_destroy(first);
        wrap->nr_consumed_list--;
    }
    return true;
}

bool pchvml_rwswrap_reconsume_last_char(struct pchvml_rwswrap* wrap)
{
    if (!wrap->nr_consumed_list) {
        return true;
    }

    struct pchvml_uc* last = list_last_entry(
            &wrap->consumed_list, struct pchvml_uc, list);
    list_del_init(&last->list);
    wrap->nr_consumed_list--;

    list_add(&last->list, &wrap->reconsume_list);
    return true;
}

struct pchvml_uc* pchvml_rwswrap_next_char(struct pchvml_rwswrap* wrap)
{
    struct pchvml_uc* ret = NULL;
    if (list_empty(&wrap->reconsume_list)) {
        ret = pchvml_rwswrap_read_from_rwstream(wrap);
    }
    else {
        ret = pchvml_rwswrap_read_from_reconsume_list(wrap);
    }

    if (pchvml_rwswrap_add_consumed(wrap, ret)) {
        return ret;
    }
    return NULL;
}

void pchvml_rwswrap_destroy(struct pchvml_rwswrap* wrap)
{
    if (wrap) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, &wrap->reconsume_list) {
            struct pchvml_uc* puc = list_entry(p, struct pchvml_uc, list);
            list_del_init(&puc->list);
            pchvml_uc_destroy(puc);
        }
        list_for_each_safe(p, n, &wrap->consumed_list) {
            struct pchvml_uc* puc = list_entry(p, struct pchvml_uc, list);
            list_del_init(&puc->list);
            pchvml_uc_destroy(puc);
        }
        PCHVML_FREE(wrap);
    }
}
