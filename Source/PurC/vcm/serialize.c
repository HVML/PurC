/*
 * @file serialize.c
 * @author XueShuming
 * @date 2021/09/06
 * @brief The impl of vcm node serialize.
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
#include "purc-rwstream.h"

#include "private/errors.h"
#include "private/stack.h"
#include "private/interpreter.h"
#include "private/utils.h"
#include "private/vcm.h"

#include "eval.h"

typedef
void (*pcvcm_node_handle)(purc_rwstream_t rws, struct pcvcm_node *node,
        bool ignore_string_quoted);

static
void pcvcm_node_write_to_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
        bool ignore_string_quoted);

static
void pcvcm_node_serialize_to_rwstream(purc_rwstream_t rws,
        struct pcvcm_node *node, bool ignore_string_quoted);

static struct pcvcm_node *
pcvcm_node_next_child(struct pcvcm_node *node)
{
    if (node) {
        return (struct pcvcm_node *)pctree_node_next(&node->tree_node);
    }
    return NULL;
}

static void
write_child_node_rwstream_ex(purc_rwstream_t rws, struct pcvcm_node *node,
        bool print_comma, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = pcvcm_node_first_child(node);
    while (child) {
        if (node->type == PCVCM_NODE_TYPE_CONSTANT) {
            purc_atom_t atom = (purc_atom_t)child->u64;
            const char *s = purc_atom_to_string(atom);
            purc_rwstream_write(rws, s, strlen(s));
            child = pcvcm_node_next_child(child);
            if (child) {
                purc_rwstream_write(rws, " ", 1);
            }
        }
        else {
            handle(rws, child, false);
            child = pcvcm_node_next_child(child);
            if (child && print_comma) {
                purc_rwstream_write(rws, ",", 1);
            }
        }
    }
}

static void
write_child_node_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
         pcvcm_node_handle handle)
{
    write_child_node_rwstream_ex(rws, node, true, handle);
}

static void
write_object_serialize_to_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
         pcvcm_node_handle handle)
{
    struct pcvcm_node *child = pcvcm_node_first_child(node);
    int i = 0;
    while (child) {
        handle(rws, child, false);
        child = pcvcm_node_next_child(child);
        if (child) {
            if (i % 2 == 0) {
                purc_rwstream_write(rws, ":", 1);
            }
            else {
                purc_rwstream_write(rws, ", ", 2);
            }
        }
        i++;
    }
}

static void
write_concat_string_node_serialize_rwstream(purc_rwstream_t rws,
        struct pcvcm_node *node, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = pcvcm_node_first_child(node);
    while (child) {
        handle(rws, child, true);
        child = pcvcm_node_next_child(child);
    }
}

static void
write_sibling_node_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
        bool print_comma, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = pcvcm_node_next_child(node);
    while (child) {
        handle(rws, child, false);
        child = pcvcm_node_next_child(child);
        if (child && print_comma) {
            purc_rwstream_write(rws, ", ", 2);
        }
    }
}


static void
write_variant_to_rwstream(purc_rwstream_t rws, purc_variant_t v)
{
    size_t len_expected = 0;
    purc_variant_serialize(v, rws, 0,
            PCVARIANT_SERIALIZE_OPT_REAL_EJSON |
            PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BASE64 |
            PCVARIANT_SERIALIZE_OPT_PLAIN,
            &len_expected);
}

void
pcvcm_node_write_to_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
        bool ignore_string_quoted)
{
    UNUSED_PARAM(ignore_string_quoted);
    pcvcm_node_handle handle = pcvcm_node_write_to_rwstream;
    switch(node->type)
    {
    case PCVCM_NODE_TYPE_UNDEFINED:
        purc_rwstream_write(rws, "undefined", 9);
        break;

    case PCVCM_NODE_TYPE_OBJECT:
        purc_rwstream_write(rws, "make_object(", 12);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_ARRAY:
        purc_rwstream_write(rws, "make_array(", 11);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_TUPLE:
        purc_rwstream_write(rws, "make_tuple(", 11);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_STRING:
        purc_rwstream_write(rws, "\"", 1);
        purc_rwstream_write(rws, (char*)node->sz_ptr[1],
                node->sz_ptr[0]);
        purc_rwstream_write(rws, "\"", 1);
        break;

    case PCVCM_NODE_TYPE_NULL:
        purc_rwstream_write(rws, "null", 4);
        break;

    case PCVCM_NODE_TYPE_BOOLEAN:
    {
        purc_variant_t v = purc_variant_make_boolean(node->b);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_NUMBER:
    {
        purc_variant_t v = purc_variant_make_number(node->d);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_INT:
    {
        purc_variant_t v = purc_variant_make_longint(node->i64);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_ULONG_INT:
    {
        purc_variant_t v = purc_variant_make_ulongint(node->u64);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_DOUBLE:
    {
        purc_variant_t v = purc_variant_make_longdouble(node->ld);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
    {
        purc_variant_t v;
        if (node->sz_ptr[0]) {
            v = purc_variant_make_byte_sequence(
                (void*)node->sz_ptr[1], node->sz_ptr[0]);
        }
        else {
            v = purc_variant_make_byte_sequence_empty();
        }
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CONCAT_STRING:
        purc_rwstream_write(rws, "concatString(", 13);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_VARIABLE:
        purc_rwstream_write(rws, "getVariable(", 12);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_ELEMENT:
        purc_rwstream_write(rws, "getElement(", 11);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_CALL_GETTER:
        purc_rwstream_write(rws, "callGetter(", 11);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_CALL_SETTER:
        purc_rwstream_write(rws, "callSetter(", 11);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, ")", 1);
        break;
    case PCVCM_NODE_TYPE_CJSONEE:
        purc_rwstream_write(rws, "{{ ", 3);
        write_child_node_rwstream_ex(rws, node, false, handle);
        purc_rwstream_write(rws, " }}", 3);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
        purc_rwstream_write(rws, " && ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
        purc_rwstream_write(rws, " || ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        purc_rwstream_write(rws, " ; ", 3);
        break;
    case PCVCM_NODE_TYPE_CONSTANT:
        purc_rwstream_write(rws, "`", 1);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, "`", 1);
        break;
    }
}

void
pcvcm_node_serialize_to_rwstream(purc_rwstream_t rws, struct pcvcm_node *node,
        bool ignore_string_quoted)
{
    pcvcm_node_handle handle = pcvcm_node_serialize_to_rwstream;
    switch(node->type)
    {
    case PCVCM_NODE_TYPE_UNDEFINED:
        purc_rwstream_write(rws, "undefined", 9);
        break;

    case PCVCM_NODE_TYPE_OBJECT:
        purc_rwstream_write(rws, "{ ", 2);
        write_object_serialize_to_rwstream(rws, node, handle);
        purc_rwstream_write(rws, " }", 2);
        break;

    case PCVCM_NODE_TYPE_ARRAY:
        purc_rwstream_write(rws, "[ ", 2);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, " ]", 2);
        break;

    case PCVCM_NODE_TYPE_TUPLE:
        purc_rwstream_write(rws, "[! ", 2);
        write_child_node_rwstream(rws, node, handle);
        purc_rwstream_write(rws, " ]", 2);
        break;

    case PCVCM_NODE_TYPE_STRING:
    {
        char *buf = (char*)node->sz_ptr[1];
        size_t nr_buf = node->sz_ptr[0];
        char c[4] = {0};
        c[0] = '"';
        if (strchr(buf, '"')) {
            c[0] = '\'';
        }
        if (strchr(buf, '\n')) {
            c[0] = '"';
            c[1] = '"';
            c[2] = '"';
        }
        if (!ignore_string_quoted) {
            purc_rwstream_write(rws, &c, strlen(c));
        }
        for (size_t i = 0; i < nr_buf; i++) {
            if (buf[i] == '\\') {
                purc_rwstream_write(rws, "\\", 1);
            }
            purc_rwstream_write(rws, buf + i, 1);
        }
        if (!ignore_string_quoted) {
            purc_rwstream_write(rws, &c, strlen(c));
        }
        break;
    }

    case PCVCM_NODE_TYPE_NULL:
        purc_rwstream_write(rws, "null", 4);
        break;

    case PCVCM_NODE_TYPE_BOOLEAN:
    {
        purc_variant_t v = purc_variant_make_boolean(node->b);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_NUMBER:
    {
        purc_variant_t v = purc_variant_make_number(node->d);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_INT:
    {
        purc_variant_t v = purc_variant_make_longint(node->i64);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_ULONG_INT:
    {
        purc_variant_t v = purc_variant_make_ulongint(node->u64);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_DOUBLE:
    {
        purc_variant_t v = purc_variant_make_longdouble(node->ld);
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_BYTE_SEQUENCE:
    {
        purc_variant_t v;
        if (node->sz_ptr[0]) {
            v = purc_variant_make_byte_sequence(
                (void*)node->sz_ptr[1], node->sz_ptr[0]);
        }
        else {
            v = purc_variant_make_byte_sequence_empty();
        }
        write_variant_to_rwstream(rws, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CONCAT_STRING:
        purc_rwstream_write(rws, "\"", 1);
        write_concat_string_node_serialize_rwstream(rws, node, handle);
        purc_rwstream_write(rws, "\"", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_VARIABLE:
    {
        purc_rwstream_write(rws, "$", 1);
        struct pcvcm_node *child = pcvcm_node_first_child(node);
        handle(rws, child, true);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_GET_ELEMENT:
    {
        struct pcvcm_node *child = pcvcm_node_first_child(node);
        handle(rws, child, true);

        child = pcvcm_node_next_child(child);
        if (child->type == PCVCM_NODE_TYPE_STRING) {
            purc_rwstream_write(rws, ".", 1);
            handle(rws, child, true);
        }
        else {
            purc_rwstream_write(rws, "[", 1);
            handle(rws, child, true);
            purc_rwstream_write(rws, "]", 1);
        }
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CALL_GETTER:
    {
        struct pcvcm_node *child = pcvcm_node_first_child(node);
        handle(rws, child, true);
        purc_rwstream_write(rws, "( ", 2);
        write_sibling_node_rwstream(rws, child, true, handle);
        purc_rwstream_write(rws, " )", 2);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CALL_SETTER:
    {
        struct pcvcm_node *child = pcvcm_node_first_child(node);
        handle(rws, child, true);
        purc_rwstream_write(rws, "(! ", 2);
        write_sibling_node_rwstream(rws, child, true, handle);
        purc_rwstream_write(rws, " )", 2);
        break;
    }

    case PCVCM_NODE_TYPE_CJSONEE:
    {
        purc_rwstream_write(rws, "{{ ", 3);
        write_child_node_rwstream_ex(rws, node, false, handle);
        purc_rwstream_write(rws, " }}", 3);
        break;
    }

    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
        purc_rwstream_write(rws, " && ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
        purc_rwstream_write(rws, " || ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        purc_rwstream_write(rws, " ; ", 3);
        break;

    case PCVCM_NODE_TYPE_CONSTANT:
    {
        purc_rwstream_write(rws, "`", 1);
        write_child_node_rwstream_ex(rws, node, false, handle);
        purc_rwstream_write(rws, "`", 1);
        break;
    }
        break;
    }
}

static char *
pcvcm_node_dump(struct pcvcm_node *node, size_t *nr_bytes,
        pcvcm_node_handle handle)
{
    if (!node) {
        if (nr_bytes) {
            *nr_bytes = 0;
        }
        return NULL;
    }

    purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUF_SIZE, MAX_BUF_SIZE);
    if (!rws) {
        if (nr_bytes) {
            *nr_bytes = 0;
        }
        return NULL;
    }

    handle(rws, node, false);

    purc_rwstream_write(rws, "", 1); // writing null-terminator

    size_t sz_content = 0;
    char *buf = (char*)purc_rwstream_get_mem_buffer_ex(rws, &sz_content,
        NULL, true);
    if (nr_bytes) {
        *nr_bytes = sz_content - 1;
    }

    purc_rwstream_destroy(rws);
    return buf;
}


char *
pcvcm_node_to_string(struct pcvcm_node *node, size_t *nr_bytes)
{
    return pcvcm_node_dump(node, nr_bytes, pcvcm_node_write_to_rwstream);
}

char *
pcvcm_node_serialize(struct pcvcm_node *node, size_t *nr_bytes)
{
    return pcvcm_node_dump(node, nr_bytes, pcvcm_node_serialize_to_rwstream);
}

