/**
 * @file in.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of in.
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
#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/core/in.h"
#include "html/core/str_res.h"


pchtml_in_t *
pchtml_in_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_in_t));
}

unsigned int
pchtml_in_init(pchtml_in_t *incoming, size_t chunk_size)
{
    if (incoming == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (chunk_size == 0) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    incoming->nodes = pchtml_dobject_create();

    return pchtml_dobject_init(incoming->nodes, chunk_size,
                               sizeof(pchtml_in_node_t));
}

void
pchtml_in_clean(pchtml_in_t *incoming)
{
    pchtml_dobject_clean(incoming->nodes);
}

pchtml_in_t *
pchtml_in_destroy(pchtml_in_t *incoming, bool self_destroy)
{
    if (incoming == NULL) {
        return NULL;
    }

    incoming->nodes = pchtml_dobject_destroy(incoming->nodes, true);

    if (self_destroy == true) {
        return pchtml_free(incoming);
    }

    return incoming;
}

pchtml_in_node_t *
pchtml_in_node_make(pchtml_in_t *incoming, pchtml_in_node_t *last_node,
                    const unsigned char *buf, size_t buf_size)
{
    pchtml_in_node_t *node = pchtml_dobject_alloc(incoming->nodes);

    if (node == NULL) {
        return NULL;
    }

    node->opt = PCHTML_IN_OPT_UNDEF;
    node->begin = buf;
    node->end = buf + buf_size;
    node->use = buf;

    if (last_node != NULL) {
        last_node->next = node;
        node->offset = last_node->offset + (last_node->end - last_node->begin);
    }
    else {
        node->offset = 0;
    }

    node->prev = last_node;
    node->next = NULL;
    node->incoming = incoming;

    return node;
}

void
pchtml_in_node_clean(pchtml_in_node_t *node)
{
    pchtml_in_t *incoming = node->incoming;

    memset(node, 0, sizeof(pchtml_in_node_t));

    node->incoming = incoming;
}

pchtml_in_node_t *
pchtml_in_node_destroy(pchtml_in_t *incoming,
                       pchtml_in_node_t *node, bool self_destroy)
{
    if (node == NULL) {
        return NULL;
    }

    if (self_destroy) {
        return pchtml_dobject_free(incoming->nodes, node);
    }

    return node;
}

pchtml_in_node_t *
pchtml_in_node_split(pchtml_in_node_t *node, const unsigned char *pos)
{
    pchtml_in_node_t *new_node;

    new_node = pchtml_dobject_alloc(node->incoming->nodes);

    if (new_node == NULL) {
        return NULL;
    }

    new_node->offset   = node->offset + (pos - node->begin);
    new_node->opt      = PCHTML_IN_OPT_UNDEF;
    new_node->begin    = pos;
    new_node->end      = node->end;
    new_node->next     = NULL;
    new_node->prev     = node;
    new_node->incoming = node->incoming;

    node->end  = pos;
    node->next = new_node;

    if (node->use > pos) {
        new_node->use = node->use;
        node->use = pos;
    }
    else {
        new_node->use = pos;
    }

    return new_node;
}

pchtml_in_node_t *
pchtml_in_node_find(pchtml_in_node_t *node, const unsigned char *pos)
{
    while (node->next) {
        node = node->next;
    }

    while (node && (node->begin > pos || node->end < pos)) {
        node = node->prev;
    }

    return node;
}

const unsigned char *
pchtml_in_node_pos_up(pchtml_in_node_t *node, pchtml_in_node_t **return_node,
                      const unsigned char *pos, size_t offset)
{
    do {
        pos = pos + offset;

        if (node->end >= pos) {
            if (return_node != NULL && *return_node != node) {
                *return_node = node;
            }

            return pos;
        }

        if (node->next == NULL) {
            if (return_node != NULL && *return_node != node) {
                *return_node = node;
            }

            return node->end;
        }

        offset = pos - node->end;
        node = node->next;
        pos = node->begin;

    }
    while (1);

    return NULL;
}

const unsigned char *
pchtml_in_node_pos_down(pchtml_in_node_t *node, pchtml_in_node_t **return_node,
                        const unsigned char *pos, size_t offset)
{
    do {
        pos = pos - offset;

        if (node->begin <= pos) {
            if (return_node != NULL && *return_node != node) {
                *return_node = node;
            }

            return pos;
        }

        if (node->prev == NULL) {
            if (return_node != NULL && *return_node != node) {
                *return_node = node;
            }

            return node->begin;
        }

        offset = node->begin - pos;
        node = node->prev;
        pos = node->end;

    }
    while (1);

    return NULL;
}

/*
* No inline functions for ABI.
*/
const unsigned char *
pchtml_in_node_begin_noi(const pchtml_in_node_t *node)
{
    return pchtml_in_node_begin(node);
}

const unsigned char *
pchtml_in_node_end_noi(const pchtml_in_node_t *node)
{
    return pchtml_in_node_end(node);
}

size_t
pchtml_in_node_offset_noi(const pchtml_in_node_t *node)
{
    return pchtml_in_node_offset(node);
}

pchtml_in_node_t *
pchtml_in_node_next_noi(const pchtml_in_node_t *node)
{
    return pchtml_in_node_next(node);
}

pchtml_in_node_t *
pchtml_in_node_prev_noi(const pchtml_in_node_t *node)
{
    return pchtml_in_node_prev(node);
}

pchtml_in_t *
pchtml_in_node_in_noi(const pchtml_in_node_t *node)
{
    return pchtml_in_node_in(node);
}

bool
pchtml_in_segment_noi(const pchtml_in_node_t *node, const unsigned char *data)
{
    return pchtml_in_segment(node, data);
}
