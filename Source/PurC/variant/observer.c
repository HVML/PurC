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
#include "private/errors.h"
#include "private/variant.h"

#include <stdlib.h>

void*
purc_variant_register_listener(purc_variant_t v, purc_atom_t name,
        pcvar_msg_handler handler, void *ctxt)
{
    if (v == PURC_VARIANT_INVALID || !name || !handler) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return NULL;
    }

    enum purc_variant_type type;
    type = purc_variant_get_type(v);

    switch (type)
    {
        case PURC_VARIANT_TYPE_OBJECT:
            break;
        case PURC_VARIANT_TYPE_ARRAY:
            break;
        case PURC_VARIANT_TYPE_SET:
            break;
        default:
            pcinst_set_error(PCVARIANT_ERROR_NOT_SUPPORTED);
            return NULL;
    }

    struct list_head *p, *n;
    list_for_each_safe(p, n, &v->listeners) {
        struct pcvar_listener *listener;
        listener = container_of(p, struct pcvar_listener, list_node);
        if (listener->name == name) {
            pcinst_set_error(PCVARIANT_ERROR_DUPLICATED);
            return NULL;
        }
    }

    struct pcvar_listener *listener;
    listener = (struct pcvar_listener*)calloc(1, sizeof(*listener));
    if (!listener) {
        pcinst_set_error(PCVARIANT_ERROR_OUT_OF_MEMORY);
        return false;
    }

    listener->name           = name;
    listener->ctxt           = ctxt;
    listener->handler        = handler;
    list_add_tail(&listener->list_node, &v->listeners);

    return listener;
}

bool
purc_variant_revoke_listener(purc_variant_t v, void *handle)
{
    if (v == PURC_VARIANT_INVALID || !handle) {
        pcinst_set_error(PCVARIANT_ERROR_WRONG_ARGS);
        return false;
    }

    enum purc_variant_type type;
    type = purc_variant_get_type(v);

    switch (type)
    {
        case PURC_VARIANT_TYPE_OBJECT:
            break;
        case PURC_VARIANT_TYPE_ARRAY:
            break;
        case PURC_VARIANT_TYPE_SET:
            break;
        default:
            pcinst_set_error(PCVARIANT_ERROR_NOT_SUPPORTED);
            return false;
    }

    struct list_head *p, *n;
    list_for_each_safe(p, n, &v->listeners) {
        struct pcvar_listener *listener;
        listener = container_of(p, struct pcvar_listener, list_node);
        if (listener == handle) {
            list_del(p);
            free(listener);
            return true;
        }
    }

    return false;
}

