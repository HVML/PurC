/**
 * @file in.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for in.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_IN_H
#define PCHTML_IN_H

#include "config.h"

#include "html/base.h"
#include "private/dobject.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct pchtml_in_node pchtml_in_node_t;
typedef int pchtml_in_opt_t;

enum pchtml_in_opt {
    PCHTML_IN_OPT_UNDEF    = 0x00,
    PCHTML_IN_OPT_READONLY = 0x01,
    PCHTML_IN_OPT_DONE     = 0x02,
    PCHTML_IN_OPT_FAKE     = 0x04,
    PCHTML_IN_OPT_ALLOC    = 0x08
};

typedef struct {
    pcutils_dobject_t *nodes;
}
pchtml_in_t;

struct pchtml_in_node {
    size_t            offset;
    pchtml_in_opt_t   opt;

    const unsigned char  *begin;
    const unsigned char  *end;
    const unsigned char  *use;

    pchtml_in_node_t  *next;
    pchtml_in_node_t  *prev;

    pchtml_in_t       *incoming;
};


pchtml_in_t *
pchtml_in_create(void) WTF_INTERNAL;

unsigned int
pchtml_in_init(pchtml_in_t *incoming, size_t chunk_size) WTF_INTERNAL;

void
pchtml_in_clean(pchtml_in_t *incoming) WTF_INTERNAL;

pchtml_in_t *
pchtml_in_destroy(pchtml_in_t *incoming, bool self_destroy) WTF_INTERNAL;


pchtml_in_node_t *
pchtml_in_node_make(pchtml_in_t *incoming, pchtml_in_node_t *last_node,
                    const unsigned char *buf, size_t buf_size) WTF_INTERNAL;

void
pchtml_in_node_clean(pchtml_in_node_t *node) WTF_INTERNAL;

pchtml_in_node_t *
pchtml_in_node_destroy(pchtml_in_t *incoming,
                       pchtml_in_node_t *node, bool self_destroy) WTF_INTERNAL;


pchtml_in_node_t *
pchtml_in_node_split(pchtml_in_node_t *node, const unsigned char *pos) WTF_INTERNAL;

pchtml_in_node_t *
pchtml_in_node_find(pchtml_in_node_t *node, const unsigned char *pos) WTF_INTERNAL;

/**
 * Get position by `offset`.
 * If position outside of nodes return `begin` position of first node
 * in nodes chain.
 */
const unsigned char *
pchtml_in_node_pos_up(pchtml_in_node_t *node, pchtml_in_node_t **return_node,
                      const unsigned char *pos, size_t offset) WTF_INTERNAL;

/**
 * Get position by `offset`.
 * If position outside of nodes return `end`
 * position of last node in nodes chain.
 */
const unsigned char *
pchtml_in_node_pos_down(pchtml_in_node_t *node, pchtml_in_node_t **return_node,
                        const unsigned char *pos, size_t offset) WTF_INTERNAL;

/*
 * Inline functions
 */
static inline const unsigned char *
pchtml_in_node_begin(const pchtml_in_node_t *node)
{
    return node->begin;
}

static inline const unsigned char *
pchtml_in_node_end(const pchtml_in_node_t *node)
{
    return node->end;
}

static inline size_t
pchtml_in_node_offset(const pchtml_in_node_t *node)
{
    return node->offset;
}

static inline pchtml_in_node_t *
pchtml_in_node_next(const pchtml_in_node_t *node)
{
    return node->next;
}

static inline pchtml_in_node_t *
pchtml_in_node_prev(const pchtml_in_node_t *node)
{
    return node->prev;
}

static inline pchtml_in_t *
pchtml_in_node_in(const pchtml_in_node_t *node)
{
    return node->incoming;
}

static inline bool
pchtml_in_segment(const pchtml_in_node_t *node, const unsigned char *data)
{
    return node->begin <= data && node->end >= data;
}

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_IN_H */
