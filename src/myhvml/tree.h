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

#ifndef MyHVML_TREE_H
#define MyHVML_TREE_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "myosi.h"
#include "myhvml_internals.h"
#include "mystring.h"
#include "token.h"
#include "stream.h"
#include "mycore/thread_queue.h"
#include "mycore/utils/mcsync.h"
#include "mycore/utils/mchar_async.h"
#include "mycore/utils/mcobject.h"
#include "mycore/utils/mcobject_async.h"

#define myhvml_tree_get(tree, attr) tree->attr
#define myhvml_tree_set(tree, attr) tree->attr

#define myhvml_tree_token_current(tree) myhvml_tree_get(tree, token_current)
#define myhvml_tree_token_attr_current(tree) myhvml_tree_get(tree, attr_current)

#define myhvml_tree_node_get(tree, node_id, attr) tree->nodes[node_id].attr

#define myhvml_tree_node_callback_insert(tree, node) \
    if(tree->callback_tree_node_insert) \
        tree->callback_tree_node_insert(tree, node, tree->callback_tree_node_insert_ctx)

#define myhvml_tree_node_callback_remove(tree, node) \
    if(tree->callback_tree_node_remove) \
        tree->callback_tree_node_remove(tree, node, tree->callback_tree_node_remove_ctx)

enum myhvml_tree_node_type {
    MyHVML_TYPE_NONE    = 0,
    MyHVML_TYPE_BLOCK   = 1,
    MyHVML_TYPE_INLINE  = 2,
    MyHVML_TYPE_TABLE   = 3,
    MyHVML_TYPE_META    = 4,
    MyHVML_TYPE_COMMENT = 5
};

enum myhvml_close_type {
    MyHVML_CLOSE_TYPE_NONE  = 0,
    MyHVML_CLOSE_TYPE_NOW   = 1,
    MyHVML_CLOSE_TYPE_SELF  = 2,
    MyHVML_CLOSE_TYPE_BLOCK = 3
};

enum myhvml_tree_node_flags {
    MyHVML_TREE_NODE_UNDEF           = 0,
    MyHVML_TREE_NODE_PARSER_INSERTED = 1,
    MyHVML_TREE_NODE_BLOCKING        = 2
};

struct myhvml_tree_node {
    enum myhvml_tree_node_flags flags;
    
    myhvml_tag_id_t tag_id;
    enum myhvml_namespace ns;
    
    myhvml_tree_node_t* prev;
    myhvml_tree_node_t* next;
    myhvml_tree_node_t* child;
    myhvml_tree_node_t* parent;
    
    myhvml_tree_node_t* last_child;
    
    myhvml_token_node_t* token;
    void* data;
    
    myhvml_tree_t* tree;
};

enum myhvml_tree_compat_mode {
    MyHVML_TREE_COMPAT_MODE_NO_QUIRKS       = 0x00,
    MyHVML_TREE_COMPAT_MODE_QUIRKS          = 0x01,
    MyHVML_TREE_COMPAT_MODE_LIMITED_QUIRKS  = 0x02
};

enum myhvml_tree_doctype_id {
    MyHVML_TREE_DOCTYPE_ID_NAME   = 0x00,
    MyHVML_TREE_DOCTYPE_ID_SYSTEM = 0x01,
    MyHVML_TREE_DOCTYPE_ID_PUBLIC = 0x02
};

enum myhvml_tree_insertion_mode {
    MyHVML_TREE_INSERTION_MODE_DEFAULT     = 0x00,
    MyHVML_TREE_INSERTION_MODE_BEFORE      = 0x01,
    MyHVML_TREE_INSERTION_MODE_AFTER       = 0x02
};

struct myhvml_async_args {
    size_t mchar_node_id;
};

struct myhvml_tree_doctype {
    bool is_hvml;
    char* attr_name;
    char* attr_public;
    char* attr_system;
};

struct myhvml_tree_list {
    myhvml_tree_node_t** list;
    volatile size_t length;
    size_t size;
};

struct myhvml_tree_token_list {
    myhvml_token_node_t** list;
    size_t length;
    size_t size;
};

struct myhvml_tree_insertion_list {
    enum myhvml_insertion_mode* list;
    size_t length;
    size_t size;
};

struct myhvml_tree_temp_tag_name {
    char   *data;
    size_t  length;
    size_t  size;
};

struct myhvml_tree_special_token {
    myhvml_token_node_t *token;
    myhvml_namespace_t ns;
}
typedef myhvml_tree_special_token_t;

struct myhvml_tree_special_token_list {
    myhvml_tree_special_token_t *list;
    size_t  length;
    size_t  size;
}
typedef myhvml_tree_special_token_list_t;

struct myhvml_tree_temp_stream {
    struct myhvml_tree_temp_tag_name** data;
    size_t length;
    size_t size;
    
    myencoding_result_t res;
    struct myhvml_tree_temp_tag_name* current;
};

struct myhvml_tree {
    // ref
    myhvml_t*                    myhvml;
    mchar_async_t*               mchar;
    myhvml_token_t*              token;
    mcobject_async_t*            tree_obj;
    mcsync_t*                    sync;
    mythread_queue_list_entry_t* queue_entry;
    mythread_queue_t*            queue;
    myhvml_tag_t*                tags;
    void*                        modest;
    void*                        context;
    
    // init id's
    size_t                  mcasync_rules_token_id;
    size_t                  mcasync_rules_attr_id;
    size_t                  mcasync_tree_id;
    /* 
     * mchar_node_id
     * for rules, or if single mode,
     * or for main thread only after parsing
     */
    size_t                  mchar_node_id;
    myhvml_token_attr_t*    attr_current;
    myhvml_tag_id_t         tmp_tag_id;
    myhvml_token_node_t*    current_token_node;
    mythread_queue_node_t*  current_qnode;
    
    mcobject_t*                mcobject_incoming_buf;
    mycore_incoming_buffer_t*  incoming_buf;
    mycore_incoming_buffer_t*  incoming_buf_first;
    
    // ref for nodes
    myhvml_tree_node_t*   document;
    myhvml_tree_node_t*   fragment;
    myhvml_tree_node_t*   node_head;
    myhvml_tree_node_t*   node_hvml;
    myhvml_tree_node_t*   node_body;
    myhvml_tree_node_t*   node_form;
    myhvml_tree_doctype_t doctype;
    
    // for build tree
    myhvml_tree_list_t*           active_formatting;
    myhvml_tree_list_t*           open_elements;
    myhvml_tree_list_t*           other_elements;
    myhvml_tree_token_list_t*     token_list;
    myhvml_tree_insertion_list_t* template_insertion;
    myhvml_async_args_t*          async_args;
    myhvml_stream_buffer_t*       stream_buffer;
    myhvml_token_node_t* volatile token_last_done;
    
    // for detect namespace out of tree builder
    myhvml_token_node_t*          token_namespace;
    
    // tree params
    enum myhvml_tokenizer_state        state;
    enum myhvml_tokenizer_state        state_of_builder;
    enum myhvml_insertion_mode         insert_mode;
    enum myhvml_insertion_mode         orig_insert_mode;
    enum myhvml_tree_compat_mode       compat_mode;
    volatile enum myhvml_tree_flags    flags;
    volatile myhvml_tree_parse_flags_t parse_flags;
    bool                               foster_parenting;
    size_t                             global_offset;
    mystatus_t                         tokenizer_status;
    
    myencoding_t            encoding;
    myencoding_t            encoding_usereq;
    myhvml_tree_temp_tag_name_t  temp_tag_name;
    
    /* callback */
    myhvml_callback_token_f callback_before_token;
    myhvml_callback_token_f callback_after_token;
    
    void* callback_before_token_ctx;
    void* callback_after_token_ctx;
    
    myhvml_callback_tree_node_f callback_tree_node_insert;
    myhvml_callback_tree_node_f callback_tree_node_remove;
    
    void* callback_tree_node_insert_ctx;
    void* callback_tree_node_remove_ctx;
};

// base
myhvml_tree_t * myhvml_tree_create(void);
mystatus_t myhvml_tree_init(myhvml_tree_t* tree, myhvml_t* myhvml);
void myhvml_tree_clean(myhvml_tree_t* tree);
void myhvml_tree_clean_all(myhvml_tree_t* tree);
myhvml_tree_t * myhvml_tree_destroy(myhvml_tree_t* tree);

/* parse flags */
myhvml_tree_parse_flags_t myhvml_tree_parse_flags(myhvml_tree_t* tree);
void myhvml_tree_parse_flags_set(myhvml_tree_t* tree, myhvml_tree_parse_flags_t flags);

myhvml_t * myhvml_tree_get_myhvml(myhvml_tree_t* tree);
myhvml_tag_t * myhvml_tree_get_tag(myhvml_tree_t* tree);
myhvml_tree_node_t * myhvml_tree_get_document(myhvml_tree_t* tree);
myhvml_tree_node_t * myhvml_tree_get_node_hvml(myhvml_tree_t* tree);
myhvml_tree_node_t * myhvml_tree_get_node_head(myhvml_tree_t* tree);
myhvml_tree_node_t * myhvml_tree_get_node_body(myhvml_tree_t* tree);

mchar_async_t * myhvml_tree_get_mchar(myhvml_tree_t* tree);
size_t myhvml_tree_get_mchar_node_id(myhvml_tree_t* tree);

// list
myhvml_tree_list_t * myhvml_tree_list_init(void);
void myhvml_tree_list_clean(myhvml_tree_list_t* list);
myhvml_tree_list_t * myhvml_tree_list_destroy(myhvml_tree_list_t* list, bool destroy_self);

void myhvml_tree_list_append(myhvml_tree_list_t* list, myhvml_tree_node_t* node);
void myhvml_tree_list_append_after_index(myhvml_tree_list_t* list, myhvml_tree_node_t* node, size_t index);
void myhvml_tree_list_insert_by_index(myhvml_tree_list_t* list, myhvml_tree_node_t* node, size_t index);
myhvml_tree_node_t * myhvml_tree_list_current_node(myhvml_tree_list_t* list);

// token list
myhvml_tree_token_list_t * myhvml_tree_token_list_init(void);
void myhvml_tree_token_list_clean(myhvml_tree_token_list_t* list);
myhvml_tree_token_list_t * myhvml_tree_token_list_destroy(myhvml_tree_token_list_t* list, bool destroy_self);

void myhvml_tree_token_list_append(myhvml_tree_token_list_t* list, myhvml_token_node_t* token);
void myhvml_tree_token_list_append_after_index(myhvml_tree_token_list_t* list, myhvml_token_node_t* token, size_t index);
myhvml_token_node_t * myhvml_tree_token_list_current_node(myhvml_tree_token_list_t* list);

// active formatting
myhvml_tree_list_t * myhvml_tree_active_formatting_init(myhvml_tree_t* tree);
void myhvml_tree_active_formatting_clean(myhvml_tree_t* tree);
myhvml_tree_list_t * myhvml_tree_active_formatting_destroy(myhvml_tree_t* tree);

bool myhvml_tree_active_formatting_is_marker(myhvml_tree_t* tree, myhvml_tree_node_t* idx);
myhvml_tree_node_t* myhvml_tree_active_formatting_between_last_marker(myhvml_tree_t* tree, myhvml_tag_id_t tag_idx, size_t* return_idx);

void myhvml_tree_active_formatting_append(myhvml_tree_t* tree, myhvml_tree_node_t* node);
void myhvml_tree_active_formatting_append_with_check(myhvml_tree_t* tree, myhvml_tree_node_t* node);
void myhvml_tree_active_formatting_pop(myhvml_tree_t* tree);
void myhvml_tree_active_formatting_remove(myhvml_tree_t* tree, myhvml_tree_node_t* node);
void myhvml_tree_active_formatting_remove_by_index(myhvml_tree_t* tree, size_t idx);

void myhvml_tree_active_formatting_reconstruction(myhvml_tree_t* tree);
void myhvml_tree_active_formatting_up_to_last_marker(myhvml_tree_t* tree);

bool myhvml_tree_active_formatting_find(myhvml_tree_t* tree, myhvml_tree_node_t* idx, size_t* return_idx);
myhvml_tree_node_t* myhvml_tree_active_formatting_current_node(myhvml_tree_t* tree);

// open elements
myhvml_tree_list_t * myhvml_tree_open_elements_init(myhvml_tree_t* tree);
void myhvml_tree_open_elements_clean(myhvml_tree_t* tree);
myhvml_tree_list_t * myhvml_tree_open_elements_destroy(myhvml_tree_t* tree);

myhvml_tree_node_t* myhvml_tree_current_node(myhvml_tree_t* tree);
myhvml_tree_node_t * myhvml_tree_adjusted_current_node(myhvml_tree_t* tree);

void myhvml_tree_open_elements_append(myhvml_tree_t* tree, myhvml_tree_node_t* node);
void myhvml_tree_open_elements_append_after_index(myhvml_tree_t* tree, myhvml_tree_node_t* node, size_t index);
void myhvml_tree_open_elements_pop(myhvml_tree_t* tree);
void myhvml_tree_open_elements_pop_until(myhvml_tree_t* tree, myhvml_tag_id_t tag_idx, myhvml_namespace_t mynamespace, bool is_exclude);
void myhvml_tree_open_elements_pop_until_by_node(myhvml_tree_t* tree, myhvml_tree_node_t* node_idx, bool is_exclude);
void myhvml_tree_open_elements_pop_until_by_index(myhvml_tree_t* tree, size_t idx, bool is_exclude);
void myhvml_tree_open_elements_remove(myhvml_tree_t* tree, myhvml_tree_node_t* node);

bool myhvml_tree_open_elements_find(myhvml_tree_t* tree, myhvml_tree_node_t* idx, size_t* pos);
bool myhvml_tree_open_elements_find_reverse(myhvml_tree_t* tree, myhvml_tree_node_t* idx, size_t* pos);
myhvml_tree_node_t * myhvml_tree_open_elements_find_by_tag_idx(myhvml_tree_t* tree, myhvml_tag_id_t tag_idx, myhvml_namespace_t mynamespace, size_t* return_index);
myhvml_tree_node_t * myhvml_tree_open_elements_find_by_tag_idx_reverse(myhvml_tree_t* tree, myhvml_tag_id_t tag_idx, myhvml_namespace_t mynamespace, size_t* return_index);
myhvml_tree_node_t * myhvml_tree_element_in_scope(myhvml_tree_t* tree, myhvml_tag_id_t tag_idx, myhvml_namespace_t mynamespace, enum myhvml_tag_categories category);
bool myhvml_tree_element_in_scope_by_node(myhvml_tree_node_t* node, enum myhvml_tag_categories category);
void myhvml_tree_generate_implied_end_tags(myhvml_tree_t* tree, myhvml_tag_id_t exclude_tag_idx, myhvml_namespace_t mynamespace);
void myhvml_tree_generate_all_implied_end_tags(myhvml_tree_t* tree, myhvml_tag_id_t exclude_tag_idx, myhvml_namespace_t mynamespace);
myhvml_tree_node_t * myhvml_tree_appropriate_place_inserting(myhvml_tree_t* tree, myhvml_tree_node_t* override_target, enum myhvml_tree_insertion_mode* mode);
myhvml_tree_node_t * myhvml_tree_appropriate_place_inserting_in_tree(myhvml_tree_node_t* target, enum myhvml_tree_insertion_mode* mode);

// template insertion
myhvml_tree_insertion_list_t * myhvml_tree_template_insertion_init(myhvml_tree_t* tree);
void myhvml_tree_template_insertion_clean(myhvml_tree_t* tree);
myhvml_tree_insertion_list_t * myhvml_tree_template_insertion_destroy(myhvml_tree_t* tree);

void myhvml_tree_template_insertion_append(myhvml_tree_t* tree, enum myhvml_insertion_mode insert_mode);
void myhvml_tree_template_insertion_pop(myhvml_tree_t* tree);

void myhvml_tree_reset_insertion_mode_appropriately(myhvml_tree_t* tree);

bool myhvml_tree_adoption_agency_algorithm(myhvml_tree_t* tree, myhvml_token_node_t* token, myhvml_tag_id_t subject_tag_idx);
size_t myhvml_tree_template_insertion_length(myhvml_tree_t* tree);

// other for a tree
myhvml_tree_node_t * myhvml_tree_node_create(myhvml_tree_t* tree);
void myhvml_tree_node_delete(myhvml_tree_node_t* node);
void myhvml_tree_node_delete_recursive(myhvml_tree_node_t* node);
void myhvml_tree_node_clean(myhvml_tree_node_t* tree_node);
void myhvml_tree_node_free(myhvml_tree_node_t* node);
myhvml_tree_node_t * myhvml_tree_node_clone(myhvml_tree_node_t* node);

void myhvml_tree_node_add_child(myhvml_tree_node_t* root, myhvml_tree_node_t* node);
void myhvml_tree_node_insert_before(myhvml_tree_node_t* root, myhvml_tree_node_t* node);
void myhvml_tree_node_insert_after(myhvml_tree_node_t* root, myhvml_tree_node_t* node);
void myhvml_tree_node_insert_by_mode(myhvml_tree_node_t* adjusted_location, myhvml_tree_node_t* node, enum myhvml_tree_insertion_mode mode);
myhvml_tree_node_t * myhvml_tree_node_remove(myhvml_tree_node_t* node);

myhvml_tree_node_t * myhvml_tree_node_insert_hvml_element(myhvml_tree_t* tree, myhvml_token_node_t* token);
myhvml_tree_node_t * myhvml_tree_node_insert_foreign_element(myhvml_tree_t* tree, myhvml_token_node_t* token);
myhvml_tree_node_t * myhvml_tree_node_insert_by_token(myhvml_tree_t* tree, myhvml_token_node_t* token, myhvml_namespace_t ns);
myhvml_tree_node_t * myhvml_tree_node_insert(myhvml_tree_t* tree, myhvml_tag_id_t tag_idx, myhvml_namespace_t ns);
myhvml_tree_node_t * myhvml_tree_node_insert_by_node(myhvml_tree_t* tree, myhvml_tree_node_t* idx);
myhvml_tree_node_t * myhvml_tree_node_insert_comment(myhvml_tree_t* tree, myhvml_token_node_t* token, myhvml_tree_node_t* parent);
myhvml_tree_node_t * myhvml_tree_node_insert_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token);
myhvml_tree_node_t * myhvml_tree_node_insert_root(myhvml_tree_t* tree, myhvml_token_node_t* token, myhvml_namespace_t ns);
myhvml_tree_node_t * myhvml_tree_node_insert_text(myhvml_tree_t* tree, myhvml_token_node_t* token);
myhvml_tree_node_t * myhvml_tree_node_find_parent_by_tag_id(myhvml_tree_node_t* node, myhvml_tag_id_t tag_id);

// other
void myhvml_tree_wait_for_last_done_token(myhvml_tree_t* tree, myhvml_token_node_t* token_for_wait);

void myhvml_tree_tags_close_p(myhvml_tree_t* tree, myhvml_token_node_t* token);
myhvml_tree_node_t * myhvml_tree_generic_raw_text_element_parsing_algorithm(myhvml_tree_t* tree, myhvml_token_node_t* token_node);
void myhvml_tree_clear_stack_back_table_context(myhvml_tree_t* tree);
void myhvml_tree_clear_stack_back_table_body_context(myhvml_tree_t* tree);
void myhvml_tree_clear_stack_back_table_row_context(myhvml_tree_t* tree);
void myhvml_tree_close_cell(myhvml_tree_t* tree, myhvml_tree_node_t* tr_or_th_node, myhvml_token_node_t* token);

bool myhvml_tree_is_mathml_integration_point(myhvml_tree_t* tree, myhvml_tree_node_t* node);
bool myhvml_tree_is_hvml_integration_point(myhvml_tree_t* tree, myhvml_tree_node_t* node);

// temp tag name
mystatus_t myhvml_tree_temp_tag_name_init(myhvml_tree_temp_tag_name_t* temp_tag_name);
void myhvml_tree_temp_tag_name_clean(myhvml_tree_temp_tag_name_t* temp_tag_name);
myhvml_tree_temp_tag_name_t * myhvml_tree_temp_tag_name_destroy(myhvml_tree_temp_tag_name_t* temp_tag_name, bool self_destroy);
mystatus_t myhvml_tree_temp_tag_name_append(myhvml_tree_temp_tag_name_t* temp_tag_name, const char* name, size_t name_len);
mystatus_t myhvml_tree_temp_tag_name_append_one(myhvml_tree_temp_tag_name_t* temp_tag_name, const char name);

/* special tonek list */
mystatus_t myhvml_tree_special_list_init(myhvml_tree_special_token_list_t* special);
mystatus_t myhvml_tree_special_list_append(myhvml_tree_special_token_list_t* special, myhvml_token_node_t *token, myhvml_namespace_t ns);
size_t myhvml_tree_special_list_length(myhvml_tree_special_token_list_t* special);
myhvml_tree_special_token_t * myhvml_tree_special_list_get_last(myhvml_tree_special_token_list_t* special);
size_t myhvml_tree_special_list_pop(myhvml_tree_special_token_list_t* special);

/* incoming buffer */
mycore_incoming_buffer_t * myhvml_tree_incoming_buffer_first(myhvml_tree_t *tree);
const char * myhvml_tree_incomming_buffer_make_data(myhvml_tree_t *tree, size_t begin, size_t length);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_TREE_H */

