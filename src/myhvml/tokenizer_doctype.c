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
size_t
myhvml_tokenizer_state_doctype (
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_NAME;
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// BEFORE DOCTYPE NAME
//// <!DOCTYPE %HERE%hvml
/////////////////////////////////////////////////////////
size_t
myhvml_tokenizer_state_before_doctype_name (
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_parser_skip_whitespace()
    
    if(hvml_offset >= hvml_size)
        return hvml_offset;
    
    if(hvml[hvml_offset] == '>')
    {
        tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
        
        hvml_offset++;
        
        token_node->element_length =
            (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        tree->attr_current =
            myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
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
size_t myhvml_tokenizer_state_doctype_name(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while (hvml_offset < hvml_size) {
        if (hvml[hvml_offset] == '>') {
            tree->attr_current->raw_key_length =
                (hvml_offset + tree->global_offset) - tree->attr_current->raw_key_begin;
            
            hvml_offset++;
            
            token_node->element_length =
                (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if (myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            tree->attr_current =
                myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if (tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) =
                    MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
            
            break;
        }
        else if (myhvml_whithspace(hvml[hvml_offset], ==, ||)) {
            tree->attr_current->raw_key_length =
                (hvml_offset + tree->global_offset) - tree->attr_current->raw_key_begin;
            
            tree->attr_current =
                myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if (tree->attr_current == NULL) {
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
size_t myhvml_tokenizer_state_after_doctype_name(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_parser_skip_whitespace()
    
    if (hvml_offset >= hvml_size)
        return hvml_offset;
    
    if (hvml[hvml_offset] == '>') {
        hvml_offset++;
        
        token_node->element_length =
            (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if (myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
        return hvml_offset;
    }
    
    /* temporarily */
    token_node->str.length = (hvml_offset + tree->global_offset);
    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_CUSTOM_AFTER_DOCTYPE_NAME_A_Z;
    return hvml_offset;
}

size_t myhvml_tokenizer_state_custom_after_doctype_name_a_z(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if ((token_node->str.length + 6) > (hvml_size + tree->global_offset)) {
        return hvml_size;
    }
    
    const char *param =
        myhvml_tree_incomming_buffer_make_data(tree, token_node->str.length, 6);
    
    if (mycore_strncasecmp(param, "PREFIX", 6) == 0) {
        myhvml_parser_queue_set_attr(tree, token_node);
        
        tree->attr_current->raw_value_begin  = token_node->str.length;
        tree->attr_current->raw_value_length = 6;
        
        tree->attr_current =
            myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
        if(tree->attr_current == NULL) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) =
            MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_PREFIX_IDENTIFIER;
        
        hvml_offset = (token_node->str.length + 6) - tree->incoming_buf->offset;
    }
    else if (mycore_strncasecmp(param, "TARGET", 6) == 0) {
        myhvml_parser_queue_set_attr(tree, token_node);
        
        tree->attr_current->raw_value_begin  = token_node->str.length;
        tree->attr_current->raw_value_length = 6;
        
        tree->attr_current =
            myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
        if(tree->attr_current == NULL) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) =
            MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_PREFIX_IDENTIFIER;
        
        hvml_offset = (token_node->str.length + 6) - tree->incoming_buf->offset;
    }
    else if (mycore_strncasecmp(param, "SYSTEM", 6) == 0) {
        myhvml_parser_queue_set_attr(tree, token_node);
        
        tree->attr_current->raw_value_begin  = token_node->str.length;
        tree->attr_current->raw_value_length = 6;
        
        tree->attr_current =
            myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
        if(tree->attr_current == NULL) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) =
            MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_PREFIX_IDENTIFIER;
        
        hvml_offset = (token_node->str.length + 6) - tree->incoming_buf->offset;
    }
    else {
        tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// BEFORE DOCTYPE PREFIX IDENTIFIER
//// <!DOCTYPE hvml PREFIX %HERE%"
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_before_doctype_prefix_identifier(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_parser_skip_whitespace()
    
    if (hvml_offset >= hvml_size)
        return hvml_offset;
    
    if (hvml[hvml_offset] == '"') {
        tree->attr_current->raw_value_begin  = (hvml_offset + tree->global_offset) + 1;
        tree->attr_current->raw_value_length = 0;
        
        myhvml_tokenizer_state_set(tree) =
            MyHVML_TOKENIZER_STATE_DOCTYPE_PREFIX_IDENTIFIER_DOUBLE_QUOTED;
    }
    else if (hvml[hvml_offset] == '\'') {
        tree->attr_current->raw_value_begin  = (hvml_offset + tree->global_offset) + 1;
        tree->attr_current->raw_value_length = 0;
        
        myhvml_tokenizer_state_set(tree) =
            MyHVML_TOKENIZER_STATE_DOCTYPE_PREFIX_IDENTIFIER_SINGLE_QUOTED;
    }
    else if (hvml[hvml_offset] == '>') {
        tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
        
        hvml_offset++;
        
        token_node->element_length =
            (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
        return hvml_offset;
    }
    else {
        tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE;
    }
    
    return (hvml_offset + 1);
}

/////////////////////////////////////////////////////////
//// DOCTYPE PREFIX IDENTIFIER DOUBLE or SINGLE QUOTED
//// <!DOCTYPE hvml PREFIX %HERE%"
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_doctype_prefix_identifier_dsq(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size, char quote)
{
    while (hvml_offset < hvml_size) {
        if(hvml[hvml_offset] == quote) {
            tree->attr_current->raw_value_length =
                (hvml_offset + tree->global_offset) - tree->attr_current->raw_value_begin;
            
            myhvml_parser_queue_set_attr(tree, token_node);
            
            tree->attr_current =
                myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if (tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_tokenizer_state_set(tree) =
                MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_PREFIX_IDENTIFIER;
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '>') {
            tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
            
            if(tree->attr_current->raw_value_begin < (hvml_offset + tree->global_offset)) {
                tree->attr_current->raw_value_length =
                    (hvml_offset + tree->global_offset) -
                    tree->attr_current->raw_value_begin;
                
                myhvml_parser_queue_set_attr(tree, token_node);
                
                tree->attr_current =
                    myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
                if (tree->attr_current == NULL) {
                    myhvml_tokenizer_state_set(tree) =
                        MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
            }
            
            hvml_offset++;
            
            token_node->element_length =
                (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if (myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
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

size_t myhvml_tokenizer_state_doctype_prefix_identifier_double_quoted(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    return myhvml_tokenizer_doctype_prefix_identifier_dsq(tree, token_node,
            hvml, hvml_offset, hvml_size, '"');
}

size_t myhvml_tokenizer_state_doctype_prefix_identifier_single_quoted(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    return myhvml_tokenizer_doctype_prefix_identifier_dsq(tree, token_node,
            hvml, hvml_offset, hvml_size, '\'');
}

/////////////////////////////////////////////////////////
//// AFTER DOCTYPE PREFIX IDENTIFIER
//// <!DOCTYPE hvml PREFIX "blah-blah-blah"%HERE%"
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_after_doctype_prefix_identifier(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_parser_skip_whitespace()
    
    if (hvml_offset >= hvml_size)
        return hvml_offset;
    
    if (hvml[hvml_offset] == '"') {
        tree->attr_current->raw_value_begin  = (hvml_offset + tree->global_offset) + 1;
        tree->attr_current->raw_value_length = 0;
        
        myhvml_tokenizer_state_set(tree) =
            MyHVML_TOKENIZER_STATE_DOCTYPE_PREFIX_IDENTIFIER_DOUBLE_QUOTED;
    }
    else if (hvml[hvml_offset] == '\'') {
        tree->attr_current->raw_value_begin  = (hvml_offset + tree->global_offset) + 1;
        tree->attr_current->raw_value_length = 0;
        
        myhvml_tokenizer_state_set(tree) =
            MyHVML_TOKENIZER_STATE_DOCTYPE_PREFIX_IDENTIFIER_SINGLE_QUOTED;
    }
    else if (hvml[hvml_offset] == '>') {
        hvml_offset++;
        
        token_node->element_length =
            (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if (myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
        return hvml_offset;
    }
    else {
        tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE;
        return hvml_offset;
    }
    
    hvml_offset++;
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// DOCTYPE TARGET IDENTIFIER DOUBLE or SINGLE QUOTED
//// <!DOCTYPE hvml PREFIX "v:" TARGET %HERE%"
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_doctype_target_identifier_dsq(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size, char quote)
{
    while (hvml_offset < hvml_size) {
        if (hvml[hvml_offset] == quote) {
            tree->attr_current->raw_value_length =
                (hvml_offset + tree->global_offset) - tree->attr_current->raw_value_begin;
            
            myhvml_parser_queue_set_attr(tree, token_node);
            
            tree->attr_current =
                myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if(tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_tokenizer_state_set(tree) =
                MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_TARGET_IDENTIFIER;
            
            hvml_offset++;
            break;
        }
        else if (hvml[hvml_offset] == '>') {
            tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
            
            if(tree->attr_current->raw_value_begin < (hvml_offset + tree->global_offset)) {
                tree->attr_current->raw_value_length =
                    (hvml_offset + tree->global_offset) -
                    tree->attr_current->raw_value_begin;
                
                myhvml_parser_queue_set_attr(tree, token_node);
                
                tree->attr_current =
                    myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
                if (tree->attr_current == NULL) {
                    myhvml_tokenizer_state_set(tree) =
                        MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
            }
            
            hvml_offset++;
            
            token_node->element_length =
                (tree->global_offset + hvml_offset) - token_node->element_begin;
            
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

size_t myhvml_tokenizer_state_doctype_target_identifier_double_quoted(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    return myhvml_tokenizer_doctype_target_identifier_dsq(tree, token_node,
            hvml, hvml_offset, hvml_size, '"');
}

size_t myhvml_tokenizer_state_doctype_target_identifier_single_quoted(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    return myhvml_tokenizer_doctype_target_identifier_dsq(tree, token_node,
            hvml, hvml_offset, hvml_size, '\'');
}

/////////////////////////////////////////////////////////
//// AFTER DOCTYPE TARGET IDENTIFIER
//// <!DOCTYPE hvml TARGET "blah-blah-blah"%HERE%"
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_after_doctype_target_identifier(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_parser_skip_whitespace();
    
    if (hvml_offset >= hvml_size)
        return hvml_offset;
    
    if (hvml[hvml_offset] == '>') {
        hvml_offset++;
        
        token_node->element_length =
            (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if (myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// DOCTYPE SYSTEM IDENTIFIER DOUBLE or SINGLE QUOTED
//// <!DOCTYPE hvml PREFIX "v:" TARGET "python" SYSTEM %HERE%"
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_doctype_system_identifier_dsq (
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size, char quote)
{
    while (hvml_offset < hvml_size) {
        if (hvml[hvml_offset] == quote) {
            tree->attr_current->raw_value_length =
                (hvml_offset + tree->global_offset) - tree->attr_current->raw_value_begin;
            
            myhvml_parser_queue_set_attr(tree, token_node);
            
            tree->attr_current =
                myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
            if (tree->attr_current == NULL) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                return 0;
            }
            
            myhvml_tokenizer_state_set(tree) =
                MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_SYSTEM_IDENTIFIER;
            
            hvml_offset++;
            break;
        }
        else if (hvml[hvml_offset] == '>') {
            tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
            
            if (tree->attr_current->raw_value_begin < (hvml_offset + tree->global_offset)) {
                tree->attr_current->raw_value_length =
                    (hvml_offset + tree->global_offset) -
                    tree->attr_current->raw_value_begin;
                
                myhvml_parser_queue_set_attr(tree, token_node);
                
                tree->attr_current =
                    myhvml_token_attr_create(tree->token, tree->token->mcasync_attr_id);
                if (tree->attr_current == NULL) {
                    myhvml_tokenizer_state_set(tree) =
                        MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
            }
            
            hvml_offset++;
            
            token_node->element_length =
                (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if (myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
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

size_t myhvml_tokenizer_state_doctype_system_identifier_double_quoted(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    return myhvml_tokenizer_doctype_system_identifier_dsq(tree, token_node,
            hvml, hvml_offset, hvml_size, '"');
}

size_t myhvml_tokenizer_state_doctype_system_identifier_single_quoted(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    return myhvml_tokenizer_doctype_system_identifier_dsq(tree, token_node,
            hvml, hvml_offset, hvml_size, '\'');
}

/////////////////////////////////////////////////////////
//// AFTER DOCTYPE SYSTEM IDENTIFIER
//// <!DOCTYPE hvml PREFIX "v:" TARGET "python" SYSTEM "blah-blah-blah"%HERE%"
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_after_doctype_system_identifier(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    myhvml_parser_skip_whitespace();
    
    if (hvml_offset >= hvml_size)
        return hvml_offset;
    
    if (hvml[hvml_offset] == '>') {
        hvml_offset++;
        
        token_node->element_length =
            (tree->global_offset + hvml_offset) - token_node->element_begin;
        
        if (myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
            return 0;
        }
        
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE;
    }
    
    return hvml_offset;
}

/////////////////////////////////////////////////////////
//// BOGUS DOCTYPE
//// find >
/////////////////////////////////////////////////////////
size_t myhvml_tokenizer_state_bogus_doctype(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while (hvml_offset < hvml_size) {
        if(hvml[hvml_offset] == '>') {
            hvml_offset++;
            
            token_node->element_length =
                (tree->global_offset + hvml_offset) - token_node->element_begin;
            
            if (myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
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


