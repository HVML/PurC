/*
** @file stacking-context.c
** @author Vincent Wei
** @date 2022/12/05
** @brief The implementation of stacking context.
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#undef NDEBUG

#include "udom.h"

#include <assert.h>

static void free_head(uint64_t sortv, void *data)
{
    (void)sortv;
#ifndef NDEBUG
    struct list_head *head = data;
    assert(list_empty(head));
#endif

    free(data);
}

static int cmp_zidx(uint64_t v1, uint64_t v2)
{
    int64_t sortv1 = (int64_t)v1;
    int64_t sortv2 = (int64_t)v2;

    if (sortv1 > sortv2)
        return 1;
    else if (sortv1 < sortv2)
        return -1;

    return 0;
}

foil_stacking_context *foil_stacking_context_new(
        foil_stacking_context *parent, int zidx, foil_rdrbox *creator)
{
    foil_stacking_context *ctxt = calloc(1, sizeof(*ctxt));
    if (ctxt == NULL)
        goto failed;

    if (parent != NULL) {
        struct list_head *head;
        if (sorted_array_find(parent->zidx2child, zidx, (void **)&head) < 0) {
            head = malloc(sizeof(*head));
            if (head == NULL)
                goto failed;

            INIT_LIST_HEAD(head);
            sorted_array_add(parent->zidx2child, zidx, head);
        }

        list_add_tail(head, &ctxt->list);
    }

    ctxt->zidx2child = sorted_array_create(SAFLAG_ORDER_ASC,
            0, free_head, cmp_zidx);
    if (ctxt->zidx2child)
        goto failed;

    ctxt->parent = parent;
    ctxt->creator = creator;
    ctxt->zidx = zidx;
    return ctxt;

failed:
    if (ctxt)
        free(ctxt);
    return NULL;
}

int foil_stacking_context_detach(foil_stacking_context *parent,
        foil_stacking_context *ctxt)
{
    assert(parent);

    struct list_head *head;
    int ret;
    ret = sorted_array_find(parent->zidx2child, ctxt->zidx, (void **)&head);
    assert(ret >= 0);
    assert(head);

    list_del(&ctxt->list);
    if (list_empty(head)) {
        sorted_array_remove(parent->zidx2child, ctxt->zidx);
    }

    return 0;
}

int foil_stacking_context_delete(foil_stacking_context *ctxt)
{
    /* detach it from parent context */
    if (ctxt->parent) {
        foil_stacking_context_detach(ctxt->parent, ctxt);
    }

    size_t n = sorted_array_count(ctxt->zidx2child);

    for (size_t i = 0; i < n; i++) {
        struct list_head *head;
        sorted_array_get(ctxt->zidx2child, i, (void **)&head);

        foil_stacking_context *p, *n;
        list_for_each_entry_safe(p, n, head, list) {
            list_del(&p->list);

            p->parent = NULL;   // mark detached
            foil_stacking_context_delete(p);
        }
    }

    sorted_array_destroy(ctxt->zidx2child);
    free(ctxt);
    return 0;
}

