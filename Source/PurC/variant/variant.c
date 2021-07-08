/*
 * @file variant-public.c
 * @author 
 * @date 2021/07/02
 * @brief The implementation of public part for variant.
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
#include <string.h>

#include "purc-variant.h"
#include "private/variant.h"
#include "private/debug.h"

// TODO: initialize the table here
static pcvariant_release_fn variant_releasers[PURC_VARIANT_TYPE_MAX];

bool purc_variant_is_type(const purc_variant_t value, enum purc_variant_type type)
{
    return (value->type == type);
}

enum purc_variant_type purc_variant_get_type(const purc_variant_t value)
{
    return value->type;
}

unsigned int purc_variant_ref (purc_variant_t value)
{
    PURC_VARIANT_ASSERT(value);

    purc_variant_t variant = NULL;
    switch((int)value->type)
    {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_SEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            value->refc ++;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            foreach_value_in_variant_object(value, variant)
                purc_variant_ref(variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            foreach_value_in_variant_array(value, variant)
                purc_variant_ref(variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_SET:
            foreach_value_in_variant_set(value, variant)
                purc_variant_ref(variant);
            end_foreach;
            break;

        default:
            break;
    }
    return value->refc;
}

unsigned int purc_variant_unref (purc_variant_t value)
{
    PURC_VARIANT_ASSERT(value);
    PURC_VARIANT_ASSERT(value->refc);

    purc_variant_t variant = NULL;

    /* this should not occure */
    if (value->refc == 0) {
        PC_ASSERT (0);
        return 0;
    }

    switch((int)value->type)
    {
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_SEQUENCE:
        case PURC_VARIANT_TYPE_DYNAMIC:
        case PURC_VARIANT_TYPE_NATIVE:
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            foreach_value_in_variant_object(value, variant)
                purc_variant_unref(variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            foreach_value_in_variant_array(value, variant)
                purc_variant_unref(variant);
            end_foreach;
            break;

        case PURC_VARIANT_TYPE_SET:
            foreach_value_in_variant_set(value, variant)
                purc_variant_unref(variant);
            end_foreach;
            break;

        default:
            break;
    }

    value->refc --;

    if(value->refc == 0)
    {
        // release resource occupied by variant
        pcvariant_release_fn release = variant_releasers[value->type];
        if(release) 
            release(value);

        if(value->flags & PCVARIANT_FLAG_NOFREE)
        {
        }
        else
        {
            // release variant
            pcvariant_free_mem(sizeof(struct purc_variant), value);
            return 0;
        }
    }

    return value->refc;
}

#if 0 /* TODO */
purc_variant_t purc_variant_make_from_json_string (const char* json, size_t sz)
{
}

purc_variant_t purc_variant_load_from_json_file (const char* file)
{
}

purc_variant_t purc_variant_load_from_json_stream (purc_rwstream_t stream)
{
}

purc_variant_t purc_variant_dynamic_value_load_from_so (const char* so_name, \
                                                        const char* var_name)
{
}

size_t purc_variant_serialize (purc_variant_t value, purc_rwstream_t stream, \
                                                            unsigned int opts)
{
}

int purc_variant_compare (purc_variant_t v1, purc_variant v2)
{
}
#endif
