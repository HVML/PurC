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

struct pcvdom_dump_ctxt
{
    purc_rwstream_t rws;
    purc_rwstream_t err_rws;
    struct pcvcm_node *err_node;
    bool oneline;
};

typedef
void (*pcvcm_node_handle)(struct pcvdom_dump_ctxt *ctxt, struct pcvcm_node *node,
        bool ignore_string_quoted);

static
void pcvcm_node_write_to_rwstream(struct pcvdom_dump_ctxt *ctxt,
        struct pcvcm_node *node, bool ignore_string_quoted);

static
void pcvcm_node_serialize_to_rwstream(struct pcvdom_dump_ctxt *ctxt,
        struct pcvcm_node *node, bool ignore_string_quoted);

static void
write_space(purc_rwstream_t rws, size_t count)
{
    char buf[count + 1];
    memset(buf, ' ', count);
    purc_rwstream_write(rws, buf, count);
}


static ssize_t
pcvdom_dump_write(struct pcvdom_dump_ctxt *ctxt, const void* buf, size_t count)
{
    ssize_t r = purc_rwstream_write(ctxt->rws, buf, count);
    if (ctxt->err_rws && r > 0) {
        write_space(ctxt->err_rws, r);
    }
    return r;
}

static struct pcvcm_node *
pcvcm_node_next_child(struct pcvcm_node *node)
{
    if (node) {
        return (struct pcvcm_node *)pctree_node_next(&node->tree_node);
    }
    return NULL;
}

static void
write_child_node_rwstream_ex(struct pcvdom_dump_ctxt *ctxt, struct pcvcm_node *node,
        bool print_comma, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = pcvcm_node_first_child(node);
    while (child) {
        if (node->type == PCVCM_NODE_TYPE_CONSTANT) {
            purc_atom_t atom = (purc_atom_t)child->u64;
            const char *s = purc_atom_to_string(atom);
            pcvdom_dump_write(ctxt, s, strlen(s));
            child = pcvcm_node_next_child(child);
            if (child) {
                pcvdom_dump_write(ctxt, " ", 1);
            }
        }
        else {
            handle(ctxt, child, false);
            child = pcvcm_node_next_child(child);
            if (child && print_comma) {
                pcvdom_dump_write(ctxt, ", ", 2);
            }
        }
    }
}

static void
write_child_node_rwstream(struct pcvdom_dump_ctxt *ctxt, struct pcvcm_node *node,
         pcvcm_node_handle handle)
{
    write_child_node_rwstream_ex(ctxt, node, true, handle);
}

static void
write_object_serialize_to_rwstream(struct pcvdom_dump_ctxt *ctxt,
        struct pcvcm_node *node, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = pcvcm_node_first_child(node);
    int i = 0;
    while (child) {
        handle(ctxt, child, false);
        child = pcvcm_node_next_child(child);
        if (child) {
            if (i % 2 == 0) {
                pcvdom_dump_write(ctxt, ":", 1);
            }
            else {
                pcvdom_dump_write(ctxt, ", ", 2);
            }
        }
        i++;
    }
}

static void
write_concat_string_node_serialize_rwstream(struct pcvdom_dump_ctxt *ctxt,
        struct pcvcm_node *node, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = pcvcm_node_first_child(node);
    while (child) {
        handle(ctxt, child, true);
        child = pcvcm_node_next_child(child);
    }
}

static void
write_sibling_node_rwstream(struct pcvdom_dump_ctxt *ctxt, struct pcvcm_node *node,
        bool print_comma, pcvcm_node_handle handle)
{
    struct pcvcm_node *child = pcvcm_node_next_child(node);
    while (child) {
        handle(ctxt, child, false);
        child = pcvcm_node_next_child(child);
        if (child && print_comma) {
            pcvdom_dump_write(ctxt, ", ", 2);
        }
    }
}


static void
write_variant_to_rwstream(struct pcvdom_dump_ctxt *ctxt, purc_variant_t v)
{
    size_t len_expected = 0;
    ssize_t r = purc_variant_serialize(v, ctxt->rws, 0,
            PCVRNT_SERIALIZE_OPT_REAL_EJSON |
            PCVRNT_SERIALIZE_OPT_BSEQUENCE_BASE64 |
            PCVRNT_SERIALIZE_OPT_PLAIN |
            PCVRNT_SERIALIZE_OPT_NOSLASHESCAPE |
            PCVRNT_SERIALIZE_OPT_RUNTIME_STRING,
            &len_expected);

    if (ctxt->err_rws && r > 0) {
        write_space(ctxt->err_rws, r);
    }
}

void
pcvcm_node_write_to_rwstream(struct pcvdom_dump_ctxt *ctxt, struct pcvcm_node *node,
        bool ignore_string_quoted)
{
    UNUSED_PARAM(ignore_string_quoted);
    pcvcm_node_handle handle = pcvcm_node_write_to_rwstream;
    if (node == ctxt->err_node && ctxt->err_rws) {
        purc_rwstream_write(ctxt->err_rws, "^", 1);
    }
    switch(node->type)
    {
    case PCVCM_NODE_TYPE_UNDEFINED:
        pcvdom_dump_write(ctxt, "undefined", 9);
        break;

    case PCVCM_NODE_TYPE_OBJECT:
        pcvdom_dump_write(ctxt, "make_object(", 12);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, ")", 1);
        break;

    case PCVCM_NODE_TYPE_ARRAY:
        pcvdom_dump_write(ctxt, "make_array(", 11);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, ")", 1);
        break;

    case PCVCM_NODE_TYPE_TUPLE:
        pcvdom_dump_write(ctxt, "make_tuple(", 11);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, ")", 1);
        break;

    case PCVCM_NODE_TYPE_STRING:
    {
        char *buf = (char*)node->sz_ptr[1];
        size_t nr_buf = node->sz_ptr[0];

        pcvdom_dump_write(ctxt, "\"", 1);
        for (size_t i = 0; i < nr_buf; i++) {
            if (ctxt->oneline) {
                if (buf[i] == '\n') {
                    pcvdom_dump_write(ctxt, "\\", 1);
                    pcvdom_dump_write(ctxt, "n", 1);
                    continue;
                }
                if (buf[i] == '\r') {
                    pcvdom_dump_write(ctxt, "\\", 1);
                    pcvdom_dump_write(ctxt, "r", 1);
                    continue;
                }
                if (buf[i] == '\t') {
                    pcvdom_dump_write(ctxt, "\\", 1);
                    pcvdom_dump_write(ctxt, "t", 1);
                    continue;
                }
            }
            pcvdom_dump_write(ctxt, buf + i, 1);
        }
        pcvdom_dump_write(ctxt, "\"", 1);
        break;
    }

    case PCVCM_NODE_TYPE_NULL:
        pcvdom_dump_write(ctxt, "null", 4);
        break;

    case PCVCM_NODE_TYPE_BOOLEAN:
    {
        purc_variant_t v = purc_variant_make_boolean(node->b);
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_NUMBER:
    {
        purc_variant_t v = purc_variant_make_number(node->d);
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_INT:
    {
        purc_variant_t v = purc_variant_make_longint(node->i64);
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_ULONG_INT:
    {
        purc_variant_t v = purc_variant_make_ulongint(node->u64);
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_DOUBLE:
    {
        purc_variant_t v = purc_variant_make_longdouble(node->ld);
        write_variant_to_rwstream(ctxt, v);;
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
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CONCAT_STRING:
        pcvdom_dump_write(ctxt, "concatString(", 13);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_VARIABLE:
        pcvdom_dump_write(ctxt, "getVariable(", 12);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_GET_ELEMENT:
        pcvdom_dump_write(ctxt, "getElement(", 11);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_CALL_GETTER:
        pcvdom_dump_write(ctxt, "callGetter(", 11);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, ")", 1);
        break;

    case PCVCM_NODE_TYPE_FUNC_CALL_SETTER:
        pcvdom_dump_write(ctxt, "callSetter(", 11);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, ")", 1);
        break;
    case PCVCM_NODE_TYPE_CJSONEE:
        pcvdom_dump_write(ctxt, "{{ ", 3);
        write_child_node_rwstream_ex(ctxt, node, false, handle);
        pcvdom_dump_write(ctxt, " }}", 3);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
        pcvdom_dump_write(ctxt, " && ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
        pcvdom_dump_write(ctxt, " || ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        pcvdom_dump_write(ctxt, " ; ", 3);
        break;
    case PCVCM_NODE_TYPE_CONSTANT:
        pcvdom_dump_write(ctxt, "`", 1);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, "`", 1);
        break;
    }
}

void
pcvcm_node_serialize_to_rwstream(struct pcvdom_dump_ctxt *ctxt,
        struct pcvcm_node *node, bool ignore_string_quoted)
{
    pcvcm_node_handle handle = pcvcm_node_serialize_to_rwstream;
    if (node == ctxt->err_node && ctxt->err_rws) {
        purc_rwstream_write(ctxt->err_rws, "^", 1);
    }
    switch(node->type)
    {
    case PCVCM_NODE_TYPE_UNDEFINED:
        pcvdom_dump_write(ctxt, "undefined", 9);
        break;

    case PCVCM_NODE_TYPE_OBJECT:
        pcvdom_dump_write(ctxt, "{ ", 2);
        write_object_serialize_to_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, " }", 2);
        break;

    case PCVCM_NODE_TYPE_ARRAY:
        pcvdom_dump_write(ctxt, "[ ", 2);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, " ]", 2);
        break;

    case PCVCM_NODE_TYPE_TUPLE:
        pcvdom_dump_write(ctxt, "[! ", 2);
        write_child_node_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, " ]", 2);
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
        if (!ctxt->oneline && strchr(buf, '\n')) {
            c[0] = '"';
            c[1] = '"';
            c[2] = '"';
        }
        if (!ignore_string_quoted) {
            pcvdom_dump_write(ctxt, &c, strlen(c));
        }
        for (size_t i = 0; i < nr_buf; i++) {
            if (buf[i] == '\\') {
                pcvdom_dump_write(ctxt, "\\", 1);
            }
            if (ctxt->oneline) {
                if (buf[i] == '\n') {
                    pcvdom_dump_write(ctxt, "\\", 1);
                    pcvdom_dump_write(ctxt, "n", 1);
                    continue;
                }
                if (buf[i] == '\r') {
                    pcvdom_dump_write(ctxt, "\\", 1);
                    pcvdom_dump_write(ctxt, "r", 1);
                    continue;
                }
                if (buf[i] == '\t') {
                    pcvdom_dump_write(ctxt, "\\", 1);
                    pcvdom_dump_write(ctxt, "t", 1);
                    continue;
                }
            }
            pcvdom_dump_write(ctxt, buf + i, 1);
        }
        if (!ignore_string_quoted) {
            pcvdom_dump_write(ctxt, &c, strlen(c));
        }
        break;
    }

    case PCVCM_NODE_TYPE_NULL:
        pcvdom_dump_write(ctxt, "null", 4);
        break;

    case PCVCM_NODE_TYPE_BOOLEAN:
    {
        purc_variant_t v = purc_variant_make_boolean(node->b);
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_NUMBER:
    {
        purc_variant_t v = purc_variant_make_number(node->d);
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_INT:
    {
        purc_variant_t v = purc_variant_make_longint(node->i64);
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_ULONG_INT:
    {
        purc_variant_t v = purc_variant_make_ulongint(node->u64);
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_LONG_DOUBLE:
    {
        purc_variant_t v = purc_variant_make_longdouble(node->ld);
        write_variant_to_rwstream(ctxt, v);;
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
        write_variant_to_rwstream(ctxt, v);;
        purc_variant_unref(v);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CONCAT_STRING:
    {
        char c[4] = {0};
        c[0] = '"';

        if (!ctxt->oneline) {
            struct pcvcm_node *child = pcvcm_node_first_child(node);
            while (child) {
                if (child->type == PCVCM_NODE_TYPE_STRING) {
                    char *buf = (char*)child->sz_ptr[1];
                    if (buf && strchr(buf, '\n')) {
                        c[0] = '"';
                        c[1] = '"';
                        c[2] = '"';
                        break;
                    }
                }
                child = pcvcm_node_next_child(child);
            }
        }

        pcvdom_dump_write(ctxt, &c, strlen(c));
        write_concat_string_node_serialize_rwstream(ctxt, node, handle);
        pcvdom_dump_write(ctxt, &c, strlen(c));
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_GET_VARIABLE:
    {
        pcvdom_dump_write(ctxt, "$", 1);
        struct pcvcm_node *child = pcvcm_node_first_child(node);
        handle(ctxt, child, true);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_GET_ELEMENT:
    {
        struct pcvcm_node *child = pcvcm_node_first_child(node);
        handle(ctxt, child, true);

        child = pcvcm_node_next_child(child);
        if (child->type == PCVCM_NODE_TYPE_STRING) {
            pcvdom_dump_write(ctxt, ".", 1);
            handle(ctxt, child, true);
        }
        else {
            pcvdom_dump_write(ctxt, "[", 1);
            handle(ctxt, child, true);
            pcvdom_dump_write(ctxt, "]", 1);
        }
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CALL_GETTER:
    {
        struct pcvcm_node *child = pcvcm_node_first_child(node);
        handle(ctxt, child, true);
        pcvdom_dump_write(ctxt, "( ", 2);
        write_sibling_node_rwstream(ctxt, child, true, handle);
        pcvdom_dump_write(ctxt, " )", 2);
        break;
    }

    case PCVCM_NODE_TYPE_FUNC_CALL_SETTER:
    {
        struct pcvcm_node *child = pcvcm_node_first_child(node);
        handle(ctxt, child, true);
        pcvdom_dump_write(ctxt, "(! ", 2);
        write_sibling_node_rwstream(ctxt, child, true, handle);
        pcvdom_dump_write(ctxt, " )", 2);
        break;
    }

    case PCVCM_NODE_TYPE_CJSONEE:
    {
        pcvdom_dump_write(ctxt, "{{ ", 3);
        write_child_node_rwstream_ex(ctxt, node, false, handle);
        pcvdom_dump_write(ctxt, " }}", 3);
        break;
    }

    case PCVCM_NODE_TYPE_CJSONEE_OP_AND:
        pcvdom_dump_write(ctxt, " && ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_OR:
        pcvdom_dump_write(ctxt, " || ", 4);
        break;
    case PCVCM_NODE_TYPE_CJSONEE_OP_SEMICOLON:
        pcvdom_dump_write(ctxt, " ; ", 3);
        break;

    case PCVCM_NODE_TYPE_CONSTANT:
    {
        pcvdom_dump_write(ctxt, "`", 1);
        write_child_node_rwstream_ex(ctxt, node, false, handle);
        pcvdom_dump_write(ctxt, "`", 1);
        break;
    }
        break;
    }
}

static char *
pcvcm_node_dump(struct pcvcm_node *node, size_t *nr_bytes,
        struct pcvcm_node *err_node,  char **err_msg, size_t *nr_err_msg,
        pcvcm_node_handle handle)
{
    if (!node) {
        if (nr_bytes) {
            *nr_bytes = 0;
        }
        return NULL;
    }

    struct pcvdom_dump_ctxt ctxt = {0};

    purc_rwstream_t rws = purc_rwstream_new_buffer(MIN_BUF_SIZE, MAX_BUF_SIZE);
    if (!rws) {
        if (nr_bytes) {
            *nr_bytes = 0;
        }
        return NULL;
    }

    purc_rwstream_t err_rws = NULL;
    if (err_msg) {
        err_rws = purc_rwstream_new_buffer(MIN_BUF_SIZE, MAX_BUF_SIZE);
        if (!err_rws) {
            if (nr_err_msg) {
                *nr_err_msg = 0;
            }
            purc_rwstream_destroy(rws);
            return NULL;
        }
    }

    ctxt.rws = rws;
    ctxt.err_rws = err_rws;
    ctxt.err_node = err_node;
    ctxt.oneline = err_node ? true : false;

    handle(&ctxt, node, false);

    pcvdom_dump_write(&ctxt, "", 1); // writing null-terminator

    size_t sz_content = 0;
    char *buf = (char*)purc_rwstream_get_mem_buffer_ex(rws, &sz_content,
        NULL, true);
    if (nr_bytes) {
        *nr_bytes = sz_content - 1;
    }

    if (err_rws) {
        size_t sz_err_buf = 0;
        char *err_buf = (char*)purc_rwstream_get_mem_buffer_ex(err_rws, &sz_err_buf,
            NULL, true);
        *err_msg = err_buf;
        char *end = err_buf + sz_err_buf;
        while (end > err_buf) {
            if (!purc_isspace(*(end-1))) {
                break;
            }
            end--;
        }
        sz_err_buf = end - err_buf;
        if (nr_err_msg) {
            *nr_err_msg = sz_err_buf;
        }
    }

    purc_rwstream_destroy(rws);
    return buf;
}


char *
pcvcm_node_to_string_ex(struct pcvcm_node *node, size_t *nr_bytes,
        struct pcvcm_node *err_node, char **err_msg, size_t *nr_err_msg)
{
    return pcvcm_node_dump(node, nr_bytes, err_node, err_msg, nr_err_msg,
            pcvcm_node_write_to_rwstream);
}

char *
pcvcm_node_serialize_ex(struct pcvcm_node *node, size_t *nr_bytes,
        struct pcvcm_node *err_node, char **err_msg, size_t *nr_err_msg)
{
    return pcvcm_node_dump(node, nr_bytes, err_node, err_msg, nr_err_msg,
            pcvcm_node_serialize_to_rwstream);
}

