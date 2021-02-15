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

#include "tokenizer_end.h"


size_t myhvml_tokenizer_end_state_data(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_size + tree->global_offset), MyHVML_TOKEN_TYPE_DATA);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(token_node->raw_begin < (hvml_size + tree->global_offset)) {
        if(token_node->raw_begin) {
            token_node->raw_length = (hvml_offset + tree->global_offset) - token_node->raw_begin;
            myhvml_check_tag_parser(tree, token_node, hvml, hvml_offset);
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
        }
        else {
            token_node->type ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
            myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_size + tree->global_offset), MyHVML_TOKEN_TYPE_DATA);
        }
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_size + tree->global_offset), MyHVML_TOKEN_TYPE_DATA);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(token_node->raw_begin < (hvml_size + tree->global_offset))
    {
        token_node->raw_length = (hvml_offset + tree->global_offset) - token_node->raw_begin;
        token_node->type ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
        myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_size + tree->global_offset), MyHVML_TOKEN_TYPE_DATA);
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_self_closing_start_tag(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_size + tree->global_offset), MyHVML_TOKEN_TYPE_DATA);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_markup_declaration_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(token_node->raw_begin > 1) {
        tree->incoming_buf->length = myhvml_tokenizer_state_bogus_comment(tree, token_node, hvml, token_node->raw_begin, hvml_size);
        
        if(token_node != tree->current_token_node)
        {
            token_node = tree->current_token_node;
            token_node->raw_length = (hvml_size + tree->global_offset) - token_node->raw_begin;
            
            if(token_node->raw_length)
            {
                token_node->type       ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
                token_node->tag_id = MyHVML_TAG__TEXT;
                token_node->type       |= MyHVML_TOKEN_TYPE_DATA;
                
                token_node->raw_length = (hvml_size + tree->global_offset) - token_node->raw_begin;
                
                if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
            }
        }
        else {
            token_node->type       ^= (token_node->type & MyHVML_TOKEN_TYPE_WHITESPACE);
            token_node->tag_id = MyHVML_TAG__COMMENT;
            token_node->type       |= MyHVML_TOKEN_TYPE_COMMENT;
            
            token_node->raw_length = (hvml_size + tree->global_offset) - token_node->raw_begin;
            
            if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
        }
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_before_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    tree->attr_current->raw_key_length = (hvml_offset + tree->global_offset) - tree->attr_current->raw_key_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_after_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_before_attribute_value(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
    if(tree->attr_current == NULL) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_attribute_value_double_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_attribute_value_single_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_attribute_value_unquoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    tree->attr_current->raw_value_length = (hvml_offset + tree->global_offset) - tree->attr_current->raw_value_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
    if(tree->attr_current == NULL) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_after_attribute_value_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_comment_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_comment_start_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_comment(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_comment_end(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(token_node->raw_length > 2) {
        token_node->raw_length -= 2;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_comment_end_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_comment_end_bang(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_bogus_comment(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_cdata_section(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    token_node->raw_length = ((hvml_offset + tree->global_offset) - token_node->raw_begin);
    
    if(token_node->raw_length) {
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_rcdata(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(token_node->raw_begin < (hvml_size + tree->global_offset)) {
        token_node->type |= MyHVML_TOKEN_TYPE_RCDATA;
        token_node->tag_id = MyHVML_TAG__TEXT;
        token_node->raw_length = (hvml_size + tree->global_offset) - token_node->raw_begin;
        
        if(myhvml_queue_add(tree, 0, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_rcdata_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_RCDATA);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_rcdata_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_RCDATA);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_rcdata_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_RCDATA);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_rawtext(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_RAWTEXT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_rawtext_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_RAWTEXT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_rawtext_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_RAWTEXT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_rawtext_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_RAWTEXT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_plaintext(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    // all we need inside myhvml_tokenizer_state_plaintext
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_RAWTEXT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_before_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    tree->attr_current->raw_key_length = (hvml_offset + tree->global_offset) - tree->attr_current->raw_key_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_after_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_custom_after_doctype_name_a_z(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_before_doctype_public_identifier(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_doctype_public_identifier_double_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
    
    if(tree->attr_current->raw_key_begin && hvml_size) {
        tree->attr_current->raw_key_length = (hvml_offset + tree->global_offset) - tree->attr_current->raw_key_begin;
    }
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_doctype_public_identifier_single_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_end_state_doctype_public_identifier_double_quoted(tree, token_node, hvml, (hvml_offset + tree->global_offset), hvml_size);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_after_doctype_public_identifier(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_doctype_system_identifier_double_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
    
    if(tree->attr_current->raw_key_begin && hvml_size) {
        tree->attr_current->raw_key_length = (hvml_offset + tree->global_offset) - tree->attr_current->raw_key_begin;
    }
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_doctype_system_identifier_single_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_end_state_doctype_system_identifier_double_quoted(tree, token_node, hvml, (hvml_offset + tree->global_offset), hvml_size);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_after_doctype_system_identifier(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_bogus_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
        return 0;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_escape_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_escape_start_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_escaped(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_escaped_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_escaped_dash_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_escaped_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_escaped_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_escaped_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_double_escape_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_double_escaped(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_double_escaped_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_double_escaped_dash_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_double_escaped_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_script_data_double_escape_end(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    token_node->element_length = (tree->global_offset + hvml_size) - token_node->element_begin;
    
    myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, (hvml_offset + tree->global_offset), MyHVML_TOKEN_TYPE_SCRIPT);
    return hvml_offset;
}

size_t myhvml_tokenizer_end_state_parse_error_stop(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    return hvml_size;
}
