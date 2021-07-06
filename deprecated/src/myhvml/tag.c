/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML parser
** and interpreter.
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

#include "tag.h"

myhvml_tag_t * myhvml_tag_create(void)
{
    return (myhvml_tag_t*)mycore_malloc(sizeof(myhvml_tag_t));
}

mystatus_t myhvml_tag_init(myhvml_tree_t *tree, myhvml_tag_t *tags)
{
    mystatus_t status;
    
    tags->mcsimple_context = mcsimple_create();
    
    if(tags->mcsimple_context == NULL)
        return MyHVML_STATUS_TAGS_ERROR_MEMORY_ALLOCATION;
    
    mcsimple_init(tags->mcsimple_context, 128, 1024, sizeof(myhvml_tag_context_t));
    
    tags->mchar_node = mchar_async_node_add(tree->mchar, &status);
    tags->tree       = mctree_create(2);
    tags->mchar      = tree->mchar;
    tags->tags_count = MyHVML_TAG_LAST_ENTRY;
    
    if(status)
        return status;
    
    if(tags->tree == NULL)
        return MyCORE_STATUS_ERROR_MEMORY_ALLOCATION;
    
    myhvml_tag_clean(tags);
    
    return MyHVML_STATUS_OK;
}

void myhvml_tag_clean(myhvml_tag_t* tags)
{
    tags->tags_count = MyHVML_TAG_LAST_ENTRY;
    
    mcsimple_clean(tags->mcsimple_context);
    mchar_async_node_clean(tags->mchar, tags->mchar_node);
    mctree_clean(tags->tree);
}

myhvml_tag_t * myhvml_tag_destroy(myhvml_tag_t* tags)
{
    if(tags == NULL)
        return NULL;
    
    tags->tree = mctree_destroy(tags->tree);
    tags->mcsimple_context = mcsimple_destroy(tags->mcsimple_context, true);
    
    mchar_async_node_delete(tags->mchar, tags->mchar_node);
    
    mycore_free(tags);
    
    return NULL;
}

myhvml_tag_id_t myhvml_tag_add(myhvml_tag_t* tags, const char* key, size_t key_size,
                              enum myhvml_tokenizer_state data_parser, bool to_lcase)
{
    char* cache = mchar_async_malloc(tags->mchar, tags->mchar_node, (key_size + 1));
    
    if(to_lcase) {
        size_t i;
        for(i = 0; i < key_size; i++) {
            cache[i] = key[i] > 0x40 && key[i] < 0x5b ? (key[i]|0x60) : key[i];
        }
        cache[i] = '\0';
    }
    else {
        strncpy(cache, key, key_size);
        cache[key_size] = '\0';
    }
    
    // add tags
    
    myhvml_tag_context_t *tag_ctx = mcsimple_malloc(tags->mcsimple_context);
    
    mctree_insert(tags->tree, cache, key_size, (void *)tag_ctx, NULL);
    
    tag_ctx->id          = tags->tags_count;
    tag_ctx->name        = cache;
    tag_ctx->name_length = key_size;
    tag_ctx->data_parser = data_parser;
    // VW: memset(tag_ctx->cats, 0, sizeof(enum myhtml_tag_categories) * MyHTML_NAMESPACE_LAST_ENTRY);
    tag_ctx->cats = 0;
    
    tags->tags_count++;
    
    return tag_ctx->id;
}

void myhvml_tag_set_category(myhvml_tag_t* tags, myhvml_tag_id_t tag_idx,
                                       enum myhvml_namespace ns, enum myhvml_tag_categories cats)
{
    if(tag_idx < MyHVML_TAG_LAST_ENTRY)
        return;
    
    myhvml_tag_context_t *tag_ctx = mcsimple_get_by_absolute_position(tags->mcsimple_context, (tag_idx - MyHVML_TAG_LAST_ENTRY));

    // VW: tag_ctx->cats[ns] = cats;
    tag_ctx->cats = cats;
}

const myhvml_tag_context_t * myhvml_tag_get_by_id(myhvml_tag_t* tags, myhvml_tag_id_t tag_id)
{
    if(tag_id >= MyHVML_TAG_LAST_ENTRY) {
        return mcsimple_get_by_absolute_position(tags->mcsimple_context, (tag_id - MyHVML_TAG_LAST_ENTRY));
    }
    
    return myhvml_tag_static_get_by_id(tag_id);
}

const myhvml_tag_context_t * myhvml_tag_get_by_name(myhvml_tag_t* tags, const char* name, size_t length)
{
    const myhvml_tag_context_t *ctx = myhvml_tag_static_search(name, length);
    
    if(ctx)
        return ctx;
    
    mctree_index_t idx = mctree_search_lowercase(tags->tree, name, length);
    
    return (myhvml_tag_context_t*)tags->tree->nodes[idx].value;
}
