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

#include "myhvml_internals.h"

void myhvml_init_marker(myhvml_t* myhvml)
{
    myhvml->marker = (myhvml_tree_node_t*)mycore_malloc(sizeof(myhvml_tree_node_t));
    
    if(myhvml->marker)
        myhvml_tree_node_clean(myhvml->marker);
}

void myhvml_destroy_marker(myhvml_t* myhvml)
{
    if(myhvml->marker)
        mycore_free(myhvml->marker);
}

#ifndef PARSER_BUILD_WITHOUT_THREADS
mystatus_t myhvml_stream_create(myhvml_t* myhvml, mystatus_t* status, size_t count, size_t id_increase)
{
    if(count == 0) {
        myhvml->thread_stream = NULL;
        
        *status = MyHVML_STATUS_OK;
        return *status;
    }
    
    myhvml->thread_stream = mythread_create();
    if(myhvml->thread_stream == NULL)
        *status = MyCORE_STATUS_THREAD_ERROR_MEMORY_ALLOCATION;
    
    *status = mythread_init(myhvml->thread_stream, MyTHREAD_TYPE_STREAM, count, id_increase);
    
    if(*status)
        myhvml->thread_stream = mythread_destroy(myhvml->thread_stream, NULL, NULL, true);
    
    return *status;
}

mystatus_t myhvml_batch_create(myhvml_t* myhvml, mystatus_t* status, size_t count, size_t id_increase)
{
    if(count == 0) {
        myhvml->thread_batch = NULL;
        
        *status = MyHVML_STATUS_OK;
        return *status;
    }
    
    myhvml->thread_batch = mythread_create();
    if(myhvml->thread_stream == NULL) {
        myhvml->thread_stream = mythread_destroy(myhvml->thread_stream, NULL, NULL, true);
        *status = MyCORE_STATUS_THREAD_ERROR_MEMORY_ALLOCATION;
    }
    
    *status = mythread_init(myhvml->thread_batch, MyTHREAD_TYPE_BATCH, count, id_increase);
    
    if(*status)
        myhvml->thread_batch = mythread_destroy(myhvml->thread_batch , NULL, NULL, true);
    
    return *status;
}

mystatus_t myhvml_create_stream_and_batch(myhvml_t* myhvml, size_t stream_count, size_t batch_count)
{
    mystatus_t status;
    
    /* stream */
    if(myhvml_stream_create(myhvml, &status, stream_count, 0)) {
        return status;
    }
    
    /* batch */
    if(myhvml_batch_create(myhvml, &status, batch_count, stream_count)) {
        myhvml->thread_stream = mythread_destroy(myhvml->thread_stream, NULL, NULL, true);
        return status;
    }
    
    return status;
}
#endif /* if undef PARSER_BUILD_WITHOUT_THREADS */

myhvml_t * myhvml_create(void)
{
    return (myhvml_t*)mycore_calloc(1, sizeof(myhvml_t));
}

mystatus_t myhvml_init(myhvml_t* myhvml, enum myhvml_options opt, size_t thread_count, size_t queue_size)
{
    mystatus_t status;
    
    myhvml->opt = opt;
    myhvml_init_marker(myhvml);
    
    status = myhvml_tokenizer_state_init(myhvml);
    if(status)
        return status;
    
    status = myhvml_rules_init(myhvml);

#ifdef PARSER_BUILD_WITHOUT_THREADS
    
    myhvml->thread_stream = NULL;
    myhvml->thread_batch  = NULL;
    myhvml->thread_total  = 0;
    
#else /* if undef PARSER_BUILD_WITHOUT_THREADS */
    if(status)
        return status;

    if(thread_count == 0) {
        thread_count = 1;
    }

    switch (opt) {
        case MyHVML_OPTIONS_PARSE_MODE_SINGLE:
            if((status = myhvml_create_stream_and_batch(myhvml, 0, 0)))
                return status;
            
            break;
            
        case MyHVML_OPTIONS_PARSE_MODE_ALL_IN_ONE:
            if((status = myhvml_create_stream_and_batch(myhvml, 1, 0)))
                return status;
            
            myhvml->thread_stream->context = mythread_queue_list_create(&status);
            status = myhread_entry_create(myhvml->thread_stream, mythread_function_queue_stream, myhvml_parser_worker_stream, MyTHREAD_OPT_STOP);
            
            break;
            
        default:
            // default MyHVML_OPTIONS_PARSE_MODE_SEPARATELY
            if(thread_count < 2)
                thread_count = 2;
            
            if((status = myhvml_create_stream_and_batch(myhvml, 1, (thread_count - 1))))
                return status;
            
            myhvml->thread_stream->context = mythread_queue_list_create(&status);
            myhvml->thread_batch->context  = myhvml->thread_stream->context;
            
            status = myhread_entry_create(myhvml->thread_stream, mythread_function_queue_stream, myhvml_parser_stream, MyTHREAD_OPT_STOP);
            if(status)
                return status;
            
            for(size_t i = 0; i < myhvml->thread_batch->entries_size; i++) {
                status = myhread_entry_create(myhvml->thread_batch, mythread_function_queue_batch, myhvml_parser_worker, MyTHREAD_OPT_STOP);
                
                if(status)
                    return status;
            }
            
            break;
    }
    
    myhvml->thread_total = thread_count;
    
    myhvml->thread_list[0] = myhvml->thread_stream;
    myhvml->thread_list[1] = myhvml->thread_batch;
    myhvml->thread_list[2] = NULL;
    
#endif /* if undef PARSER_BUILD_WITHOUT_THREADS */
    
    if(status)
        return status;
    
    myhvml_clean(myhvml);
    
    return status;
}

void myhvml_clean(myhvml_t* myhvml)
{
    /* some code */
}

myhvml_t* myhvml_destroy(myhvml_t* myhvml)
{
    if(myhvml == NULL)
        return NULL;
    
    myhvml_destroy_marker(myhvml);
    
#ifndef PARSER_BUILD_WITHOUT_THREADS
    if(myhvml->thread_stream) {
        mythread_queue_list_t* queue_list = myhvml->thread_stream->context;

        if(queue_list)
            mythread_queue_list_wait_for_done(myhvml->thread_stream, queue_list);
        
        myhvml->thread_stream = mythread_destroy(myhvml->thread_stream, mythread_callback_quit, NULL, true);
        
        if(myhvml->thread_batch)
            myhvml->thread_batch = mythread_destroy(myhvml->thread_batch, mythread_callback_quit, NULL, true);
        
        if(queue_list)
            mythread_queue_list_destroy(queue_list);
    }
#endif /* if undef PARSER_BUILD_WITHOUT_THREADS */
    
    myhvml_tokenizer_state_destroy(myhvml);
    
    if(myhvml->insertion_func)
        mycore_free(myhvml->insertion_func);
    
    mycore_free(myhvml);
    
    return NULL;
}

mystatus_t myhvml_parse(myhvml_tree_t* tree, const char* hvml, size_t hvml_size)
{
    if(tree->flags & MyHVML_TREE_FLAGS_PARSE_END) {
        myhvml_tree_clean(tree);
    }
    
    mystatus_t status = myhvml_tokenizer_begin(tree, hvml, hvml_size);
    
    if(status)
        return status;
    
    return myhvml_tokenizer_end(tree);
}

mystatus_t myhvml_parse_fragment(myhvml_tree_t* tree, const char* hvml, size_t hvml_size, myhvml_tag_id_t tag_id, enum myhvml_namespace ns)
{
    if(tree->flags & MyHVML_TREE_FLAGS_PARSE_END) {
        myhvml_tree_clean(tree);
    }
    
#if 0 /* TODO: VW */
    if(tag_id == 0)
        tag_id = MyHVML_TAG_DIV;
#else
    if(tag_id == 0)
        tag_id = MyHVML_TAG__UNDEF;
#endif
    
    if(ns == 0)
        ns = MyHVML_NAMESPACE_HVML;
    
    if(myhvml_tokenizer_fragment_init(tree, tag_id, ns) == NULL)
        return MyHVML_STATUS_TOKENIZER_ERROR_FRAGMENT_INIT;
    
    mystatus_t status = myhvml_tokenizer_begin(tree, hvml, hvml_size);
    
    if(status)
        return status;
    
    return myhvml_tokenizer_end(tree);
}

mystatus_t myhvml_parse_single(myhvml_tree_t* tree, const char* hvml, size_t hvml_size)
{
    if(tree->flags & MyHVML_TREE_FLAGS_PARSE_END) {
        myhvml_tree_clean(tree);
    }
    
    tree->flags |= MyHVML_TREE_FLAGS_SINGLE_MODE;
    
    mystatus_t status = myhvml_tokenizer_begin(tree, hvml, hvml_size);
    
    if(status)
        return status;
    
    return myhvml_tokenizer_end(tree);
}

mystatus_t myhvml_parse_fragment_single(myhvml_tree_t* tree, const char* hvml, size_t hvml_size, myhvml_tag_id_t tag_id, enum myhvml_namespace ns)
{
    if(tree->flags & MyHVML_TREE_FLAGS_PARSE_END) {
        myhvml_tree_clean(tree);
    }
    
#if 0 /* TODO: VW */
    if(tag_id == 0)
        tag_id = MyHVML_TAG_DIV;
#else
    if(tag_id == 0)
        tag_id = MyHVML_TAG__UNDEF;
#endif
    
    if(ns == 0)
        ns = MyHVML_NAMESPACE_HVML;
    
    tree->flags |= MyHVML_TREE_FLAGS_SINGLE_MODE;
    
    if(myhvml_tokenizer_fragment_init(tree, tag_id, ns) == NULL)
        return MyHVML_STATUS_TOKENIZER_ERROR_FRAGMENT_INIT;
    
    mystatus_t status = myhvml_tokenizer_begin(tree, hvml, hvml_size);
    
    if(status)
        return status;
    
    return myhvml_tokenizer_end(tree);
}

mystatus_t myhvml_parse_chunk(myhvml_tree_t* tree, const char* hvml, size_t hvml_size)
{
    if(tree->flags & MyHVML_TREE_FLAGS_PARSE_END) {
        myhvml_tree_clean(tree);
    }
    
    return  myhvml_tokenizer_chunk(tree, hvml, hvml_size);
}

mystatus_t myhvml_parse_chunk_fragment(myhvml_tree_t* tree, const char* hvml, size_t hvml_size, myhvml_tag_id_t tag_id, enum myhvml_namespace ns)
{
    if(tree->flags & MyHVML_TREE_FLAGS_PARSE_END) {
        myhvml_tree_clean(tree);
    }
    
#if 0 /* TODO: VW */
    if(tag_id == 0)
        tag_id = MyHVML_TAG_DIV;
#else
    if(tag_id == 0)
        tag_id = MyHVML_TAG__UNDEF;
#endif
    
    if(ns == 0)
        ns = MyHVML_NAMESPACE_HVML;
    
    if(myhvml_tokenizer_fragment_init(tree, tag_id, ns) == NULL)
        return MyHVML_STATUS_TOKENIZER_ERROR_FRAGMENT_INIT;
    
    return myhvml_tokenizer_chunk(tree, hvml, hvml_size);
}

mystatus_t myhvml_parse_chunk_single(myhvml_tree_t* tree, const char* hvml, size_t hvml_size)
{
    if((tree->flags & MyHVML_TREE_FLAGS_SINGLE_MODE) == 0)
        tree->flags |= MyHVML_TREE_FLAGS_SINGLE_MODE;
    
    return myhvml_parse_chunk(tree, hvml, hvml_size);
}

mystatus_t myhvml_parse_chunk_fragment_single(myhvml_tree_t* tree, const char* hvml, size_t hvml_size, myhvml_tag_id_t tag_id, enum myhvml_namespace ns)
{
    if((tree->flags & MyHVML_TREE_FLAGS_SINGLE_MODE) == 0)
        tree->flags |= MyHVML_TREE_FLAGS_SINGLE_MODE;
    
    return myhvml_parse_chunk_fragment(tree, hvml, hvml_size, tag_id, ns);
}

mystatus_t myhvml_parse_chunk_end(myhvml_tree_t* tree)
{
    return myhvml_tokenizer_end(tree);
}

/*
 * Nodes
 */

mystatus_t myhvml_get_nodes_by_tag_id_in_scope_find_recursion(myhvml_tree_node_t *node, myhvml_collection_t *collection, myhvml_tag_id_t tag_id)
{
    while(node) {
        if(node->tag_id == tag_id) {
            collection->list[ collection->length ] = node;
            collection->length++;
            
            if(collection->length >= collection->size)
            {
                mystatus_t mystatus = myhvml_collection_check_size(collection, 1024, 0);
                
                if(mystatus != MyHVML_STATUS_OK)
                    return mystatus;
            }
        }
        
        if(node->child)
            myhvml_get_nodes_by_tag_id_in_scope_find_recursion(node->child, collection, tag_id);
        
        node = node->next;
    }
    
    return MyHVML_STATUS_OK;
}

myhvml_collection_t * myhvml_get_nodes_by_tag_id_in_scope(myhvml_tree_t* tree, myhvml_collection_t *collection, myhvml_tree_node_t *node, myhvml_tag_id_t tag_id, mystatus_t *status)
{
    if(node == NULL)
        return NULL;
    
    mystatus_t mystatus = MyHVML_STATUS_OK;
    
    if(collection == NULL) {
        collection = myhvml_collection_create(1024, &mystatus);
    }
    
    if(mystatus) {
        if(status)
            *status = mystatus;
        
        return collection;
    }
    
    if(node->child)
        mystatus = myhvml_get_nodes_by_tag_id_in_scope_find_recursion(node->child, collection, tag_id);
    
    collection->list[collection->length] = NULL;
    
    if(status)
        *status = mystatus;
    
    return collection;
}

myhvml_collection_t * myhvml_get_nodes_by_name_in_scope(myhvml_tree_t* tree, myhvml_collection_t *collection, myhvml_tree_node_t *node, const char* hvml, size_t length, mystatus_t *status)
{
    const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_name(tree->tags, hvml, length);
    if(tag_ctx == NULL) {
        return NULL;
    }
    return myhvml_get_nodes_by_tag_id_in_scope(tree, collection, node, tag_ctx->id, status);
}

myhvml_collection_t * myhvml_get_nodes_by_tag_id(myhvml_tree_t* tree, myhvml_collection_t *collection, myhvml_tag_id_t tag_id, mystatus_t *status)
{
    if(collection == NULL) {
        collection = myhvml_collection_create(1024, NULL);
        
        if(collection == NULL)
            return NULL;
    }
    
    myhvml_tree_node_t *node = tree->node_hvml;
    
    while(node)
    {
        if(node->tag_id == tag_id)
        {
            if(myhvml_collection_check_size(collection, 1, 1024) == MyHVML_STATUS_OK) {
                collection->list[ collection->length ] = node;
                collection->length++;
            }
            else {
                if(status)
                    *status = MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
                
                return collection;
            }
        }
        
        if(node->child)
            node = node->child;
        else {
            while(node != tree->node_hvml && node->next == NULL)
                node = node->parent;
            
            if(node == tree->node_hvml)
                break;
            
            node = node->next;
        }
    }
    
    if(myhvml_collection_check_size(collection, 1, 1024) == MyHVML_STATUS_OK) {
        collection->list[ collection->length ] = NULL;
    }
    else if(status) {
        *status = MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    return collection;
}

myhvml_collection_t * myhvml_get_nodes_by_name(myhvml_tree_t* tree, myhvml_collection_t *collection, const char* hvml, size_t length, mystatus_t *status)
{
    const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_name(tree->tags, hvml, length);
    
    if(tag_ctx == NULL)
        return NULL;
    
    return myhvml_get_nodes_by_tag_id(tree, collection, tag_ctx->id, status);
}

/*
 * Manipulate Nodes
 */
myhvml_tree_node_t * myhvml_node_first(myhvml_tree_t* tree)
{
    if(tree->fragment) {
        // document -> hvml -> need element
        if(tree->document && tree->document->child)
            return tree->document->child->child;
    }
    else if(tree->document) {
        // document -> hvml
        return tree->document->child;
    }
    
    return NULL;
}

myhvml_tree_node_t * myhvml_node_next(myhvml_tree_node_t *node)
{
    return node->next;
}

myhvml_tree_node_t * myhvml_node_prev(myhvml_tree_node_t *node)
{
    return node->prev;
}

myhvml_tree_node_t * myhvml_node_parent(myhvml_tree_node_t *node)
{
    return node->parent;
}

myhvml_tree_node_t * myhvml_node_child(myhvml_tree_node_t *node)
{
    return node->child;
}

myhvml_tree_node_t * myhvml_node_last_child(myhvml_tree_node_t *node)
{
    return node->last_child;
}

myhvml_tree_node_t * myhvml_node_create(myhvml_tree_t* tree, myhvml_tag_id_t tag_id, enum myhvml_namespace ns)
{
    myhvml_tree_node_t *node = myhvml_tree_node_create(tree);
    
    node->tag_id      = tag_id;
    node->ns = ns;
    
    return node;
}

myhvml_tree_node_t * myhvml_node_remove(myhvml_tree_node_t *node)
{
    return myhvml_tree_node_remove(node);
}

void myhvml_node_delete(myhvml_tree_node_t *node)
{
    myhvml_tree_node_delete(node);
}

void myhvml_node_delete_recursive(myhvml_tree_node_t *node)
{
    myhvml_tree_node_delete_recursive(node);
}

void myhvml_node_free(myhvml_tree_node_t *node)
{
    myhvml_tree_node_free(node);
}

myhvml_tree_node_t * myhvml_node_insert_before(myhvml_tree_node_t *target, myhvml_tree_node_t *node)
{
    if(target == NULL || node == NULL)
        return NULL;
    
    myhvml_tree_node_insert_before(target, node);
    
    return node;
}

myhvml_tree_node_t * myhvml_node_insert_after(myhvml_tree_node_t *target, myhvml_tree_node_t *node)
{
    if(target == NULL || node == NULL)
        return NULL;
    
    myhvml_tree_node_insert_after(target, node);
    
    return node;
}

myhvml_tree_node_t * myhvml_node_append_child(myhvml_tree_node_t *target, myhvml_tree_node_t *node)
{
    if(target == NULL || node == NULL)
        return NULL;
    
    myhvml_tree_node_add_child(target, node);
    
    return node;
}

myhvml_tree_node_t * myhvml_node_insert_to_appropriate_place(myhvml_tree_node_t *target, myhvml_tree_node_t *node)
{
    if(target == NULL || node == NULL)
        return NULL;
    
    enum myhvml_tree_insertion_mode mode;
    
    target->tree->foster_parenting = true;
    target = myhvml_tree_appropriate_place_inserting_in_tree(target, &mode);
    target->tree->foster_parenting = false;
    
    myhvml_tree_node_insert_by_mode(target, node, mode);
    
    return node;
}

mycore_string_t * myhvml_node_text_set(myhvml_tree_node_t *node, const char* text, size_t length)
{
    if(node == NULL)
        return NULL;
    
    myhvml_tree_t* tree = node->tree;
    
    if(node->token == NULL) {
        node->token = myhvml_token_node_create(tree->token, tree->mcasync_rules_token_id);
        
        if(node->token == NULL)
            return NULL;
     
        node->token->type |= MyHVML_TOKEN_TYPE_DONE;
    }
    
    if(node->token->str.data == NULL) {
        mycore_string_init(tree->mchar, tree->mchar_node_id, &node->token->str, (length + 2));
    }
    else {
        if(node->token->str.size < length) {
            mchar_async_free(tree->mchar, node->token->str.node_idx, node->token->str.data);
            mycore_string_init(tree->mchar, tree->mchar_node_id, &node->token->str, length);
        }
        else
            node->token->str.length = 0;
    }
    
    mycore_string_append(&node->token->str, text, length);
    
    node->token->raw_begin  = 0;
    node->token->raw_length = 0;
    
    return &node->token->str;
}

mycore_string_t * myhvml_node_text_set_with_charef(myhvml_tree_node_t *node, const char* text, size_t length)
{
    if(node == NULL)
        return NULL;
    
    myhvml_tree_t* tree = node->tree;
    
    if(node->token == NULL) {
        node->token = myhvml_token_node_create(tree->token, tree->mcasync_rules_token_id);
        
        if(node->token == NULL)
            return NULL;
        
        node->token->type |= MyHVML_TOKEN_TYPE_DONE;
    }
    
    if(node->token->str.data == NULL) {
        mycore_string_init(tree->mchar, tree->mchar_node_id, &node->token->str, (length + 2));
    }
    else {
        if(node->token->str.size < length) {
            mchar_async_free(tree->mchar, node->token->str.node_idx, node->token->str.data);
            mycore_string_init(tree->mchar, tree->mchar_node_id, &node->token->str, length);
        }
        else
            node->token->str.length = 0;
    }
    
    myhvml_data_process_entry_t proc_entry;
    myhvml_data_process_entry_clean(&proc_entry);
    
    proc_entry.encoding = MyENCODING_UTF_8;
    myencoding_result_clean(&proc_entry.res);
    
    myhvml_data_process(&proc_entry, &node->token->str, text, length);
    myhvml_data_process_end(&proc_entry, &node->token->str);
    
    node->token->raw_begin  = 0;
    node->token->raw_length = 0;
    
    return &node->token->str;
}

myhvml_token_node_t* myhvml_node_token(myhvml_tree_node_t *node)
{
    return node->token;
}

myhvml_namespace_t myhvml_node_namespace(myhvml_tree_node_t *node)
{
    return node->ns;
}

void myhvml_node_namespace_set(myhvml_tree_node_t *node, myhvml_namespace_t ns)
{
    node->ns = ns;
}

myhvml_tag_id_t myhvml_node_tag_id(myhvml_tree_node_t *node)
{
    return node->tag_id;
}

const char * myhvml_tag_name_by_id(myhvml_tree_t* tree, myhvml_tag_id_t tag_id, size_t *length)
{
    if(length)
        *length = 0;
    
    if(tree == NULL || tree->tags == NULL)
        return NULL;
    
    const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, tag_id);
    
    if(tag_ctx == NULL)
        return NULL;
    
    if(length)
        *length = tag_ctx->name_length;
    
    return tag_ctx->name;
}

myhvml_tag_id_t myhvml_tag_id_by_name(myhvml_tree_t* tree, const char *tag_name, size_t length)
{
    if(tree == NULL || tree->tags == NULL)
        return MyHVML_TAG__UNDEF;
    
    const myhvml_tag_context_t *ctx = myhvml_tag_get_by_name(tree->tags, tag_name, length);
    
    if(ctx == NULL)
        return MyHVML_TAG__UNDEF;
    
    return ctx->id;
}

bool myhvml_node_is_close_self(myhvml_tree_node_t *node)
{
    if(node->token)
        return (node->token->type & MyHVML_TOKEN_TYPE_CLOSE_SELF);
    
    return false;
}

bool myhvml_node_is_void_element(myhvml_tree_node_t *node)
{
#if 1 /* TODO: VW */
    return false;
#else
    // http://w3c.github.io/hvml-reference/syntax.hvml#void-elements
    switch (node->tag_id)
    {
        case MyHVML_TAG_AREA:
        case MyHVML_TAG_BASE:
        case MyHVML_TAG_BR:
        case MyHVML_TAG_COL:
        case MyHVML_TAG_COMMAND:
        case MyHVML_TAG_EMBED:
        case MyHVML_TAG_HR:
        case MyHVML_TAG_IMG:
        case MyHVML_TAG_INPUT:
        case MyHVML_TAG_KEYGEN:
        case MyHVML_TAG_LINK:
        case MyHVML_TAG_META:
        case MyHVML_TAG_PARAM:
        case MyHVML_TAG_SOURCE:
        case MyHVML_TAG_TRACK:
        case MyHVML_TAG_WBR:
        {
            return true;
        }
        default:
        {
            return false;
        }
    }
#endif
}

myhvml_tree_attr_t * myhvml_node_attribute_first(myhvml_tree_node_t *node)
{
    if(node->token)
        return node->token->attr_first;
    
    return NULL;
}

myhvml_tree_attr_t * myhvml_node_attribute_last(myhvml_tree_node_t *node)
{
    if(node->token)
        return node->token->attr_last;
    
    return NULL;
}

const char * myhvml_node_text(myhvml_tree_node_t *node, size_t *length)
{
    if(node->token && node->token->str.length && node->token->str.data)
    {
        if(length)
            *length = node->token->str.length;
        
        return node->token->str.data;
    }
    
    if(length)
        *length = 0;
    
    return NULL;
}

mycore_string_t * myhvml_node_string(myhvml_tree_node_t *node)
{
    if(node && node->token)
        return &node->token->str;
    
    return NULL;
}

myhvml_position_t myhvml_node_raw_position(myhvml_tree_node_t *node)
{
    if(node && node->token)
        return (myhvml_position_t){node->token->raw_begin, node->token->raw_length};
    
    return (myhvml_position_t){0, 0};
}

myhvml_position_t myhvml_node_element_position(myhvml_tree_node_t *node)
{
    if(node && node->token)
        return (myhvml_position_t){node->token->element_begin, node->token->element_length};
    
    return (myhvml_position_t){0, 0};
}

void myhvml_node_set_data(myhvml_tree_node_t *node, void* data)
{
    node->data = data;
}

void * myhvml_node_get_data(myhvml_tree_node_t *node)
{
    return node->data;
}

myhvml_tree_t * myhvml_node_tree(myhvml_tree_node_t *node)
{
    return node->tree;
}

mystatus_t myhvml_get_nodes_by_attribute_key_recursion(myhvml_tree_node_t* node, myhvml_collection_t* collection, const char* key, size_t key_len)
{
    myhvml_tree_node_t *root = node;

    while(node != NULL) {
        if(node->token && node->token->attr_first) {
            myhvml_tree_attr_t* attr = node->token->attr_first;

            while(attr) {
                mycore_string_t* str_key = &attr->key;

                if(str_key->length == key_len && mycore_strncasecmp(str_key->data, key, key_len) == 0) {
                    collection->list[ collection->length ] = node;

                    collection->length++;
                    if(collection->length >= collection->size) {
                        mystatus_t status = myhvml_collection_check_size(collection, 1024, 0);

                        if(status)
                            return status;
                    }
                }

                attr = attr->next;
            }
        }

        if(node->child != NULL) {
            node = node->child;
        }
        else {
            while(node->next == NULL) {
                node = node->parent;

                if(node == root) {
                    return MyHVML_STATUS_OK;
                }
            }

            node = node->next;
        }
    }

    return MyHVML_STATUS_OK;
}

myhvml_collection_t * myhvml_get_nodes_by_attribute_key(myhvml_tree_t *tree, myhvml_collection_t* collection, myhvml_tree_node_t* scope_node, const char* key, size_t key_len, mystatus_t* status)
{
    if(collection == NULL) {
        collection = myhvml_collection_create(1024, status);
        
        if((status && *status) || collection == NULL)
            return NULL;
    }
    
    if(scope_node == NULL)
        scope_node = tree->node_hvml;
    
    mystatus_t rec_status = myhvml_get_nodes_by_attribute_key_recursion(scope_node, collection, key, key_len);
    
    if(rec_status && status)
        *status = rec_status;
    
    return collection;
}

/* find by attribute value; case-sensitivity */
bool myhvml_get_nodes_by_attribute_value_recursion_eq(mycore_string_t* str, const char* value, size_t value_len)
{
    return str->length == value_len && mycore_strncmp(str->data, value, value_len) == 0;
}

bool myhvml_get_nodes_by_attribute_value_recursion_whitespace_separated(mycore_string_t* str, const char* value, size_t value_len)
{
    if(str->length < value_len)
        return false;
    
    const char *data = str->data;
    
    if(mycore_strncmp(data, value, value_len) == 0) {
        if((str->length > value_len && mycore_utils_whithspace(data[value_len], ==, ||)) || str->length == value_len)
            return true;
    }
    
    for(size_t i = 1; (str->length - i) >= value_len; i++)
    {
        if(mycore_utils_whithspace(data[(i - 1)], ==, ||)) {
            if(mycore_strncmp(&data[i], value, value_len) == 0) {
                if((i > value_len && mycore_utils_whithspace(data[(i + value_len)], ==, ||)) || (str->length - i) == value_len)
                    return true;
            }
        }
    }
    
    return false;
}

bool myhvml_get_nodes_by_attribute_value_recursion_begin(mycore_string_t* str, const char* value, size_t value_len)
{
    if(str->length < value_len)
        return false;
    
    return mycore_strncmp(str->data, value, value_len) == 0;
}

bool myhvml_get_nodes_by_attribute_value_recursion_end(mycore_string_t* str, const char* value, size_t value_len)
{
    if(str->length < value_len)
        return false;
    
    return mycore_strncmp(&str->data[ (str->length - value_len) ], value, value_len) == 0;
}

bool myhvml_get_nodes_by_attribute_value_recursion_contain(mycore_string_t* str, const char* value, size_t value_len)
{
    if(str->length < value_len)
        return false;
    
    const char *data = str->data;
    
    for(size_t i = 0; (str->length - i) >= value_len; i++)
    {
        if(mycore_strncmp(&data[i], value, value_len) == 0) {
            return true;
        }
    }
    
    return false;
}

bool myhvml_get_nodes_by_attribute_value_recursion_hyphen_separated(mycore_string_t* str, const char* value, size_t value_len)
{
    const char *data = str->data;
    
    if(str->length < value_len)
        return false;
    else if(str->length == value_len && mycore_strncmp(data, value, value_len) == 0) {
        return true;
    }
    else if(mycore_strncmp(data, value, value_len) == 0 && data[value_len] == '-') {
        return true;
    }
    
    return false;
}

/* find by attribute value; case-insensitive */
bool myhvml_get_nodes_by_attribute_value_recursion_eq_i(mycore_string_t* str, const char* value, size_t value_len)
{
    return str->length == value_len && mycore_strncasecmp(str->data, value, value_len) == 0;
}

bool myhvml_get_nodes_by_attribute_value_recursion_whitespace_separated_i(mycore_string_t* str, const char* value, size_t value_len)
{
    if(str->length < value_len)
        return false;
    
    const char *data = str->data;
    
    if(mycore_strncasecmp(data, value, value_len) == 0) {
        if((str->length > value_len && mycore_utils_whithspace(data[value_len], ==, ||)) || str->length == value_len)
            return true;
    }
    
    for(size_t i = 1; (str->length - i) >= value_len; i++)
    {
        if(mycore_utils_whithspace(data[(i - 1)], ==, ||)) {
            if(mycore_strncasecmp(&data[i], value, value_len) == 0) {
                if((i > value_len && mycore_utils_whithspace(data[(i + value_len)], ==, ||)) || (str->length - i) == value_len)
                    return true;
            }
        }
    }
    
    return false;
}

bool myhvml_get_nodes_by_attribute_value_recursion_begin_i(mycore_string_t* str, const char* value, size_t value_len)
{
    if(str->length < value_len)
        return false;
    
    return mycore_strncasecmp(str->data, value, value_len) == 0;
}

bool myhvml_get_nodes_by_attribute_value_recursion_end_i(mycore_string_t* str, const char* value, size_t value_len)
{
    if(str->length < value_len)
        return false;
    
    return mycore_strncasecmp(&str->data[ (str->length - value_len) ], value, value_len) == 0;
}

bool myhvml_get_nodes_by_attribute_value_recursion_contain_i(mycore_string_t* str, const char* value, size_t value_len)
{
    if(str->length < value_len)
        return false;
    
    const char *data = str->data;
    
    for(size_t i = 0; (str->length - i) >= value_len; i++)
    {
        if(mycore_strncasecmp(&data[i], value, value_len) == 0) {
            return true;
        }
    }
    
    return false;
}

bool myhvml_get_nodes_by_attribute_value_recursion_hyphen_separated_i(mycore_string_t* str, const char* value, size_t value_len)
{
    const char *data = str->data;
    
    if(str->length < value_len)
        return false;
    else if(str->length == value_len && mycore_strncasecmp(data, value, value_len) == 0) {
        return true;
    }
    else if(mycore_strncasecmp(data, value, value_len) == 0 && data[value_len] == '-') {
        return true;
    }
    
    return false;
}

/* find by attribute value; basic functions */
mystatus_t myhvml_get_nodes_by_attribute_value_recursion(myhvml_tree_node_t* node, myhvml_collection_t* collection,
                                                         myhvml_attribute_value_find_f func_eq,
                                                         const char* value, size_t value_len)
{
    myhvml_tree_node_t *root = node;
    
    while(node != NULL) {
        if(node->token && node->token->attr_first) {
            myhvml_tree_attr_t* attr = node->token->attr_first;

            while(attr) {
                mycore_string_t* str = &attr->value;

                if(func_eq(str, value, value_len)) {
                    collection->list[ collection->length ] = node;

                    collection->length++;
                    if(collection->length >= collection->size) {
                        mystatus_t status = myhvml_collection_check_size(collection, 1024, 0);

                        if(status)
                            return status;
                    }
                }
                
                attr = attr->next;
            }
        }

        if(node->child != NULL) {
            node = node->child;
        }
        else {
            while(node->next == NULL) {
                node = node->parent;

                if(node == root) {
                    return MyHVML_STATUS_OK;
                }
            }

            node = node->next;
        }
    }

    return MyHVML_STATUS_OK;
}

/* TODO: need to rename function. Remove recursion word */
mystatus_t myhvml_get_nodes_by_attribute_value_recursion_by_key(myhvml_tree_node_t* node, myhvml_collection_t* collection,
                                                                myhvml_attribute_value_find_f func_eq,
                                                                const char* key, size_t key_len,
                                                                const char* value, size_t value_len)
{
    myhvml_tree_node_t *root = node;
    
    while(node != NULL) {
        if(node->token && node->token->attr_first) {
            myhvml_tree_attr_t* attr = node->token->attr_first;

            while(attr) {
                mycore_string_t* str_key = &attr->key;
                mycore_string_t* str = &attr->value;

                if(str_key->length == key_len && mycore_strncasecmp(str_key->data, key, key_len) == 0)
                {
                    if(func_eq(str, value, value_len)) {
                        collection->list[ collection->length ] = node;

                        collection->length++;
                        if(collection->length >= collection->size) {
                            mystatus_t status = myhvml_collection_check_size(collection, 1024, 0);

                            if(status)
                                return status;
                        }
                    }
                }

                attr = attr->next;
            }
        }

        if(node->child != NULL) {
            node = node->child;
        }
        else {
            while(node->next == NULL) {
                node = node->parent;
                
                if(node == root) {
                    return MyHVML_STATUS_OK;
                }
            }
            
            node = node->next;
        }
    }

    return MyHVML_STATUS_OK;
}

myhvml_collection_t * _myhvml_get_nodes_by_attribute_value(myhvml_tree_t *tree, myhvml_collection_t* collection, myhvml_tree_node_t* node,
                                                           myhvml_attribute_value_find_f func_eq,
                                                           const char* key, size_t key_len,
                                                           const char* value, size_t value_len,
                                                           mystatus_t* status)
{
    if(collection == NULL) {
        collection = myhvml_collection_create(1024, status);
        
        if((status && *status) || collection == NULL)
            return NULL;
    }
    
    if(node == NULL)
        node = tree->node_hvml;
    
    mystatus_t rec_status;
    
    if(key && key_len)
        rec_status = myhvml_get_nodes_by_attribute_value_recursion_by_key(node, collection, func_eq, key, key_len, value, value_len);
    else
        rec_status = myhvml_get_nodes_by_attribute_value_recursion(node, collection, func_eq, value, value_len);
    
    if(rec_status && status)
        *status = rec_status;
    
    return collection;
}

myhvml_collection_t * myhvml_get_nodes_by_attribute_value(myhvml_tree_t *tree, myhvml_collection_t* collection, myhvml_tree_node_t* node,
                                                          bool case_insensitive,
                                                          const char* key, size_t key_len,
                                                          const char* value, size_t value_len,
                                                          mystatus_t* status)
{
    if(case_insensitive) {
        return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                    myhvml_get_nodes_by_attribute_value_recursion_eq_i,
                                                    key, key_len, value, value_len, status);
    }
    
    return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                myhvml_get_nodes_by_attribute_value_recursion_eq,
                                                key, key_len, value, value_len, status);
}

myhvml_collection_t * myhvml_get_nodes_by_attribute_value_whitespace_separated(myhvml_tree_t *tree, myhvml_collection_t* collection, myhvml_tree_node_t* node,
                                                                               bool case_insensitive,
                                                                               const char* key, size_t key_len,
                                                                               const char* value, size_t value_len,
                                                                               mystatus_t* status)
{
    if(case_insensitive) {
        return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                    myhvml_get_nodes_by_attribute_value_recursion_whitespace_separated_i,
                                                    key, key_len, value, value_len, status);
    }
    
    return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                myhvml_get_nodes_by_attribute_value_recursion_whitespace_separated,
                                                key, key_len, value, value_len, status);
}

myhvml_collection_t * myhvml_get_nodes_by_attribute_value_begin(myhvml_tree_t *tree, myhvml_collection_t* collection, myhvml_tree_node_t* node,
                                                                bool case_insensitive,
                                                                const char* key, size_t key_len,
                                                                const char* value, size_t value_len,
                                                                mystatus_t* status)
{
    if(case_insensitive) {
        return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                    myhvml_get_nodes_by_attribute_value_recursion_begin_i,
                                                    key, key_len, value, value_len, status);
    }
    
    return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                myhvml_get_nodes_by_attribute_value_recursion_begin,
                                                key, key_len, value, value_len, status);
}

myhvml_collection_t * myhvml_get_nodes_by_attribute_value_end(myhvml_tree_t *tree, myhvml_collection_t* collection, myhvml_tree_node_t* node,
                                                              bool case_insensitive,
                                                              const char* key, size_t key_len,
                                                              const char* value, size_t value_len,
                                                              mystatus_t* status)
{
    if(case_insensitive) {
        return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                    myhvml_get_nodes_by_attribute_value_recursion_end_i,
                                                    key, key_len, value, value_len, status);
    }
    
    return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                myhvml_get_nodes_by_attribute_value_recursion_end,
                                                key, key_len, value, value_len, status);
}

myhvml_collection_t * myhvml_get_nodes_by_attribute_value_contain(myhvml_tree_t *tree, myhvml_collection_t* collection, myhvml_tree_node_t* node,
                                                                  bool case_insensitive,
                                                                  const char* key, size_t key_len,
                                                                  const char* value, size_t value_len,
                                                                  mystatus_t* status)
{
    if(case_insensitive) {
        return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                    myhvml_get_nodes_by_attribute_value_recursion_contain_i,
                                                    key, key_len, value, value_len, status);
    }
    
    return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                myhvml_get_nodes_by_attribute_value_recursion_contain,
                                                key, key_len, value, value_len, status);
}

myhvml_collection_t * myhvml_get_nodes_by_attribute_value_hyphen_separated(myhvml_tree_t *tree, myhvml_collection_t* collection, myhvml_tree_node_t* node,
                                                                           bool case_insensitive,
                                                                           const char* key, size_t key_len,
                                                                           const char* value, size_t value_len,
                                                                           mystatus_t* status)
{
    if(case_insensitive) {
        return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                    myhvml_get_nodes_by_attribute_value_recursion_hyphen_separated_i,
                                                    key, key_len, value, value_len, status);
    }
    
    return _myhvml_get_nodes_by_attribute_value(tree, collection, node,
                                                myhvml_get_nodes_by_attribute_value_recursion_hyphen_separated,
                                                key, key_len, value, value_len, status);
}

/*
 * Attributes
 */
myhvml_tree_attr_t * myhvml_attribute_next(myhvml_tree_attr_t *attr)
{
    return attr->next;
}

myhvml_tree_attr_t * myhvml_attribute_prev(myhvml_tree_attr_t *attr)
{
    return attr->prev;
}

enum myhvml_namespace myhvml_attribute_namespace(myhvml_tree_attr_t *attr)
{
    return attr->ns;
}

void myhvml_attribute_namespace_set(myhvml_tree_attr_t *attr, myhvml_namespace_t ns)
{
    attr->ns = ns;
}

const char * myhvml_attribute_key(myhvml_tree_attr_t *attr, size_t *length)
{
    if(attr->key.data && attr->key.length)
    {
        if(length)
            *length = attr->key.length;
        
        return attr->key.data;
    }
    
    if(length)
        *length = 0;
    
    return NULL;
}

const char * myhvml_attribute_value(myhvml_tree_attr_t *attr, size_t *length)
{
    if(attr->value.data && attr->value.length)
    {
        if(length)
            *length = attr->value.length;
        
        return attr->value.data;
    }
    
    if(length)
        *length = 0;
    
    return NULL;
}

mycore_string_t * myhvml_attribute_key_string(myhvml_tree_attr_t* attr)
{
    if(attr)
        return &attr->key;
    
    return NULL;
}

mycore_string_t * myhvml_attribute_value_string(myhvml_tree_attr_t* attr)
{
    if(attr)
        return &attr->value;
    
    return NULL;
}

myhvml_tree_attr_t * myhvml_attribute_by_key(myhvml_tree_node_t *node, const char *key, size_t key_len)
{
    if(node == NULL || node->token == NULL)
        return NULL;
    
    return myhvml_token_attr_by_name(node->token, key, key_len);
}

myhvml_tree_attr_t * myhvml_attribute_add(myhvml_tree_node_t *node, const char *key, size_t key_len, const char *value, size_t value_len)
{
    if(node == NULL)
        return NULL;
    
    myhvml_tree_t *tree = node->tree;
    
    if(node->token == NULL) {
        node->token = myhvml_token_node_create(tree->token, tree->mcasync_rules_token_id);
        
        if(node->token == NULL)
            return NULL;
        
        node->token->type |= MyHVML_TOKEN_TYPE_DONE;
    }
    
    return myhvml_token_node_attr_append_with_convert_encoding(tree->token, node->token, key, key_len,
                                                               value, value_len, tree->mcasync_rules_token_id, MyENCODING_UTF_8);
}

myhvml_tree_attr_t * myhvml_attribute_remove(myhvml_tree_node_t *node, myhvml_tree_attr_t *attr)
{
    if(node == NULL || node->token == NULL)
        return NULL;
    
    return myhvml_token_attr_remove(node->token, attr);
}

myhvml_tree_attr_t * myhvml_attribute_remove_by_key(myhvml_tree_node_t *node, const char *key, size_t key_len)
{
    if(node == NULL || node->token == NULL)
        return NULL;
    
    return myhvml_token_attr_remove_by_name(node->token, key, key_len);
}

void myhvml_attribute_delete(myhvml_tree_t *tree, myhvml_tree_node_t *node, myhvml_tree_attr_t *attr)
{
    if(node == NULL || node->token == NULL)
        return;
    
    myhvml_token_attr_remove(node->token, attr);
    myhvml_attribute_free(tree, attr);
}

void myhvml_attribute_free(myhvml_tree_t *tree, myhvml_tree_attr_t *attr)
{
    if(attr->key.data)
        mchar_async_free(attr->key.mchar, attr->key.node_idx, attr->key.data);
    if(attr->value.data)
        mchar_async_free(attr->value.mchar, attr->value.node_idx, attr->value.data);
    
    mcobject_async_free(tree->token->attr_obj, attr);
}

myhvml_position_t myhvml_attribute_key_raw_position(myhvml_tree_attr_t *attr)
{
    if(attr)
        return (myhvml_position_t){attr->raw_key_begin, attr->raw_key_length};
    
    return (myhvml_position_t){0, 0};
}

myhvml_position_t myhvml_attribute_value_raw_position(myhvml_tree_attr_t *attr)
{
    if(attr)
        return (myhvml_position_t){attr->raw_value_begin, attr->raw_value_length};
    
    return (myhvml_position_t){0, 0};
}

/*
 * Collections
 */
myhvml_collection_t * myhvml_collection_create(size_t size, mystatus_t *status)
{
    myhvml_collection_t *collection = (myhvml_collection_t*)mycore_malloc(sizeof(myhvml_collection_t));
    
    if(collection == NULL) {
        if(status)
            *status = MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
        
        return NULL;
    }
    
    collection->size   = size;
    collection->length = 0;
    collection->list   = (myhvml_tree_node_t **)mycore_malloc(sizeof(myhvml_tree_node_t*) * size);
    
    if(collection->list == NULL) {
        mycore_free(collection);
        
        if(status)
            *status = MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
        
        return NULL;
    }
    
    if(status)
        *status = MyHVML_STATUS_OK;
    
    return collection;
}

mystatus_t myhvml_collection_check_size(myhvml_collection_t *collection, size_t need, size_t upto_length)
{
    if((collection->length + need) >= collection->size)
    {
        size_t tmp_size = collection->length + need + upto_length + 1;
        myhvml_tree_node_t **tmp = (myhvml_tree_node_t **)mycore_realloc(collection->list, sizeof(myhvml_tree_node_t*) * tmp_size);
        
        if(tmp) {
            collection->size = tmp_size;
            collection->list = tmp;
        }
        else
            return MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    return MyHVML_STATUS_OK;
}

void myhvml_collection_clean(myhvml_collection_t *collection)
{
    if(collection)
        collection->length = 0;
}

myhvml_collection_t * myhvml_collection_destroy(myhvml_collection_t *collection)
{
    if(collection == NULL)
        return NULL;
    
    if(collection->list)
        mycore_free(collection->list);
    
    mycore_free(collection);
    
    return NULL;
}

/* queue */
mystatus_t myhvml_queue_add(myhvml_tree_t *tree, size_t begin, myhvml_token_node_t* token)
{
    // TODO: need refactoring this code
    // too many conditions
    mythread_queue_node_t *qnode = tree->current_qnode;
    
    if(tree->parse_flags & MyHVML_TREE_PARSE_FLAGS_SKIP_WHITESPACE_TOKEN) {
        if(token && token->tag_id == MyHVML_TAG__TEXT && token->type & MyHVML_TOKEN_TYPE_WHITESPACE)
        {
            myhvml_token_node_clean(token);
            token->raw_begin = token->element_begin = (tree->global_offset + begin);
            
            return MyHVML_STATUS_OK;
        }
    }
    
#ifndef PARSER_BUILD_WITHOUT_THREADS
    
    if(tree->flags & MyHVML_TREE_FLAGS_SINGLE_MODE) {
        if(qnode && token) {
            qnode->args = token;
            
            myhvml_parser_worker(0, qnode);
            myhvml_parser_stream(0, qnode);
        }
        
        tree->current_qnode = mythread_queue_node_malloc_limit(tree->myhvml->thread_stream, tree->queue, 4, NULL);
    }
    else {
        if(qnode)
            qnode->args = token;
        
        tree->current_qnode = mythread_queue_node_malloc_round(tree->myhvml->thread_stream, tree->queue_entry);
        
        /* we have a clean queue list */
        if(tree->queue_entry->queue->nodes_length == 0) {
            mythread_queue_list_entry_make_batch(tree->myhvml->thread_batch, tree->queue_entry);
            mythread_queue_list_entry_make_stream(tree->myhvml->thread_stream, tree->queue_entry);
        }
    }
    
#else
    
    if(qnode && token) {
        qnode->args = token;
        
        myhvml_parser_worker(0, qnode);
        myhvml_parser_stream(0, qnode);
    }
    
    tree->current_qnode = mythread_queue_node_malloc_limit(tree->myhvml->thread_stream, tree->queue, 4, NULL);
    
#endif /* PARSER_BUILD_WITHOUT_THREADS */
    
    if(tree->current_qnode == NULL)
        return MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
    
    tree->current_qnode->context = tree;
    tree->current_qnode->prev = qnode;
    
    if(qnode && token)
        myhvml_tokenizer_calc_current_namespace(tree, token);
    
    tree->current_token_node = myhvml_token_node_create(tree->token, tree->token->mcasync_token_id);
    if(tree->current_token_node == NULL)
        return MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
    
    tree->current_token_node->raw_begin = tree->current_token_node->element_begin = (tree->global_offset + begin);
    
    return MyHVML_STATUS_OK;
}

bool myhvml_utils_strcmp(const char* ab, const char* to_lowercase, size_t size)
{
    size_t i = 0;
    
    for(;;) {
        if(i == size)
            return true;
        
        if((const unsigned char)(to_lowercase[i] > 0x40 && to_lowercase[i] < 0x5b ?
                                 (to_lowercase[i]|0x60) : to_lowercase[i]) != (const unsigned char)ab[i])
        {
            return false;
        }
        
        i++;
    }
    
    return false;
}

bool myhvml_is_hvml_node(myhvml_tree_node_t *node, myhvml_tag_id_t tag_id)
{
    if(node == NULL)
        return false;
    
    return node->tag_id == tag_id && node->ns == MyHVML_NAMESPACE_HVML;
}

myhvml_tree_node_t * myhvml_node_clone(myhvml_tree_t* dest_tree, myhvml_tree_node_t* src)
{
    myhvml_tag_id_t tag_id;
    const myhvml_tag_context_t* tag_to, * tag_from;
    myhvml_tree_node_t* new_node = myhvml_tree_node_create(dest_tree);

    tag_id = src->tag_id;

    if (tag_id >= MyHVML_TAG_LAST_ENTRY) {
        tag_to = myhvml_tag_get_by_id(dest_tree->tags, src->tag_id);
        tag_from = myhvml_tag_get_by_id(src->tree->tags, src->tag_id);

        if (tag_to == NULL
            || tag_to->name_length != tag_from->name_length
            || mycore_strncmp(tag_to->name, tag_from->name, tag_from->name_length) != 0)
        {
            tag_id = myhvml_tag_add(dest_tree->tags, tag_from->name, tag_from->name_length,
                                    MyHVML_TOKENIZER_STATE_DATA, true);
        }
    }

    new_node->token        = myhvml_token_node_clone(dest_tree->token, src->token,
                                                     dest_tree->mcasync_rules_token_id,
                                                     dest_tree->mcasync_rules_attr_id);
    new_node->tag_id        = tag_id;
    new_node->ns            = src->ns;

    if(new_node->token) {
        new_node->token->tag_id = tag_id;
        new_node->token->type |= MyHVML_TOKEN_TYPE_DONE;
    }

    return new_node;
}

myhvml_tree_node_t * myhvml_node_clone_deep(myhvml_tree_t* dest_tree, myhvml_tree_node_t* src)
{
    myhvml_tree_node_t* cloned, *root, *node;
    myhvml_tree_node_t* scope_node = src;

    if(scope_node && scope_node->tree && scope_node->tree->document == scope_node) {
        src = scope_node->child;
    }

    root = node = myhvml_node_clone(dest_tree, src);
    if (root == NULL) {
        return NULL;
    }

    src = src->child;

    while(src != NULL) {
        cloned = myhvml_node_clone(dest_tree, src);
        if (cloned == NULL) {
            return NULL;
        }

        myhvml_tree_node_add_child(node, cloned);

        if(src->child) {
            src = src->child;
            node = cloned;
        }
        else {
            while(src != scope_node && src->next == NULL) {
                node = node->parent;
                src = src->parent;
            }

            if(src == scope_node) {
                break;
            }

            src = src->next;
        }
    }

    return root;
}
