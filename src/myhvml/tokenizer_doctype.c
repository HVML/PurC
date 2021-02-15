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

#include "tokenizer_doctype.h"

/////////////////////////////////////////////////////////
//// DOCTYPE
//// <!DOCTYPE%HERE%
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    //myhvml_t* myhvml = tree->myhvml;
    
    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_NAME;
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// BEFORE DOCTYPE NAME
//// <!DOCTYPE %HERE%hvml
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_before_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_parser_skip_whitespace()
    
    if(hvml_offset >= hvml_size)
        return hvml_offset;
    
    if(hvml[hvml_offset] == '>')
    {
        tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
        
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
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    }
    else {
        myhvml_parser_queue_set_attr(tree, token_node);
        tree->attr_current->raw_key_begin = (hvml_offset + tree->global_offset);
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DOCTYPE_NAME;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// DOCTYPE NAME
//// <!DOCTYPE %HERE%hvml
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '>')
        {
            tree->attr_current->raw_key_length = (hvml_offset + tree->global_offset) - tree->attr_current->raw_key_begin;
            
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
            
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
            
            break;
        }
        else if(myhvml_whithspace(hvml[hvml_offset], ==, ||))
        {
            tree->attr_current->raw_key_length = (hvml_offset + tree->global_offset) - tree->attr_current->raw_key_begin;
            
            tree->attr_current = myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_NAME;
            
            hvml_offset++;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// AFTER DOCTYPE NAME
//// <!DOCTYPE hvml%HERE%
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_after_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_parser_skip_whitespace()
    
    if(hvml_offset >= hvml_size)
        return hvml_offset;
    
    if(hvml[hvml_offset] == '>') {
        hvml_offset++;
        
        token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
        return hvml_offset;
    }
    
    token_node->str.length = (hvml_offset + tree->global_offset);
    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE;
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// BOGUS DOCTYPE
//// find >
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_bogus_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '>')
        {
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


