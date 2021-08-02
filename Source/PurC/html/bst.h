/**
 * @file bst.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for bst.
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

#ifndef PCHTML_BST_H
#define PCHTML_BST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "config.h"
#include "html/core_base.h"
#include "html/dobject.h"


#define pchtml_bst_root(bst) (bst)->root
#define pchtml_bst_root_ref(bst) &((bst)->root)


typedef struct pchtml_bst_entry pchtml_bst_entry_t;
typedef struct pchtml_bst pchtml_bst_t;

typedef bool (*pchtml_bst_entry_f)(pchtml_bst_t *bst,
                                   pchtml_bst_entry_t *entry, void *ctx);

struct pchtml_bst_entry {
    void               *value;

    pchtml_bst_entry_t *right;
    pchtml_bst_entry_t *left;
    pchtml_bst_entry_t *next;
    pchtml_bst_entry_t *parent;

    size_t             size;
};

struct pchtml_bst {
    pchtml_dobject_t   *dobject;
    pchtml_bst_entry_t *root;

    size_t             tree_length;
};


pchtml_bst_t *
pchtml_bst_create(void) WTF_INTERNAL;

unsigned int
pchtml_bst_init(pchtml_bst_t *bst, size_t size) WTF_INTERNAL;

void
pchtml_bst_clean(pchtml_bst_t *bst) WTF_INTERNAL;

pchtml_bst_t *
pchtml_bst_destroy(pchtml_bst_t *bst, bool self_destroy) WTF_INTERNAL;

pchtml_bst_entry_t *
pchtml_bst_entry_make(pchtml_bst_t *bst, size_t size) WTF_INTERNAL;

pchtml_bst_entry_t *
pchtml_bst_insert(pchtml_bst_t *bst, pchtml_bst_entry_t **scope,
                  size_t size, void *value) WTF_INTERNAL;

pchtml_bst_entry_t *
pchtml_bst_insert_not_exists(pchtml_bst_t *bst, pchtml_bst_entry_t **scope,
                             size_t size) WTF_INTERNAL;


pchtml_bst_entry_t *
pchtml_bst_search(pchtml_bst_t *bst, pchtml_bst_entry_t *scope, size_t size) WTF_INTERNAL;

pchtml_bst_entry_t *
pchtml_bst_search_close(pchtml_bst_t *bst, pchtml_bst_entry_t *scope,
                        size_t size) WTF_INTERNAL;


void *
pchtml_bst_remove(pchtml_bst_t *bst, pchtml_bst_entry_t **root, size_t size) WTF_INTERNAL;

void *
pchtml_bst_remove_close(pchtml_bst_t *bst, pchtml_bst_entry_t **root,
                        size_t size, size_t *found_size) WTF_INTERNAL;

void *
pchtml_bst_remove_by_pointer(pchtml_bst_t *bst, pchtml_bst_entry_t *entry,
                             pchtml_bst_entry_t **root) WTF_INTERNAL;


void
pchtml_bst_serialize(pchtml_bst_t *bst, pchtml_callback_f callback, void *ctx) WTF_INTERNAL;

void
pchtml_bst_serialize_entry(pchtml_bst_entry_t *entry,
                           pchtml_callback_f callback, void *ctx, size_t tabs) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_BST_H */



