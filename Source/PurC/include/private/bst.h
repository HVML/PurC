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
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PURC_PRIVATE_BST_H
#define PURC_PRIVATE_BST_H

#include <string.h>

#include "config.h"

#include "private/dobject.h"

#ifdef __cplusplus
extern "C" {
#endif

#define pcutils_bst_root(bst) (bst)->root
#define pcutils_bst_root_ref(bst) &((bst)->root)

typedef struct pcutils_bst_entry pcutils_bst_entry_t;
typedef struct pcutils_bst pcutils_bst_t;

typedef bool (*pcutils_bst_entry_f)(pcutils_bst_t *bst,
                                   pcutils_bst_entry_t *entry, void *ctx);

struct pcutils_bst_entry {
    void               *value;

    pcutils_bst_entry_t *right;
    pcutils_bst_entry_t *left;
    pcutils_bst_entry_t *next;
    pcutils_bst_entry_t *parent;

    size_t             size;
};

struct pcutils_bst {
    pcutils_dobject_t   *dobject;
    pcutils_bst_entry_t *root;

    size_t             tree_length;
};


pcutils_bst_t *
pcutils_bst_create(void) WTF_INTERNAL;

unsigned int
pcutils_bst_init(pcutils_bst_t *bst, size_t size) WTF_INTERNAL;

void
pcutils_bst_clean(pcutils_bst_t *bst) WTF_INTERNAL;

pcutils_bst_t *
pcutils_bst_destroy(pcutils_bst_t *bst, bool self_destroy) WTF_INTERNAL;

pcutils_bst_entry_t *
pcutils_bst_entry_make(pcutils_bst_t *bst, size_t size) WTF_INTERNAL;

pcutils_bst_entry_t *
pcutils_bst_insert(pcutils_bst_t *bst, pcutils_bst_entry_t **scope,
                  size_t size, void *value) WTF_INTERNAL;

pcutils_bst_entry_t *
pcutils_bst_insert_not_exists(pcutils_bst_t *bst, pcutils_bst_entry_t **scope,
                             size_t size) WTF_INTERNAL;


pcutils_bst_entry_t *
pcutils_bst_search(pcutils_bst_t *bst, pcutils_bst_entry_t *scope, size_t size) WTF_INTERNAL;

pcutils_bst_entry_t *
pcutils_bst_search_close(pcutils_bst_t *bst, pcutils_bst_entry_t *scope,
                        size_t size) WTF_INTERNAL;


void *
pcutils_bst_remove(pcutils_bst_t *bst, pcutils_bst_entry_t **root, size_t size) WTF_INTERNAL;

void *
pcutils_bst_remove_close(pcutils_bst_t *bst, pcutils_bst_entry_t **root,
                        size_t size, size_t *found_size) WTF_INTERNAL;

void *
pcutils_bst_remove_by_pointer(pcutils_bst_t *bst, pcutils_bst_entry_t *entry,
                             pcutils_bst_entry_t **root) WTF_INTERNAL;


/* Callbacks */
typedef unsigned int (*pcutils_bst_callback_f)(const unsigned char *buffer,
                                          size_t size, void *ctx);

void
pcutils_bst_serialize(pcutils_bst_t *bst, pcutils_bst_callback_f callback, void *ctx) WTF_INTERNAL;

void
pcutils_bst_serialize_entry(pcutils_bst_entry_t *entry,
                           pcutils_bst_callback_f callback, void *ctx, size_t tabs) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PURC_PRIVATE_BST_H */



