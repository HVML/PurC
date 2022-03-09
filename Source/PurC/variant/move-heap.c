/*
 * @file move-heap.c
 * @author Vincent Wei
 * @date 2022/03/08
 * @brief The implementation of internal interfaces to move variant.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2022
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

#include "private/instance.h"
#include "private/variant.h"

#include "variant-internals.h"

#include <stdlib.h>
#include <string.h>

#if HAVE(GLIB)
    #include <gmodule.h>
#endif

struct pcvariant_heap move_heap;

void pcvariant_move_heap_init_once(void)
{
    move_heap.v_undefined.type = PURC_VARIANT_TYPE_UNDEFINED;
    move_heap.v_undefined.refc = 0;
    move_heap.v_undefined.flags = PCVARIANT_FLAG_NOFREE;
    // INIT_LIST_HEAD(&move_heap.v_undefined.listeners);

    move_heap.v_null.type = PURC_VARIANT_TYPE_NULL;
    move_heap.v_null.refc = 0;
    move_heap.v_null.flags = PCVARIANT_FLAG_NOFREE;
    // INIT_LIST_HEAD(&move_heap.v_null.listeners);

    move_heap.v_false.type = PURC_VARIANT_TYPE_BOOLEAN;
    move_heap.v_false.refc = 0;
    move_heap.v_false.flags = PCVARIANT_FLAG_NOFREE;
    move_heap.v_false.b = false;
    // INIT_LIST_HEAD(&move_heap.v_false.listeners);

    move_heap.v_true.type = PURC_VARIANT_TYPE_BOOLEAN;
    move_heap.v_true.refc = 0;
    move_heap.v_true.flags = PCVARIANT_FLAG_NOFREE;
    move_heap.v_true.b = true;
}

#if HAVE(GLIB)
static inline purc_variant *alloc_variant(void) {
    return (purc_variant *)g_slice_alloc(sizeof(purc_variant));
}

static inline void free_variant(purc_variant *v) {
    return g_slice_free1(sizeof(purc_variant), (gpointer)v);
}
#else
static inline purc_variant *alloc_variant(void) {
    return (purc_variant *)malloc(sizeof(purc_variant));
}

static inline void free_variant(purc_variant *v) {
    return free(v);
}
#endif

// move the variant from the current instance to the move heap.
purc_variant_t pcvariant_move_from(purc_variant_t v)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    struct pcinst *inst = pcinst_current();
    if (v == &inst->variant_heap.v_undefined) {
        retv = &move_heap.v_undefined;
    }
    else if (v == &inst->variant_heap.v_null) {
        retv = &move_heap.v_null;
    }
    else if (v == &inst->variant_heap.v_false) {
        retv = &move_heap.v_false;
    }
    else if (v == &inst->variant_heap.v_true) {
        retv = &move_heap.v_true;
    }
    else if (v->refc == 1) {
        retv = v;

        /* change the stat info */
        if ((v->type == PURC_VARIANT_TYPE_STRING ||
                v->type == PURC_VARIANT_TYPE_BSEQUENCE) &&
                (v->flags & PCVARIANT_FLAG_EXTRA_SIZE)) {
            inst->variant_heap.stat.sz_mem[v->type] -= v->sz_ptr[0];
            inst->variant_heap.stat.sz_total_mem -= v->sz_ptr[0];

            move_heap.stat.sz_mem[v->type] += v->sz_ptr[0];
            move_heap.stat.sz_total_mem += v->sz_ptr[0];
        }

        inst->variant_heap.stat.nr_values[v->type]--;
        inst->variant_heap.stat.nr_total_values--;
        move_heap.stat.nr_values[v->type]++;
        move_heap.stat.nr_total_values++;

        // TODO: check descendants
    }
    else if (IS_CONTAINER(v->type)) {
        retv = purc_variant_container_clone_recursively(v);

        // TODO: check descendants
    }
    else {
        // clone an immutable variant

        retv = alloc_variant();
        memcpy(retv, v, sizeof(*retv));
        retv->refc = 1;

        /* copy the extra space */
        if ((v->type == PURC_VARIANT_TYPE_STRING ||
                v->type == PURC_VARIANT_TYPE_BSEQUENCE) &&
                (v->flags & PCVARIANT_FLAG_EXTRA_SIZE)) {

            retv->sz_ptr[1] = (uintptr_t)malloc(v->sz_ptr[0]);
            memcpy((void *)retv->sz_ptr[1], (void *)v->sz_ptr[1], v->sz_ptr[0]);

            move_heap.stat.sz_mem[v->type] += v->sz_ptr[0];
            move_heap.stat.sz_total_mem += v->sz_ptr[0];
        }

        move_heap.stat.nr_values[v->type]++;
        move_heap.stat.nr_total_values++;
    }

    if (retv != v) {
        purc_variant_unref(v);
    }

    return retv;
}

// move the variant from the move heap to the current instance.
// we only need to update the stat information.
purc_variant_t pcvariant_move_to(purc_variant_t v)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    struct pcinst *inst = pcinst_current();
    if (v == &move_heap.v_undefined) {
        retv = &inst->variant_heap.v_undefined;
        retv->refc++;
    }
    else if (v == &move_heap.v_null) {
        retv = &inst->variant_heap.v_null;
        retv->refc++;
    }
    else if (v == &move_heap.v_false) {
        retv = &inst->variant_heap.v_false;
        retv->refc++;
    }
    else if (v == &move_heap.v_true) {
        retv = &inst->variant_heap.v_true;
        retv->refc++;
    }
    else {
        retv = v;

        /* change the stat info */
        if ((v->type == PURC_VARIANT_TYPE_STRING ||
                v->type == PURC_VARIANT_TYPE_BSEQUENCE) &&
                (v->flags & PCVARIANT_FLAG_EXTRA_SIZE)) {
            inst->variant_heap.stat.sz_mem[v->type] += v->sz_ptr[0];
            inst->variant_heap.stat.sz_total_mem += v->sz_ptr[0];

            move_heap.stat.sz_mem[v->type] -= v->sz_ptr[0];
            move_heap.stat.sz_total_mem -= v->sz_ptr[0];
        }

        inst->variant_heap.stat.nr_values[v->type]++;
        inst->variant_heap.stat.nr_total_values++;
        move_heap.stat.nr_values[v->type]--;
        move_heap.stat.nr_total_values--;

        // TODO: check descendants
    }

    return retv;
}

// if we can change the variant heap temporarily to be the move heap
// it is enough to call purc_variant_unref().
void pcvariant_grind(purc_variant_t v)
{
    UNUSED_PARAM(v);
}

