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

#include "tokenizer.h"
#include "mycore/utils/resources.h"

mystatus_t myhvml_tokenizer_set_first_settings(myhvml_tree_t* tree, const char* hvml, size_t hvml_length)
{
    tree->current_qnode = mythread_queue_get_current_node(tree->queue);
    mythread_queue_node_clean(tree->current_qnode);
    
    tree->current_qnode->context = tree;
    tree->current_token_node = myhvml_token_node_create(tree->token, tree->token->mcasync_token_id);
    
    if(tree->current_token_node == NULL)
        return MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
    
    tree->incoming_buf_first = tree->incoming_buf;
    
    return MyHVML_STATUS_OK;
}

mystatus_t myhvml_tokenizer_begin(myhvml_tree_t* tree, const char* hvml, size_t hvml_length)
{
    return myhvml_tokenizer_chunk(tree, hvml, hvml_length);
}

mystatus_t myhvml_tokenizer_chunk_process(myhvml_tree_t* tree, const char* hvml, size_t hvml_length)
{
    myhvml_t* myhvml = tree->myhvml;
    myhvml_tokenizer_state_f* state_f = myhvml->parse_state_func;
    
    // add for a chunk
    tree->incoming_buf = mycore_incoming_buffer_add(tree->incoming_buf, tree->mcobject_incoming_buf, hvml, hvml_length);
    
#ifndef PARSER_BUILD_WITHOUT_THREADS
    
    if(myhvml->opt & MyHVML_OPTIONS_PARSE_MODE_SINGLE)
        tree->flags |= MyHVML_TREE_FLAGS_SINGLE_MODE;
    
    if((tree->flags & MyHVML_TREE_FLAGS_SINGLE_MODE) == 0)
    {
        if(tree->queue_entry == NULL) {
            mystatus_t status = MyHVML_STATUS_OK;
            tree->queue_entry = mythread_queue_list_entry_push(myhvml->thread_list, 2,
                                                               myhvml->thread_stream->context, tree->queue,
                                                               myhvml->thread_total, &status);
            
            if(status)
                return status;
        }
        
        myhvml_tokenizer_post(tree);
    }
    
#else
    
    tree->flags |= MyHVML_TREE_FLAGS_SINGLE_MODE;
    
#endif
    
    if(tree->current_qnode == NULL) {
        mystatus_t status = myhvml_tokenizer_set_first_settings(tree, hvml, hvml_length);
        if(status)
            return status;
    }
    
    size_t offset = 0;
    
    while (offset < hvml_length) {
        offset = state_f[tree->state](tree, tree->current_token_node, hvml, offset, hvml_length);
    }
    
    tree->global_offset += hvml_length;
    
    return MyHVML_STATUS_OK;
}

mystatus_t myhvml_tokenizer_chunk(myhvml_tree_t* tree, const char* hvml, size_t hvml_length)
{
    if(tree->encoding_usereq == MyENCODING_UTF_16LE ||
       tree->encoding_usereq == MyENCODING_UTF_16BE)
    {
        return myhvml_tokenizer_chunk_with_stream_buffer(tree, hvml, hvml_length);
    }
    
    return myhvml_tokenizer_chunk_process(tree, hvml, hvml_length);
}

mystatus_t myhvml_tokenizer_chunk_with_stream_buffer(myhvml_tree_t* tree, const char* hvml, size_t hvml_length)
{
    unsigned const char* u_hvml = (unsigned const char*)hvml;
    const myencoding_custom_f func = myencoding_get_function_by_id(tree->encoding);
    
    if(tree->stream_buffer == NULL) {
        tree->stream_buffer = myhvml_stream_buffer_create();
        
        if(tree->stream_buffer == NULL)
            return MyHVML_STATUS_STREAM_BUFFER_ERROR_CREATE;
        
        mystatus_t status = myhvml_stream_buffer_init(tree->stream_buffer, 1024);
        
        if(status)
            return status;
        
        if(myhvml_stream_buffer_add_entry(tree->stream_buffer, (4096 * 4)) == NULL)
            return MyHVML_STATUS_STREAM_BUFFER_ERROR_ADD_ENTRY;
    }
    
    myhvml_stream_buffer_t *stream_buffer = tree->stream_buffer;
    myhvml_stream_buffer_entry_t *stream_entry = myhvml_stream_buffer_current_entry(stream_buffer);
    
    size_t temp_curr_pos = stream_entry->length;
    
    for (size_t i = 0; i < hvml_length; i++)
    {
        if(func(u_hvml[i], &stream_buffer->res) == MyENCODING_STATUS_OK)
        {
            if((stream_entry->length + 4) >= stream_entry->size)
            {
                tree->encoding = MyENCODING_UTF_8;
                myhvml_tokenizer_chunk_process(tree, &stream_entry->data[temp_curr_pos], (stream_entry->length - temp_curr_pos));
                
                stream_entry = myhvml_stream_buffer_add_entry(stream_buffer, (4096 * 4));
                
                if(stream_entry == NULL)
                    return MyHVML_STATUS_STREAM_BUFFER_ERROR_ADD_ENTRY;
                
                temp_curr_pos = stream_entry->length;
            }
            
            stream_entry->length += myencoding_codepoint_to_ascii_utf_8(stream_buffer->res.result, &stream_entry->data[ stream_entry->length ]);
        }
    }
    
    if((stream_entry->length - temp_curr_pos)) {
        tree->encoding = MyENCODING_UTF_8;
        myhvml_tokenizer_chunk_process(tree, &stream_entry->data[temp_curr_pos], (stream_entry->length - temp_curr_pos));
    }
    
    return MyHVML_STATUS_OK;
}

mystatus_t myhvml_tokenizer_end(myhvml_tree_t* tree)
{
    if(tree->incoming_buf)
    {
        tree->global_offset -= tree->incoming_buf->size;
        
        tree->myhvml->parse_state_func[(tree->state + MyHVML_TOKENIZER_STATE_LAST_ENTRY)]
        (tree, tree->current_token_node, tree->incoming_buf->data, tree->incoming_buf->size, tree->incoming_buf->size);
    }
    
    tree->current_token_node->tag_id = MyHVML_TAG__END_OF_FILE;
    
    if(myhvml_queue_add(tree, 0, tree->current_token_node) != MyHVML_STATUS_OK) {
        tree->tokenizer_status = MyHVML_STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    mystatus_t status = tree->tokenizer_status;
    
#ifndef PARSER_BUILD_WITHOUT_THREADS
    
    if((tree->flags & MyHVML_TREE_FLAGS_SINGLE_MODE) == 0)
    {
        mythread_queue_list_entry_wait_for_done(tree->myhvml->thread_stream, tree->queue_entry);
        
        tree->queue_entry = mythread_queue_list_entry_delete(tree->myhvml->thread_list, 2,
                                                             tree->myhvml->thread_stream->context,
                                                             tree->queue_entry, false);
        
        /* Further, any work with tree... */
        if(mythread_queue_list_get_count(tree->myhvml->thread_stream->context) == 0)
            myhvml_tokenizer_pause(tree);
        
        if(status == MyHVML_STATUS_OK)
            status = mythread_check_status(tree->myhvml->thread_stream);
    }
    
#endif
    
    tree->flags |= MyHVML_TREE_FLAGS_PARSE_END;
    
    return status;
}

myhvml_tree_node_t * myhvml_tokenizer_fragment_init(myhvml_tree_t* tree, myhvml_tag_id_t tag_idx, enum myhvml_namespace ns)
{
    // step 3
    tree->fragment = myhvml_tree_node_create(tree);
    tree->fragment->ns = ns;
    tree->fragment->tag_id = tag_idx;
    
    // step 4, is already done
    if(ns == MyHVML_NAMESPACE_HTML) {
#if 0  /* TODO: VW */
        if(tag_idx == MyHVML_TAG_NOSCRIPT) {
            if(tree->flags & MyHVML_TREE_FLAGS_SCRIPT) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RAWTEXT;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
            }
        }
#else
        if (0) {
        }
#endif  /* TODO: VW */
        else {
            const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, tag_idx);
            myhvml_tokenizer_state_set(tree) = tag_ctx->data_parser;
        }
    }
    
    tree->fragment->token = myhvml_token_node_create(tree->token, tree->token->mcasync_token_id);
    
    if(tree->fragment->token == NULL)
        return NULL;
    
    myhvml_token_set_done(tree->fragment->token);
    tree->token_namespace = tree->fragment->token;
    
    // step 5-7
    myhvml_tree_node_t* root = myhvml_tree_node_insert_root(tree, NULL, MyHVML_NAMESPACE_HTML);
    
#if 0 /* TODO: VW */
    if(tag_idx == MyHVML_TAG_TEMPLATE)
        myhvml_tree_template_insertion_append(tree, MyHVML_INSERTION_MODE_IN_TEMPLATE);
#endif /* TODO: VW */
    
    myhvml_tree_reset_insertion_mode_appropriately(tree);
    
    return root;
}

void myhvml_tokenizer_wait(myhvml_tree_t* tree)
{
#ifndef PARSER_BUILD_WITHOUT_THREADS
    if(tree->myhvml->thread_stream)
        mythread_queue_list_entry_wait_for_done(tree->myhvml->thread_stream, tree->queue_entry);
#endif
}

void myhvml_tokenizer_post(myhvml_tree_t* tree)
{
#ifndef PARSER_BUILD_WITHOUT_THREADS
    if(tree->myhvml->thread_stream)
        mythread_resume(tree->myhvml->thread_stream, MyTHREAD_OPT_UNDEF);
    
    if(tree->myhvml->thread_batch)
        mythread_resume(tree->myhvml->thread_batch, MyTHREAD_OPT_UNDEF);
#endif
}

void myhvml_tokenizer_pause(myhvml_tree_t* tree)
{
#ifndef PARSER_BUILD_WITHOUT_THREADS
    if(tree->myhvml->thread_stream)
        mythread_stop(tree->myhvml->thread_stream);
    
    if(tree->myhvml->thread_batch)
        mythread_stop(tree->myhvml->thread_batch);
#endif
}

void myhvml_tokenizer_calc_current_namespace(myhvml_tree_t* tree, myhvml_token_node_t* token_node)
{
    if((tree->parse_flags & MyHVML_TREE_PARSE_FLAGS_WITHOUT_BUILD_TREE) == 0) {
        if(tree->flags & MyHVML_TREE_FLAGS_SINGLE_MODE)
        {
            myhvml_tokenizer_state_set(tree) = tree->state_of_builder;
        }
#if 0 /* TODO: VW */
        else {
            if(token_node->tag_id == MyHVML_TAG_MATH ||
               token_node->tag_id == MyHVML_TAG_SVG ||
               token_node->tag_id == MyHVML_TAG_FRAMESET)
            {
                tree->token_namespace = token_node;
            }
            else if(tree->token_namespace && (token_node->type & MyHVML_TOKEN_TYPE_CLOSE) == 0) {
                const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, token_node->tag_id);
                
                if(tag_ctx->data_parser != MyHVML_TOKENIZER_STATE_DATA)
                {
                    myhvml_tree_wait_for_last_done_token(tree, token_node);
                    myhvml_tokenizer_state_set(tree) = tree->state_of_builder;
                }
            }
        }
#else
        else if(tree->token_namespace && (token_node->type & MyHVML_TOKEN_TYPE_CLOSE) == 0) {
            const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, token_node->tag_id);

            if(tag_ctx->data_parser != MyHVML_TOKENIZER_STATE_DATA)
            {
                myhvml_tree_wait_for_last_done_token(tree, token_node);
                myhvml_tokenizer_state_set(tree) = tree->state_of_builder;
            }
        }
#endif /* TODO: VW */
    }
}

void myhvml_check_tag_parser(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset)
{
    myhvml_tag_t* tags = tree->tags;
    const myhvml_tag_context_t *tag_ctx = NULL;
    
    if(hvml_offset < token_node->raw_length) {
        const char *tagname = myhvml_tree_incomming_buffer_make_data(tree, token_node->raw_begin, token_node->raw_length);
        tag_ctx = myhvml_tag_get_by_name(tags, tagname, token_node->raw_length);
    }
    else {
        tag_ctx = myhvml_tag_get_by_name(tags, &hvml[ (token_node->raw_begin - tree->global_offset) ], token_node->raw_length);
    }
    
    if(tag_ctx) {
        token_node->tag_id = tag_ctx->id;
    }
    else {
        if(hvml_offset < token_node->raw_length) {
            const char *tagname = myhvml_tree_incomming_buffer_make_data(tree, token_node->raw_begin, token_node->raw_length);
            token_node->tag_id = myhvml_tag_add(tags, tagname, token_node->raw_length, MyHVML_TOKENIZER_STATE_DATA, true);
        }
        else {
            token_node->tag_id = myhvml_tag_add(tags, &hvml[ (token_node->raw_begin - tree->global_offset) ], token_node->raw_length, MyHVML_TOKENIZER_STATE_DATA, true);
        }
        
        myhvml_tag_set_category(tags, token_node->tag_id, MyHVML_NAMESPACE_HTML, MyHVML_TAG_CATEGORIES_ORDINARY);
    }
}

////
myhvml_token_node_t * myhvml_tokenizer_queue_create_text_node_if_need(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t absolute_hvml_offset, enum myhvml_token_type type)
{
    if(token_node->tag_id == MyHVML_TAG__UNDEF)
    {
        if(absolute_hvml_offset > token_node->raw_begin)
        {
            size_t tmp_begin = token_node->element_begin;
            
            token_node->type |= type;
            token_node->tag_id = MyHVML_TAG__TEXT;
            token_node->element_begin = token_node->raw_begin;
            token_node->raw_length = token_node->element_length = absolute_hvml_offset - token_node->raw_begin;
            
            if(myhvml_queue_add(tree, tmp_begin, token_node) != MyHVML_STATUS_OK) {
                return NULL;
            }
            
            return tree->current_token_node;
        }
    }
    
    return token_node;
}

void myhvml_tokenizer_set_state(myhvml_tree_t* tree, myhvml_token_node_t* token_node)
{
    if((token_node->type & MyHVML_TOKEN_TYPE_CLOSE) == 0)
    {
#if 0 /* TODO: VW */
        if(token_node->tag_id == MyHVML_TAG_NOSCRIPT &&
           (tree->flags & MyHVML_TREE_FLAGS_SCRIPT) == 0)
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
        }
#else
        if (0) {
        }
#endif /* TODO: VW */
        else {
            const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, token_node->tag_id);
            myhvml_tokenizer_state_set(tree) = tag_ctx->data_parser;
        }
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    }
}

/////////////////////////////////////////////////////////
//// RCDATA
////
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_rcdata(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(tree->tmp_tag_id == 0) {
        token_node->raw_begin = (hvml_offset + tree->global_offset);
        
        mythread_queue_node_t* prev_qnode = mythread_queue_get_prev_node(tree->current_qnode);
        
        if(prev_qnode && prev_qnode->args) {
            tree->tmp_tag_id = ((myhvml_token_node_t*)(prev_qnode->args))->tag_id;
        }
        else if(tree->fragment) {
            tree->tmp_tag_id = tree->fragment->tag_id;
        }
    }
    
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '<')
        {
            token_node->element_begin = (hvml_offset + tree->global_offset);
            
            hvml_offset++;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RCDATA_LESS_THAN_SIGN;
            
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_rcdata_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '/')
    {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_OPEN;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RCDATA;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_rcdata_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] == MyCORE_STRING_MAP_CHAR_A_Z_a_z)
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_NAME;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RCDATA;
    }
    
    return hvml_offset;
}

bool _myhvml_tokenizer_state_andata_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t *hvml_offset, size_t tmp_begin, enum myhvml_token_type type)
{
    token_node->raw_length = (*hvml_offset + tree->global_offset) - token_node->raw_begin;
    myhvml_check_tag_parser(tree, token_node, hvml, *hvml_offset);
    
    if(token_node->tag_id != tree->tmp_tag_id)
    {
        token_node->raw_begin  = tmp_begin;
        token_node->raw_length = 0;
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RCDATA;
        
        (*hvml_offset)++;
        return false;
    }
    
    if((token_node->raw_begin - 2) > tmp_begin)
    {
        size_t tmp_element_begin = token_node->element_begin;
        size_t tmp_raw_begin     = token_node->raw_begin;
        
        token_node->raw_length      = (token_node->raw_begin - 2) - tmp_begin;
        token_node->raw_begin       = tmp_begin;
        token_node->element_begin   = tmp_begin;
        token_node->element_length  = token_node->raw_length;
        token_node->type           |= type;
        token_node->type           ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
        token_node->tag_id          = MyHVML_TAG__TEXT;
        
        /* TODO: return error */
        myhvml_queue_add(tree, *hvml_offset, token_node);
        
        /* return true values */
        token_node = tree->current_token_node;
        token_node->element_begin = tmp_element_begin;
        token_node->raw_begin = tmp_raw_begin;
    }
    
    token_node->tag_id         = tree->tmp_tag_id;
    token_node->type          |= MyHVML_TOKEN_TYPE_CLOSE;
    token_node->raw_length     = (tree->global_offset + *hvml_offset) - token_node->raw_begin;
    
    return true;
}

size_t myhvml_tokenizer_state_rcdata_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    size_t tmp_begin = token_node->raw_begin;
    token_node->raw_begin = hvml_offset + tree->global_offset;
    
    while(hvml_offset < hvml_size)
    {
        if(mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] == MyCORE_STRING_MAP_CHAR_WHITESPACE)
        {
            if(_myhvml_tokenizer_state_andata_end_tag_name(tree, token_node, hvml, &hvml_offset, tmp_begin, MyHVML_TOKEN_TYPE_RCDATA)) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
                
                tree->tmp_tag_id = 0;
                hvml_offset++;
                
                return hvml_offset;
            }
            
            break;
        }
        else if(hvml[hvml_offset] == '>')
        {
            if(_myhvml_tokenizer_state_andata_end_tag_name(tree, token_node, hvml, &hvml_offset, tmp_begin, MyHVML_TOKEN_TYPE_RCDATA)) {
                hvml_offset++;
                
                token_node = tree->current_token_node;
                token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
                
                if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                tree->tmp_tag_id = 0;
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
                
                return hvml_offset;
            }
            
            break;
        }
        // check end of tag
        else if(hvml[hvml_offset] == '/')
        {
            if(_myhvml_tokenizer_state_andata_end_tag_name(tree, token_node, hvml, &hvml_offset, tmp_begin, MyHVML_TOKEN_TYPE_RCDATA)) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
                
                tree->tmp_tag_id = 0;
                hvml_offset++;
                
                return hvml_offset;
            }
            
            break;
        }
        else if (mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] != MyCORE_STRING_MAP_CHAR_A_Z_a_z) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RCDATA;
            break;
        }
        
        hvml_offset++;
    }
    
    token_node->raw_begin = tmp_begin;
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// RAWTEXT
////
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_rawtext(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(tree->tmp_tag_id == 0) {
        token_node->raw_begin = (hvml_offset + tree->global_offset);
        
        mythread_queue_node_t* prev_qnode = mythread_queue_get_prev_node(tree->current_qnode);
        
        if(prev_qnode && prev_qnode->args) {
            tree->tmp_tag_id = ((myhvml_token_node_t*)prev_qnode->args)->tag_id;
        }
        else if(tree->fragment) {
            tree->tmp_tag_id = tree->fragment->tag_id;
        }
    }

    
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '<')
        {
            token_node->element_begin = (hvml_offset + tree->global_offset);
            
            hvml_offset++;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RAWTEXT_LESS_THAN_SIGN;
            
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_rawtext_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '/')
    {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_OPEN;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RAWTEXT;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_rawtext_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] == MyCORE_STRING_MAP_CHAR_A_Z_a_z)
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_NAME;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RAWTEXT;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_rawtext_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    size_t tmp_begin = token_node->raw_begin;
    token_node->raw_begin = hvml_offset + tree->global_offset;
    
    while(hvml_offset < hvml_size)
    {
        if(mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] == MyCORE_STRING_MAP_CHAR_WHITESPACE)
        {
            if(_myhvml_tokenizer_state_andata_end_tag_name(tree, token_node, hvml, &hvml_offset, tmp_begin, MyHVML_TOKEN_TYPE_RAWTEXT)) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
                
                tree->tmp_tag_id = 0;
                hvml_offset++;
            }
            
            return hvml_offset;
        }
        else if(hvml[hvml_offset] == '>')
        {
            if(_myhvml_tokenizer_state_andata_end_tag_name(tree, token_node, hvml, &hvml_offset, tmp_begin, MyHVML_TOKEN_TYPE_RAWTEXT)) {
                hvml_offset++;
                
                token_node = tree->current_token_node;
                token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
                
                if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                tree->tmp_tag_id = 0;
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
            }
            
            return hvml_offset;
        }
        // check end of tag
        else if(hvml[hvml_offset] == '/')
        {
            if(_myhvml_tokenizer_state_andata_end_tag_name(tree, token_node, hvml, &hvml_offset, tmp_begin, MyHVML_TOKEN_TYPE_RAWTEXT)) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
                
                tree->tmp_tag_id = 0;
                hvml_offset++;
            }
            
            return hvml_offset;
        }
        else if (mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] != MyCORE_STRING_MAP_CHAR_A_Z_a_z) {
            token_node->raw_begin = tmp_begin;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_RAWTEXT;
            
            return hvml_offset;
        }
        
        hvml_offset++;
    }
    
    token_node->raw_begin = tmp_begin;
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// PLAINTEXT
////
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_plaintext(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if((token_node->type & MyHVML_TOKEN_TYPE_PLAINTEXT) == 0)
        token_node->type |= MyHVML_TOKEN_TYPE_PLAINTEXT;
    
    token_node->type      ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
    token_node->raw_begin  = (hvml_offset + tree->global_offset);
    token_node->raw_length = token_node->element_length = (hvml_size + tree->global_offset) - token_node->raw_begin;
    token_node->tag_id     = MyHVML_TAG__TEXT;
    
    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    
    if(myhvml_queue_add(tree, hvml_size, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_size;
}

/////////////////////////////////////////////////////////
//// CDATA
////
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_cdata_section(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if((token_node->type & MyHVML_TOKEN_TYPE_CDATA) == 0)
        token_node->type |= MyHVML_TOKEN_TYPE_CDATA;
    
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '>')
        {
            const char *tagname;
            if(hvml_offset < 2)
                tagname = myhvml_tree_incomming_buffer_make_data(tree,((hvml_offset + tree->global_offset) - 2), 2);
            else
                tagname = &hvml[hvml_offset - 2];
            
            if(tagname[0] == ']' && tagname[1] == ']')
            {
               token_node->raw_length = (((hvml_offset + tree->global_offset) - 2) - token_node->raw_begin);
                hvml_offset++;
                
                if(token_node->raw_length) {
                    token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
                    
                    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                        return 0;
                    }
                    
                }
                else {
                    token_node->raw_begin = hvml_offset + tree->global_offset;
                }
                
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
                break;
            }
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// outside of tag
//// %HERE%<div>%HERE%</div>%HERE%
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_data(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '<')
        {
            token_node->element_begin = (tree->global_offset + hvml_offset);
            
            hvml_offset++;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_TAG_OPEN;
            
            break;
        }
        else if(hvml[hvml_offset] == '\0' && (token_node->type & MyHVML_TOKEN_TYPE_NULL) == 0) {
            // parse error
            /* %EXTERNAL% VALIDATOR:TOKENIZER POSITION STATUS:CHAR_NULL LEVEL:ERROR BEGIN:hvml_offset LENGTH:1 */
            
            token_node->type |= MyHVML_TOKEN_TYPE_NULL;
        }
        else if(token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE &&
                mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] != MyCORE_STRING_MAP_CHAR_WHITESPACE) {
            token_node->type ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
            token_node->type |= MyHVML_TOKEN_TYPE_DATA;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag
//// <%HERE%div></div>
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] == MyCORE_STRING_MAP_CHAR_A_Z_a_z)
    {
        token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((tree->global_offset + hvml_offset) - 1), MyHVML_TOKEN_TYPE_DATA);
        if(token_node == NULL) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        token_node->raw_begin = tree->global_offset + hvml_offset;
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_TAG_NAME;
    }
    else if(hvml[hvml_offset] == '!')
    {
        token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((tree->global_offset + hvml_offset) - 1), MyHVML_TOKEN_TYPE_DATA);
        if(token_node == NULL) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        hvml_offset++;
        token_node->raw_begin = tree->global_offset + hvml_offset;
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_MARKUP_DECLARATION_OPEN;
    }
    else if(hvml[hvml_offset] == '/')
    {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_END_TAG_OPEN;
    }
    else if(hvml[hvml_offset] == '?')
    {
        // parse error
        /* %EXTERNAL% VALIDATOR:TOKENIZER POSITION STATUS:CHAR_BAD LEVEL:ERROR BEGIN:hvml_offset LENGTH:1 */
        
        token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((tree->global_offset + hvml_offset) - 1), MyHVML_TOKEN_TYPE_DATA);
        if(token_node == NULL) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        token_node->raw_begin = tree->global_offset + hvml_offset;
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_COMMENT;
    }
    else {
        // parse error
        /* %EXTERNAL% VALIDATOR:TOKENIZER POSITION STATUS:NOT_EXPECTED LEVEL:ERROR BEGIN:hvml_offset LENGTH:1 */
        
        token_node->type ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag
//// </%HERE%
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] == MyCORE_STRING_MAP_CHAR_A_Z_a_z)
    {
        token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((tree->global_offset + hvml_offset) - 2), MyHVML_TOKEN_TYPE_DATA);
        if(token_node == NULL) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        token_node->raw_begin = tree->global_offset + hvml_offset;
        token_node->type = MyHVML_TOKEN_TYPE_CLOSE;
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_TAG_NAME;
    }
    else if(hvml[hvml_offset] == '>')
    {
        // parse error
        /* %EXTERNAL% VALIDATOR:TOKENIZER POSITION STATUS:CHAR_BAD LEVEL:ERROR BEGIN:hvml_offset LENGTH:1 */
        
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    }
    else {
        // parse error
        /* %EXTERNAL% VALIDATOR:TOKENIZER POSITION STATUS:CHAR_BAD LEVEL:ERROR BEGIN:hvml_offset LENGTH:1 */
        
        token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((tree->global_offset + hvml_offset) - 2), MyHVML_TOKEN_TYPE_DATA);
        if(token_node == NULL) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        token_node->raw_begin = tree->global_offset + hvml_offset;
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_COMMENT;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag
//// <!%HERE%
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_markup_declaration_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if((token_node->raw_begin + 2) > (hvml_size + tree->global_offset)) {
        tree->incoming_buf->length = hvml_offset;
        return hvml_size;
    }
    
    const char *tagname = myhvml_tree_incomming_buffer_make_data(tree, token_node->raw_begin, 2);
    
    // for a comment
    if(tagname[0] == '-' && tagname[1] == '-')
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT_START;
        
        hvml_offset += 2;
        
        token_node->raw_begin  = hvml_offset + tree->global_offset;
        token_node->raw_length = 0;
        
        return hvml_offset;
    }
    
    if((token_node->raw_begin + 7) > (hvml_size + tree->global_offset)) {
        tree->incoming_buf->length = hvml_offset;
        return hvml_size;
    }
    
    tagname = myhvml_tree_incomming_buffer_make_data(tree, token_node->raw_begin, 7);
    
    if(mycore_strncasecmp(tagname, "DOCTYPE", 7) == 0)
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DOCTYPE;
        
        hvml_offset = (token_node->raw_begin + 7) - tree->incoming_buf->offset;
        
        token_node->raw_length  = 7;
        token_node->tag_id = MyHVML_TAG__DOCTYPE;
        
        return hvml_offset;
    }
    
    // CDATA sections can only be used in foreign content (MathML or SVG)
    if(strncmp(tagname, "[CDATA[", 7) == 0) {
        if(tree->current_qnode->prev && tree->current_qnode->prev->args)
        {
            myhvml_tree_wait_for_last_done_token(tree, tree->current_qnode->prev->args);
            myhvml_tree_node_t *adjusted_current_node = myhvml_tree_adjusted_current_node(tree);
            
            if(adjusted_current_node &&
               adjusted_current_node->ns != MyHVML_NAMESPACE_HTML)
            {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_CDATA_SECTION;
                
                hvml_offset = (token_node->raw_begin + 7) - tree->incoming_buf->offset;
                
                token_node->raw_begin += 7;
                token_node->raw_length = 0;
                token_node->tag_id = MyHVML_TAG__TEXT;
                token_node->type ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
                
                return hvml_offset;
            }
        }
    }
    
    token_node->raw_length = 0;
    
    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_COMMENT;
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag
//// <%HERE%
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(mycore_string_tokenizer_chars_map[ (unsigned char)hvml[hvml_offset] ] == MyCORE_STRING_MAP_CHAR_WHITESPACE)
        {
            token_node->raw_length = (tree->global_offset + hvml_offset) - token_node->raw_begin;
            myhvml_check_tag_parser(tree, token_node, hvml, hvml_offset);
            
            hvml_offset++;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
            
            break;
        }
        else if(hvml[hvml_offset] == '/')
        {
            token_node->raw_length = (tree->global_offset + hvml_offset) - token_node->raw_begin;
            myhvml_check_tag_parser(tree, token_node, hvml, hvml_offset);
            
            hvml_offset++;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SELF_CLOSING_START_TAG;
            
            break;
        }
        else if(hvml[hvml_offset] == '>')
        {
            token_node->raw_length = (tree->global_offset + hvml_offset) - token_node->raw_begin;
            
            myhvml_check_tag_parser(tree, token_node, hvml, hvml_offset);
            myhvml_tokenizer_set_state(tree, token_node);
            
            hvml_offset++;
            
            token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag
//// <%HERE%
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_self_closing_start_tag(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '>') {
        token_node->type |= MyHVML_TOKEN_TYPE_CLOSE_SELF;
        myhvml_tokenizer_set_state(tree, token_node);
        
        hvml_offset++;
        
        // TODO: ??????
        token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag, after tag name
//// <div%HERE% class="bla"></div>
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_before_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    // skip WS
    myhvml_parser_skip_whitespace()
    
    if(hvml_offset >= hvml_size) {
        return hvml_offset;
    }
    
    if(hvml[hvml_offset] == '>')
    {
        myhvml_tokenizer_set_state(tree, token_node);
        
        hvml_offset++;
        
        token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
    }
    else if(hvml[hvml_offset] == '/') {
        token_node->type |= MyHVML_TOKEN_TYPE_CLOSE_SELF;
        
        hvml_offset++;
    }
    else {
        myhvml_parser_queue_set_attr(tree, token_node)
        
        tree->attr_current->raw_key_begin    = hvml_offset + tree->global_offset;
        tree->attr_current->raw_key_length   = 0;
        tree->attr_current->raw_value_begin  = 0;
        tree->attr_current->raw_value_length = 0;
        
        if(hvml[hvml_offset] == '=') {
            // parse error
            /* %EXTERNAL% VALIDATOR:TOKENIZER POSITION STATUS:NOT_EXPECTED LEVEL:ERROR BEGIN:hvml_offset LENGTH:1 */
            
            hvml_offset++;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_ATTRIBUTE_NAME;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag, inside of attr key
//// <div cla%HERE%ss="bla"></div>
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(myhvml_whithspace(hvml[hvml_offset], ==, ||))
        {
            tree->attr_current->raw_key_length = (tree->global_offset + hvml_offset) - tree->attr_current->raw_key_begin;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_NAME;
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '=')
        {
            tree->attr_current->raw_key_length = (tree->global_offset + hvml_offset) - tree->attr_current->raw_key_begin;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_VALUE;
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '>')
        {
            tree->attr_current->raw_key_length = (tree->global_offset + hvml_offset) - tree->attr_current->raw_key_begin;
            myhvml_tokenizer_set_state(tree, token_node);
            
            hvml_offset++;
            
            token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            break;
        }
        else if(hvml[hvml_offset] == '/')
        {
            tree->attr_current->raw_key_length = (tree->global_offset + hvml_offset) - tree->attr_current->raw_key_begin;
            
            token_node->type |= MyHVML_TOKEN_TYPE_CLOSE_SELF;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            hvml_offset++;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag, after attr key
//// <div class%HERE%="bla"></div>
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_after_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '=')
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_VALUE;
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '>')
        {
            myhvml_tokenizer_set_state(tree, token_node);
            
            hvml_offset++;
            
            token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            break;
        }
        else if(hvml[hvml_offset] == '"' || hvml[hvml_offset] == '\'' || hvml[hvml_offset] == '<')
        {
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_parser_queue_set_attr(tree, token_node)
            
            tree->attr_current->raw_key_begin   = (tree->global_offset + hvml_offset);
            tree->attr_current->raw_key_length  = 0;
            tree->attr_current->raw_value_begin  = 0;
            tree->attr_current->raw_value_length = 0;
            
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_ATTRIBUTE_NAME;
            break;
        }
        else if(myhvml_whithspace(hvml[hvml_offset], !=, &&))
        {
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_parser_queue_set_attr(tree, token_node)
            
            tree->attr_current->raw_key_begin   = (hvml_offset + tree->global_offset);
            tree->attr_current->raw_key_length  = 0;
            tree->attr_current->raw_value_begin  = 0;
            tree->attr_current->raw_value_length = 0;
            
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_ATTRIBUTE_NAME;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag, after attr key
//// <div class=%HERE%"bla"></div>
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_before_attribute_value(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '>') {
            // parse error
            /* %EXTERNAL% VALIDATOR:TOKENIZER POSITION STATUS:NOT_EXPECTED LEVEL:ERROR BEGIN:hvml_offset LENGTH:1 */
            
            myhvml_tokenizer_set_state(tree, token_node);
            
            hvml_offset++;
            
            token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            break;
        }
        else if(myhvml_whithspace(hvml[hvml_offset], !=, &&))
        {
            if(hvml[hvml_offset] == '"') {
                hvml_offset++;
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED;
            }
            else if(hvml[hvml_offset] == '\'') {
                hvml_offset++;
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_UNQUOTED;
            }
            
            tree->attr_current->raw_value_begin = (tree->global_offset + hvml_offset);
            
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag, inside of attr value
//// <div class="bla%HERE%"></div>
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_attribute_value_double_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    //myhvml_t* myhvml = tree->myhvml;
    
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '"')
        {
            tree->attr_current->raw_value_length = (tree->global_offset + hvml_offset) - tree->attr_current->raw_value_begin;
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED;
            
            hvml_offset++;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag, inside of attr value
//// <div class="bla%HERE%"></div>
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_attribute_value_single_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    //myhvml_t* myhvml = tree->myhvml;
    
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '\'')
        {
            tree->attr_current->raw_value_length = (tree->global_offset + hvml_offset) - tree->attr_current->raw_value_begin;
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED;
            
            hvml_offset++;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// inside of tag, inside of attr value
//// <div class="bla%HERE%"></div>
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_attribute_value_unquoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(myhvml_whithspace(hvml[hvml_offset], ==, ||))
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
            
            tree->attr_current->raw_value_length = (tree->global_offset + hvml_offset) - tree->attr_current->raw_value_begin;
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '>') {
            // parse error
            /* %EXTERNAL% VALIDATOR:TOKENIZER POSITION STATUS:UNSAFE_USE LEVEL:INFO BEGIN:hvml_offset LENGTH:1 */
            
            tree->attr_current->raw_value_length = (tree->global_offset + hvml_offset) - tree->attr_current->raw_value_begin;
            
            myhvml_tokenizer_set_state(tree, token_node);
            
            hvml_offset++;
            
            token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_after_attribute_value_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(myhvml_whithspace(hvml[hvml_offset], ==, ||)) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
        hvml_offset++;
    }
    else if(hvml[hvml_offset] == '/') {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SELF_CLOSING_START_TAG;
        hvml_offset++;
    }
    else if(hvml[hvml_offset] == '>') {
        myhvml_tokenizer_set_state(tree, token_node);
        
        hvml_offset++;
        
        token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// COMMENT
//// <!--%HERE%
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_comment_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->tag_id = MyHVML_TAG__COMMENT;
    
    if(hvml[hvml_offset] == '-')
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT_START_DASH;
    }
    else if(hvml[hvml_offset] == '>')
    {
        hvml_offset++;
        
        token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
        token_node->raw_length = 0;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
        
        return hvml_offset;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT;
    }
    
    hvml_offset++;
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_comment_start_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->tag_id = MyHVML_TAG__COMMENT;
    
    if(hvml[hvml_offset] == '-')
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT_END;
    }
    else if(hvml[hvml_offset] == '>')
    {
        hvml_offset++;
        
        token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
        token_node->raw_length = 0;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
        
        return hvml_offset;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT;
    }
    
    hvml_offset++;
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_comment(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->tag_id = MyHVML_TAG__COMMENT;
    
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '-')
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT_END_DASH;
            hvml_offset++;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_comment_end_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '-')
    {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT_END;
    }
    else {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_comment_end(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '>')
    {
        token_node->raw_length = ((tree->global_offset + hvml_offset) - token_node->raw_begin);
        
        if(token_node->raw_length >= 2)
            token_node->raw_length -= 2;
        else
            token_node->raw_length = 0;
        
        hvml_offset++;
        
        token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    }
    else if(hvml[hvml_offset] == '!') {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT_END_BANG;
    }
    else if(hvml[hvml_offset] == '-') {
        hvml_offset++;
    }
    else {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_comment_end_bang(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '>')
    {
        if(((tree->global_offset + hvml_offset) - 3) >= token_node->raw_begin) {
            token_node->raw_length = ((tree->global_offset + hvml_offset) - token_node->raw_begin) - 3;
            
            hvml_offset++;
            
            token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
        }
        else {
            hvml_offset++;
            
            token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
            token_node->raw_length = 0;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    }
    else if(hvml[hvml_offset] == '-') {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT_END_DASH;
    }
    else {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_COMMENT;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// BOGUS COMMENT
//// find >
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_bogus_comment(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->tag_id = MyHVML_TAG__COMMENT;
    token_node->type |= MyHVML_TOKEN_TYPE_COMMENT;
    
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '>')
        {
            token_node->raw_length = ((tree->global_offset + hvml_offset) - token_node->raw_begin);
            
            hvml_offset++;
            
            token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// Parse error
//// find >
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_parse_error_stop(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    tree->tokenizer_status = MyHVML_STATUS_TOKENIZER_ERROR_MEMORY_ALLOCATION;
    return hvml_size;
}

mystatus_t myhvml_tokenizer_state_init(myhvml_t* myhvml)
{
    myhvml->parse_state_func = (myhvml_tokenizer_state_f*)mycore_malloc(sizeof(myhvml_tokenizer_state_f) *
                                                                   ((MyHVML_TOKENIZER_STATE_LAST_ENTRY *
                                                                     MyHVML_TOKENIZER_STATE_LAST_ENTRY) + 1));
    
    if(myhvml->parse_state_func == NULL)
        return MyHVML_STATUS_TOKENIZER_ERROR_MEMORY_ALLOCATION;
    
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_DATA]                          = myhvml_tokenizer_state_data;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_TAG_OPEN]                      = myhvml_tokenizer_state_tag_open;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_TAG_NAME]                      = myhvml_tokenizer_state_tag_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_END_TAG_OPEN]                  = myhvml_tokenizer_state_end_tag_open;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SELF_CLOSING_START_TAG]        = myhvml_tokenizer_state_self_closing_start_tag;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_MARKUP_DECLARATION_OPEN]       = myhvml_tokenizer_state_markup_declaration_open;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME]         = myhvml_tokenizer_state_before_attribute_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_ATTRIBUTE_NAME]                = myhvml_tokenizer_state_attribute_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_NAME]          = myhvml_tokenizer_state_after_attribute_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_VALUE]        = myhvml_tokenizer_state_before_attribute_value;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED]  = myhvml_tokenizer_state_after_attribute_value_quoted;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED] = myhvml_tokenizer_state_attribute_value_double_quoted;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED] = myhvml_tokenizer_state_attribute_value_single_quoted;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_UNQUOTED]      = myhvml_tokenizer_state_attribute_value_unquoted;
    
    // comments
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_COMMENT_START]                 = myhvml_tokenizer_state_comment_start;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_COMMENT_START_DASH]            = myhvml_tokenizer_state_comment_start_dash;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_COMMENT]                       = myhvml_tokenizer_state_comment;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_COMMENT_END]                   = myhvml_tokenizer_state_comment_end;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_COMMENT_END_DASH]              = myhvml_tokenizer_state_comment_end_dash;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_COMMENT_END_BANG]              = myhvml_tokenizer_state_comment_end_bang;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_BOGUS_COMMENT]                 = myhvml_tokenizer_state_bogus_comment;
    
    // cdata
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_CDATA_SECTION]                 = myhvml_tokenizer_state_cdata_section;
    
    // rcdata
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_RCDATA]                        = myhvml_tokenizer_state_rcdata;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_RCDATA_LESS_THAN_SIGN]         = myhvml_tokenizer_state_rcdata_less_than_sign;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_OPEN]           = myhvml_tokenizer_state_rcdata_end_tag_open;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_NAME]           = myhvml_tokenizer_state_rcdata_end_tag_name;
    
    // rawtext
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_RAWTEXT]                        = myhvml_tokenizer_state_rawtext;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_RAWTEXT_LESS_THAN_SIGN]         = myhvml_tokenizer_state_rawtext_less_than_sign;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_OPEN]           = myhvml_tokenizer_state_rawtext_end_tag_open;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_NAME]           = myhvml_tokenizer_state_rawtext_end_tag_name;
    
    // plaintext
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_PLAINTEXT]                     = myhvml_tokenizer_state_plaintext;
    
    // doctype
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_DOCTYPE]                                 = myhvml_tokenizer_state_doctype;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_NAME]                     = myhvml_tokenizer_state_before_doctype_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_DOCTYPE_NAME]                            = myhvml_tokenizer_state_doctype_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_NAME]                      = myhvml_tokenizer_state_after_doctype_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_CUSTOM_AFTER_DOCTYPE_NAME_A_Z]           = myhvml_tokenizer_state_custom_after_doctype_name_a_z;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER]        = myhvml_tokenizer_state_before_doctype_system_identifier;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED] = myhvml_tokenizer_state_doctype_system_identifier_double_quoted;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED] = myhvml_tokenizer_state_doctype_system_identifier_single_quoted;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_SYSTEM_IDENTIFIER]         = myhvml_tokenizer_state_after_doctype_system_identifier;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE]                           = myhvml_tokenizer_state_bogus_doctype;
    
#if 0 /* VW */
    // script
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA]                               = myhvml_tokenizer_state_script_data;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_LESS_THAN_SIGN]                = myhvml_tokenizer_state_script_data_less_than_sign;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_OPEN]                  = myhvml_tokenizer_state_script_data_end_tag_open;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_NAME]                  = myhvml_tokenizer_state_script_data_end_tag_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START]                  = myhvml_tokenizer_state_script_data_escape_start;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START_DASH]             = myhvml_tokenizer_state_script_data_escape_start_dash;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED]                       = myhvml_tokenizer_state_script_data_escaped;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH]                  = myhvml_tokenizer_state_script_data_escaped_dash;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH]             = myhvml_tokenizer_state_script_data_escaped_dash_dash;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN]        = myhvml_tokenizer_state_script_data_escaped_less_than_sign;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_OPEN]          = myhvml_tokenizer_state_script_data_escaped_end_tag_open;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_NAME]          = myhvml_tokenizer_state_script_data_escaped_end_tag_name;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_START]           = myhvml_tokenizer_state_script_data_double_escape_start;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED]                = myhvml_tokenizer_state_script_data_double_escaped;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH]           = myhvml_tokenizer_state_script_data_double_escaped_dash;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH]      = myhvml_tokenizer_state_script_data_double_escaped_dash_dash;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN] = myhvml_tokenizer_state_script_data_double_escaped_less_than_sign;
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_END]             = myhvml_tokenizer_state_script_data_double_escape_end;
#endif /* VW */
    
    myhvml->parse_state_func[MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP]                          = myhvml_tokenizer_state_parse_error_stop;
    
    
    // ***********
    // for ends
    // *********
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_DATA)]                          = myhvml_tokenizer_end_state_data;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_TAG_OPEN)]                      = myhvml_tokenizer_end_state_tag_open;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_TAG_NAME)]                      = myhvml_tokenizer_end_state_tag_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_END_TAG_OPEN)]                  = myhvml_tokenizer_end_state_end_tag_open;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SELF_CLOSING_START_TAG)]        = myhvml_tokenizer_end_state_self_closing_start_tag;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_MARKUP_DECLARATION_OPEN)]       = myhvml_tokenizer_end_state_markup_declaration_open;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME)]         = myhvml_tokenizer_end_state_before_attribute_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_ATTRIBUTE_NAME)]                = myhvml_tokenizer_end_state_attribute_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_NAME)]          = myhvml_tokenizer_end_state_after_attribute_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_VALUE)]        = myhvml_tokenizer_end_state_before_attribute_value;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED)] = myhvml_tokenizer_end_state_attribute_value_double_quoted;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED)] = myhvml_tokenizer_end_state_attribute_value_single_quoted;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_UNQUOTED)]      = myhvml_tokenizer_end_state_attribute_value_unquoted;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED)]  = myhvml_tokenizer_end_state_after_attribute_value_quoted;
    
    // for ends comments
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_COMMENT_START)]                 = myhvml_tokenizer_end_state_comment_start;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_COMMENT_START_DASH)]            = myhvml_tokenizer_end_state_comment_start_dash;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_COMMENT)]                       = myhvml_tokenizer_end_state_comment;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_COMMENT_END)]                   = myhvml_tokenizer_end_state_comment_end;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_COMMENT_END_DASH)]              = myhvml_tokenizer_end_state_comment_end_dash;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_COMMENT_END_BANG)]              = myhvml_tokenizer_end_state_comment_end_bang;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_BOGUS_COMMENT)]                 = myhvml_tokenizer_end_state_bogus_comment;
    
    // for ends cdata
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_CDATA_SECTION)]                 = myhvml_tokenizer_end_state_cdata_section;
    
    // rcdata
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_RCDATA)]                        = myhvml_tokenizer_end_state_rcdata;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_RCDATA_LESS_THAN_SIGN)]         = myhvml_tokenizer_end_state_rcdata_less_than_sign;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_OPEN)]           = myhvml_tokenizer_end_state_rcdata_end_tag_open;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_NAME)]           = myhvml_tokenizer_end_state_rcdata_end_tag_name;
    
    // rawtext
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_RAWTEXT)]                        = myhvml_tokenizer_end_state_rawtext;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_RAWTEXT_LESS_THAN_SIGN)]         = myhvml_tokenizer_end_state_rawtext_less_than_sign;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_OPEN)]           = myhvml_tokenizer_end_state_rawtext_end_tag_open;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_NAME)]           = myhvml_tokenizer_end_state_rawtext_end_tag_name;
    
    // for ends plaintext
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_PLAINTEXT)]                     = myhvml_tokenizer_end_state_plaintext;
    
    // for ends doctype
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_DOCTYPE)]                                 = myhvml_tokenizer_end_state_doctype;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_NAME)]                     = myhvml_tokenizer_end_state_before_doctype_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_DOCTYPE_NAME)]                            = myhvml_tokenizer_end_state_doctype_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_NAME)]                      = myhvml_tokenizer_end_state_after_doctype_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_CUSTOM_AFTER_DOCTYPE_NAME_A_Z)]           = myhvml_tokenizer_end_state_custom_after_doctype_name_a_z;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER)]        = myhvml_tokenizer_end_state_before_doctype_system_identifier;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED)] = myhvml_tokenizer_end_state_doctype_system_identifier_double_quoted;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED)] = myhvml_tokenizer_end_state_doctype_system_identifier_single_quoted;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_SYSTEM_IDENTIFIER)]         = myhvml_tokenizer_end_state_after_doctype_system_identifier;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE)]                           = myhvml_tokenizer_end_state_bogus_doctype;
    
    // for ends script
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA)]                               = myhvml_tokenizer_end_state_script_data;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_LESS_THAN_SIGN)]                = myhvml_tokenizer_end_state_script_data_less_than_sign;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_OPEN)]                  = myhvml_tokenizer_end_state_script_data_end_tag_open;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_NAME)]                  = myhvml_tokenizer_end_state_script_data_end_tag_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START)]                  = myhvml_tokenizer_end_state_script_data_escape_start;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START_DASH)]             = myhvml_tokenizer_end_state_script_data_escape_start_dash;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED)]                       = myhvml_tokenizer_end_state_script_data_escaped;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH)]                  = myhvml_tokenizer_end_state_script_data_escaped_dash;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH)]             = myhvml_tokenizer_end_state_script_data_escaped_dash_dash;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN)]        = myhvml_tokenizer_end_state_script_data_escaped_less_than_sign;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_OPEN)]          = myhvml_tokenizer_end_state_script_data_escaped_end_tag_open;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_NAME)]          = myhvml_tokenizer_end_state_script_data_escaped_end_tag_name;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_START)]           = myhvml_tokenizer_end_state_script_data_double_escape_start;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED)]                = myhvml_tokenizer_end_state_script_data_double_escaped;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH)]           = myhvml_tokenizer_end_state_script_data_double_escaped_dash;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH)]      = myhvml_tokenizer_end_state_script_data_double_escaped_dash_dash;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN)] = myhvml_tokenizer_end_state_script_data_double_escaped_less_than_sign;
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_END)]             = myhvml_tokenizer_end_state_script_data_double_escape_end;
    
    // parse error
    myhvml->parse_state_func[(MyHVML_TOKENIZER_STATE_LAST_ENTRY
                              + MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP)]                          = myhvml_tokenizer_end_state_parse_error_stop;
    
    return MyHVML_STATUS_OK;
}

void myhvml_tokenizer_state_destroy(myhvml_t* myhvml)
{
    if(myhvml->parse_state_func)
        mycore_free(myhvml->parse_state_func);
}



