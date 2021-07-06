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

#ifndef MyHVML_TOKEN_H
#define MyHVML_TOKEN_H

#pragma once

#include <string.h>

#include "myosi.h"
#include "mycore/utils.h"
#include "tag.h"
#include "myhvml_internals.h"
#include "mystring.h"
#include "mycore/utils/mcobject_async.h"
#include "mycore/utils/mchar_async.h"
#include "mycore/utils/mcsync.h"

#define myhvml_token_node_set_done(token_node) token_node->type |= MyHVML_TOKEN_TYPE_DONE

struct myhvml_token_replacement_entry {
    char* from;
    size_t from_size;
    
    char* to;
    size_t to_size;
};

struct myhvml_token_namespace_replacement {
    char* from;
    size_t from_size;
    
    char* to;
    size_t to_size;
    
    enum myhvml_namespace ns;
};

struct myhvml_token_attr {
    myhvml_token_attr_t* next;
    myhvml_token_attr_t* prev;
    
    mycore_string_t key;
    mycore_string_t value;
    
    size_t raw_key_begin;
    size_t raw_key_length;
    size_t raw_value_begin;
    size_t raw_value_length;
    
    enum myhvml_namespace ns;
};

struct myhvml_token_node {
    myhvml_tag_id_t tag_id;
    
    mycore_string_t str;
    
    size_t raw_begin;
    size_t raw_length;
    
    size_t element_begin;
    size_t element_length;
    
    myhvml_token_attr_t* attr_first;
    myhvml_token_attr_t* attr_last;
    
    volatile enum myhvml_token_type type;
};

struct myhvml_token {
    myhvml_tree_t* tree; // ref
    
    mcobject_async_t* nodes_obj; // myhvml_token_node_t
    mcobject_async_t* attr_obj;  // myhvml_token_attr_t
    
    // def thread node id
    size_t mcasync_token_id;
    size_t mcasync_attr_id;
    
    bool is_new_tmp;
};

#ifdef __cplusplus
extern "C" {
#endif

myhvml_token_t * myhvml_token_create(myhvml_tree_t* tree, size_t size);
void myhvml_token_clean(myhvml_token_t* token);
void myhvml_token_clean_all(myhvml_token_t* token);
myhvml_token_t * myhvml_token_destroy(myhvml_token_t* token);

myhvml_tag_id_t myhvml_token_node_tag_id(myhvml_token_node_t *token_node);
myhvml_position_t myhvml_token_node_raw_position(myhvml_token_node_t *token_node);
myhvml_position_t myhvml_token_node_element_position(myhvml_token_node_t *token_node);

myhvml_tree_attr_t * myhvml_token_node_attribute_first(myhvml_token_node_t *token_node);
myhvml_tree_attr_t * myhvml_token_node_attribute_last(myhvml_token_node_t *token_node);

const char * myhvml_token_node_text(myhvml_token_node_t *token_node, size_t *length);
mycore_string_t * myhvml_token_node_string(myhvml_token_node_t *token_node);

bool myhvml_token_node_is_close(myhvml_token_node_t *token_node);
bool myhvml_token_node_is_close_self(myhvml_token_node_t *token_node);

myhvml_token_node_t * myhvml_token_node_create(myhvml_token_t* token, size_t async_node_id);
void myhvml_token_node_clean(myhvml_token_node_t* node);

myhvml_token_attr_t * myhvml_token_attr_create(myhvml_token_t* token, size_t async_node_id);
void myhvml_token_attr_clean(myhvml_token_attr_t* attr);
myhvml_token_attr_t * myhvml_token_attr_remove(myhvml_token_node_t* node, myhvml_token_attr_t* attr);
myhvml_token_attr_t * myhvml_token_attr_remove_by_name(myhvml_token_node_t* node, const char* name, size_t name_length);
void myhvml_token_attr_delete_all(myhvml_token_t* token, myhvml_token_node_t* node);

void myhvml_token_delete(myhvml_token_t* token, myhvml_token_node_t* node);
void myhvml_token_node_wait_for_done(myhvml_token_t* token, myhvml_token_node_t* node);
void myhvml_token_set_done(myhvml_token_node_t* node);

myhvml_token_attr_t * myhvml_token_attr_match(myhvml_token_t* token, myhvml_token_node_t* target, const char* key, size_t key_size, const char* value, size_t value_size);
myhvml_token_attr_t * myhvml_token_attr_match_case(myhvml_token_t* token, myhvml_token_node_t* target, const char* key, size_t key_size, const char* value, size_t value_size);

bool myhvml_token_release_and_check_doctype_attributes(myhvml_token_t* token, myhvml_token_node_t* target, myhvml_tree_doctype_t* return_doctype);

void myhvml_token_adjust_mathml_attributes(myhvml_token_node_t* target);
void myhvml_token_adjust_svg_attributes(myhvml_token_node_t* target);
void myhvml_token_adjust_foreign_attributes(myhvml_token_node_t* target);

myhvml_token_attr_t * myhvml_token_node_attr_append(myhvml_token_t* token, myhvml_token_node_t* dest, const char* key, size_t key_len, const char* value, size_t value_len, size_t thread_idx);
myhvml_token_attr_t * myhvml_token_node_attr_append_with_convert_encoding(myhvml_token_t* token, myhvml_token_node_t* dest, const char* key, size_t key_len, const char* value, size_t value_len, size_t thread_idx, myencoding_t encoding);
void myhvml_token_node_text_append(myhvml_token_t* token, myhvml_token_node_t* dest, const char* text, size_t text_len);
void myhvml_token_node_attr_copy(myhvml_token_t* token, myhvml_token_node_t* target, myhvml_token_node_t* dest, size_t thread_idx);
void myhvml_token_node_attr_copy_with_check(myhvml_token_t* token, myhvml_token_node_t* target, myhvml_token_node_t* dest, size_t thread_idx);
myhvml_token_node_t * myhvml_token_node_clone(myhvml_token_t* token, myhvml_token_node_t* node, size_t token_thread_idx, size_t attr_thread_idx);
bool myhvml_token_attr_copy(myhvml_token_t* token, myhvml_token_attr_t* attr, myhvml_token_node_t* dest, size_t thread_idx);
myhvml_token_attr_t * myhvml_token_attr_by_name(myhvml_token_node_t* node, const char* name, size_t name_size);
bool myhvml_token_attr_compare(myhvml_token_node_t* target, myhvml_token_node_t* dest);
myhvml_token_node_t * myhvml_token_merged_two_token_string(myhvml_tree_t* tree, myhvml_token_node_t* token_to, myhvml_token_node_t* token_from, bool cp_reverse);
void myhvml_token_set_replacement_character_for_null_token(myhvml_tree_t* tree, myhvml_token_node_t* node);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_TOKEN_H */
