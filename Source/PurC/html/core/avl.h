/**
 * @file avl.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for avl tree.
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
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_AVL_H
#define PCHTML_AVL_H


#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"
#include "html/core/dobject.h"


typedef struct pchtml_avl_node pchtml_avl_node_t;

typedef void (*pchtml_avl_node_f)(pchtml_avl_node_t *avl_node, void *ctx);

struct pchtml_avl_node {
    size_t            type;
    short             height;
    void              *value;

    pchtml_avl_node_t *left;
    pchtml_avl_node_t *right;
    pchtml_avl_node_t *parent;
};

typedef struct {
    pchtml_dobject_t *nodes;
}
pchtml_avl_t;


pchtml_avl_t *
pchtml_avl_create(void) WTF_INTERNAL;

unsigned int
pchtml_avl_init(pchtml_avl_t *avl, size_t chunk_len) WTF_INTERNAL;

void
pchtml_avl_clean(pchtml_avl_t *avl) WTF_INTERNAL;

pchtml_avl_t *
pchtml_avl_destroy(pchtml_avl_t *avl, bool self_destroy) WTF_INTERNAL;


pchtml_avl_node_t *
pchtml_avl_node_make(pchtml_avl_t *avl, size_t type, 
                                        void *value) WTF_INTERNAL;

void
pchtml_avl_node_clean(pchtml_avl_node_t *node) WTF_INTERNAL;

pchtml_avl_node_t *
pchtml_avl_node_destroy(pchtml_avl_t *avl, pchtml_avl_node_t *node,
                        bool self_destroy) WTF_INTERNAL;


pchtml_avl_node_t *
pchtml_avl_insert(pchtml_avl_t *avl, pchtml_avl_node_t **scope,
                  size_t type, void *value) WTF_INTERNAL;

pchtml_avl_node_t *
pchtml_avl_search(pchtml_avl_t *avl, pchtml_avl_node_t *scope, 
                                        size_t type) WTF_INTERNAL;

void *
pchtml_avl_remove(pchtml_avl_t *avl, pchtml_avl_node_t **scope, 
                                        size_t type) WTF_INTERNAL;


void
pchtml_avl_foreach_recursion(pchtml_avl_t *avl, pchtml_avl_node_t *scope,
                             pchtml_avl_node_f callback, void *ctx) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_AVL_H */
