/*
 * @file observer.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation for variant observer
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

#include "purc-errors.h"
#include "private/debug.h"
#include "private/errors.h"
#include "private/variant.h"

#include <stdlib.h>

#define TO_DEBUG 1

static pcvar_listener*
register_listener(purc_variant_t v, unsigned int flags,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt)
{
    struct list_head *listeners;
    listeners = &v->listeners;

    struct pcvar_listener *listener;
    listener = (struct pcvar_listener*)calloc(1, sizeof(*listener));
    if (!listener) {
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    listener->flags          = flags;
    listener->op             = op;
    listener->ctxt           = ctxt;
    listener->handler        = handler;

    if ((flags & PCVAR_LISTENER_PRE_OR_POST) == PCVAR_LISTENER_PRE) {
        list_add_tail(&listener->list_node, listeners);
    } else {
        list_add(&listener->list_node, listeners);
    }

    return listener;
}

struct pcvar_listener*
purc_variant_register_pre_listener(purc_variant_t v,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt)
{
    if (v == PURC_VARIANT_INVALID || !op || !handler) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return NULL;
    }

    if (!IS_CONTAINER(v->type)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_SUPPORTED);
        return NULL;
    }

    return register_listener(v, PCVAR_LISTENER_PRE, op, handler, ctxt);
}

struct pcvar_listener*
purc_variant_register_post_listener(purc_variant_t v,
        pcvar_op_t op, pcvar_op_handler handler, void *ctxt)
{
    if (v == PURC_VARIANT_INVALID || !op || !handler) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return NULL;
    }

    if (!IS_CONTAINER(v->type)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_SUPPORTED);
        return NULL;
    }

    return register_listener(v, PCVAR_LISTENER_POST, op, handler, ctxt);
}

bool
purc_variant_revoke_listener(purc_variant_t v,
        struct pcvar_listener *listener)
{
    if (v == PURC_VARIANT_INVALID || !listener) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return false;
    }

    if (!IS_CONTAINER(v->type)) {
        pcinst_set_error(PCVARIANT_ERROR_NOT_SUPPORTED);
        return false;
    }

    struct list_head *listeners;
    listeners = &v->listeners;

    struct list_head *p, *n;
    list_for_each_safe(p, n, listeners) {
        struct pcvar_listener *curr;
        curr = container_of(p, struct pcvar_listener, list_node);
        if (curr != listener)
            continue;

        list_del(p);
        free(curr);
        return true;
    }

    return false;
}

bool pcvariant_on_pre_fired(
        purc_variant_t source,  // the source variant.
        pcvar_op_t op,          // the operation identifier.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        )
{
    PC_ASSERT(op != PCVAR_OPERATION_ANY);

    struct list_head *listeners;
    listeners = &source->listeners;

    struct list_head *p, *n;
    list_for_each_safe(p, n, listeners) {
        struct pcvar_listener *curr;
        curr = container_of(p, struct pcvar_listener, list_node);
        if (curr->op != op && curr->op != PCVAR_OPERATION_ANY)
            continue;

        if ((curr->flags & PCVAR_LISTENER_PRE_OR_POST) != PCVAR_LISTENER_PRE)
            break;

        bool ok = curr->handler(source, op, curr->ctxt, nr_args, argv);
        if (!ok)
            return false;
    }

    return true;
}

void pcvariant_on_post_fired(
        purc_variant_t source,  // the source variant.
        pcvar_op_t op,          // the operation identifier.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        )
{
    PC_ASSERT(op != PCVAR_OPERATION_ANY);

    struct list_head *listeners;
    listeners = &source->listeners;

    struct pcvar_listener *p, *n;
    list_for_each_entry_reverse_safe(p, n, listeners, list_node) {
        struct pcvar_listener *curr = p;
        PC_ASSERT(curr);
        if (curr->op != op && curr->op != PCVAR_OPERATION_ANY)
            continue;

        if ((curr->flags & PCVAR_LISTENER_PRE_OR_POST) == PCVAR_LISTENER_PRE)
            break;

        bool ok = curr->handler(source, op, curr->ctxt, nr_args, argv);
        PC_ASSERT(ok);
    }
}

