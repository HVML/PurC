/*
** Copyright (C) 2015-2017 Alexander Borisov
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
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#include "rules.h"

/* TODO */
bool myhvml_insertion_mode_in_body(myhvml_tree_t* tree, myhvml_token_node_t* token);

void myhvml_insertion_fix_emit_for_text_begin_ws(myhvml_token_t* token, myhvml_token_node_t* node)
{
    myhvml_token_node_wait_for_done(token, node);
    mycore_string_crop_whitespace_from_begin(&node->str);
}

myhvml_token_node_t * myhvml_insertion_fix_split_for_text_begin_ws(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    myhvml_token_node_wait_for_done(tree->token, token);
    size_t len = mycore_string_whitespace_from_begin(&token->str);
    
    if(len == 0)
        return NULL;
    
    // create new ws token and insert
    myhvml_token_node_t* new_token = myhvml_token_node_create(tree->token, tree->mcasync_rules_token_id);
    
    if(new_token == NULL)
        return NULL;
    
    mycore_string_init(tree->mchar, tree->mchar_node_id, &new_token->str, (len + 2));
    
    mycore_string_append(&new_token->str, token->str.data, len);
    
    new_token->type |= MyHVML_TOKEN_TYPE_DONE;
    
    // and cut ws for original
    token->str.data    = mchar_async_crop_first_chars_without_cache(token->str.data, len);
    token->str.length -= len;
    
    return new_token;
}

void myhvml_insertion_fix_for_null_char_drop_all(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    myhvml_token_node_wait_for_done(tree->token, token);
    
    mycore_string_t *str = &token->str;
    size_t len = str->length;
    size_t offset = 0;
    
    for (size_t i = 0; i < len; ++i)
    {
        if (str->data[i] == '\0')
        {
            size_t next_non_null = i;
            while ((next_non_null < len) && str->data[next_non_null] == '\0') {++next_non_null;}
            
            str->length = str->length - (next_non_null - i);
            
            size_t next_null = next_non_null;
            while ((next_null < len) && str->data[next_null] != '\0') {++next_null;}
            
            memmove(&str->data[(i - offset)], &str->data[next_non_null], (next_null - next_non_null));
            
            i = next_null - 1;
            
            offset++;
        }
    }
}

static bool myhvml_insertion_mode_initial(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    switch (token->tag_id)
    {
        case MyHVML_TAG__TEXT:
        {
            if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE) {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:INFO */
                return false;
            }
            
            myhvml_insertion_fix_emit_for_text_begin_ws(tree->token, token);
            
            // default, other token
            tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
            tree->insert_mode = MyHVML_INSERTION_MODE_BEFORE_HVML;
            break;
        }
            
        case MyHVML_TAG__COMMENT:
        {
            myhvml_tree_node_insert_comment(tree, token, tree->document);
            return false;
        }
            
        case MyHVML_TAG__DOCTYPE:
        {
            myhvml_token_node_wait_for_done(tree->token, token);
            
            myhvml_token_release_and_check_doctype_attributes(tree->token, token, &tree->doctype);
            
            if((tree->parse_flags & MyHVML_TREE_PARSE_FLAGS_WITHOUT_DOCTYPE_IN_TREE) == 0)
                myhvml_tree_node_insert_doctype(tree, token);
            
            // fix for tokenizer
            if(tree->doctype.is_hvml == false &&
               (tree->doctype.attr_public == NULL ||
               tree->doctype.attr_system == NULL))
            {
                tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
            }
            
            tree->insert_mode = MyHVML_INSERTION_MODE_BEFORE_HVML;
            return false;
        }
            
        default:
            tree->compat_mode = MyHVML_TREE_COMPAT_MODE_QUIRKS;
            tree->insert_mode = MyHVML_INSERTION_MODE_BEFORE_HVML;
            break;
    }
    
    return true;
}

static bool myhvml_insertion_mode_before_hvml(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_HVML:
            case MyHVML_TAG_HEAD:
            case MyHVML_TAG_BODY:
            {
                myhvml_tree_node_insert_root(tree, NULL, MyHVML_NAMESPACE_HVML);
                
                /* %EXTERNAL% VALIDATOR:RULES TAG STATUS:ELEMENT_MISSING_NEED LEVEL:INFO TAG_ID:MyHVML_TAG_HVML NS:MyHVML_NAMESPACE_HVML */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_BEFORE_HEAD;
                return true;
            }
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:WARNING */
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__DOCTYPE: {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:WARNING */
                break;
            }
                
            case MyHVML_TAG__COMMENT:
            {
                myhvml_tree_node_insert_comment(tree, token, tree->document);
                break;
            }
                
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE) {
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:INFO */
                    break;
                }
                
                myhvml_insertion_fix_emit_for_text_begin_ws(tree->token, token);
                
                // default, other token
                myhvml_tree_node_insert_root(tree, NULL, MyHVML_NAMESPACE_HVML);
                tree->insert_mode = MyHVML_INSERTION_MODE_BEFORE_HEAD;
                return true;
            }
                
            case MyHVML_TAG_HVML:
            {
                myhvml_tree_node_insert_root(tree, token, MyHVML_NAMESPACE_HVML);
                tree->insert_mode = MyHVML_INSERTION_MODE_BEFORE_HEAD;
                break;
            }
            
            default:
            {
                myhvml_tree_node_insert_root(tree, NULL, MyHVML_NAMESPACE_HVML);
                /* %EXTERNAL% VALIDATOR:RULES TAG STATUS:ELEMENT_MISSING_NEED LEVEL:INFO TAG_ID:MyHVML_TAG_HVML NS:MyHVML_NAMESPACE_HVML */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_BEFORE_HEAD;
                return true;
            }
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_before_head(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_HVML:
            case MyHVML_TAG_HEAD:
            case MyHVML_TAG_BODY:
            {
                tree->node_head = myhvml_tree_node_insert(tree, MyHVML_TAG_HEAD, MyHVML_NAMESPACE_HVML);
                /* %EXTERNAL% VALIDATOR:RULES TAG STATUS:ELEMENT_MISSING_NEED LEVEL:INFO TAG_ID:MyHVML_TAG_HEAD NS:MyHVML_NAMESPACE_HVML */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD;
                return true;
            }
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE) {
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:INFO */
                    break;
                }
                
                myhvml_insertion_fix_emit_for_text_begin_ws(tree->token, token);
                
                // default, other token
                tree->node_head = myhvml_tree_node_insert(tree, MyHVML_TAG_HEAD, MyHVML_NAMESPACE_HVML);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD;
                return true;
            }
                
            case MyHVML_TAG__COMMENT:
            {
                myhvml_tree_node_insert_comment(tree, token, 0);
                break;
            }
                
            case MyHVML_TAG__DOCTYPE: {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            case MyHVML_TAG_HVML:
            {
                return myhvml_insertion_mode_in_body(tree, token);
            }
                
            case MyHVML_TAG_HEAD:
            {
                tree->node_head = myhvml_tree_node_insert_hvml_element(tree, token);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD;
                break;
            }
                
            default:
            {
                tree->node_head = myhvml_tree_node_insert(tree, MyHVML_TAG_HEAD, MyHVML_NAMESPACE_HVML);
                /* %EXTERNAL% VALIDATOR:RULES TAG STATUS:ELEMENT_MISSING_NEED LEVEL:INFO TAG_ID:MyHVML_TAG_HEAD NS:MyHVML_NAMESPACE_HVML */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD;
                return true;
            }
        }
    }
    
    return false;
}

#if 0 /* TODO: VW */
static bool myhvml_insertion_mode_in_head(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_HEAD:
            {
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_HEAD;
                break;
            }
                
            case MyHVML_TAG_HVML:
            case MyHVML_TAG_BODY:
            {
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_HEAD;
                return true;
            }
                
            case MyHVML_TAG_TEMPLATE:
            {
                if(myhvml_tree_open_elements_find_by_tag_idx_reverse(tree, MyHVML_TAG_TEMPLATE, MyHVML_NAMESPACE_HVML, NULL) == NULL)
                {
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:WARNING */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_ANY NEED_TAG_ID:MyHVML_TAG_TEMPLATE NEED_NS:MyHVML_NAMESPACE_HVML */
                    
                    break;
                }
                
                // oh God...
                myhvml_tree_generate_all_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(current_node && current_node->tag_id != MyHVML_TAG_TEMPLATE) {
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED_CLOSE_BEFORE LEVEL:WARNING */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_TEMPLATE NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_TEMPLATE, MyHVML_NAMESPACE_HVML, false);
                myhvml_tree_active_formatting_up_to_last_marker(tree);
                myhvml_tree_template_insertion_pop(tree);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                break;
            }
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:WARNING */
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE)
                {
                    myhvml_tree_node_insert_text(tree, token);
                    break;
                }
                
                myhvml_token_node_t* new_token = myhvml_insertion_fix_split_for_text_begin_ws(tree, token);
                if(new_token)
                    myhvml_tree_node_insert_text(tree, new_token);
                
                // default, other token
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_HEAD;
                return true;
            }
                
            case MyHVML_TAG__COMMENT:
            {
                myhvml_tree_node_insert_comment(tree, token, 0);
                break;
            }
                
            case MyHVML_TAG__DOCTYPE: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            case MyHVML_TAG_HVML:
            {
                return myhvml_insertion_mode_in_body(tree, token);
            }
                
            case MyHVML_TAG_BASE:
            case MyHVML_TAG_BASEFONT:
            case MyHVML_TAG_BGSOUND:
            case MyHVML_TAG_LINK:
            {
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                break;
            }
                
            case MyHVML_TAG_META:
            {
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                
                // if the element has an http-equiv attribute
                break;
            }
                
            case MyHVML_TAG_TITLE:
            {
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->orig_insert_mode = tree->insert_mode;
                tree->insert_mode = MyHVML_INSERTION_MODE_TEXT;
                tree->state_of_builder = MyHVML_TOKENIZER_STATE_RCDATA;
                
                break;
            }
                
            case MyHVML_TAG_NOSCRIPT:
            {
                if(tree->flags & MyHVML_TREE_FLAGS_SCRIPT) {
                    myhvml_tree_node_insert_hvml_element(tree, token);
                    
                    tree->orig_insert_mode = tree->insert_mode;
                    tree->insert_mode = MyHVML_INSERTION_MODE_TEXT;
                    tree->state_of_builder = MyHVML_TOKENIZER_STATE_RAWTEXT;
                }
                else {
                    myhvml_tree_node_insert_hvml_element(tree, token);
                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD_NOSCRIPT;
                }
                
                break;
            }
                
            case MyHVML_TAG_STYLE:
            case MyHVML_TAG_NOFRAMES:
            {
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->orig_insert_mode = tree->insert_mode;
                tree->insert_mode = MyHVML_INSERTION_MODE_TEXT;
                tree->state_of_builder = MyHVML_TOKENIZER_STATE_RAWTEXT;
                
                break;
            }
                
            case MyHVML_TAG_SCRIPT:
            {
                // state 1
                enum myhvml_tree_insertion_mode insert_mode;
                myhvml_tree_node_t* adjusted_location = myhvml_tree_appropriate_place_inserting(tree, NULL, &insert_mode);
                
                // state 2
                myhvml_tree_node_t* node = myhvml_tree_node_create(tree);
                
                node->tag_id      = MyHVML_TAG_SCRIPT;
                node->token        = token;
                node->ns = MyHVML_NAMESPACE_HVML;
                node->flags        = MyHVML_TREE_NODE_PARSER_INSERTED|MyHVML_TREE_NODE_BLOCKING;
                
                myhvml_tree_node_insert_by_mode(adjusted_location, node, insert_mode);
                myhvml_tree_open_elements_append(tree, node);
                
                tree->orig_insert_mode = tree->insert_mode;
                tree->insert_mode = MyHVML_INSERTION_MODE_TEXT;
                tree->state_of_builder = MyHVML_TOKENIZER_STATE_SCRIPT_DATA;
                
                break;
            }
                
            case MyHVML_TAG_TEMPLATE:
            {
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_active_formatting_append(tree, tree->myhvml->marker); // set marker
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TEMPLATE;
                myhvml_tree_template_insertion_append(tree, MyHVML_INSERTION_MODE_IN_TEMPLATE);
                
                break;
            }
                
            case MyHVML_TAG_HEAD: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY LEVEL:WARNING */
                break;
            }
                
            default:
            {
                myhvml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_HEAD;
                return true;
            }
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_head_noscript(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_NOSCRIPT:
            {
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD;
                break;
            }
                
            case MyHVML_TAG_BR:
            {
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD;
                return true;
            }
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__DOCTYPE: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            case MyHVML_TAG_HVML:
            {
                return myhvml_insertion_mode_in_body(tree, token);
            }
                
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE)
                    return myhvml_insertion_mode_in_head(tree, token);
                
                // default, other token
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD;
                return true;
            }
                
            case MyHVML_TAG_BASEFONT:
            case MyHVML_TAG_BGSOUND:
            case MyHVML_TAG_LINK:
            case MyHVML_TAG_META:
            case MyHVML_TAG_NOFRAMES:
            case MyHVML_TAG_STYLE:
            case MyHVML_TAG__COMMENT:
                return myhvml_insertion_mode_in_head(tree, token);
                
            case MyHVML_TAG_HEAD:
            case MyHVML_TAG_NOSCRIPT: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY LEVEL:WARNING */
                break;
            }
                
            default:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD;
                return true;
            }
        }
    }
    
    return false;
}

bool myhvml_insertion_mode_after_head(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_BR:
            case MyHVML_TAG_HVML:
            case MyHVML_TAG_BODY:
            {
                tree->node_body = myhvml_tree_node_insert(tree, MyHVML_TAG_BODY, MyHVML_NAMESPACE_HVML);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                
                /* %EXTERNAL% VALIDATOR:RULES TAG STATUS:ELEMENT_MISSING_NEED LEVEL:INFO TAG_ID:MyHVML_TAG_BODY NS:MyHVML_NAMESPACE_HVML */
                return true;
            }
                
            case MyHVML_TAG_TEMPLATE:
            {
                return myhvml_insertion_mode_in_head(tree, token);
            }
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE)
                {
                    myhvml_tree_node_insert_text(tree, token);
                    break;
                }
                
                myhvml_token_node_t* new_token = myhvml_insertion_fix_split_for_text_begin_ws(tree, token);
                if(new_token)
                    myhvml_tree_node_insert_text(tree, new_token);
                
                // default, other token
                tree->node_body = myhvml_tree_node_insert(tree, MyHVML_TAG_BODY, MyHVML_NAMESPACE_HVML);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                return true;
            }
                
            case MyHVML_TAG__COMMENT:
                myhvml_tree_node_insert_comment(tree, token, 0);
                break;
                
            case MyHVML_TAG__DOCTYPE: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            case MyHVML_TAG_HVML:
                return myhvml_insertion_mode_in_body(tree, token);
                
            case MyHVML_TAG_BODY:
            {
                tree->node_body = myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                break;
            }
                
            case MyHVML_TAG_FRAMESET:
                myhvml_tree_node_insert_hvml_element(tree, token);
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_FRAMESET;
                break;
                
            case MyHVML_TAG_BASE:
            case MyHVML_TAG_BASEFONT:
            case MyHVML_TAG_BGSOUND:
            case MyHVML_TAG_LINK:
            case MyHVML_TAG_META:
            case MyHVML_TAG_NOFRAMES:
            case MyHVML_TAG_SCRIPT:
            case MyHVML_TAG_STYLE:
            case MyHVML_TAG_TEMPLATE:
            case MyHVML_TAG_TITLE:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_open_elements_append(tree, tree->node_head);
                myhvml_insertion_mode_in_head(tree, token);
                myhvml_tree_open_elements_remove(tree, tree->node_head);
            }
                
            case MyHVML_TAG_HEAD: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY LEVEL:WARNING */
                break;
            }
                
            default:
            {
                tree->node_body = myhvml_tree_node_insert(tree, MyHVML_TAG_BODY, MyHVML_NAMESPACE_HVML);
                /* %EXTERNAL% VALIDATOR:RULES TAG STATUS:ELEMENT_MISSING_NEED LEVEL:INFO TAG_ID:MyHVML_TAG_BODY NS:MyHVML_NAMESPACE_HVML */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                return true;
            }
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_body_other_end_tag(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    // step 1
    size_t i = tree->open_elements->length;
    while(i) {
        i--;
        
        myhvml_tree_node_t* node = tree->open_elements->list[i];
        
        // step 2
        if(node->tag_id == token->tag_id && node->ns == MyHVML_NAMESPACE_HVML) {
            myhvml_tree_generate_implied_end_tags(tree, token->tag_id, MyHVML_NAMESPACE_HVML);
            
            myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
            if(current_node->tag_id != node->tag_id) {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:node->tag_id NEED_NS:node->ns */
            }
            
            myhvml_tree_open_elements_pop_until_by_node(tree, node, false);
            
            return false;
        }
        
        const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, node->tag_id);
        if(tag_ctx->cats[ node->ns ] & MyHVML_TAG_CATEGORIES_SPECIAL) {
            // parse error
            /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
            break;
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_body(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_TEMPLATE:
            {
                return myhvml_insertion_mode_in_head(tree, token);
            }
                
            case MyHVML_TAG_BODY:
            {
                myhvml_tree_node_t* body_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_BODY, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE);
                
                if(body_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                for (size_t i = 0; i < tree->open_elements->length; i++) {
                    switch (tree->open_elements->list[i]->tag_id) {
                        case MyHVML_TAG_DD:
                        case MyHVML_TAG_DT:
                        case MyHVML_TAG_LI:
                        case MyHVML_TAG_MENUITEM:
                        case MyHVML_TAG_OPTGROUP:
                        case MyHVML_TAG_OPTION:
                        case MyHVML_TAG_P:
                        case MyHVML_TAG_RB:
                        case MyHVML_TAG_RP:
                        case MyHVML_TAG_RT:
                        case MyHVML_TAG_RTC:
                        case MyHVML_TAG_TBODY:
                        case MyHVML_TAG_TD:
                        case MyHVML_TAG_TFOOT:
                        case MyHVML_TAG_TH:
                        case MyHVML_TAG_THEAD:
                        case MyHVML_TAG_TR:
                        case MyHVML_TAG_BODY:
                        case MyHVML_TAG_HVML:
                            // set parse error
                            break;
                            
                        default:
                            break;
                    }
                }
                
                tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_BODY;
                break;
            }
                
            case MyHVML_TAG_HVML:
            {
                myhvml_tree_node_t* body_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_BODY, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE);
                
                if(body_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                for (size_t i = 0; i < tree->open_elements->length; i++) {
                    switch (tree->open_elements->list[i]->tag_id) {
                        case MyHVML_TAG_DD:
                        case MyHVML_TAG_DT:
                        case MyHVML_TAG_LI:
                        case MyHVML_TAG_MENUITEM:
                        case MyHVML_TAG_OPTGROUP:
                        case MyHVML_TAG_OPTION:
                        case MyHVML_TAG_P:
                        case MyHVML_TAG_RB:
                        case MyHVML_TAG_RP:
                        case MyHVML_TAG_RT:
                        case MyHVML_TAG_RTC:
                        case MyHVML_TAG_TBODY:
                        case MyHVML_TAG_TD:
                        case MyHVML_TAG_TFOOT:
                        case MyHVML_TAG_TH:
                        case MyHVML_TAG_THEAD:
                        case MyHVML_TAG_TR:
                        case MyHVML_TAG_BODY:
                        case MyHVML_TAG_HVML:
                            // set parse error
                            break;
                            
                        default:
                            break;
                    }
                }
                tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_BODY;
                
                return true;
            }
                
            case MyHVML_TAG_ADDRESS:
            case MyHVML_TAG_ARTICLE:
            case MyHVML_TAG_ASIDE:
            case MyHVML_TAG_BLOCKQUOTE:
            case MyHVML_TAG_BUTTON:
            case MyHVML_TAG_CENTER:
            case MyHVML_TAG_DETAILS:
            case MyHVML_TAG_DIALOG:
            case MyHVML_TAG_DIR:
            case MyHVML_TAG_DIV:
            case MyHVML_TAG_DL:
            case MyHVML_TAG_FIELDSET:
            case MyHVML_TAG_FIGCAPTION:
            case MyHVML_TAG_FIGURE:
            case MyHVML_TAG_FOOTER:
            case MyHVML_TAG_HEADER:
            case MyHVML_TAG_HGROUP:
            case MyHVML_TAG_LISTING:
            case MyHVML_TAG_MAIN:
            case MyHVML_TAG_MENU:
            case MyHVML_TAG_NAV:
            case MyHVML_TAG_OL:
            case MyHVML_TAG_PRE:
            case MyHVML_TAG_SECTION:
            case MyHVML_TAG_SUMMARY:
            case MyHVML_TAG_UL:
            {
                if(myhvml_tree_element_in_scope(tree, token->tag_id, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE) == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                // step 1
                myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                
                // step 2
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(myhvml_is_hvml_node(current_node, token->tag_id) == false) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:token->tag_id NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                // step 3
                myhvml_tree_open_elements_pop_until(tree, token->tag_id, MyHVML_NAMESPACE_HVML, false);
                break;
            }
                
            case MyHVML_TAG_FORM:
            {
                myhvml_tree_node_t* template_node = myhvml_tree_open_elements_find_by_tag_idx(tree, MyHVML_TAG_TEMPLATE, MyHVML_NAMESPACE_HVML, NULL);
                
                if(template_node == NULL)
                {
                    // step 1
                    myhvml_tree_node_t* node = tree->node_form;
                    
                    // step 2
                    tree->node_form = NULL;
                    
                    // step 3
                    if(node == NULL || myhvml_tree_element_in_scope_by_node(node, MyHVML_TAG_CATEGORIES_SCOPE) == false) {
                        // parse error
                        /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                        
                        break;
                    }
                    
                    // step 4
                    myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                    
                    // step 5
                    myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                    if(current_node != node) {
                        // parse error
                        /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                        /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:node->tag_id NEED_NS:node->ns */
                    }
                    
                    // step 6
                    myhvml_tree_open_elements_remove(tree, node);
                }
                else {
                    // step 1
                    myhvml_tree_node_t* form_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_FORM, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE);
                    
                    if(form_node == NULL) {
                        // parse error
                        /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                        
                        break;
                    }
                    
                    // step 2
                    myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                    
                    // step 3
                    myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                    if(myhvml_is_hvml_node(current_node, MyHVML_TAG_FORM) == false) {
                        // parse error
                        /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                        /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_FORM NEED_NS:MyHVML_NAMESPACE_HVML */
                    }
                    
                    // step 4
                    myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_FORM, MyHVML_NAMESPACE_HVML, false);
                }
                
                break;
            }
                
            case MyHVML_TAG_P:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON) == NULL) {
                    // parse error
                    myhvml_tree_node_insert(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML);
                }
                
                myhvml_tree_tags_close_p(tree, token);
                break;
            }
                
            case MyHVML_TAG_LI:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_LI, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_LIST_ITEM) == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                // step 1
                myhvml_tree_generate_implied_end_tags(tree, MyHVML_TAG_LI, MyHVML_NAMESPACE_HVML);
                
                // step 2
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_LI) == false) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_LI NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                // step 3
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_LI, MyHVML_NAMESPACE_HVML, false);
                
                break;
            }
               
            case MyHVML_TAG_DT:
            case MyHVML_TAG_DD:
            {
                if(myhvml_tree_element_in_scope(tree, token->tag_id, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE) == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                // step 1
                myhvml_tree_generate_implied_end_tags(tree, token->tag_id, MyHVML_NAMESPACE_HVML);
                
                // step 2
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(myhvml_is_hvml_node(current_node, token->tag_id) == false) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:token->tag_id NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                // step 3
                myhvml_tree_open_elements_pop_until(tree, token->tag_id, MyHVML_NAMESPACE_HVML, false);
                
                break;
            }
                
            case MyHVML_TAG_H1:
            case MyHVML_TAG_H2:
            case MyHVML_TAG_H3:
            case MyHVML_TAG_H4:
            case MyHVML_TAG_H5:
            case MyHVML_TAG_H6:
            {
                myhvml_tree_node_t** list = tree->open_elements->list;
                
                myhvml_tree_node_t* node = NULL;
                size_t i = tree->open_elements->length;
                while(i) {
                    i--;
                    
                    const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, list[i]->tag_id);
                    
                    if((list[i]->tag_id == MyHVML_TAG_H1 ||
                       list[i]->tag_id == MyHVML_TAG_H2  ||
                       list[i]->tag_id == MyHVML_TAG_H3  ||
                       list[i]->tag_id == MyHVML_TAG_H4  ||
                       list[i]->tag_id == MyHVML_TAG_H5  ||
                       list[i]->tag_id == MyHVML_TAG_H6) &&
                       list[i]->ns == MyHVML_NAMESPACE_HVML) {
                        node = list[i];
                        break;
                    }
                    else if(tag_ctx->cats[list[i]->ns] & MyHVML_TAG_CATEGORIES_SCOPE)
                        break;
                }
                
                if(node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                // step 1
                myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                
                // step 2
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(myhvml_is_hvml_node(current_node, token->tag_id) == false) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:token->tag_id NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                // step 3
                while(tree->open_elements->length) {
                    tree->open_elements->length--;
                    
                    if((list[tree->open_elements->length]->tag_id == MyHVML_TAG_H1 ||
                       list[tree->open_elements->length]->tag_id == MyHVML_TAG_H2 ||
                       list[tree->open_elements->length]->tag_id == MyHVML_TAG_H3 ||
                       list[tree->open_elements->length]->tag_id == MyHVML_TAG_H4 ||
                       list[tree->open_elements->length]->tag_id == MyHVML_TAG_H5 ||
                       list[tree->open_elements->length]->tag_id == MyHVML_TAG_H6) &&
                       list[tree->open_elements->length]->ns == MyHVML_NAMESPACE_HVML)
                    {
                        break;
                    }
                }
                
                break;
            }
                
            case MyHVML_TAG_A:
            case MyHVML_TAG_B:
            case MyHVML_TAG_BIG:
            case MyHVML_TAG_CODE:
            case MyHVML_TAG_EM:
            case MyHVML_TAG_FONT:
            case MyHVML_TAG_I:
            case MyHVML_TAG_NOBR:
            case MyHVML_TAG_S:
            case MyHVML_TAG_SMALL:
            case MyHVML_TAG_STRIKE:
            case MyHVML_TAG_STRONG:
            case MyHVML_TAG_TT:
            case MyHVML_TAG_U:
            {
                myhvml_tree_adoption_agency_algorithm(tree, token, token->tag_id);
                    //myhvml_insertion_mode_in_body_other_end_tag(tree, token);
                
                break;
            }
                
            case MyHVML_TAG_APPLET:
            case MyHVML_TAG_MARQUEE:
            case MyHVML_TAG_OBJECT:
            {
                if(myhvml_tree_element_in_scope(tree, token->tag_id, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE) == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                // step 1
                myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                
                // step 2
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(myhvml_is_hvml_node(current_node, token->tag_id) == false) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:token->tag_id NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                // step 3
                myhvml_tree_open_elements_pop_until(tree, token->tag_id, MyHVML_NAMESPACE_HVML, false);
                
                // step 4
                myhvml_tree_active_formatting_up_to_last_marker(tree);
                
                break;
            }
                
            case MyHVML_TAG_BR:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES CONVERT STATUS:ELEMENT_BAD LEVEL:ERROR FROM_TAG_ID:MyHVML_TAG_BR FROM_NS:MyHVML_NAMESPACE_HVML FROM_TYPE:MyHVML_TOKEN_TYPE_CLOSE TO_TAG_ID:MyHVML_TAG_BR TO_NS:MyHVML_NAMESPACE_HVML TO_TYPE:MyHVML_TOKEN_TYPE_OPEN */
                
                if(token->attr_first) {
                    token->attr_first = NULL;
                }
                
                if(token->attr_last) {
                    token->attr_last = NULL;
                }
                
                myhvml_tree_active_formatting_reconstruction(tree);
                
                token->type = MyHVML_TOKEN_TYPE_OPEN;
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                
                break;
            }
                
            default:
            {
                return myhvml_insertion_mode_in_body_other_end_tag(tree, token);
            }
        }
    }
    // open elements
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:NULL_CHAR ACTION:IGNORE LEVEL:ERROR */
                    
                    myhvml_insertion_fix_for_null_char_drop_all(tree, token);
                    
                    if(token->str.length) {
                        myhvml_tree_active_formatting_reconstruction(tree);
                        myhvml_tree_node_insert_text(tree, token);
                        
                        if((token->type & MyHVML_TOKEN_TYPE_WHITESPACE) == 0)
                            tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                    }
                }
                else {
                    myhvml_tree_active_formatting_reconstruction(tree);
                    myhvml_tree_node_insert_text(tree, token);
                    
                    if((token->type & MyHVML_TOKEN_TYPE_WHITESPACE) == 0)
                        tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                }
                
                break;
            }
                
            case MyHVML_TAG__COMMENT:
                myhvml_tree_node_insert_comment(tree, token, 0);
                break;
                
            case MyHVML_TAG__DOCTYPE: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:WARNING */
                break;
            }
                
            case MyHVML_TAG_HVML:
            {
                if(myhvml_tree_open_elements_find_by_tag_idx(tree, MyHVML_TAG_TEMPLATE, MyHVML_NAMESPACE_HVML, NULL)) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:WARNING */
                    break;
                }
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:WARNING */
                
                if(tree->open_elements->length > 0) {
                    myhvml_tree_node_t* top_node = tree->open_elements->list[0];
                    
                    if(top_node->token) {
                        myhvml_token_node_wait_for_done(tree->token, token);
                        myhvml_token_node_wait_for_done(tree->token, top_node->token);
                        myhvml_token_node_attr_copy_with_check(tree->token, token, top_node->token, tree->mcasync_rules_attr_id);
                    }
                    else {
                        top_node->token = token;
                    }
                }
                
                break;
            }
                
            case MyHVML_TAG_BASE:
            case MyHVML_TAG_BASEFONT:
            case MyHVML_TAG_BGSOUND:
            case MyHVML_TAG_LINK:
            case MyHVML_TAG_META:
            case MyHVML_TAG_NOFRAMES:
            case MyHVML_TAG_SCRIPT:
            case MyHVML_TAG_STYLE:
            case MyHVML_TAG_TEMPLATE:
            case MyHVML_TAG_TITLE:
            {
                return myhvml_insertion_mode_in_head(tree, token);
            }
                
            case MyHVML_TAG_BODY:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:WARNING */
                
                if(tree->open_elements->length > 1)
                {
                    if(!(tree->open_elements->list[1]->tag_id == MyHVML_TAG_BODY &&
                         tree->open_elements->list[1]->ns == MyHVML_NAMESPACE_HVML) ||
                       myhvml_tree_open_elements_find_by_tag_idx(tree, MyHVML_TAG_TEMPLATE, MyHVML_NAMESPACE_HVML, NULL))
                    {
                        // parse error
                        /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:WARNING */
                        
                        break;
                    }
                }
                else
                    break;
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                
                if(tree->open_elements->length > 1) {
                    myhvml_tree_node_t* top_node = tree->open_elements->list[1];
                    
                    if(top_node->token) {
                        myhvml_token_node_wait_for_done(tree->token, token);
                        myhvml_token_node_wait_for_done(tree->token, top_node->token);
                        myhvml_token_node_attr_copy_with_check(tree->token, token, top_node->token, tree->mcasync_rules_attr_id);
                    }
                    else {
                        top_node->token = token;
                    }
                }
                
                break;
            }
                
            case MyHVML_TAG_FRAMESET:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                if(tree->open_elements->length > 1)
                {
                    if(!(tree->open_elements->list[1]->tag_id == MyHVML_TAG_BODY &&
                         tree->open_elements->list[1]->ns == MyHVML_NAMESPACE_HVML))
                    {
                        // parse error
                        /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                        
                        break;
                    }
                }
                else
                    break;
                
                if((tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK) == 0) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                myhvml_tree_node_t* node = tree->open_elements->list[1];
                
                myhvml_tree_node_remove(node);
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_HVML, MyHVML_NAMESPACE_HVML, true);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_FRAMESET;
                break;
            }
                
            case MyHVML_TAG__END_OF_FILE:
            {
                if(tree->template_insertion->length)
                    return myhvml_insertion_mode_in_template(tree, token);
                
                myhvml_tree_node_t** list = tree->open_elements->list;
                for(size_t i = 0; i < tree->open_elements->length; i++) {
                    if(list[i]->tag_id != MyHVML_TAG_DD       && list[i]->tag_id != MyHVML_TAG_DT       &&
                       list[i]->tag_id != MyHVML_TAG_LI       && list[i]->tag_id != MyHVML_TAG_MENUITEM &&
                       list[i]->tag_id != MyHVML_TAG_OPTGROUP && list[i]->tag_id != MyHVML_TAG_OPTION   &&
                       list[i]->tag_id != MyHVML_TAG_P        && list[i]->tag_id != MyHVML_TAG_RB       &&
                       list[i]->tag_id != MyHVML_TAG_RP       && list[i]->tag_id != MyHVML_TAG_RT       &&
                       list[i]->tag_id != MyHVML_TAG_RTC      && list[i]->tag_id != MyHVML_TAG_TBODY    &&
                       list[i]->tag_id != MyHVML_TAG_TD       && list[i]->tag_id != MyHVML_TAG_TFOOT    &&
                       list[i]->tag_id != MyHVML_TAG_TH       && list[i]->tag_id != MyHVML_TAG_THEAD    &&
                       list[i]->tag_id != MyHVML_TAG_TR       && list[i]->tag_id != MyHVML_TAG_BODY     &&
                       list[i]->tag_id != MyHVML_TAG_HVML     && list[i]->ns != MyHVML_NAMESPACE_HVML)
                    {
                        // parse error
                    }
                }
                
                myhvml_rules_stop_parsing(tree);
                break;
            }
                
            case MyHVML_TAG_ADDRESS:
            case MyHVML_TAG_ARTICLE:
            case MyHVML_TAG_ASIDE:
            case MyHVML_TAG_BLOCKQUOTE:
            case MyHVML_TAG_CENTER:
            case MyHVML_TAG_DETAILS:
            case MyHVML_TAG_DIALOG:
            case MyHVML_TAG_DIR:
            case MyHVML_TAG_DIV:
            case MyHVML_TAG_DL:
            case MyHVML_TAG_FIELDSET:
            case MyHVML_TAG_FIGCAPTION:
            case MyHVML_TAG_FIGURE:
            case MyHVML_TAG_FOOTER:
            case MyHVML_TAG_HEADER:
            case MyHVML_TAG_HGROUP:
            case MyHVML_TAG_MAIN:
            case MyHVML_TAG_NAV:
            case MyHVML_TAG_OL:
            case MyHVML_TAG_P:
            case MyHVML_TAG_SECTION:
            case MyHVML_TAG_SUMMARY:
            case MyHVML_TAG_UL:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_MENU:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_MENUITEM))
                    myhvml_tree_open_elements_pop(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_H1:
            case MyHVML_TAG_H2:
            case MyHVML_TAG_H3:
            case MyHVML_TAG_H4:
            case MyHVML_TAG_H5:
            case MyHVML_TAG_H6:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                switch (current_node->tag_id) {
                    case MyHVML_TAG_H1:
                        case MyHVML_TAG_H2:
                        case MyHVML_TAG_H3:
                        case MyHVML_TAG_H4:
                        case MyHVML_TAG_H5:
                        case MyHVML_TAG_H6:
                        
                        if(current_node->ns == MyHVML_NAMESPACE_HVML) {
                            // parse error
                            /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:WARNING */
                            myhvml_tree_open_elements_pop(tree);
                        }
                        
                        break;
                        
                    default:
                        break;
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_PRE:
            case MyHVML_TAG_LISTING:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                // If the next token is a U+000A LINE FEED (LF) character token, then ignore that token and move on to the next one.
                // (Newlines at the start of pre blocks are ignored as an authoring convenience.)
                // !!! see dispatcher (myhvml_rules_tree_dispatcher) for this
                tree->flags |= MyHVML_TREE_FLAGS_PARSE_FLAG|MyHVML_TREE_FLAGS_PARSE_FLAG_EMIT_NEWLINE;
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
                
            case MyHVML_TAG_FORM:
            {
                myhvml_tree_node_t* is_in_node = myhvml_tree_open_elements_find_by_tag_idx(tree, MyHVML_TAG_TEMPLATE, MyHVML_NAMESPACE_HVML, NULL);
                if(tree->node_form && is_in_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_t* current = myhvml_tree_node_insert_hvml_element(tree, token);
                
                if(is_in_node == NULL)
                    tree->node_form = current;
                
                break;
            }
                
            case MyHVML_TAG_LI:
            {
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                
                size_t oel_index = tree->open_elements->length;
                
                while (oel_index) {
                    oel_index--;
                    
                    myhvml_tree_node_t* node = tree->open_elements->list[oel_index];
                    const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, node->tag_id);
                    
                    /* 3 */
                    if(myhvml_is_hvml_node(node, MyHVML_TAG_LI)) {
                        /* 3.1 */
                        myhvml_tree_generate_implied_end_tags(tree, MyHVML_TAG_LI, MyHVML_NAMESPACE_HVML);
                        
                        /* 3.2 */
                        myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                        if(myhvml_is_hvml_node(current_node, MyHVML_TAG_LI) == false) {
                            // parse error
                            /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                            /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_LI NEED_NS:MyHVML_NAMESPACE_HVML */
                        }
                        
                        /* 3.3 */
                        myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_LI, MyHVML_NAMESPACE_HVML, false);
                        break;
                    }
                    else if(tag_ctx->cats[node->ns] & MyHVML_TAG_CATEGORIES_SPECIAL)
                    {
                        if(!((node->tag_id == MyHVML_TAG_ADDRESS || node->tag_id == MyHVML_TAG_DIV ||
                             node->tag_id == MyHVML_TAG_P) && node->ns == MyHVML_NAMESPACE_HVML))
                                break;
                    }
                }
                
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
            
            case MyHVML_TAG_DT:
            case MyHVML_TAG_DD:
            {
                // this is copy/past
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                
                size_t oel_index = tree->open_elements->length;
                
                while (oel_index) {
                    oel_index--;
                    
                    myhvml_tree_node_t* node = tree->open_elements->list[oel_index];
                    const myhvml_tag_context_t *tag_ctx = myhvml_tag_get_by_id(tree->tags, node->tag_id);
                    
                    if(myhvml_is_hvml_node(node, MyHVML_TAG_DD)) {
                        myhvml_tree_generate_implied_end_tags(tree, MyHVML_TAG_DD, MyHVML_NAMESPACE_HVML);
                        
                        /* 3.2 */
                        myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                        if(myhvml_is_hvml_node(current_node, MyHVML_TAG_DD)) {
                            // parse error
                            /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                            /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_DD NEED_NS:MyHVML_NAMESPACE_HVML */
                        }
                        
                        myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_DD, MyHVML_NAMESPACE_HVML, false);
                        break;
                    }
                    else if(myhvml_is_hvml_node(node, MyHVML_TAG_DT)) {
                        myhvml_tree_generate_implied_end_tags(tree, MyHVML_TAG_DT, MyHVML_NAMESPACE_HVML);
                        
                        /* 3.2 */
                        myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                        if(myhvml_is_hvml_node(current_node, MyHVML_TAG_DT)) {
                            // parse error
                            /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                            /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_DT NEED_NS:MyHVML_NAMESPACE_HVML */
                        }
                        
                        myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_DT, MyHVML_NAMESPACE_HVML, false);
                        break;
                    }
                    else if(tag_ctx->cats[node->ns] & MyHVML_TAG_CATEGORIES_SPECIAL)
                    {
                        if(!((node->tag_id == MyHVML_TAG_ADDRESS || node->tag_id == MyHVML_TAG_DIV ||
                             node->tag_id == MyHVML_TAG_P) && node->ns == MyHVML_NAMESPACE_HVML))
                                break;
                    }
                }
                
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_PLAINTEXT:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->state_of_builder = MyHVML_TOKENIZER_STATE_PLAINTEXT;
                break;
            }
                
            case MyHVML_TAG_BUTTON:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_BUTTON, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE)) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                    
                    myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                    myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_BUTTON, MyHVML_NAMESPACE_HVML, false);
                }
                
                myhvml_tree_active_formatting_reconstruction(tree);
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
               
            case MyHVML_TAG_A:
            {
                myhvml_tree_node_t* node = myhvml_tree_active_formatting_between_last_marker(tree, MyHVML_TAG_A, NULL);
                
                if(node) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                    
                    myhvml_tree_adoption_agency_algorithm(tree, token, MyHVML_TAG_A);
                    node = myhvml_tree_active_formatting_between_last_marker(tree, MyHVML_TAG_A, NULL);
                    
                    if(node) {
                        myhvml_tree_open_elements_remove(tree, node);
                        myhvml_tree_active_formatting_remove(tree, node);
                    }
                }
                
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_tree_node_t* current = myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_active_formatting_append_with_check(tree, current);
                break;
            }
                
            case MyHVML_TAG_B:
            case MyHVML_TAG_BIG:
            case MyHVML_TAG_CODE:
            case MyHVML_TAG_EM:
            case MyHVML_TAG_FONT:
            case MyHVML_TAG_I:
            case MyHVML_TAG_S:
            case MyHVML_TAG_SMALL:
            case MyHVML_TAG_STRIKE:
            case MyHVML_TAG_STRONG:
            case MyHVML_TAG_TT:
            case MyHVML_TAG_U:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_tree_node_t* current = myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_active_formatting_append_with_check(tree, current);
                break;
            }

            case MyHVML_TAG_NOBR:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_NOBR, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE)) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                    
                    myhvml_tree_adoption_agency_algorithm(tree, token, MyHVML_TAG_NOBR);
                    myhvml_tree_active_formatting_reconstruction(tree);
                }
                
                myhvml_tree_node_t* current = myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_active_formatting_append_with_check(tree, current);
                break;
            }

            case MyHVML_TAG_APPLET:
            case MyHVML_TAG_MARQUEE:
            case MyHVML_TAG_OBJECT:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_active_formatting_append(tree, tree->myhvml->marker); // marker
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
                
            case MyHVML_TAG_TABLE:
            {
                if((tree->compat_mode & MyHVML_TREE_COMPAT_MODE_QUIRKS) == 0 &&
                   myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON))
                {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                break;
            }
                
            case MyHVML_TAG_AREA:
            case MyHVML_TAG_BR:
            case MyHVML_TAG_EMBED:
            case MyHVML_TAG_IMG:
            case MyHVML_TAG_KEYGEN:
            case MyHVML_TAG_WBR:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
                
            case MyHVML_TAG_INPUT:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                
                myhvml_token_node_wait_for_done(tree->token, token);
                if(myhvml_token_attr_match_case(tree->token, token, "type", 4, "hidden", 6) == NULL) {
                    tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                }
                
                break;
            }
                
            case MyHVML_TAG_PARAM:
            case MyHVML_TAG_SOURCE:
            case MyHVML_TAG_TRACK:
            {
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                break;
            }
                
            case MyHVML_TAG_HR:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_MENUITEM))
                    myhvml_tree_open_elements_pop(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
                
            case MyHVML_TAG_IMAGE:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES CONVERT STATUS:ELEMENT_CONVERT LEVEL:ERROR FROM_TAG_ID:MyHVML_TAG_IMAGE FROM_NS:MyHVML_NAMESPACE_ANY FROM_TYPE:MyHVML_TOKEN_TYPE_OPEN TO_TAG_ID:MyHVML_TAG_IMG TO_NS:MyHVML_NAMESPACE_ANY TO_TYPE:MyHVML_TOKEN_TYPE_OPEN */
                
                token->tag_id = MyHVML_TAG_IMG;
                return true;
            }
                
            case MyHVML_TAG_TEXTAREA:
            {
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                // If the next token is a U+000A LINE FEED (LF) character token,
                // then ignore that token and move on to the next one.
                // (Newlines at the start of textarea elements are ignored as an authoring convenience.)
                // !!! see dispatcher (myhvml_rules_tree_dispatcher) for this
                tree->flags |= MyHVML_TREE_FLAGS_PARSE_FLAG|MyHVML_TREE_FLAGS_PARSE_FLAG_EMIT_NEWLINE;
                
                tree->orig_insert_mode = tree->insert_mode;
                tree->flags           ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                tree->insert_mode      = MyHVML_INSERTION_MODE_TEXT;
                tree->state_of_builder = MyHVML_TOKENIZER_STATE_RCDATA;
                
                break;
            }

            case MyHVML_TAG_XMP:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_P, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhvml_tree_tags_close_p(tree, token);
                }
                
                myhvml_tree_active_formatting_reconstruction(tree);
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                tree->state_of_builder = MyHVML_TOKENIZER_STATE_RAWTEXT;
                
                myhvml_tree_generic_raw_text_element_parsing_algorithm(tree, token);
                break;
            }

            case MyHVML_TAG_IFRAME:
            {
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                tree->state_of_builder = MyHVML_TOKENIZER_STATE_RAWTEXT;
                
                myhvml_tree_generic_raw_text_element_parsing_algorithm(tree, token);
                break;
            }
                
            case MyHVML_TAG_NOEMBED:
            {
                tree->state_of_builder = MyHVML_TOKENIZER_STATE_RAWTEXT;
                myhvml_tree_generic_raw_text_element_parsing_algorithm(tree, token);
                break;
            }
                
            case MyHVML_TAG_NOSCRIPT:
            {
                if(tree->flags & MyHVML_TREE_FLAGS_SCRIPT) {
                    tree->state_of_builder = MyHVML_TOKENIZER_STATE_RAWTEXT;
                    myhvml_tree_generic_raw_text_element_parsing_algorithm(tree, token);
                }
                else {
                    myhvml_tree_active_formatting_reconstruction(tree);
                    myhvml_tree_node_insert_hvml_element(tree, token);
                }
//                else {
//                    myhvml_tree_node_insert_hvml_element(tree, token);
//                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_HEAD_NOSCRIPT;
//                }
                
                break;
            }
                
            case MyHVML_TAG_SELECT:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                
                if(tree->insert_mode == MyHVML_INSERTION_MODE_IN_TABLE ||
                   tree->insert_mode == MyHVML_INSERTION_MODE_IN_CAPTION ||
                   tree->insert_mode == MyHVML_INSERTION_MODE_IN_TABLE_BODY ||
                   tree->insert_mode == MyHVML_INSERTION_MODE_IN_ROW ||
                   tree->insert_mode == MyHVML_INSERTION_MODE_IN_CELL)
                {
                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_SELECT_IN_TABLE;
                }
                else
                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_SELECT;
                
                break;
            }
                
            case MyHVML_TAG_OPTGROUP:
            case MyHVML_TAG_OPTION:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_OPTION))
                    myhvml_tree_open_elements_pop(tree);
                
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_MENUITEM:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_MENUITEM))
                    myhvml_tree_open_elements_pop(tree);
                
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_RB:
            case MyHVML_TAG_RTC:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_RUBY, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE)) {
                    myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                }
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(current_node->tag_id != MyHVML_TAG_RUBY) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_RUBY NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_RP:
            case MyHVML_TAG_RT:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_RUBY, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE)) {
                    myhvml_tree_generate_implied_end_tags(tree, MyHVML_TAG_RTC, MyHVML_NAMESPACE_HVML);
                }
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(current_node->tag_id != MyHVML_TAG_RTC && current_node->tag_id != MyHVML_TAG_RUBY) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_RTC NEED_NS:MyHVML_NAMESPACE_HVML */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_RUBY NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_MATH:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_token_node_wait_for_done(tree->token, token);
                
                myhvml_token_adjust_mathml_attributes(token);
                myhvml_token_adjust_foreign_attributes(token);
                
                myhvml_tree_node_t* current_node = myhvml_tree_node_insert_foreign_element(tree, token);
                current_node->ns = MyHVML_NAMESPACE_MATHML;
                
                if(token->type & MyHVML_TOKEN_TYPE_CLOSE_SELF)
                    myhvml_tree_open_elements_pop(tree);
                
                break;
            }
                
            case MyHVML_TAG_SVG:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                
                myhvml_token_node_wait_for_done(tree->token, token);
                
                myhvml_token_adjust_svg_attributes(token);
                myhvml_token_adjust_foreign_attributes(token);
                
                myhvml_tree_node_t* current_node = myhvml_tree_node_insert_foreign_element(tree, token);
                current_node->ns = MyHVML_NAMESPACE_SVG;
                
                if(token->type & MyHVML_TOKEN_TYPE_CLOSE_SELF)
                    myhvml_tree_open_elements_pop(tree);
                
                break;
            }
                
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_FRAME:
            case MyHVML_TAG_HEAD:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_TH:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            default:
            {
                myhvml_tree_active_formatting_reconstruction(tree);
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                break;
            }
        }
    }
    
    return false;
}

bool myhvml_insertion_mode_text(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_SCRIPT:
            {
                // new document.write is not works; set back
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = tree->orig_insert_mode;
                break;
            }
                
            default:
            {
                myhvml_tree_open_elements_pop(tree);
                tree->insert_mode = tree->orig_insert_mode;
                break;
            }
        }
    }
    else {
        if(token->tag_id == MyHVML_TAG__END_OF_FILE)
        {
            // parse error
            /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:PREMATURE_TERMINATION LEVEL:ERROR */
            
            myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
            
            if(current_node->tag_id == MyHVML_TAG_SCRIPT)
                current_node->flags |= MyHVML_TREE_FLAGS_ALREADY_STARTED;
            
            myhvml_tree_open_elements_pop(tree);
            
            tree->insert_mode = tree->orig_insert_mode;
            return true;
        }
        
        myhvml_tree_node_insert_text(tree, token);
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_table(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_TABLE:
            {
                myhvml_tree_node_t* table_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TABLE, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(table_node == NULL) {
                     // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_open_elements_pop_until_by_node(tree, table_node, false);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                break;
            }
                
            case MyHVML_TAG_BODY:
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_HVML:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_TH:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            case MyHVML_TAG_TEMPLATE:
            {
                return myhvml_insertion_mode_in_head(tree, token);
            }
                
            default: {
                // parse error
                tree->foster_parenting = true;
                myhvml_insertion_mode_in_body(tree, token);
                tree->foster_parenting = false;
                
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if((current_node->tag_id == MyHVML_TAG_TABLE  ||
                   current_node->tag_id  == MyHVML_TAG_TBODY  ||
                   current_node->tag_id  == MyHVML_TAG_TFOOT  ||
                   current_node->tag_id  == MyHVML_TAG_THEAD  ||
                   current_node->tag_id  == MyHVML_TAG_TR)    &&
                   current_node->ns == MyHVML_NAMESPACE_HVML)
                {
                    myhvml_tree_token_list_clean(tree->token_list);
                    
                    tree->orig_insert_mode = tree->insert_mode;
                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE_TEXT;
                    
                    return true;
                }
                else {
                    tree->foster_parenting = true;
                    myhvml_insertion_mode_in_body(tree, token);
                    tree->foster_parenting = false;
                    
                    break;
                }
            }
            
            case MyHVML_TAG__COMMENT:
                myhvml_tree_node_insert_comment(tree, token, 0);
                break;
                
            case MyHVML_TAG__DOCTYPE: {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:WARNING */
                break;
            }
                
            case MyHVML_TAG_CAPTION:
            {
                myhvml_tree_clear_stack_back_table_context(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_active_formatting_append(tree, tree->myhvml->marker);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_CAPTION;
                break;
            }
                
            case MyHVML_TAG_COLGROUP:
            {
                myhvml_tree_clear_stack_back_table_context(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_COLUMN_GROUP;
                break;
            }
                
            case MyHVML_TAG_COL:
            {
                myhvml_tree_clear_stack_back_table_context(tree);
                myhvml_tree_node_insert(tree, MyHVML_TAG_COLGROUP, MyHVML_NAMESPACE_HVML);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_COLUMN_GROUP;
                return true;
            }
                
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
            {
                myhvml_tree_clear_stack_back_table_context(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE_BODY;
                break;
            }
               
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TH:
            case MyHVML_TAG_TR:
            {
                myhvml_tree_clear_stack_back_table_context(tree);
                myhvml_tree_node_insert(tree, MyHVML_TAG_TBODY, MyHVML_NAMESPACE_HVML);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
            }
                
            case MyHVML_TAG_TABLE:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_node_t* table_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TABLE, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(table_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_TABLE, MyHVML_NAMESPACE_HVML, false);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
                
            case MyHVML_TAG_STYLE:
            case MyHVML_TAG_SCRIPT:
            case MyHVML_TAG_TEMPLATE:
            {
                return myhvml_insertion_mode_in_head(tree, token);
            }
                
            case MyHVML_TAG_INPUT:
            {
                myhvml_token_node_wait_for_done(tree->token, token);
                
                if(myhvml_token_attr_match_case(tree->token, token, "type", 4, "hidden", 6) == NULL) {
                    tree->foster_parenting = true;
                    myhvml_insertion_mode_in_body(tree, token);
                    tree->foster_parenting = false;
                    
                    break;
                }
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                
                token->type |= MyHVML_TOKEN_TYPE_CLOSE_SELF;
                break;
            }
                
            case MyHVML_TAG_FORM:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_node_t* template = myhvml_tree_open_elements_find_by_tag_idx(tree, MyHVML_TAG_TEMPLATE, MyHVML_NAMESPACE_HVML, NULL);
                if(tree->node_form || template)
                    break;
                
                tree->node_form = myhvml_tree_node_insert_hvml_element(tree, token);
                
                myhvml_tree_open_elements_pop(tree);
            }
                
            case MyHVML_TAG__END_OF_FILE:
                return myhvml_insertion_mode_in_body(tree, token);
                
            default:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                tree->foster_parenting = true;
                myhvml_insertion_mode_in_body(tree, token);
                tree->foster_parenting = false;
                
                break;
            }
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_table_text(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    // skip NULL, we replaced earlier
    if(token->tag_id == MyHVML_TAG__TEXT)
    {
        if(token->type & MyHVML_TOKEN_TYPE_NULL) {
            // parse error
            /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:NULL_CHAR ACTION:IGNORE LEVEL:ERROR */
            
            myhvml_insertion_fix_for_null_char_drop_all(tree, token);
            
            if(token->str.length)
                myhvml_tree_token_list_append(tree->token_list, token);
        }
        else
            myhvml_tree_token_list_append(tree->token_list, token);
    }
    else {
        myhvml_tree_token_list_t* token_list = tree->token_list;
        bool is_not_ws = false;
        
        for(size_t i = 0; i < token_list->length; i++) {
            if((token_list->list[i]->type & MyHVML_TOKEN_TYPE_WHITESPACE) == 0) {
                is_not_ws = true;
                break;
            }
        }
        
        if(is_not_ws)
        {
            for(size_t i = 0; i < token_list->length; i++) {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR TOKEN:token_list->list[i] */
                
                tree->foster_parenting = true;
                myhvml_insertion_mode_in_body(tree, token_list->list[i]);
                tree->foster_parenting = false;
            }
        }
        else {
            for(size_t i = 0; i < token_list->length; i++) {
                myhvml_tree_node_insert_text(tree, token_list->list[i]);
            }
        }
        
        tree->insert_mode = tree->orig_insert_mode;
        return true;
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_caption(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_CAPTION:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_CAPTION, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE) == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_CAPTION) == false) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_CAPTION NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_CAPTION, MyHVML_NAMESPACE_HVML, false);
                myhvml_tree_active_formatting_up_to_last_marker(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                break;
            }
              
            case MyHVML_TAG_TABLE:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_CAPTION, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE) == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_CAPTION) == false) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_CAPTION NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_CAPTION, MyHVML_NAMESPACE_HVML, false);
                myhvml_tree_active_formatting_up_to_last_marker(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                return true;
            }
                
            case MyHVML_TAG_BODY:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_HVML:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_TH:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            default:
                return myhvml_insertion_mode_in_body(tree, token);
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_TH:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            {
                if(myhvml_tree_element_in_scope(tree, MyHVML_TAG_CAPTION, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE) == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_CAPTION) == false) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:MyHVML_TAG_CAPTION NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_CAPTION, MyHVML_NAMESPACE_HVML, false);
                myhvml_tree_active_formatting_up_to_last_marker(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                return true;
            }
                
            default:
                return myhvml_insertion_mode_in_body(tree, token);
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_column_group(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_COLGROUP:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(current_node && myhvml_is_hvml_node(current_node, MyHVML_TAG_COLGROUP)) {
                    myhvml_tree_open_elements_pop(tree);
                    
                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                    return false;
                }
                
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            case MyHVML_TAG_COL:
            {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
                
            case MyHVML_TAG_TEMPLATE:
            {
                return myhvml_insertion_mode_in_head(tree, token);
            }
                
            default: {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(current_node && myhvml_is_hvml_node(current_node, MyHVML_TAG_COLGROUP)) {
                    myhvml_tree_open_elements_pop(tree);
                    
                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                    return true;
                }
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE) {
                    myhvml_tree_node_insert_text(tree, token);
                    break;
                }
                
                myhvml_token_node_t* new_token = myhvml_insertion_fix_split_for_text_begin_ws(tree, token);
                if(new_token)
                    myhvml_tree_node_insert_text(tree, new_token);
                
                /* default: */
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(current_node && myhvml_is_hvml_node(current_node, MyHVML_TAG_COLGROUP)) {
                    myhvml_tree_open_elements_pop(tree);
                    
                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                    return true;
                }
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            case MyHVML_TAG__COMMENT:
            {
                myhvml_tree_node_insert_comment(tree, token, 0);
                break;
            }
                
            case MyHVML_TAG__DOCTYPE: {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                break;
            }
            case MyHVML_TAG_HVML:
            {
                return myhvml_insertion_mode_in_body(tree, token);
            }
                
            case MyHVML_TAG_COL:
            {
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                break;
            }
                
            case MyHVML_TAG_TEMPLATE:
            {
                return myhvml_insertion_mode_in_head(tree, token);
            }
                
            case MyHVML_TAG__END_OF_FILE:
                return myhvml_insertion_mode_in_body(tree, token);
                
            default:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(current_node && myhvml_is_hvml_node(current_node, MyHVML_TAG_COLGROUP)) {
                    myhvml_tree_open_elements_pop(tree);
                    
                    tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                    return true;
                }
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                break;
            }
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_table_body(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
            {
                myhvml_tree_node_t* node = myhvml_tree_element_in_scope(tree, token->tag_id, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_clear_stack_back_table_body_context(tree);
                myhvml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                break;
            }
                
            case MyHVML_TAG_TABLE:
            {
                myhvml_tree_node_t* tbody_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TBODY, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                myhvml_tree_node_t* tfoot_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TFOOT, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                myhvml_tree_node_t* thead_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_THEAD, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tbody_node == NULL && tfoot_node == NULL && thead_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_THEAD NEED_NS:MyHVML_NAMESPACE_HVML */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_TBODY NEED_NS:MyHVML_NAMESPACE_HVML */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_TFOOT NEED_NS:MyHVML_NAMESPACE_HVML */
                    break;
                }
                
                myhvml_tree_clear_stack_back_table_body_context(tree);
                myhvml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                return true;
            }
               
            case MyHVML_TAG_BODY:
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_HVML:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TH:
            case MyHVML_TAG_TR:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            default:
                return myhvml_insertion_mode_in_table(tree, token);
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG_TR:
            {
                myhvml_tree_clear_stack_back_table_body_context(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_ROW;
                break;
            }
                
            case MyHVML_TAG_TH:
            case MyHVML_TAG_TD:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_clear_stack_back_table_body_context(tree);
                
                myhvml_tree_node_insert(tree, MyHVML_TAG_TR, MyHVML_NAMESPACE_HVML);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_ROW;
                return true;
            }
                
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
            {
                myhvml_tree_node_t* tbody_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TBODY, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                myhvml_tree_node_t* tfoot_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TFOOT, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                myhvml_tree_node_t* thead_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_THEAD, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tbody_node == NULL && tfoot_node == NULL && thead_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_THEAD NEED_NS:MyHVML_NAMESPACE_HVML */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_TBODY NEED_NS:MyHVML_NAMESPACE_HVML */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_TFOOT NEED_NS:MyHVML_NAMESPACE_HVML */
                    break;
                }
                
                myhvml_tree_clear_stack_back_table_body_context(tree);
                myhvml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                return true;
            }
                
            default:
                return myhvml_insertion_mode_in_table(tree, token);
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_row(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_TR:
            {
                myhvml_tree_node_t* tr_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TR, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tr_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_clear_stack_back_table_row_context(tree);
                
                myhvml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE_BODY;
                break;
            }
                
            case MyHVML_TAG_TABLE:
            {
                myhvml_tree_node_t* tr_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TR, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tr_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_TR NEED_NS:MyHVML_NAMESPACE_HVML */
                    break;
                }
                
                myhvml_tree_clear_stack_back_table_row_context(tree);
                myhvml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
            }
                
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
            {
                myhvml_tree_node_t* node = myhvml_tree_element_in_scope(tree, token->tag_id, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                if(node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_node_t* tr_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TR, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                if(tr_node == NULL)
                    break;
                
                myhvml_tree_clear_stack_back_table_row_context(tree);
                myhvml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
            }
                
            case MyHVML_TAG_BODY:
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_HVML:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TH:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            default:
                return myhvml_insertion_mode_in_table(tree, token);
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG_TH:
            case MyHVML_TAG_TD:
            {
                myhvml_tree_clear_stack_back_table_row_context(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_active_formatting_append(tree, tree->myhvml->marker);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_CELL;
                break;
            }
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            {
                myhvml_tree_node_t* tr_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TR, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tr_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_TR NEED_NS:MyHVML_NAMESPACE_HVML */
                    break;
                }
                
                myhvml_tree_clear_stack_back_table_row_context(tree);
                myhvml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
            }
                
            default:
                return myhvml_insertion_mode_in_table(tree, token);
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_cell(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TH:
            {
                myhvml_tree_node_t* node = myhvml_tree_element_in_scope(tree, token->tag_id, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_generate_implied_end_tags(tree, 0, MyHVML_NAMESPACE_UNDEF);
                
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, token->tag_id) == false)
                {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:NULL HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:token->tag_id NEED_NS:MyHVML_NAMESPACE_HVML */
                }
                
                myhvml_tree_open_elements_pop_until(tree, token->tag_id, MyHVML_NAMESPACE_HVML, false);
                
                myhvml_tree_active_formatting_up_to_last_marker(tree);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_ROW;
                break;
            }
                
            case MyHVML_TAG_BODY:
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_HVML:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
                
            case MyHVML_TAG_TABLE:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            {
                myhvml_tree_node_t* node = myhvml_tree_element_in_scope(tree, token->tag_id, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TD, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                if(node) {
                    myhvml_tree_close_cell(tree, node, token);
                }
                else {
                    node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TH, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                    if(node)
                        myhvml_tree_close_cell(tree, node, token);
                }
                
                return true;
            }
                
            default:
                return myhvml_insertion_mode_in_table(tree, token);
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COL:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_TH:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            {
                myhvml_tree_node_t* td_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TD, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                myhvml_tree_node_t* th_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_TH, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(td_node == NULL && th_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_TD NEED_NS:MyHVML_NAMESPACE_HVML */
                    /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:NULL NEED:NULL HAVE_TAG_ID:MyHVML_TAG__UNDEF HAVE_NS:MyHVML_NAMESPACE_UNDEF NEED_TAG_ID:MyHVML_TAG_TH NEED_NS:MyHVML_NAMESPACE_HVML */
                    
                    break;
                }
                
                myhvml_tree_close_cell(tree, (td_node == NULL ? th_node : td_node), token);
                
                return true;
            }
                
            default:
                return myhvml_insertion_mode_in_body(tree, token);
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_select(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_OPTGROUP:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_OPTION))
                {
                    if(tree->open_elements->length > 1) {
                        myhvml_tree_node_t *optgrp_node = tree->open_elements->list[ tree->open_elements->length - 2 ];
                        
                        if(myhvml_is_hvml_node(optgrp_node, MyHVML_TAG_OPTGROUP))
                        {
                            myhvml_tree_open_elements_pop(tree);
                        }
                    }
                }
                
                current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_OPTGROUP))
                    myhvml_tree_open_elements_pop(tree);
                else {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_NO_EXPECTED ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                break;
            }
                
            case MyHVML_TAG_OPTION:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, MyHVML_TAG_OPTION))
                    myhvml_tree_open_elements_pop(tree);
                else {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                break;
            }
                
            case MyHVML_TAG_SELECT:
            {
                myhvml_tree_node_t* select_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_SELECT, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_SELECT);
                
                if(select_node == NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    break;
                }
                
                myhvml_tree_open_elements_pop_until_by_node(tree, select_node, false);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                break;
            }
                
            case MyHVML_TAG_TEMPLATE:
                return myhvml_insertion_mode_in_head(tree, token);
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT: {
                if(token->type & MyHVML_TOKEN_TYPE_NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:NULL_CHAR ACTION:IGNORE LEVEL:ERROR */
                    
                    myhvml_insertion_fix_for_null_char_drop_all(tree, token);
                    
                    if(token->str.length)
                        myhvml_tree_node_insert_text(tree, token);
                }
                else
                    myhvml_tree_node_insert_text(tree, token);
                
                break;
            }
                
            case MyHVML_TAG__COMMENT:
                myhvml_tree_node_insert_comment(tree, token, NULL);
                break;
                
            case MyHVML_TAG__DOCTYPE: {
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
                
            case MyHVML_TAG_HVML:
                return myhvml_insertion_mode_in_body(tree, token);
                
            case MyHVML_TAG_OPTION:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(myhvml_is_hvml_node(current_node, token->tag_id))
                    myhvml_tree_open_elements_pop(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_OPTGROUP:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(current_node->tag_id == MyHVML_TAG_OPTION &&
                   current_node->ns == MyHVML_NAMESPACE_HVML)
                    myhvml_tree_open_elements_pop(tree);
                
                current_node = myhvml_tree_current_node(tree);
                
                if(current_node->tag_id == token->tag_id &&
                   current_node->ns == MyHVML_NAMESPACE_HVML)
                    myhvml_tree_open_elements_pop(tree);
                
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
            }
                
            case MyHVML_TAG_SELECT:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_node_t* select_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_SELECT, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_SELECT);
                
                if(select_node == NULL) {
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                myhvml_tree_open_elements_pop_until_by_node(tree, select_node, false);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                break;
            }
                
            case MyHVML_TAG_INPUT:
            case MyHVML_TAG_KEYGEN:
            case MyHVML_TAG_TEXTAREA:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_node_t* select_node = myhvml_tree_element_in_scope(tree, MyHVML_TAG_SELECT, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_SELECT);
                
                if(select_node == NULL) {
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                myhvml_tree_open_elements_pop_until_by_node(tree, select_node, false);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
                
            case MyHVML_TAG_SCRIPT:
            case MyHVML_TAG_TEMPLATE:
                return myhvml_insertion_mode_in_head(tree, token);
                
            case MyHVML_TAG__END_OF_FILE:
                return myhvml_insertion_mode_in_body(tree, token);
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                break;
            }
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_select_in_table(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_TABLE:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TH:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_node_t* some_node = myhvml_tree_element_in_scope(tree, token->tag_id, MyHVML_NAMESPACE_HVML, MyHVML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(some_node == NULL) {
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_OPEN_NOT_FOUND ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_SELECT, MyHVML_NAMESPACE_HVML, false);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
                
            default:
                return myhvml_insertion_mode_in_select(tree, token);
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_TABLE:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
            case MyHVML_TAG_TR:
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TH:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION LEVEL:ERROR */
                
                myhvml_tree_open_elements_pop_until(tree, MyHVML_TAG_SELECT, MyHVML_NAMESPACE_HVML, false);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
            
            default:
                return myhvml_insertion_mode_in_select(tree, token);
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_template(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_TEMPLATE:
                return myhvml_insertion_mode_in_body(tree, token);
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            case MyHVML_TAG__COMMENT:
            case MyHVML_TAG__DOCTYPE:
                return myhvml_insertion_mode_in_body(tree, token);
                
            case MyHVML_TAG_BASE:
            case MyHVML_TAG_BASEFONT:
            case MyHVML_TAG_BGSOUND:
            case MyHVML_TAG_LINK:
            case MyHVML_TAG_META:
            case MyHVML_TAG_NOFRAMES:
            case MyHVML_TAG_SCRIPT:
            case MyHVML_TAG_STYLE:
            case MyHVML_TAG_TEMPLATE:
            case MyHVML_TAG_TITLE:
                return myhvml_insertion_mode_in_head(tree, token);
                
            case MyHVML_TAG_CAPTION:
            case MyHVML_TAG_COLGROUP:
            case MyHVML_TAG_TBODY:
            case MyHVML_TAG_TFOOT:
            case MyHVML_TAG_THEAD:
                myhvml_tree_template_insertion_pop(tree);
                myhvml_tree_template_insertion_append(tree, MyHVML_INSERTION_MODE_IN_TABLE);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE;
                return true;
                
            case MyHVML_TAG_COL:
                myhvml_tree_template_insertion_pop(tree);
                myhvml_tree_template_insertion_append(tree, MyHVML_INSERTION_MODE_IN_COLUMN_GROUP);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_COLUMN_GROUP;
                return true;
                
            case MyHVML_TAG_TR:
                myhvml_tree_template_insertion_pop(tree);
                myhvml_tree_template_insertion_append(tree, MyHVML_INSERTION_MODE_IN_TABLE_BODY);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
                
            case MyHVML_TAG_TD:
            case MyHVML_TAG_TH:
                myhvml_tree_template_insertion_pop(tree);
                myhvml_tree_template_insertion_append(tree, MyHVML_INSERTION_MODE_IN_ROW);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_ROW;
                return true;
                
            case MyHVML_TAG__END_OF_FILE:
            {
                myhvml_tree_node_t* node = myhvml_tree_open_elements_find_by_tag_idx(tree, MyHVML_TAG_TEMPLATE, MyHVML_NAMESPACE_HVML, NULL);
                
                if(node == NULL) {
                    myhvml_rules_stop_parsing(tree);
                    break;
                }
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TAG STATUS:ELEMENT_NOT_CLOSED LEVEL:ERROR TAG_ID:MyHVML_TAG_TEMPLATE NS:MyHVML_NAMESPACE_HVML */
                
                myhvml_tree_open_elements_pop_until_by_node(tree, node, false);
                myhvml_tree_active_formatting_up_to_last_marker(tree);
                myhvml_tree_template_insertion_pop(tree);
                myhvml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
                
             default:
                myhvml_tree_template_insertion_pop(tree);
                myhvml_tree_template_insertion_append(tree, MyHVML_INSERTION_MODE_IN_BODY);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                return true;
        }
    }
    
    return false;
}

bool myhvml_insertion_mode_after_body(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_HVML:
            {
                if(tree->fragment) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_NO_EXPECTED ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_AFTER_BODY;
                break;
            }
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY LEVEL:ERROR */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                return true;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE)
                    return myhvml_insertion_mode_in_body(tree, token);
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                return true;
            }
                
            case MyHVML_TAG__COMMENT:
            {
                if(tree->open_elements->length == 0) {
                    MyCORE_DEBUG_ERROR("after body state; open_elements length < 1");
                    break;
                }
                
                myhvml_tree_node_t* adjusted_location = tree->open_elements->list[0];
                
                // state 2
                myhvml_tree_node_t* node = myhvml_tree_node_create(tree);
                
                node->tag_id      = MyHVML_TAG__COMMENT;
                node->token        = token;
                node->ns = adjusted_location->ns;
                
                myhvml_tree_node_add_child(adjusted_location, node);
                
                break;
            }
                
            case MyHVML_TAG__DOCTYPE: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
            case MyHVML_TAG_HVML:
                return myhvml_insertion_mode_in_body(tree, token);
                
            case MyHVML_TAG__END_OF_FILE:
                myhvml_rules_stop_parsing(tree);
                break;
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY LEVEL:ERROR */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                return true;
            }
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_frameset(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_FRAMESET:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(current_node == tree->document->child) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_NO_EXPECTED ACTION:IGNORE LEVEL:ERROR */
                    
                    break;
                }
                
                myhvml_tree_open_elements_pop(tree);
                
                current_node = myhvml_tree_current_node(tree);
                
                if(tree->fragment == NULL &&
                   !(current_node->tag_id == MyHVML_TAG_FRAMESET &&
                     current_node->ns == MyHVML_NAMESPACE_HVML))
                {
                    tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_FRAMESET;
                }
                
                break;
            }
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE) {
                    myhvml_tree_node_insert_text(tree, token);
                    break;
                }
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                
                myhvml_token_node_wait_for_done(tree->token, token);
                mycore_string_stay_only_whitespace(&token->str);
                
                if(token->str.length)
                    myhvml_tree_node_insert_text(tree, token);
                
                break;
            }
                
            case MyHVML_TAG__COMMENT:
            {
                myhvml_tree_node_insert_comment(tree, token, NULL);
                break;
            }
                
            case MyHVML_TAG__DOCTYPE: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
                
            case MyHVML_TAG_HVML:
                return myhvml_insertion_mode_in_body(tree, token);
                
            case MyHVML_TAG_FRAMESET:
                myhvml_tree_node_insert_hvml_element(tree, token);
                break;
                
            case MyHVML_TAG_FRAME:
                myhvml_tree_node_insert_hvml_element(tree, token);
                myhvml_tree_open_elements_pop(tree);
                break;
                
            case MyHVML_TAG_NOFRAMES:
                return myhvml_insertion_mode_in_head(tree, token);
                
            case MyHVML_TAG__END_OF_FILE:
            {
                myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
                
                if(current_node == tree->document->child) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                }
                
                myhvml_rules_stop_parsing(tree);
                break;
            }
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
        }
    }
    
    return false;
}

bool myhvml_insertion_mode_after_frameset(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_id) {
            case MyHVML_TAG_HVML:
                tree->insert_mode = MyHVML_INSERTION_MODE_AFTER_AFTER_FRAMESET;
                break;
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
        }
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE) {
                    myhvml_tree_node_insert_text(tree, token);
                    break;
                }
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                
                myhvml_token_node_wait_for_done(tree->token, token);
                mycore_string_stay_only_whitespace(&token->str);
                
                if(token->str.length)
                    myhvml_tree_node_insert_text(tree, token);
                
                break;
            }
                
            case MyHVML_TAG__COMMENT:
            {
                myhvml_tree_node_insert_comment(tree, token, NULL);
                break;
            }
                
            case MyHVML_TAG__DOCTYPE: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                break;
            }
                
            case MyHVML_TAG_HVML:
                return myhvml_insertion_mode_in_body(tree, token);
                
            case MyHVML_TAG_NOFRAMES:
                return myhvml_insertion_mode_in_head(tree, token);
                
            case MyHVML_TAG__END_OF_FILE:
                myhvml_rules_stop_parsing(tree);
                break;
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
        }
    }
    
    return false;
}

bool myhvml_insertion_mode_after_after_body(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE)
    {
        tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
        return true;
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__COMMENT:
            {
                myhvml_tree_node_t* adjusted_location = tree->document;
                myhvml_tree_node_t* node = myhvml_tree_node_create(tree);
                
                node->tag_id      = MyHVML_TAG__COMMENT;
                node->token        = token;
                node->ns = adjusted_location->ns;
                
                myhvml_tree_node_add_child(adjusted_location, node);
                break;
            }
                
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE)
                    return myhvml_insertion_mode_in_body(tree, token);
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                return true;
            }
                
            case MyHVML_TAG_HVML:
            case MyHVML_TAG__DOCTYPE:
                return myhvml_insertion_mode_in_body(tree, token);
                
            case MyHVML_TAG__END_OF_FILE:
                myhvml_rules_stop_parsing(tree);
                break;
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                
                tree->insert_mode = MyHVML_INSERTION_MODE_IN_BODY;
                return true;
            }
        }
    }
    
    return false;
}

bool myhvml_insertion_mode_after_after_frameset(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE) {
        // parse error
        /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY LEVEL:ERROR */
        
        return false;
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__COMMENT:
            {
                myhvml_tree_node_t* adjusted_location = tree->document;
                myhvml_tree_node_t* node = myhvml_tree_node_create(tree);
                
                node->tag_id      = MyHVML_TAG__COMMENT;
                node->token        = token;
                node->ns = adjusted_location->ns;
                
                myhvml_tree_node_add_child(adjusted_location, node);
                break;
            }
                
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_WHITESPACE)
                    return myhvml_insertion_mode_in_body(tree, token);
                
                myhvml_token_node_t* new_token = myhvml_insertion_fix_split_for_text_begin_ws(tree, token);
                if(new_token)
                    return myhvml_insertion_mode_in_body(tree, new_token);
                
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY LEVEL:ERROR */
                
                break;
            }
                
            case MyHVML_TAG_HVML:
            case MyHVML_TAG__DOCTYPE:
                return myhvml_insertion_mode_in_body(tree, token);
                
            case MyHVML_TAG__END_OF_FILE:
                myhvml_rules_stop_parsing(tree);
                break;
                
            case MyHVML_TAG_NOFRAMES:
                return myhvml_insertion_mode_in_head(tree, token);
                
            default: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_UNNECESSARY LEVEL:ERROR */
                break;
            }
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_foreign_content_end_other(myhvml_tree_t* tree, myhvml_tree_node_t* current_node, myhvml_token_node_t* token)
{
    if(current_node->tag_id != token->tag_id) {
        // parse error
        /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
        /* %EXTERNAL% VALIDATOR:RULES HAVE_NEED_ADD HAVE:current_node->token NEED:token HAVE_TAG_ID:current_node->tag_id HAVE_NS:current_node->ns NEED_TAG_ID:token->tag_id NEED_NS:MyHVML_NAMESPACE_HVML */
    }
    
    if(tree->open_elements->length)
    {
        myhvml_tree_node_t** list = tree->open_elements->list;
        size_t i = tree->open_elements->length - 1;
        
        while (i)
        {
            current_node = list[i];
            
            if(current_node->tag_id == token->tag_id) {
                myhvml_tree_open_elements_pop_until_by_node(tree, current_node, false);
                return false;
            }
            
            i--;
            
            if(list[i]->ns == MyHVML_NAMESPACE_HVML)
                break;
        }
    }
    
    return tree->myhvml->insertion_func[tree->insert_mode](tree, token);
}

static bool myhvml_insertion_mode_in_foreign_content_start_other(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    myhvml_tree_node_t* adjusted_node = myhvml_tree_adjusted_current_node(tree);
    
    myhvml_token_node_wait_for_done(tree->token, token);
    
    if(adjusted_node->ns == MyHVML_NAMESPACE_MATHML) {
        myhvml_token_adjust_mathml_attributes(token);
    }
    else if(adjusted_node->ns == MyHVML_NAMESPACE_SVG) {
        myhvml_token_adjust_svg_attributes(token);
    }
    
    myhvml_token_adjust_foreign_attributes(token);
    
    myhvml_tree_node_t* node = myhvml_tree_node_insert_foreign_element(tree, token);
    node->ns = adjusted_node->ns;
    
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE_SELF)
    {
        if(token->tag_id == MyHVML_TAG_SCRIPT &&
           node->ns == MyHVML_NAMESPACE_SVG)
        {
            return myhvml_insertion_mode_in_foreign_content_end_other(tree, myhvml_tree_current_node(tree), token);
        }
        else {
            myhvml_tree_open_elements_pop(tree);
        }
    }
    
    return false;
}

static bool myhvml_insertion_mode_in_foreign_content(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(token->type & MyHVML_TOKEN_TYPE_CLOSE) {
        myhvml_tree_node_t* current_node = myhvml_tree_current_node(tree);
        
        if(token->tag_id == MyHVML_TAG_SCRIPT &&
           current_node->tag_id == MyHVML_TAG_SCRIPT &&
           current_node->ns == MyHVML_NAMESPACE_SVG)
        {
            myhvml_tree_open_elements_pop(tree);
            // TODO: now script is disable, skip this
            return false;
        }
        
        return myhvml_insertion_mode_in_foreign_content_end_other(tree, current_node, token);
    }
    else {
        switch (token->tag_id)
        {
            case MyHVML_TAG__TEXT:
            {
                if(token->type & MyHVML_TOKEN_TYPE_NULL) {
                    // parse error
                    /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:NULL_CHAR LEVEL:ERROR */
                    
                    myhvml_token_node_wait_for_done(tree->token, token);
                    myhvml_token_set_replacement_character_for_null_token(tree, token);
                }
                
                myhvml_tree_node_insert_text(tree, token);
                
                if((token->type & MyHVML_TOKEN_TYPE_WHITESPACE) == 0)
                    tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_FRAMESET_OK);
                
                break;
            }
                
            case MyHVML_TAG__COMMENT:
                myhvml_tree_node_insert_comment(tree, token, NULL);
                break;
                
            case MyHVML_TAG__DOCTYPE: {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_WRONG_LOCATION ACTION:IGNORE LEVEL:ERROR */
                
                break;
            }
                
            case MyHVML_TAG_B:
            case MyHVML_TAG_BIG:
            case MyHVML_TAG_BLOCKQUOTE:
            case MyHVML_TAG_BODY:
            case MyHVML_TAG_BR:
            case MyHVML_TAG_CENTER:
            case MyHVML_TAG_CODE:
            case MyHVML_TAG_DD:
            case MyHVML_TAG_DIV:
            case MyHVML_TAG_DL:
            case MyHVML_TAG_DT:
            case MyHVML_TAG_EM:
            case MyHVML_TAG_EMBED:
            case MyHVML_TAG_H1:
            case MyHVML_TAG_H2:
            case MyHVML_TAG_H3:
            case MyHVML_TAG_H4:
            case MyHVML_TAG_H5:
            case MyHVML_TAG_H6:
            case MyHVML_TAG_HEAD:
            case MyHVML_TAG_HR:
            case MyHVML_TAG_I:
            case MyHVML_TAG_IMG:
            case MyHVML_TAG_LI:
            case MyHVML_TAG_LISTING:
            case MyHVML_TAG_MENU:
            case MyHVML_TAG_META:
            case MyHVML_TAG_NOBR:
            case MyHVML_TAG_OL:
            case MyHVML_TAG_P:
            case MyHVML_TAG_PRE:
            case MyHVML_TAG_RUBY:
            case MyHVML_TAG_S:
            case MyHVML_TAG_SMALL:
            case MyHVML_TAG_SPAN:
            case MyHVML_TAG_STRONG:
            case MyHVML_TAG_STRIKE:
            case MyHVML_TAG_SUB:
            case MyHVML_TAG_SUP:
            case MyHVML_TAG_TABLE:
            case MyHVML_TAG_TT:
            case MyHVML_TAG_U:
            case MyHVML_TAG_UL:
            case MyHVML_TAG_VAR:
            case MyHVML_TAG_FONT:
            {
                // parse error
                /* %EXTERNAL% VALIDATOR:RULES TOKEN STATUS:ELEMENT_NO_EXPECTED LEVEL:ERROR */
                
                if(token->tag_id == MyHVML_TAG_FONT)
                {
                    myhvml_token_node_wait_for_done(tree->token, token);
                    
                    if(myhvml_token_attr_by_name(token, "color", 5) == NULL &&
                       myhvml_token_attr_by_name(token, "face" , 4) == NULL &&
                       myhvml_token_attr_by_name(token, "size" , 4) == NULL)
                    {
                        return myhvml_insertion_mode_in_foreign_content_start_other(tree, token);
                    }
                }
                
                if(tree->fragment == NULL) {
                    myhvml_tree_node_t* current_node;
                    
                    do {
                        myhvml_tree_open_elements_pop(tree);
                        current_node = myhvml_tree_current_node(tree);
                    }
                    while(current_node && !(myhvml_tree_is_mathml_integration_point(tree, current_node) ||
                                            myhvml_tree_is_hvml_integration_point(tree, current_node) ||
                                            current_node->ns == MyHVML_NAMESPACE_HVML));
                    
                    return true;
                }
            }
                
            default:
                return myhvml_insertion_mode_in_foreign_content_start_other(tree, token);
        }
    }
    
    return false;
}

void myhvml_rules_stop_parsing(myhvml_tree_t* tree)
{
    // THIS! IS! -(SPARTA!)- STOP PARSING
}

bool myhvml_rules_check_for_first_newline(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    if(tree->flags & MyHVML_TREE_FLAGS_PARSE_FLAG) {
        if(tree->flags &MyHVML_TREE_FLAGS_PARSE_FLAG_EMIT_NEWLINE)
        {
            if(token->tag_id == MyHVML_TAG__TEXT) {
                myhvml_token_node_wait_for_done(tree->token, token);
                
                if(token->str.length > 0) {
                    if(token->str.data[0] == '\n') {
                        token->str.data = mchar_async_crop_first_chars_without_cache(token->str.data, 1);
                        
                        token->str.length--;
                        
                        if(token->str.length == 0) {
                            tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_PARSE_FLAG);
                            return true;
                        }
                    }
                }
                else
                    return true;
            }
        }
        
        tree->flags ^= (tree->flags & MyHVML_TREE_FLAGS_PARSE_FLAG);
    }
    
    return false;
}

bool myhvml_rules_tree_dispatcher(myhvml_tree_t* tree, myhvml_token_node_t* token)
{
    // for textarea && pre && listen
    if(myhvml_rules_check_for_first_newline(tree, token)) {
        tree->token_last_done = token;
        
        return false;
    }
    
    if(tree->state_of_builder != MyHVML_TOKENIZER_STATE_DATA)
        tree->state_of_builder = MyHVML_TOKENIZER_STATE_DATA;
    
    bool reprocess = false;
    myhvml_tree_node_t* adjusted_node = myhvml_tree_adjusted_current_node(tree);
    
    if(tree->open_elements->length == 0 || adjusted_node->ns == MyHVML_NAMESPACE_HVML) {
        reprocess = tree->myhvml->insertion_func[tree->insert_mode](tree, token);
    }
    else if(myhvml_tree_is_mathml_integration_point(tree, adjusted_node) &&
            ((token->tag_id == MyHVML_TAG__TEXT ||
              (token->tag_id != MyHVML_TAG_MGLYPH && token->tag_id != MyHVML_TAG_MALIGNMARK)) &&
             (token->type & MyHVML_TOKEN_TYPE_CLOSE) == 0))
    {
            reprocess = tree->myhvml->insertion_func[tree->insert_mode](tree, token);
    }
    else if(adjusted_node->tag_id == MyHVML_TAG_ANNOTATION_XML &&
       adjusted_node->ns == MyHVML_NAMESPACE_MATHML &&
       token->tag_id == MyHVML_TAG_SVG && (token->type & MyHVML_TOKEN_TYPE_CLOSE) == 0)
    {
        reprocess = tree->myhvml->insertion_func[tree->insert_mode](tree, token);
    }
    else if(myhvml_tree_is_hvml_integration_point(tree, adjusted_node) &&
            ((token->type & MyHVML_TOKEN_TYPE_CLOSE) == 0 || token->tag_id == MyHVML_TAG__TEXT))
    {
        reprocess = tree->myhvml->insertion_func[tree->insert_mode](tree, token);
    }
    else if(token->tag_id == MyHVML_TAG__END_OF_FILE)
        reprocess = tree->myhvml->insertion_func[tree->insert_mode](tree, token);
    else
        reprocess = myhvml_insertion_mode_in_foreign_content(tree, token);
    
    if(reprocess == false) {
        tree->token_last_done = token;
    }
    
    return reprocess;
}

#endif /* TODO: VW */

mystatus_t myhvml_rules_init(myhvml_t* myhvml)
{
    myhvml->insertion_func = (myhvml_insertion_f*)mycore_malloc(sizeof(myhvml_insertion_f) * MyHVML_INSERTION_MODE_LAST_ENTRY);
    
    if(myhvml->insertion_func == NULL)
        return MyHVML_STATUS_RULES_ERROR_MEMORY_ALLOCATION;
    
    myhvml->insertion_func[MyHVML_INSERTION_MODE_INITIAL] = myhvml_insertion_mode_initial;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_BEFORE_HVML] = myhvml_insertion_mode_before_hvml;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_BEFORE_HEAD] = myhvml_insertion_mode_before_head;
#if 0 /* TODO: VW */
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_HEAD] = myhvml_insertion_mode_in_head;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_HEAD_NOSCRIPT] = myhvml_insertion_mode_in_head_noscript;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_AFTER_HEAD] = myhvml_insertion_mode_after_head;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_BODY] = myhvml_insertion_mode_in_body;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_TEXT] = myhvml_insertion_mode_text;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_TABLE] = myhvml_insertion_mode_in_table;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_TABLE_TEXT] = myhvml_insertion_mode_in_table_text;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_CAPTION] = myhvml_insertion_mode_in_caption;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_COLUMN_GROUP] = myhvml_insertion_mode_in_column_group;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_TABLE_BODY] = myhvml_insertion_mode_in_table_body;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_ROW] = myhvml_insertion_mode_in_row;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_CELL] = myhvml_insertion_mode_in_cell;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_SELECT] = myhvml_insertion_mode_in_select;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_SELECT_IN_TABLE] = myhvml_insertion_mode_in_select_in_table;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_TEMPLATE] = myhvml_insertion_mode_in_template;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_AFTER_BODY] = myhvml_insertion_mode_after_body;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_IN_FRAMESET] = myhvml_insertion_mode_in_frameset;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_AFTER_FRAMESET] = myhvml_insertion_mode_after_frameset;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_AFTER_AFTER_BODY] = myhvml_insertion_mode_after_after_body;
    myhvml->insertion_func[MyHVML_INSERTION_MODE_AFTER_AFTER_FRAMESET] = myhvml_insertion_mode_after_after_frameset;
#endif /* TODO: VW */
    
    return MyHVML_STATUS_OK;
}


