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

static const char *hex_chars = "0123456789abcdefABCDEF";

static size_t
serialize_string(purc_rwstream_t rws, const char* str,
        size_t len, unsigned int flags)
{
    int nr_written = 0;
    size_t pos = 0, start_offset = 0;
    unsigned char c;

    while (len--) {
        c = str[pos];
        switch (c) {
        case '\b':
        case '\n':
        case '\r':
        case '\t':
        case '\f':
        case '"':
        case '\\':
        case '/':
            if ((flags & PCVARIANT_SERIALIZE_OPT_NOSLASHESCAPE) && c == '/') {
                pos++;
                break;
            }

            if (pos - start_offset > 0)
                nr_written += purc_rwstream_write(rws,
                        str + start_offset, pos - start_offset);

            if (c == '\b')
                nr_written += purc_rwstream_write(rws, "\\b", 2);
            else if (c == '\n')
                nr_written += purc_rwstream_write(rws, "\\n", 2);
            else if (c == '\r')
                nr_written += purc_rwstream_write(rws, "\\r", 2);
            else if (c == '\t')
                nr_written += purc_rwstream_write(rws, "\\t", 2);
            else if (c == '\f')
                nr_written += purc_rwstream_write(rws, "\\f", 2);
            else if (c == '"')
                nr_written += purc_rwstream_write(rws, "\\\"", 2);
            else if (c == '\\')
                nr_written += purc_rwstream_write(rws, "\\\\", 2);
            else if (c == '/')
                nr_written += purc_rwstream_write(rws, "\\/", 2);

            start_offset = ++pos;
            break;
        default:
            if (c < ' ')
            {
                char sbuf[7];
                if (pos - start_offset > 0)
                    nr_written += purc_rwstream_write(rws, str + start_offset,
                                       pos - start_offset);
                snprintf(sbuf, sizeof(sbuf), "\\u00%c%c", hex_chars[c >> 4],
                         hex_chars[c & 0xf]);
                purc_rwstream_write(rws, sbuf, (int)sizeof(sbuf) - 1);
                start_offset = ++pos;
            }
            else
                pos++;
        }
    }
    if (pos - start_offset > 0)
        nr_written += purc_rwstream_write(rws,
                str + start_offset, pos - start_offset);

    return nr_written;
}

static size_t
serialize_bsequence(purc_rwstream_t rws, const char* content,
        size_t sz_content, unsigned int flags)
{
    UNUSED_PARAM(rws);
    UNUSED_PARAM(content);
    UNUSED_PARAM(sz_content);
    UNUSED_PARAM(flags);

    return 0;
}

static ssize_t
print_newline(purc_rwstream_t rws, int level, unsigned int flags)
{
    ssize_t nr_written = 0;

    if (flags & PCVARIANT_SERIALIZE_OPT_PRETTY) {
        nr_written += purc_rwstream_write(rws, "\n", 1);
        if (flags & PCVARIANT_SERIALIZE_OPT_PRETTY_TAB) {
            int n = level;
            while (n > 0) {
                nr_written += purc_rwstream_write(rws, "\t", 1);
                n--;
            }
        }
        else {
            int n = level * 2;
            while (n > 0) {
                nr_written += purc_rwstream_write(rws, " ", 1);
                n--;
            }
        }
    }

    return nr_written;
}

static inline ssize_t print_space(purc_rwstream_t rws, unsigned int flags)
{
    ssize_t nr_written = 0;

    if (flags & PCVARIANT_SERIALIZE_OPT_SPACED)
        nr_written = purc_rwstream_write(rws, " ", 1);

    return nr_written;
}

static inline ssize_t print_space_no_pretty(purc_rwstream_t rws,
        unsigned int flags)
{
    ssize_t nr_written = 0;
    if (flags & PCVARIANT_SERIALIZE_OPT_SPACED &&
            !(flags & PCVARIANT_SERIALIZE_OPT_PRETTY))
        nr_written = purc_rwstream_write(rws, " ", 1);

    return nr_written;
}

ssize_t purc_variant_serialize(purc_variant_t value,
        purc_rwstream_t rws, int level, unsigned int flags)
{
    ssize_t nr_written = 0;
    const char* content = NULL;
    size_t sz_content = 0;
    int i;
    char buff [128];
    purc_variant_t member = NULL;
    const char* key;

    PC_ASSERT(value);

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
                serialize_string(rws, content, sz_content, flags);
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
                    serialize_string(rws, content, sz_content, flags);
            else
                nr_written +=
                    serialize_bsequence(rws, content, sz_content, flags);
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
            i = 0;
            nr_written += purc_rwstream_write(rws, "{", 1);
            nr_written += print_newline(rws, level, flags);
            foreach_key_value_in_variant_object(value, key, member)
                if (i > 0) {
                    nr_written += purc_rwstream_write(rws, ",", 1);
                    nr_written += print_space(rws, flags);
                }

                nr_written +=
                    serialize_string(rws, key, strlen(key), flags);
                nr_written += purc_rwstream_write(rws, ":", 1);
                nr_written += print_space(rws, flags);
                nr_written += purc_variant_serialize(member,
                        rws, level + 1, flags);
                i++;
            end_foreach;
            nr_written += print_newline(rws, level, flags);
            nr_written += purc_rwstream_write(rws, "}", 1);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            content = NULL;
            i = 0;
            nr_written += purc_rwstream_write(rws, "[", 1);
            nr_written += print_newline(rws, level, flags);
            foreach_value_in_variant_array(value, member)
                if (i > 0) {
                    nr_written += purc_rwstream_write(rws, ",", 1);
                    nr_written += print_space(rws, flags);
                }
                nr_written += purc_variant_serialize(member,
                        rws, level + 1, flags);
                i++;
            end_foreach;
            nr_written += print_newline(rws, level, flags);
            nr_written += purc_rwstream_write(rws, "]", 1);
            break;

        case PURC_VARIANT_TYPE_SET:
            content = NULL;
            i = 0;
            nr_written += purc_rwstream_write(rws, "[", 1);
            foreach_value_in_variant_set(value, member)
                if (i > 0) {
                    nr_written += purc_rwstream_write(rws, ",", 1);
                    nr_written += print_space(rws, flags);
                }
                nr_written += purc_variant_serialize(member,
                        rws, level + 1, flags);
                i++;
            end_foreach;
            nr_written += purc_rwstream_write(rws, "]", 1);
            break;

        default:
            break;
    }

    if (content) {
        // for simple types
        nr_written += purc_rwstream_write(rws, content, strlen (content));
    }

    return nr_written;
}

