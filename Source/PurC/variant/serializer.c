/*
 * @file serializer.c
 * @author Vincent Wei
 * @date 2021/07/12
 * @brief The serializer of variant.
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

#include "purc-variant.h"
#include "purc-rwstream.h"
#include "private/variant.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t
serialize_string(purc_rwstream_t rws, const char* content,
        size_t sz_content, unsigned int opts)
{
    UNUSED_PARAM(rws);
    UNUSED_PARAM(content);
    UNUSED_PARAM(sz_content);
    UNUSED_PARAM(opts);

    return 0;
}

static size_t
serialize_bsequence(purc_rwstream_t rws, const char* content,
        size_t sz_content, unsigned int opts)
{
    UNUSED_PARAM(rws);
    UNUSED_PARAM(content);
    UNUSED_PARAM(sz_content);
    UNUSED_PARAM(opts);

    return 0;
}

size_t purc_variant_serialize(purc_variant_t value,
        purc_rwstream_t rws, unsigned int opts)
{
    ssize_t nr_written = 0;
    const char* content = NULL;
    size_t sz_content = 0;
    char buff [128];

    PC_ASSERT(value);

    purc_variant_t member = NULL;

    switch (value->type) {
        case PURC_VARIANT_TYPE_NULL:
            content = "null";
            sz_content = 4;
            break;

        case PURC_VARIANT_TYPE_UNDEFINED:
            content = "undefined";
            sz_content = 9;
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (value->b) {
                content = "true";
                sz_content = 4;
            }
            else {
                content = "false";
                sz_content = 5;
            }
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            sprintf(buff, "%f", value->d);
            content = buff;
            sz_content = strlen(buff);
            break;

        case PURC_VARIANT_TYPE_LONGINT:
            sprintf(buff, "%ld", value->i64);
            content = buff;
            sz_content = strlen(buff);
            break;

        case PURC_VARIANT_TYPE_LONGUINT:
            sprintf(buff, "%lu", value->u64);
            content = buff;
            sz_content = strlen(buff);
            break;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            sprintf(buff, "%Lf", value->ld);
            content = buff;
            sz_content = strlen(buff);
            break;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            content = purc_atom_to_string(value->sz_ptr[1]);
            sz_content = strlen(content);
            nr_written +=
                serialize_string(rws, content, sz_content, opts);
            content = NULL;
            break;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (value->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                content = (void*)value->sz_ptr[1];
                sz_content = (size_t)value->sz_ptr[0];
            }
            else {
                content = (char*)value->bytes;
                sz_content = value->size;
            }
            if (value->type == PURC_VARIANT_TYPE_STRING)
                nr_written +=
                    serialize_string(rws, content, sz_content, opts);
            else
                nr_written +=
                    serialize_bsequence(rws, content, sz_content, opts);
            content = NULL;
            break;

        case PURC_VARIANT_TYPE_DYNAMIC:
            content = "<dynamic>";
            sz_content = 9;
            break;

        case PURC_VARIANT_TYPE_NATIVE:
            content = "<native>";
            sz_content = 8;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            content = NULL;
            nr_written += purc_rwstream_write(rws, "{", 1);
            foreach_value_in_variant_object(value, member)
                nr_written += purc_variant_serialize(member, rws, opts);
                nr_written += purc_rwstream_write(rws, ",", 1);
            end_foreach;
            nr_written += purc_rwstream_write(rws, "}", 1);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            content = NULL;
            nr_written += purc_rwstream_write(rws, "[", 1);
            foreach_value_in_variant_array(value, member)
                nr_written += purc_variant_serialize(member, rws, opts);
                nr_written += purc_rwstream_write(rws, ",", 1);
            end_foreach;
            nr_written += purc_rwstream_write(rws, "]", 1);
            break;

        case PURC_VARIANT_TYPE_SET:
            content = NULL;
            nr_written += purc_rwstream_write(rws, "[", 1);
            foreach_value_in_variant_set(value, member)
                nr_written += purc_variant_serialize(member, rws, opts);
                nr_written += purc_rwstream_write(rws, ",", 1);
            end_foreach;
            nr_written += purc_rwstream_write(rws, "]", 1);
            break;

        default:
            break;
    }

    if (content) {
        // for simple types
        nr_written = purc_rwstream_write(rws, content, strlen (content));
    }

    buff [0] = 0;
    // write an null character.
    nr_written += purc_rwstream_write(rws, buff, 1);
    return nr_written;
}

