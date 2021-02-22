/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
** Copyright (C) 2015-2017 Alexander Borisov
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
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#include "tokenizer_script.h"


size_t myhvml_tokenizer_state_script_data(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while (hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '<') {
            token_node->element_begin = (tree->global_offset + hvml_offset);
            
            hvml_offset++;
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_LESS_THAN_SIGN;
            
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '/')
    {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_OPEN;
    }
    else if(hvml[hvml_offset] == '!')
    {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_escape_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '-') {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START_DASH;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_escape_start_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '-') {
        hvml_offset++;
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(myhvml_ascii_char_cmp(hvml[hvml_offset])) {
        token_node->str.length = (hvml_offset + tree->global_offset);
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_NAME;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(myhvml_whithspace(hvml[hvml_offset], ==, ||))
        {
            if(((hvml_offset + tree->global_offset) - token_node->str.length) != 6) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
                hvml_offset++;
                break;
            }
            
            size_t tmp_size = token_node->str.length;
            const char *tem_name = myhvml_tree_incomming_buffer_make_data(tree, tmp_size, 6);
            
            /* TODO: use appropriate end tag token */
            if(mycore_strncasecmp(tem_name, "archetype", 9) == 0)
            {
                token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((hvml_offset + tree->global_offset) - 8), MyHVML_TOKEN_TYPE_SCRIPT);
                if(token_node == NULL) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                token_node->raw_begin = tmp_size;
                token_node->raw_length = 9;
                token_node->tag_id = MyHVML_TAG_ARCHETYPE;
                token_node->type = MyHVML_TOKEN_TYPE_CLOSE;
                
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
            }
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '/')
        {
            if(((hvml_offset + tree->global_offset) - token_node->str.length) != 6) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
                hvml_offset++;
                break;
            }
            
            size_t tmp_size = token_node->str.length;
            const char *tem_name = myhvml_tree_incomming_buffer_make_data(tree, tmp_size, 6);
            
            /* TODO: use appropriate end tag token */
            if(mycore_strncasecmp(tem_name, "archetype", 9) == 0)
            {
                token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((hvml_offset + tree->global_offset) - 8), MyHVML_TOKEN_TYPE_SCRIPT);
                if(token_node == NULL) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                token_node->raw_begin = tmp_size;
                token_node->raw_length = 9;
                token_node->tag_id = MyHVML_TAG_ARCHETYPE;
                token_node->type = MyHVML_TOKEN_TYPE_CLOSE|MyHVML_TOKEN_TYPE_CLOSE_SELF;
                
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
            }
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '>')
        {
            if(((hvml_offset + tree->global_offset) - token_node->str.length) != 6) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
                hvml_offset++;
                break;
            }
            
            size_t tmp_size = token_node->str.length;
            const char *tem_name = myhvml_tree_incomming_buffer_make_data(tree, tmp_size, 6);
            
            /* TODO: use appropriate end tag token */
            if(mycore_strncasecmp(tem_name, "archetype", 9) == 0)
            {
                token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((hvml_offset + tree->global_offset) - 8), MyHVML_TOKEN_TYPE_SCRIPT);
                if(token_node == NULL) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                token_node->raw_begin = tmp_size;
                token_node->raw_length = 9;
                token_node->tag_id = MyHVML_TAG_ARCHETYPE;
                token_node->type  = MyHVML_TOKEN_TYPE_CLOSE;
                
                hvml_offset++;
                
                token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
                
                if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
                hvml_offset++;
            }
            
            break;
        }
        else if(myhvml_ascii_char_unless_cmp(hvml[hvml_offset]))
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_escaped_dash_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '-') {
        hvml_offset++;
        return hvml_offset;
    }
    
    if(hvml[hvml_offset] == '<') {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN;
    }
    else if(hvml[hvml_offset] == '>') {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
    }
    
    hvml_offset++;
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_escaped_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '/') {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_OPEN;
        hvml_offset++;
    }
    else if(myhvml_ascii_char_cmp(hvml[hvml_offset])) {
        token_node->str.length = (hvml_offset + tree->global_offset);
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_START;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_escaped_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(myhvml_ascii_char_cmp(hvml[hvml_offset])) {
        token_node->str.length = (hvml_offset + tree->global_offset);
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_NAME;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_escaped_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(myhvml_whithspace(hvml[hvml_offset], ==, ||))
        {
            if(((hvml_offset + tree->global_offset) - token_node->str.length) != 6) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
                hvml_offset++;
                break;
            }
            
            size_t tmp_size = token_node->str.length;
            const char *tem_name = myhvml_tree_incomming_buffer_make_data(tree, tmp_size, 6);
            
            /* TODO: use appropriate end tag token */
            if(mycore_strncasecmp(tem_name, "archetype", 9) == 0)
            {
                token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((hvml_offset + tree->global_offset) - 8), MyHVML_TOKEN_TYPE_SCRIPT);
                if(token_node == NULL) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                token_node->raw_begin = tmp_size;
                token_node->raw_length = 9;
                token_node->tag_id = MyHVML_TAG_ARCHETYPE;
                token_node->type = MyHVML_TOKEN_TYPE_CLOSE;
                
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
            }
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '/')
        {
            if(((hvml_offset + tree->global_offset) - token_node->str.length) != 6) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
                hvml_offset++;
                break;
            }
            
            size_t tmp_size = token_node->str.length;
            const char *tem_name = myhvml_tree_incomming_buffer_make_data(tree, tmp_size, 6);
            
            /* TODO: use appropriate end tag token */
            if(mycore_strncasecmp(tem_name, "archetype", 9) == 0)
            {
                token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((hvml_offset + tree->global_offset) - 8), MyHVML_TOKEN_TYPE_SCRIPT);
                if(token_node == NULL) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                token_node->raw_begin = tmp_size;
                token_node->raw_length = 9;
                token_node->tag_id = MyHVML_TAG_ARCHETYPE;
                token_node->type = MyHVML_TOKEN_TYPE_CLOSE|MyHVML_TOKEN_TYPE_CLOSE_SELF;
                
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
            }
            
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '>')
        {
            if(((hvml_offset + tree->global_offset) - token_node->str.length) != 6) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
                hvml_offset++;
                break;
            }
            
            size_t tmp_size = token_node->str.length;
            const char *tem_name = myhvml_tree_incomming_buffer_make_data(tree, tmp_size, 6);
            
            /* TODO: use appropriate end tag token */
            if(mycore_strncasecmp(tem_name, "archetype", 9) == 0)
            {
                token_node = myhvml_tokenizer_queue_create_text_node_if_need(tree, token_node, hvml, ((hvml_offset + tree->global_offset) - 8), MyHVML_TOKEN_TYPE_SCRIPT);
                if(token_node == NULL) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
                
                token_node->raw_begin = tmp_size;
                token_node->raw_length = 9;
                token_node->tag_id = MyHVML_TAG_ARCHETYPE;
                token_node->type = MyHVML_TOKEN_TYPE_CLOSE;
                
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_DATA;
                
                hvml_offset++;
                
                token_node->element_length = (tree->global_offset + hvml_offset) - token_node->element_begin;
                
                if(myhvml_queue_add(tree, hvml_offset, token_node) != MyHVML_STATUS_OK) {
                    myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP;
                    return 0;
                }
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
                hvml_offset++;
            }
            break;
        }
        else if(myhvml_ascii_char_unless_cmp(hvml[hvml_offset]))
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_escaped(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '-')
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH;
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '<')
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN;
            hvml_offset++;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_escaped_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '-') {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH;
        hvml_offset++;
    }
    else if(hvml[hvml_offset] == '<') {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN;
    }
    else if(hvml[hvml_offset] == '\0') {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_double_escape_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(myhvml_whithspace(hvml[hvml_offset], ==, ||) || hvml[hvml_offset] == '/' || hvml[hvml_offset] == '>')
        {
            if(((hvml_offset + tree->global_offset) - token_node->str.length) != 6) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
                hvml_offset++;
                break;
            }
            
            size_t tmp_size = token_node->str.length;
            const char *tem_name = myhvml_tree_incomming_buffer_make_data(tree, tmp_size, 6);
            
            if(mycore_strncasecmp(tem_name, "script", 6) == 0) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
            }
            
            hvml_offset++;
            break;
        }
        else if(myhvml_ascii_char_unless_cmp(hvml[hvml_offset]))
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_double_escaped(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(hvml[hvml_offset] == '-')
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH;
            hvml_offset++;
            break;
        }
        else if(hvml[hvml_offset] == '<')
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN;
            hvml_offset++;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_double_escaped_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '-')
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH;
    }
    else if(hvml[hvml_offset] == '<')
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
    }
    
    hvml_offset++;
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_double_escaped_dash_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '-') {
        hvml_offset++;
        return hvml_offset;
    }
    
    if(hvml[hvml_offset] == '<')
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN;
    }
    else if(hvml[hvml_offset] == '>')
    {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
    }
    
    hvml_offset++;
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_double_escaped_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    if(hvml[hvml_offset] == '/') {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_END;
        hvml_offset++;
        
        token_node->str.length = (hvml_offset + tree->global_offset);
    }
    else {
        myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
    }
    
    return hvml_offset;
}

size_t myhvml_tokenizer_state_script_data_double_escape_end(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size)
{
    while(hvml_offset < hvml_size)
    {
        if(myhvml_whithspace(hvml[hvml_offset], ==, ||) || hvml[hvml_offset] == '/' || hvml[hvml_offset] == '>')
        {
            if(((hvml_offset + tree->global_offset) - token_node->str.length) != 6) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
                hvml_offset++;
                break;
            }
            
            size_t tmp_size = token_node->str.length;
            const char *tem_name = myhvml_tree_incomming_buffer_make_data(tree, tmp_size, 6);
            
            if(mycore_strncasecmp(tem_name, "script", 6) == 0) {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED;
            }
            else {
                myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
            }
            
            hvml_offset++;
            break;
        }
        else if(myhvml_ascii_char_unless_cmp(hvml[hvml_offset]))
        {
            myhvml_tokenizer_state_set(tree) = MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
            break;
        }
        
        hvml_offset++;
    }
    
    return hvml_offset;
}

