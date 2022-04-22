/*
 * @file tkz-helper.c
 * @author XueShuming
 * @date 2022/04/22
 * @brief The impl of tkz helper.
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
#include "private/tkz-helper.h"

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

struct tkz_reader {
    purc_rwstream_t rws;
    struct list_head reconsume_list;
    struct list_head consumed_list;
    size_t nr_consumed_list;

    struct tkz_uc curr_uc;
    int line;
    int column;
    int consumed;
};

struct tkz_uc *tkz_uc_new(void)
{
    return PCHVML_ALLOC(sizeof(struct tkz_uc));
}

void tkz_uc_destroy(struct tkz_uc *uc)
{
    if (uc) {
        PCHVML_FREE(uc);
    }
}

struct tkz_reader *tkz_reader_new(void)
{
    struct tkz_reader *reader = PCHVML_ALLOC(sizeof(struct tkz_reader));
    if (!reader) {
        return NULL;
    }
    INIT_LIST_HEAD(&reader->reconsume_list);
    INIT_LIST_HEAD(&reader->consumed_list);
    reader->line = 1;
    reader->column = 0;
    reader->consumed = 0;
    return reader;
}

void tkz_reader_set_rwstream(struct tkz_reader *reader,
        purc_rwstream_t rws)
{
    reader->rws = rws;
}

static struct tkz_uc*
tkz_reader_read_from_rwstream(struct tkz_reader *reader)
{
    char c[8] = {0};
    uint32_t uc = 0;
    int nr_c = purc_rwstream_read_utf8_char(reader->rws, c, &uc);
    if (nr_c < 0) {
        uc = TKZ_INVALID_CHARACTER;
    }
    reader->column++;
    reader->consumed++;

    reader->curr_uc.character = uc;
    reader->curr_uc.line = reader->line;
    reader->curr_uc.column = reader->column;
    reader->curr_uc.position = reader->consumed;
    if (uc == '\n') {
        reader->line++;
        reader->column = 0;
    }
    return &reader->curr_uc;
}

static struct tkz_uc*
tkz_reader_read_from_reconsume_list(struct tkz_reader *reader)
{
    struct tkz_uc *puc = list_entry(reader->reconsume_list.next,
            struct tkz_uc, list);
    reader->curr_uc = *puc;
    list_del_init(&puc->list);
    tkz_uc_destroy(puc);
    return &reader->curr_uc;
}

#define print_uc_list(uc_list, tag)                                         \
    do {                                                                    \
        PC_DEBUG("begin print %s list\n|", tag);                            \
        struct list_head *p, *n;                                            \
        list_for_each_safe(p, n, uc_list) {                                 \
            struct tkz_uc *puc = list_entry(p, struct tkz_uc, list);        \
            PC_DEBUG("%c", puc->character);                                 \
        }                                                                   \
        PC_DEBUG("|\nend print %s list\n", tag);                            \
    } while(0)

#define PRINT_CONSUMED_LIST(reader)    \
        print_uc_list(&reader->consumed_list, "consumed")

#define PRINT_RECONSUM_LIST(reader)    \
        print_uc_list(&reader->reconsume_list, "reconsume")

static bool
tkz_reader_add_consumed(struct tkz_reader *reader, struct tkz_uc *uc)
{
    struct tkz_uc *p = tkz_uc_new();
    if (!p) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return false;
    }

    *p = *uc;
    list_add_tail(&p->list, &reader->consumed_list);
    reader->nr_consumed_list++;

    if (reader->nr_consumed_list > NR_CONSUMED_LIST_LIMIT) {
        struct tkz_uc *first = list_first_entry(
                &reader->consumed_list, struct tkz_uc, list);
        list_del_init(&first->list);
        tkz_uc_destroy(first);
        reader->nr_consumed_list--;
    }
    return true;
}

bool tkz_reader_reconsume_last_char(struct tkz_reader *reader)
{
    if (!reader->nr_consumed_list) {
        return true;
    }

    struct tkz_uc *last = list_last_entry(
            &reader->consumed_list, struct tkz_uc, list);
    list_del_init(&last->list);
    reader->nr_consumed_list--;

    list_add(&last->list, &reader->reconsume_list);
    return true;
}

struct tkz_uc *tkz_reader_next_char(struct tkz_reader *reader)
{
    struct tkz_uc *ret = NULL;
    if (list_empty(&reader->reconsume_list)) {
        ret = tkz_reader_read_from_rwstream(reader);
    }
    else {
        ret = tkz_reader_read_from_reconsume_list(reader);
    }

    if (tkz_reader_add_consumed(reader, ret)) {
        return ret;
    }
    return NULL;
}

void tkz_reader_destroy(struct tkz_reader *reader)
{
    if (reader) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, &reader->reconsume_list) {
            struct tkz_uc *puc = list_entry(p, struct tkz_uc, list);
            list_del_init(&puc->list);
            tkz_uc_destroy(puc);
        }
        list_for_each_safe(p, n, &reader->consumed_list) {
            struct tkz_uc *puc = list_entry(p, struct tkz_uc, list);
            list_del_init(&puc->list);
            tkz_uc_destroy(puc);
        }
        PCHVML_FREE(reader);
    }
}
