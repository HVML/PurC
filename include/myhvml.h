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

#ifndef PCAT2_MyHVML_H
#define PCAT2_MyHVML_H

#pragma once

/**
 * @file myhvml.h
 *
 * Fast C/C++ HVML 5 Parser.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "pcat2_version.h"
#include "pcat2_macros.h"
#include "myencoding.h"
#include "mycore.h"

/**
 * @struct basic tag ids
 */
enum myhvml_tags {
    MyHVML_TAG__UNDEF = 0x000,
    MyHVML_TAG__TEXT,
    MyHVML_TAG__COMMENT,
    MyHVML_TAG__DOCTYPE,
    MyHVML_TAG_ARCHDATA,
    MyHVML_TAG_ARCHTYPE,
    MyHVML_TAG_BACK,
    MyHVML_TAG_BODY,
    MyHVML_TAG_CALL,
    MyHVML_TAG_CATCH,
    MyHVML_TAG_CLOSE,
    MyHVML_TAG_DEFINE,
    MyHVML_TAG_EMPTY,
    MyHVML_TAG_ERROR,
    MyHVML_TAG_HEAD,
    MyHVML_TAG_HVML,
    MyHVML_TAG_INIT,
    MyHVML_TAG_INCLUDE,
    MyHVML_TAG_ITERATE,
    MyHVML_TAG_LISTEN,
    MyHVML_TAG_LOAD,
    MyHVML_TAG_MATCH,
    MyHVML_TAG_OBSERVE,
    MyHVML_TAG_REMOVE,
    MyHVML_TAG_REQUEST,
    MyHVML_TAG_REDUCE,
    MyHVML_TAG_RETURN,
    MyHVML_TAG_SET,
    MyHVML_TAG_TEST,
    MyHVML_TAG_UPDATE,
    MyHVML_TAG_FIRST_ENTRY = MyHVML_TAG__TEXT,
    MyHVML_TAG_LAST_ENTRY  = MyHVML_TAG_UPDATE,
};

/**
 * @struct myhvml statuses
 */
// base
/*
 Very important!!!
 
 for myhtml             0..00ffff;      MyHVML_STATUS_OK    == 0x000000
 for mycss and modules  010000..01ffff; MyCSS_STATUS_OK     == 0x000000
 for modest             020000..02ffff; MODEST_STATUS_OK    == 0x000000
 for myrender           030000..03ffff; MyRENDER_STATUS_OK  == 0x000000
 for mydom              040000..04ffff; MyDOM_STATUS_OK     == 0x000000
 for mynetwork          050000..05ffff; MyNETWORK_STATUS_OK == 0x000000
 for myecma             060000..06ffff; MyECMA_STATUS_OK    == 0x000000
 for myhvml             070000..07ffff; MyHVML_STATUS_OK    == 0x000000
 not occupied           080000..
*/
enum myhvml_status {
    MyHVML_STATUS_OK                                   = 0x00000,
    MyHVML_STATUS_ERROR                                = 0x00001,
    MyHVML_STATUS_ERROR_MEMORY_ALLOCATION              = 0x00002,
    MyHVML_STATUS_RULES_ERROR_MEMORY_ALLOCATION        = 0x09064,
    MyHVML_STATUS_TOKENIZER_ERROR_MEMORY_ALLOCATION    = 0x7912c,
    MyHVML_STATUS_TOKENIZER_ERROR_FRAGMENT_INIT,
    MyHVML_STATUS_TAGS_ERROR_MEMORY_ALLOCATION         = 0x79190,
    MyHVML_STATUS_TAGS_ERROR_MCOBJECT_CREATE,
    MyHVML_STATUS_TAGS_ERROR_MCOBJECT_MALLOC,
    MyHVML_STATUS_TAGS_ERROR_MCOBJECT_CREATE_NODE,
    MyHVML_STATUS_TAGS_ERROR_CACHE_MEMORY_ALLOCATION,
    MyHVML_STATUS_TAGS_ERROR_INDEX_MEMORY_ALLOCATION,
    MyHVML_STATUS_TREE_ERROR_MEMORY_ALLOCATION         = 0x791f4,
    MyHVML_STATUS_TREE_ERROR_MCOBJECT_CREATE,
    MyHVML_STATUS_TREE_ERROR_MCOBJECT_INIT,
    MyHVML_STATUS_TREE_ERROR_MCOBJECT_CREATE_NODE,
    MyHVML_STATUS_TREE_ERROR_INCOMING_BUFFER_CREATE,
    MyHVML_STATUS_ATTR_ERROR_ALLOCATION                = 0x79258,
    MyHVML_STATUS_ATTR_ERROR_CREATE,
    MyHVML_STATUS_STREAM_BUFFER_ERROR_CREATE           = 0x79300,
    MyHVML_STATUS_STREAM_BUFFER_ERROR_INIT,
    MyHVML_STATUS_STREAM_BUFFER_ENTRY_ERROR_CREATE,
    MyHVML_STATUS_STREAM_BUFFER_ENTRY_ERROR_INIT,
    MyHVML_STATUS_STREAM_BUFFER_ERROR_ADD_ENTRY,
}
typedef myhvml_status_t;

#define MyHVML_FAILED(_status_) ((_status_) != MyHVML_STATUS_OK)

/**
 * @struct myhvml namespace
 */
enum myhvml_namespace {
    MyHVML_NAMESPACE_UNDEF      = 0x00,
    MyHVML_NAMESPACE_HVML,

    /* MyHVML_NAMESPACE_ANY == MyHVML_NAMESPACE_LAST_ENTRY */
    MyHVML_NAMESPACE_ANY        = MyHVML_NAMESPACE_HVML,
    MyHVML_NAMESPACE_LAST_ENTRY = MyHVML_NAMESPACE_HVML,
}
typedef myhvml_namespace_t;

/**
 * @struct myhvml options
 */
enum myhvml_options {
    MyHVML_OPTIONS_DEFAULT                 = 0x00,
    MyHVML_OPTIONS_PARSE_MODE_SINGLE       = 0x01,
    MyHVML_OPTIONS_PARSE_MODE_ALL_IN_ONE   = 0x02,
    MyHVML_OPTIONS_PARSE_MODE_SEPARATELY   = 0x04
};

/**
 * @struct myhvml_tree parse flags
 */
enum myhvml_tree_parse_flags {
    MyHVML_TREE_PARSE_FLAGS_CLEAN                   = 0x000,
    MyHVML_TREE_PARSE_FLAGS_WITHOUT_BUILD_TREE      = 0x001,
    MyHVML_TREE_PARSE_FLAGS_WITHOUT_PROCESS_TOKEN   = 0x003,
    MyHVML_TREE_PARSE_FLAGS_SKIP_WHITESPACE_TOKEN   = 0x004, /* skip ws token, but not for RCDATA, RAWTEXT, CDATA and PLAINTEXT */
    MyHVML_TREE_PARSE_FLAGS_WITHOUT_DOCTYPE_IN_TREE = 0x008
}
typedef myhvml_tree_parse_flags_t;

/**
 * @struct myhvml_t MyHVML
 *
 * Basic structure. Create once for using many times.
*/
typedef struct myhvml myhvml_t;

/**
 * @struct myhvml_tree_t MyHVML_TREE
 *
 * Secondary structure. Create once for using many times.
 */
typedef struct myhvml_tree myhvml_tree_t;

/**
 * @struct myhvml_token_t MyHVML_TOKEN
 */
typedef struct myhvml_token myhvml_token_t;

typedef struct myhvml_token_attr myhvml_tree_attr_t;
typedef struct myhvml_tree_node myhvml_tree_node_t;

/**
 * MyHVML_TAG
 *
 */
typedef size_t myhvml_tag_id_t;
typedef struct myhvml_tag myhvml_tag_t;

/**
 * @struct myhvml_collection_t
 */
struct myhvml_collection {
    myhvml_tree_node_t **list;
    size_t size;
    size_t length;
}
typedef myhvml_collection_t;

/**
 * @struct myhvml_position_t
 */
struct myhvml_position {
    size_t begin;
    size_t length;
}
typedef myhvml_position_t;

/**
 * @struct myhvml_token_node_t
 */
typedef struct myhvml_token_node myhvml_token_node_t;

// callback functions
typedef void* (*myhvml_callback_token_f)(myhvml_tree_t* tree, myhvml_token_node_t* token, void* ctx);
typedef void (*myhvml_callback_tree_node_f)(myhvml_tree_t* tree, myhvml_tree_node_t* node, void* ctx);

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************
 *
 * MyHVML
 *
 ***********************************************************************************/

/**
 * Create a MyHVML structure
 *
 * @return myhvml_t* if successful, otherwise an NULL value.
 */
myhvml_t*
myhvml_create(void);

/**
 * Allocating and Initialization resources for a MyHVML structure
 *
 * @param[in] myhvml_t*
 * @param[in] work options, how many threads will be. 
 * Default: MyHVML_OPTIONS_PARSE_MODE_SEPARATELY
 *
 * @param[in] thread count, it depends on the choice of work options
 * Default: 1
 *
 * @param[in] queue size for a tokens. Dynamically increasing the specified number here. Default: 4096
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status value.
 */
mystatus_t
myhvml_init(myhvml_t* myhvml, enum myhvml_options opt,
            size_t thread_count, size_t queue_size);

/**
 * Clears queue and threads resources
 *
 * @param[in] myhvml_t*
 */
void
myhvml_clean(myhvml_t* myhvml);

/**
 * Destroy of a MyHVML structure
 *
 * @param[in] myhvml_t*
 * @return NULL if successful, otherwise an MyHVML structure.
 */
myhvml_t*
myhvml_destroy(myhvml_t* myhvml);

/**
 * Parsing HVML
 *
 * @param[in] previously created structure myhvml_tree_t*
 * @param[in] HVML
 * @param[in] HVML size
 *
 * The input character encoding must be utf-8
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse(myhvml_tree_t* tree, const char* hvml, size_t hvml_size);

/**
 * Parsing fragment of HVML
 *
 * @param[in] previously created structure myhvml_tree_t*
 * @param[in] HVML
 * @param[in] HVML size
 * @param[in] fragment base (root) tag id. Default: MyHVML_TAG_DIV if set 0
 * @param[in] fragment NAMESPACE. Default: MyHVML_NAMESPACE_HVML if set 0
 *
 * The input character encoding must be utf-8
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse_fragment(myhvml_tree_t* tree,
                      const char* hvml, size_t hvml_size,
                      myhvml_tag_id_t tag_id, enum myhvml_namespace ns);

/**
 * Parsing HVML in Single Mode. 
 * No matter what was said during initialization MyHVML
 *
 * @param[in] previously created structure myhvml_tree_t*
 * @param[in] HVML
 * @param[in] HVML size
 *
 * The input character encoding must be utf-8
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse_single(myhvml_tree_t* tree,
                    const char* hvml, size_t hvml_size);

/**
 * Parsing fragment of HVML in Single Mode. 
 * No matter what was said during initialization MyHVML
 *
 * @param[in] previously created structure myhvml_tree_t*
 * @param[in] HVML
 * @param[in] HVML size
 * @param[in] fragment base (root) tag id. Default: MyHVML_TAG_DIV if set 0
 * @param[in] fragment NAMESPACE. Default: MyHVML_NAMESPACE_HVML if set 0
 *
 * The input character encoding must be utf-8
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse_fragment_single(myhvml_tree_t* tree,
                             const char* hvml, size_t hvml_size,
                             myhvml_tag_id_t tag_id, enum myhvml_namespace ns);

/**
 * Parsing HVML chunk. For end parsing call myhvml_parse_chunk_end function
 *
 * @param[in] myhvml_tree_t*
 * @param[in] HVML
 * @param[in] HVML size
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse_chunk(myhvml_tree_t* tree, const char* hvml, size_t hvml_size);

/**
 * Parsing chunk of fragment HVML. For end parsing call myhvml_parse_chunk_end function
 *
 * @param[in] myhvml_tree_t*
 * @param[in] HVML
 * @param[in] HVML size
 * @param[in] fragment base (root) tag id. Default: MyHVML_TAG_DIV if set 0
 * @param[in] fragment NAMESPACE. Default: MyHVML_NAMESPACE_HVML if set 0
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse_chunk_fragment(myhvml_tree_t* tree, const char* hvml,size_t hvml_size,
                            myhvml_tag_id_t tag_id, enum myhvml_namespace ns);

/**
 * Parsing HVML chunk in Single Mode.
 * No matter what was said during initialization MyHVML
 *
 * @param[in] myhvml_tree_t*
 * @param[in] HVML
 * @param[in] HVML size
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse_chunk_single(myhvml_tree_t* tree, const char* hvml, size_t hvml_size);

/**
 * Parsing chunk of fragment of HVML in Single Mode.
 * No matter what was said during initialization MyHVML
 *
 * @param[in] myhvml_tree_t*
 * @param[in] HVML
 * @param[in] HVML size
 * @param[in] fragment base (root) tag id. Default: MyHVML_TAG_DIV if set 0
 * @param[in] fragment NAMESPACE. Default: MyHVML_NAMESPACE_HVML if set 0
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse_chunk_fragment_single(myhvml_tree_t* tree, const char* hvml, size_t hvml_size,
                                   myhvml_tag_id_t tag_id, enum myhvml_namespace ns);

/**
 * End of parsing HVML chunks
 *
 * @param[in] myhvml_tree_t*
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_parse_chunk_end(myhvml_tree_t* tree);

/***********************************************************************************
 *
 * MyHVML_TREE
 *
 ***********************************************************************************/

/**
 * Create a MyHVML_TREE structure
 *
 * @return myhvml_tree_t* if successful, otherwise an NULL value.
 */
myhvml_tree_t*
myhvml_tree_create(void);

/**
 * Allocating and Initialization resources for a MyHVML_TREE structure
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_t*
 *
 * @return MyHVML_STATUS_OK if successful, otherwise an error status
 */
mystatus_t
myhvml_tree_init(myhvml_tree_t* tree, myhvml_t* myhvml);

/**
 * Get Parse Flags of Tree
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_tree_parse_flags_t
 */
myhvml_tree_parse_flags_t
myhvml_tree_parse_flags(myhvml_tree_t* tree);

/**
 * Set Parse Flags for Tree
 * See enum myhvml_tree_parse_flags in this file
 *
 * @example myhvml_tree_parse_flags_set(tree, MyHVML_TREE_PARSE_FLAGS_WITHOUT_BUILD_TREE|
 *                                            MyHVML_TREE_PARSE_FLAGS_WITHOUT_DOCTYPE_IN_TREE|
 *                                            MyHVML_TREE_PARSE_FLAGS_SKIP_WHITESPACE_TOKEN);
 *
 * @param[in] myhvml_tree_t*
 * @param[in] parse flags. You can combine their
 */
void
myhvml_tree_parse_flags_set(myhvml_tree_t* tree, myhvml_tree_parse_flags_t parse_flags);

/**
 * Clears resources before new parsing
 *
 * @param[in] myhvml_tree_t*
 */
void
myhvml_tree_clean(myhvml_tree_t* tree);

/**
 * Add child node to node. If children already exists it will be added to the last
 *
 * @param[in] myhvml_tree_node_t* The node to which we add child node
 * @param[in] myhvml_tree_node_t* The node which adds
 */
void
myhvml_tree_node_add_child(myhvml_tree_node_t* root, myhvml_tree_node_t* node);

/**
 * Add a node immediately before the existing node
 *
 * @param[in] myhvml_tree_node_t* add for this node
 * @param[in] myhvml_tree_node_t* add this node
 */
void
myhvml_tree_node_insert_before(myhvml_tree_node_t* root, myhvml_tree_node_t* node);

/**
 * Add a node immediately after the existing node
 *
 * @param[in] myhvml_tree_node_t* add for this node
 * @param[in] myhvml_tree_node_t* add this node
 */
void
myhvml_tree_node_insert_after(myhvml_tree_node_t* root, myhvml_tree_node_t* node);

/**
 * Destroy of a MyHVML_TREE structure
 *
 * @param[in] myhvml_tree_t*
 *
 * @return NULL if successful, otherwise an MyHVML_TREE structure
 */
myhvml_tree_t*
myhvml_tree_destroy(myhvml_tree_t* tree);

/**
 * Get myhvml_t* from a myhvml_tree_t*
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_t* if exists, otherwise a NULL value
 */
myhvml_t*
myhvml_tree_get_myhvml(myhvml_tree_t* tree);

/**
 * Get myhvml_tag_t* from a myhvml_tree_t*
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_tag_t* if exists, otherwise a NULL value
 */
myhvml_tag_t*
myhvml_tree_get_tag(myhvml_tree_t* tree);

/**
 * Get Tree Document (Root of Tree)
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_tree_get_document(myhvml_tree_t* tree);

/**
 * Get node HVML (Document -> HVML, Root of HVML Document)
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_tree_get_node_hvml(myhvml_tree_t* tree);

/**
 * Get node HEAD (Document -> HVML -> HEAD)
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_tree_get_node_head(myhvml_tree_t* tree);

/**
 * Get node BODY (Document -> HVML -> BODY)
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_tree_get_node_body(myhvml_tree_t* tree);

/**
 * Get mchar_async_t object
 *
 * @param[in] myhvml_tree_t*
 *
 * @return mchar_async_t* if exists, otherwise a NULL value
 */
mchar_async_t*
myhvml_tree_get_mchar(myhvml_tree_t* tree);

/**
 * Get node_id from main thread for mchar_async_t object
 *
 * @param[in] myhvml_tree_t*
 *
 * @return size_t, node id
 */
size_t
myhvml_tree_get_mchar_node_id(myhvml_tree_t* tree);

/**
 * Get first Incoming Buffer
 *
 * @param[in] myhvml_tree_t*
 *
 * @return mycore_incoming_buffer_t* if successful, otherwise a NULL value
 */
mycore_incoming_buffer_t*
myhvml_tree_incoming_buffer_first(myhvml_tree_t *tree);

/***********************************************************************************
 *
 * MyHVML_NODE
 *
 ***********************************************************************************/

/**
 * Get first (begin) node of tree
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_node_first(myhvml_tree_t* tree);

/**
 * Get nodes by tag id
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, creates new collection if NULL
 * @param[in] tag id
 * @param[out] status of this operation
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_tag_id(myhvml_tree_t* tree, myhvml_collection_t *collection,
                           myhvml_tag_id_t tag_id, mystatus_t *status);

/**
 * Get nodes by tag name
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, creates new collection if NULL
 * @param[in] tag name
 * @param[in] tag name length
 * @param[out] status of this operation, optional
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_name(myhvml_tree_t* tree, myhvml_collection_t *collection,
                         const char* name, size_t length, mystatus_t *status);

/**
 * Get nodes by attribute key
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, optional; creates new collection if NULL
 * @param[in] myhvml_tree_node_t*, optional; scope node; hvml if NULL
 * @param[in] find key
 * @param[in] find key length
 * @param[out] status of this operation, optional
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_attribute_key(myhvml_tree_t *tree, myhvml_collection_t* collection,
                                  myhvml_tree_node_t* scope_node,
                                  const char* key, size_t key_len, mystatus_t* status);

/**
 * Get nodes by attribute value; exactly equal; like a [foo="bar"]
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, optional; creates new collection if NULL
 * @param[in] myhvml_tree_node_t*, optional; scope node; hvml if NULL
 * @param[in] case-insensitive if true
 * @param[in] find in key; if NULL find in all attributes
 * @param[in] find in key length; if 0 find in all attributes
 * @param[in] find value
 * @param[in] find value length
 * @param[out] status of this operation, optional
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_attribute_value(myhvml_tree_t *tree,
                                    myhvml_collection_t* collection,
                                    myhvml_tree_node_t* node,
                                    bool case_insensitive,
                                    const char* key, size_t key_len,
                                    const char* value, size_t value_len,
                                    mystatus_t* status);

/**
 * Get nodes by attribute value; whitespace separated; like a [foo~="bar"]
 *
 * @example if value="bar" and node attr value="lalala bar bebebe", then this node is found
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, optional; creates new collection if NULL
 * @param[in] myhvml_tree_node_t*, optional; scope node; hvml if NULL
 * @param[in] case-insensitive if true
 * @param[in] find in key; if NULL find in all attributes
 * @param[in] find in key length; if 0 find in all attributes
 * @param[in] find value
 * @param[in] find value length
 * @param[out] status of this operation, optional
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_attribute_value_whitespace_separated(myhvml_tree_t *tree,
                                                         myhvml_collection_t* collection,
                                                         myhvml_tree_node_t* node,
                                                         bool case_insensitive,
                                                         const char* key, size_t key_len,
                                                         const char* value, size_t value_len,
                                                         mystatus_t* status);

/**
 * Get nodes by attribute value; value begins exactly with the string; like a [foo^="bar"]
 *
 * @example if value="bar" and node attr value="barmumumu", then this node is found
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, optional; creates new collection if NULL
 * @param[in] myhvml_tree_node_t*, optional; scope node; hvml if NULL
 * @param[in] case-insensitive if true
 * @param[in] find in key; if NULL find in all attributes
 * @param[in] find in key length; if 0 find in all attributes
 * @param[in] find value
 * @param[in] find value length
 * @param[out] status of this operation, optional
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_attribute_value_begin(myhvml_tree_t *tree,
                                          myhvml_collection_t* collection,
                                          myhvml_tree_node_t* node,
                                          bool case_insensitive,
                                          const char* key, size_t key_len,
                                          const char* value, size_t value_len,
                                          mystatus_t* status);


/**
 * Get nodes by attribute value; value ends exactly with the string; like a [foo$="bar"]
 *
 * @example if value="bar" and node attr value="mumumubar", then this node is found
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, optional; creates new collection if NULL
 * @param[in] myhvml_tree_node_t*, optional; scope node; hvml if NULL
 * @param[in] case-insensitive if true
 * @param[in] find in key; if NULL find in all attributes
 * @param[in] find in key length; if 0 find in all attributes
 * @param[in] find value
 * @param[in] find value length
 * @param[out] status of this operation, optional
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_attribute_value_end(myhvml_tree_t *tree,
                                        myhvml_collection_t* collection,
                                        myhvml_tree_node_t* node,
                                        bool case_insensitive,
                                        const char* key, size_t key_len,
                                        const char* value, size_t value_len,
                                        mystatus_t* status);

/**
 * Get nodes by attribute value; value contains the substring; like a [foo*="bar"]
 *
 * @example if value="bar" and node attr value="bububarmumu", then this node is found
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, optional; creates new collection if NULL
 * @param[in] myhvml_tree_node_t*, optional; scope node; hvml if NULL
 * @param[in] case-insensitive if true
 * @param[in] find in key; if NULL find in all attributes
 * @param[in] find in key length; if 0 find in all attributes
 * @param[in] find value
 * @param[in] find value length
 * @param[out] status of this operation, optional
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_attribute_value_contain(myhvml_tree_t *tree,
                                            myhvml_collection_t* collection,
                                            myhvml_tree_node_t* node,
                                            bool case_insensitive,
                                            const char* key, size_t key_len,
                                            const char* value, size_t value_len,
                                            mystatus_t* status);

/**
 * Get nodes by attribute value; attribute value is a hyphen-separated list of values beginning; 
 * like a [foo|="bar"]
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, optional; creates new collection if NULL
 * @param[in] myhvml_tree_node_t*, optional; scope node; hvml if NULL
 * @param[in] case-insensitive if true
 * @param[in] find in key; if NULL find in all attributes
 * @param[in] find in key length; if 0 find in all attributes
 * @param[in] find value
 * @param[in] find value length
 * @param[out] optional; status of this operation
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_attribute_value_hyphen_separated(myhvml_tree_t *tree,
                                                     myhvml_collection_t* collection,
                                                     myhvml_tree_node_t* node,
                                                     bool case_insensitive,
                                                     const char* key, size_t key_len,
                                                     const char* value, size_t value_len,
                                                     mystatus_t* status);

/**
 * Get nodes by tag id in node scope
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, creates new collection if NULL
 * @param[in] node for search tag_id in children nodes
 * @param[in] tag_id for search
 * @param[out] status of this operation
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_tag_id_in_scope(myhvml_tree_t* tree, myhvml_collection_t *collection,
                                    myhvml_tree_node_t *node, myhvml_tag_id_t tag_id,
                                    mystatus_t *status);

/**
 * Get nodes by tag name in node scope
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_collection_t*, creates new collection if NULL
 * @param[in] node for search tag_id in children nodes
 * @param[in] tag name
 * @param[in] tag name length
 * @param[out] status of this operation
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_get_nodes_by_name_in_scope(myhvml_tree_t* tree, myhvml_collection_t *collection,
                                  myhvml_tree_node_t *node, const char* hvml, size_t length,
                                  mystatus_t *status);

/**
 * Get next sibling node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_node_t* if exists, otherwise an NULL value
 */
myhvml_tree_node_t*
myhvml_node_next(myhvml_tree_node_t *node);

/**
 * Get previous sibling node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_node_t* if exists, otherwise an NULL value
 */
myhvml_tree_node_t*
myhvml_node_prev(myhvml_tree_node_t *node);

/**
 * Get parent node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_node_t* if exists, otherwise an NULL value
 */
myhvml_tree_node_t*
myhvml_node_parent(myhvml_tree_node_t *node);

/**
 * Get child (first child) of node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_node_t* if exists, otherwise an NULL value
 */
myhvml_tree_node_t*
myhvml_node_child(myhvml_tree_node_t *node);

/**
 * Get last child of node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_node_t* if exists, otherwise an NULL value
 */
myhvml_tree_node_t*
myhvml_node_last_child(myhvml_tree_node_t *node);

/**
 * Create new node
 *
 * @param[in] myhvml_tree_t*
 * @param[in] tag id, see enum myhvml_tags
 * @param[in] enum myhvml_namespace
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_node_create(myhvml_tree_t* tree, myhvml_tag_id_t tag_id,
                   enum myhvml_namespace ns);

/**
 * Cloning a node
 *
 * @param[in] the tree into which the cloned node will be inserted. myhvml_tree_t*
 * @param[in] cloning node
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t *
myhvml_node_clone(myhvml_tree_t* dest_tree, myhvml_tree_node_t* src);

/**
 * Cloning a node with all children
 *
 * @param[in] the tree into which the cloned node will be inserted. myhvml_tree_t*
 * @param[in] cloning node
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t *
myhvml_node_clone_deep(myhvml_tree_t* dest_tree, myhvml_tree_node_t* src);

/**
 * Release allocated resources
 *
 * @param[in] myhvml_tree_node_t*
 */
void
myhvml_node_free(myhvml_tree_node_t *node);

/**
 * Remove node of tree
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_node_t* if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_node_remove(myhvml_tree_node_t *node);

/**
 * Remove node of tree and release allocated resources
 *
 * @param[in] myhvml_tree_node_t*
 */
void
myhvml_node_delete(myhvml_tree_node_t *node);

/**
 * Remove nodes of tree recursively and release allocated resources
 *
 * @param[in] myhvml_tree_node_t*
 */
void
myhvml_node_delete_recursive(myhvml_tree_node_t *node);

/**
 * The appropriate place for inserting a node. Insertion with validation.
 * If try insert <a> node to <table> node, then <a> node inserted before <table> node
 *
 * @param[in] target node
 * @param[in] insertion node
 *
 * @return insertion node if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_node_insert_to_appropriate_place(myhvml_tree_node_t *target, myhvml_tree_node_t *node);

/**
 * Append to target node as last child. Insertion without validation.
 *
 * @param[in] target node
 * @param[in] insertion node
 *
 * @return insertion node if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_node_append_child(myhvml_tree_node_t *target, myhvml_tree_node_t *node);

/**
 * Append sibling node after target node. Insertion without validation.
 *
 * @param[in] target node
 * @param[in] insertion node
 *
 * @return insertion node if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_node_insert_after(myhvml_tree_node_t *target, myhvml_tree_node_t *node);

/**
 * Append sibling node before target node. Insertion without validation.
 *
 * @param[in] target node
 * @param[in] insertion node
 *
 * @return insertion node if successful, otherwise a NULL value
 */
myhvml_tree_node_t*
myhvml_node_insert_before(myhvml_tree_node_t *target, myhvml_tree_node_t *node);

/**
 * Add text for a node with convert character encoding.
 *
 * @param[in] target node
 * @param[in] text
 * @param[in] text length
 *
 * @return mycore_string_t* if successful, otherwise a NULL value
 */
mycore_string_t*
myhvml_node_text_set(myhvml_tree_node_t *node, const char* text, size_t length);

/**
 * Add text for a node with convert character encoding.
 *
 * @param[in] target node
 * @param[in] text
 * @param[in] text length
 *
 * @return mycore_string_t* if successful, otherwise a NULL value
 */
mycore_string_t*
myhvml_node_text_set_with_charef(myhvml_tree_node_t *node, const char* text, size_t length);

/**
 * Get token node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_token_node_t*
 */
myhvml_token_node_t*
myhvml_node_token(myhvml_tree_node_t *node);

/**
 * Get node namespace
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_namespace_t
 */
myhvml_namespace_t
myhvml_node_namespace(myhvml_tree_node_t *node);

/**
 * Set node namespace
 *
 * @param[in] myhvml_tree_node_t*
 * @param[in] myhvml_namespace_t
 */
void
myhvml_node_namespace_set(myhvml_tree_node_t *node, myhvml_namespace_t ns);

/**
 * Get node tag id
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tag_id_t
 */
myhvml_tag_id_t
myhvml_node_tag_id(myhvml_tree_node_t *node);

/**
 * Node has self-closing flag?
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return true or false (1 or 0)
 */
bool
myhvml_node_is_close_self(myhvml_tree_node_t *node);

/**
 * Node is a void element?
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return true or false (1 or 0)
 */
bool
myhvml_node_is_void_element(myhvml_tree_node_t *node);

/**
 * Get first attribute of a node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_attr_t* if exists, otherwise an NULL value
 */
myhvml_tree_attr_t*
myhvml_node_attribute_first(myhvml_tree_node_t *node);

/**
 * Get last attribute of a node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_attr_t* if exists, otherwise an NULL value
 */
myhvml_tree_attr_t*
myhvml_node_attribute_last(myhvml_tree_node_t *node);

/**
 * Get text of a node. Only for a MyHVML_TAG__TEXT or MyHVML_TAG__COMMENT tags
 *
 * @param[in] myhvml_tree_node_t*
 * @param[out] optional, text length
 *
 * @return const char* if exists, otherwise an NULL value
 */
const char*
myhvml_node_text(myhvml_tree_node_t *node, size_t *length);

/**
 * Get mycore_string_t object by Tree node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return mycore_string_t* if exists, otherwise an NULL value
 */
mycore_string_t*
myhvml_node_string(myhvml_tree_node_t *node);

/**
 * Get raw position for Tree Node in Incoming Buffer
 *
 * @example <[BEGIN]div[LENGTH] attr=lalala>
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_node_t
 */
myhvml_position_t
myhvml_node_raw_position(myhvml_tree_node_t *node);

/**
 * Get element position for Tree Node in Incoming Buffer
 *
 * @example [BEGIN]<div attr=lalala>[LENGTH]
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_node_t
 */
myhvml_position_t
myhvml_node_element_position(myhvml_tree_node_t *node);

/**
 * Get data value from tree node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return void*
 */
void*
myhvml_node_get_data(myhvml_tree_node_t *node);

/**
 * Set data value to tree node
 *
 * @param[in] myhvml_tree_node_t*
 * @param[in] void*
 */
void
myhvml_node_set_data(myhvml_tree_node_t *node, void* data);

/**
 * Get current tree (myhvml_tree_t*) from node
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return myhvml_tree_t*
 */
myhvml_tree_t*
myhvml_node_tree(myhvml_tree_node_t *node);

/***********************************************************************************
 *
 * MyHVML_ATTRIBUTE
 *
 ***********************************************************************************/

/**
 * Get next sibling attribute of one node
 *
 * @param[in] myhvml_tree_attr_t*
 *
 * @return myhvml_tree_attr_t* if exists, otherwise an NULL value
 */
myhvml_tree_attr_t*
myhvml_attribute_next(myhvml_tree_attr_t *attr);

/**
 * Get previous sibling attribute of one node
 *
 * @param[in] myhvml_tree_attr_t*
 *
 * @return myhvml_tree_attr_t* if exists, otherwise an NULL value
 */
myhvml_tree_attr_t*
myhvml_attribute_prev(myhvml_tree_attr_t *attr);

/**
 * Get attribute namespace
 *
 * @param[in] myhvml_tree_attr_t*
 *
 * @return enum myhvml_namespace
 */
myhvml_namespace_t
myhvml_attribute_namespace(myhvml_tree_attr_t *attr);

/**
 * Set attribute namespace
 *
 * @param[in] myhvml_tree_attr_t*
 * @param[in] myhvml_namespace_t
 */
void
myhvml_attribute_namespace_set(myhvml_tree_attr_t *attr, myhvml_namespace_t ns);

/**
 * Get attribute key
 *
 * @param[in] myhvml_tree_attr_t*
 * @param[out] optional, name length
 *
 * @return const char* if exists, otherwise an NULL value
 */
const char*
myhvml_attribute_key(myhvml_tree_attr_t *attr, size_t *length);

/**
 * Get attribute value
 *
 * @param[in] myhvml_tree_attr_t*
 * @param[out] optional, value length
 *
 * @return const char* if exists, otherwise an NULL value
 */
const char*
myhvml_attribute_value(myhvml_tree_attr_t *attr, size_t *length);

/**
 * Get attribute key string
 *
 * @param[in] myhvml_tree_attr_t*
 *
 * @return mycore_string_t* if exists, otherwise an NULL value
 */
mycore_string_t*
myhvml_attribute_key_string(myhvml_tree_attr_t* attr);

/**
 * Get attribute value string
 *
 * @param[in] myhvml_tree_attr_t*
 *
 * @return mycore_string_t* if exists, otherwise an NULL value
 */
mycore_string_t*
myhvml_attribute_value_string(myhvml_tree_attr_t* attr);

/**
 * Get attribute by key
 *
 * @param[in] myhvml_tree_node_t*
 * @param[in] attr key name
 * @param[in] attr key name length
 *
 * @return myhvml_tree_attr_t* if exists, otherwise a NULL value
 */
myhvml_tree_attr_t*
myhvml_attribute_by_key(myhvml_tree_node_t *node,
                        const char *key, size_t key_len);

/**
 * Added attribute to tree node
 *
 * @param[in] myhvml_tree_node_t*
 * @param[in] attr key name
 * @param[in] attr key name length
 * @param[in] attr value name
 * @param[in] attr value name length
 *
 * @return created myhvml_tree_attr_t* if successful, otherwise a NULL value
 */
myhvml_tree_attr_t*
myhvml_attribute_add(myhvml_tree_node_t *node,
                     const char *key, size_t key_len,
                     const char *value, size_t value_len);

/**
 * Remove attribute reference. Not release the resources
 *
 * @param[in] myhvml_tree_node_t*
 * @param[in] myhvml_tree_attr_t*
 *
 * @return myhvml_tree_attr_t* if successful, otherwise a NULL value
 */
myhvml_tree_attr_t*
myhvml_attribute_remove(myhvml_tree_node_t *node, myhvml_tree_attr_t *attr);

/**
 * Remove attribute by key reference. Not release the resources
 *
 * @param[in] myhvml_tree_node_t*
 * @param[in] attr key name
 * @param[in] attr key name length
 *
 * @return myhvml_tree_attr_t* if successful, otherwise a NULL value
 */
myhvml_tree_attr_t*
myhvml_attribute_remove_by_key(myhvml_tree_node_t *node, const char *key, size_t key_len);

/**
 * Remove attribute and release allocated resources
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_tree_node_t*
 * @param[in] myhvml_tree_attr_t*
 *
 */
void
myhvml_attribute_delete(myhvml_tree_t *tree, myhvml_tree_node_t *node,
                        myhvml_tree_attr_t *attr);

/**
 * Release allocated resources
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_tree_attr_t*
 *
 * @return myhvml_tree_attr_t* if successful, otherwise a NULL value
 */
void
myhvml_attribute_free(myhvml_tree_t *tree, myhvml_tree_attr_t *attr);

/**
 * Get raw position for Attribute Key in Incoming Buffer
 *
 * @param[in] myhvml_tree_attr_t*
 *
 * @return myhvml_position_t
 */
myhvml_position_t
myhvml_attribute_key_raw_position(myhvml_tree_attr_t *attr);

/**
 * Get raw position for Attribute Value in Incoming Buffer
 *
 * @param[in] myhvml_tree_attr_t*
 *
 * @return myhvml_position_t
 */
myhvml_position_t
myhvml_attribute_value_raw_position(myhvml_tree_attr_t *attr);

/***********************************************************************************
 *
 * MyHVML_TOKEN_NODE
 *
 ***********************************************************************************/

/**
 * Get token node tag id
 *
 * @param[in] myhvml_token_node_t*
 *
 * @return myhvml_tag_id_t
 */
myhvml_tag_id_t
myhvml_token_node_tag_id(myhvml_token_node_t *token_node);

/**
 * Get raw position for Token Node in Incoming Buffer
 *
 * @example <[BEGIN]div[LENGTH] attr=lalala>
 *
 * @param[in] myhvml_token_node_t*
 *
 * @return myhvml_position_t
 */
myhvml_position_t
myhvml_token_node_raw_position(myhvml_token_node_t *token_node);

/**
 * Get element position for Token Node in Incoming Buffer
 *
 * @example [BEGIN]<div attr=lalala>[LENGTH]
 *
 * @param[in] myhvml_token_node_t*
 *
 * @return myhvml_position_t
 */
myhvml_position_t
myhvml_token_node_element_position(myhvml_token_node_t *token_node);

/**
 * Get first attribute of a token node
 *
 * @param[in] myhvml_token_node_t*
 *
 * @return myhvml_tree_attr_t* if exists, otherwise an NULL value
 */
myhvml_tree_attr_t*
myhvml_token_node_attribute_first(myhvml_token_node_t *token_node);

/**
 * Get last attribute of a token node
 *
 * @param[in] myhvml_token_node_t*
 *
 * @return myhvml_tree_attr_t* if exists, otherwise an NULL value
 */
myhvml_tree_attr_t*
myhvml_token_node_attribute_last(myhvml_token_node_t *token_node);

/**
 * Get text of a token node. Only for a MyHVML_TAG__TEXT or MyHVML_TAG__COMMENT tags
 *
 * @param[in] myhvml_token_node_t*
 * @param[out] optional, text length
 *
 * @return const char* if exists, otherwise an NULL value
 */
const char*
myhvml_token_node_text(myhvml_token_node_t *token_node, size_t *length);

/**
 * Get mycore_string_t object by token node
 *
 * @param[in] myhvml_token_node_t*
 *
 * @return mycore_string_t* if exists, otherwise an NULL value
 */
mycore_string_t*
myhvml_token_node_string(myhvml_token_node_t *token_node);

/**
 * Token node has closing flag?
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return true or false
 */
bool
myhvml_token_node_is_close(myhvml_token_node_t *token_node);

/**
 * Token node has self-closing flag?
 *
 * @param[in] myhvml_tree_node_t*
 *
 * @return true or false (1 or 0)
 */
bool
myhvml_token_node_is_close_self(myhvml_token_node_t *token_node);

/**
 * Wait for process token all parsing stage. Need if you use thread mode
 *
 * @param[in] myhvml_token_t*
 * @param[in] myhvml_token_node_t*
 */
void
myhvml_token_node_wait_for_done(myhvml_token_t* token, myhvml_token_node_t* node);

/***********************************************************************************
 *
 * MyHVML_TAG
 *
 ***********************************************************************************/

/**
 * Get tag name by tag id
 *
 * @param[in] myhvml_tree_t*
 * @param[in] tag id
 * @param[out] optional, name length
 *
 * @return const char* if exists, otherwise a NULL value
 */
const char*
myhvml_tag_name_by_id(myhvml_tree_t* tree,
                      myhvml_tag_id_t tag_id, size_t *length);

/**
 * Get tag id by name
 *
 * @param[in] myhvml_tree_t*
 * @param[in] tag name
 * @param[in] tag name length
 *
 * @return tag id
 */
myhvml_tag_id_t
myhvml_tag_id_by_name(myhvml_tree_t* tree,
                      const char *tag_name, size_t length);

/***********************************************************************************
 *
 * MyHVML_COLLECTION
 *
 ***********************************************************************************/

/**
 * Create collection
 *
 * @param[in] list size
 * @param[out] optional, status of operation
 *
 * @return myhvml_collection_t* if successful, otherwise an NULL value
 */
myhvml_collection_t*
myhvml_collection_create(size_t size, mystatus_t *status);

/**
 * Clears collection
 *
 * @param[in] myhvml_collection_t*
 */
void
myhvml_collection_clean(myhvml_collection_t *collection);

/**
 * Destroy allocated resources
 *
 * @param[in] myhvml_collection_t*
 *
 * @return NULL if successful, otherwise an myhvml_collection_t* structure
 */
myhvml_collection_t*
myhvml_collection_destroy(myhvml_collection_t *collection);

/**
 * Check size by length and increase if necessary
 *
 * @param[in] myhvml_collection_t*
 * @param[in] need nodes
 * @param[in] upto_length: count for up if nodes not exists 
 *            (current length + need + upto_length + 1)
 *
 * @return NULL if successful, otherwise an myhvml_collection_t* structure
 */
mystatus_t
myhvml_collection_check_size(myhvml_collection_t *collection, size_t need, size_t upto_length);

/***********************************************************************************
 *
 * MyHVML_NAMESPACE
 *
 ***********************************************************************************/

/**
 * Get namespace text by namespace type (id)
 *
 * @param[in] myhvml_namespace_t
 * @param[out] optional, length of returned text
 *
 * @return text if successful, otherwise a NULL value
 */
const char*
myhvml_namespace_name_by_id(myhvml_namespace_t ns, size_t *length);

/**
 * Get namespace type (id) by namespace text
 *
 * @param[in] const char*, namespace text
 * @param[in] size of namespace text
 * @param[out] detected namespace type (id)
 *
 * @return true if detect, otherwise false
 */
bool
myhvml_namespace_id_by_name(const char *name, size_t length, myhvml_namespace_t *ns);

/***********************************************************************************
 *
 * MyHVML_CALLBACK
 *
 ***********************************************************************************/

/**
 * Get current callback for tokens before processing
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_callback_token_f
 */
myhvml_callback_token_f
myhvml_callback_before_token_done(myhvml_tree_t* tree);

/**
 * Get current callback for tokens after processing
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_callback_token_f
 */
myhvml_callback_token_f
myhvml_callback_after_token_done(myhvml_tree_t* tree);

/**
 * Set callback for tokens before processing
 *
 * Warning!
 * If you using thread mode parsing then this callback calls from thread (not Main thread)
 * If you build MyHVML without thread or using MyHVML_OPTIONS_PARSE_MODE_SINGLE for create myhvml_t object
 *  then this callback calls from Main thread
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_callback_token_f callback function
 */
void
myhvml_callback_before_token_done_set(myhvml_tree_t* tree, myhvml_callback_token_f func, void* ctx);

/**
 * Set callback for tokens after processing
 *
 * Warning!
 * If you using thread mode parsing then this callback calls from thread (not Main thread)
 * If you build MyHVML without thread or using MyHVML_OPTIONS_PARSE_MODE_SINGLE for create myhvml_t object
 *  then this callback calls from Main thread
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_callback_token_f callback function
 */
void
myhvml_callback_after_token_done_set(myhvml_tree_t* tree, myhvml_callback_token_f func, void* ctx);

/**
 * Get current callback for tree node after inserted
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_callback_tree_node_f
 */
myhvml_callback_tree_node_f
myhvml_callback_tree_node_insert(myhvml_tree_t* tree);

/**
 * Get current callback for tree node after removed
 *
 * @param[in] myhvml_tree_t*
 *
 * @return myhvml_callback_tree_node_f
 */
myhvml_callback_tree_node_f
myhvml_callback_tree_node_remove(myhvml_tree_t* tree);

/**
 * Set callback for tree node after inserted
 *
 * Warning!
 * If you using thread mode parsing then this callback calls from thread (not Main thread)
 * If you build MyHVML without thread or using MyHVML_OPTIONS_PARSE_MODE_SINGLE for create myhvml_t object
 *  then this callback calls from Main thread
 *
 * Warning!!!
 * If you well access to attributes or text for node and you using thread mode then 
 * you need wait for token processing done. See myhvml_token_node_wait_for_done
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_callback_tree_node_f callback function
 */
void
myhvml_callback_tree_node_insert_set(myhvml_tree_t* tree, myhvml_callback_tree_node_f func, void* ctx);

/**
 * Set callback for tree node after removed
 *
 * Warning!
 * If you using thread mode parsing then this callback calls from thread (not Main thread)
 * If you build MyHVML without thread or using MyHVML_OPTIONS_PARSE_MODE_SINGLE for create myhvml_t object
 *  then this callback calls from Main thread
 *
 * Warning!!!
 * If you well access to attributes or text for node and you using thread mode then
 * you need wait for token processing done. See myhvml_token_node_wait_for_done
 *
 * @param[in] myhvml_tree_t*
 * @param[in] myhvml_callback_tree_node_f callback function
 */
void
myhvml_callback_tree_node_remove_set(myhvml_tree_t* tree, myhvml_callback_tree_node_f func, void* ctx);

/***********************************************************************************
 *
 * MyHVML_SERIALIZATION
 *
 ***********************************************************************************/

/**
 * Tree fragment serialization 
 * The same as myhvml_serialization_tree_buffer function
 */
mystatus_t
myhvml_serialization(myhvml_tree_node_t* scope_node, mycore_string_raw_t* str);

/**
 * Only one tree node serialization
 * The same as myhvml_serialization_node_buffer function
 */
mystatus_t
myhvml_serialization_node(myhvml_tree_node_t* node, mycore_string_raw_t* str);

/**
 * Serialize tree to an output string
 *
 * @param[in] myhvml_tree_t*
 * @param[in] scope node
 * @param[in] mycore_string_raw_t*
 *
 * @return true if successful, otherwise false
 */
mystatus_t
myhvml_serialization_tree_buffer(myhvml_tree_node_t* scope_node, mycore_string_raw_t* str);

/**
 * Serialize node to an output string
 *
 * @param[in] myhvml_tree_t*
 * @param[in] node
 * @param[in] mycore_string_raw_t*
 *
 * @return true if successful, otherwise false
 */
mystatus_t
myhvml_serialization_node_buffer(myhvml_tree_node_t* node, mycore_string_raw_t* str);

/**
 * The serialize function for an entire tree
 *
 * @param[in] tree        the tree to be serialized
 * @param[in] scope_node  the scope_node
 * @param[in] callback    function that will be called for all strings that have to be printed
 * @param[in] ptr         user-supplied pointer
 *
 * @return true if successful, otherwise false
 */
mystatus_t
myhvml_serialization_tree_callback(myhvml_tree_node_t* scope_node,
                                   mycore_callback_serialize_f callback, void* ptr);

/**
 * The serialize function for a single node
 *
 * @param[in] tree        the tree to be serialized
 * @param[in] node        the node that is going to be serialized
 * @param[in] callback    function that will be called for all strings that have to be printed
 * @param[in] ptr         user-supplied pointer
 *
 * @return true if successful, otherwise false
 */
mystatus_t
myhvml_serialization_node_callback(myhvml_tree_node_t* node,
                                   mycore_callback_serialize_f callback, void* ptr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCAT2_MyHVML_H */
