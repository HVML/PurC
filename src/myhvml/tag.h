/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of Purring Cat 2, a HVML parser and interpreter.
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
** Author: Vincent Wei <https://github.com/VincentWei>
*/

#ifndef MyHVML_TAG_H
#define MyHVML_TAG_H

#pragma once

#include "myosi.h"

#include "tag_const.h"
#include "tokenizer.h"
#include "tree.h"
#include "mycore/utils.h"
#include "mycore/utils/mctree.h"
#include "mycore/utils/mchar_async.h"
#include "mycore/utils/mcobject.h"
#include "mycore/utils/mcobject_async.h"
#include "mycore/utils/mcsimple.h"

#define myhvml_tag_get(tags, idx, attr) tags->context[idx].attr

#define myhvml_tag_context_clean(tags, idx)                       \
    tags->context[idx].id          = 0;                           \
    tags->context[idx].name        = NULL;                        \
    tags->context[idx].name_length = 0;                           \
    tags->context[idx].data_parser = MyHVML_TOKENIZER_STATE_DATA; \
    memset(tags->context[idx].cats, MyHVML_TAG_CATEGORIES_UNDEF, sizeof(tags->context[idx].cats));


#define myhvml_tag_context_add(tags)                                         \
    tags->context_length++;                                                  \
    if(tags->context_length == tags->context_size) {                         \
        tags->context_size += 4096;                                          \
        tags->context = (myhvml_tag_context_t*)mycore_realloc(tags->context,      \
            sizeof(myhvml_tag_context_t) * tags->context_size);              \
    }                                                                        \
    myhvml_tag_context_clean(tags, tags->context_length)

#define myhvml_tag_index_clean_node(index_node)             \
    memset(index_node, 0, sizeof(myhvml_tag_index_node_t));

struct myhvml_tag_context {
    myhvml_tag_id_t id;
    
    const char* name;
    size_t name_length;
    
    enum myhvml_tokenizer_state data_parser;
    enum myhvml_tag_categories cats[MyHVML_NAMESPACE_LAST_ENTRY];
}
typedef myhvml_tag_context_t;

struct myhvml_tag_static_list {
    const myhvml_tag_context_t* ctx;
    size_t next;
    size_t cur;
}
typedef myhvml_tag_static_list_t;

struct myhvml_tag {
    mctree_t* tree;
    mcsimple_t* mcsimple_context;
    
    size_t tags_count;
    size_t mchar_node;
    
    mchar_async_t *mchar;
};

#ifdef __cplusplus
extern "C" {
#endif

myhvml_tag_t * myhvml_tag_create(void);
mystatus_t myhvml_tag_init(myhvml_tree_t *tree, myhvml_tag_t *tags);
void myhvml_tag_clean(myhvml_tag_t* tags);
myhvml_tag_t * myhvml_tag_destroy(myhvml_tag_t* tags);

myhvml_tag_id_t myhvml_tag_add(myhvml_tag_t* tags, const char* key, size_t key_size,
                              enum myhvml_tokenizer_state data_parser, bool to_lcase);

void myhvml_tag_set_category(myhvml_tag_t* tags, myhvml_tag_id_t tag_idx,
                         enum myhvml_namespace ns, enum myhvml_tag_categories cats);

const myhvml_tag_context_t * myhvml_tag_get_by_id(myhvml_tag_t* tags, myhvml_tag_id_t tag_id);
const myhvml_tag_context_t * myhvml_tag_get_by_name(myhvml_tag_t* tags, const char* name, size_t length);

const myhvml_tag_context_t * myhvml_tag_static_get_by_id(size_t idx);
const myhvml_tag_context_t * myhvml_tag_static_search(const char* name, size_t length);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* MyHVML_TAG_H */
