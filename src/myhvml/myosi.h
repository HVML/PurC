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

#ifndef MyHVML_MYOSI_H
#define MyHVML_MYOSI_H

#pragma once

#include "myhvml.h"
#include "mycore/myosi.h"

#define MyHVML_FAILED(_status_) ((_status_) != MyHVML_STATUS_OK)

// char references
typedef struct myhvml_data_process_entry myhvml_data_process_entry_t;

// tree
enum myhvml_tree_flags {
    MyHVML_TREE_FLAGS_CLEAN                   = 0x000,
    MyHVML_TREE_FLAGS_SCRIPT                  = 0x001,
    MyHVML_TREE_FLAGS_FRAMESET_OK             = 0x002,
    MyHVML_TREE_FLAGS_IFRAME_SRCDOC           = 0x004,
    MyHVML_TREE_FLAGS_ALREADY_STARTED         = 0x008,
    MyHVML_TREE_FLAGS_SINGLE_MODE             = 0x010,
    MyHVML_TREE_FLAGS_PARSE_END               = 0x020,
    MyHVML_TREE_FLAGS_PARSE_FLAG              = 0x040,
    MyHVML_TREE_FLAGS_PARSE_FLAG_EMIT_NEWLINE = 0x080
};

typedef struct myhvml_tree_temp_tag_name myhvml_tree_temp_tag_name_t;
typedef struct myhvml_tree_insertion_list myhvml_tree_insertion_list_t;
typedef struct myhvml_tree_token_list myhvml_tree_token_list_t;
typedef struct myhvml_tree_list myhvml_tree_list_t;
typedef struct myhvml_tree_doctype myhvml_tree_doctype_t;
typedef struct myhvml_async_args myhvml_async_args_t;

typedef struct myhvml_tree_node myhvml_tree_node_t;
typedef struct myhvml_tree myhvml_tree_t;

// token
enum myhvml_token_type {
    MyHVML_TOKEN_TYPE_OPEN             = 0x000,
    MyHVML_TOKEN_TYPE_CLOSE            = 0x001,
    MyHVML_TOKEN_TYPE_CLOSE_SELF       = 0x002,
    MyHVML_TOKEN_TYPE_DONE             = 0x004,
    MyHVML_TOKEN_TYPE_WHITESPACE       = 0x008,
    MyHVML_TOKEN_TYPE_RCDATA           = 0x010,
    MyHVML_TOKEN_TYPE_RAWTEXT          = 0x020,
    MyHVML_TOKEN_TYPE_SCRIPT           = 0x040,
    MyHVML_TOKEN_TYPE_PLAINTEXT        = 0x080,
    MyHVML_TOKEN_TYPE_CDATA            = 0x100,
    MyHVML_TOKEN_TYPE_DATA             = 0x200,
    MyHVML_TOKEN_TYPE_COMMENT          = 0x400,
    MyHVML_TOKEN_TYPE_NULL             = 0x800
};

typedef size_t myhvml_token_index_t;
typedef size_t myhvml_token_attr_index_t;
typedef struct myhvml_token_replacement_entry myhvml_token_replacement_entry_t;
typedef struct myhvml_token_namespace_replacement myhvml_token_namespace_replacement_t;

typedef struct myhvml_token_attr myhvml_token_attr_t;
typedef struct myhvml_token_node myhvml_token_node_t;
typedef struct myhvml_token myhvml_token_t;

// tags
enum myhvml_tag_categories {
    MyHVML_TAG_CATEGORIES_UNDEF            = 0x000,
    MyHVML_TAG_CATEGORIES_NOUN             = 0x001,
    MyHVML_TAG_CATEGORIES_VERB             = 0x002,
    MyHVML_TAG_CATEGORIES_FOREIGN          = 0x003,
    MyHVML_TAG_CATEGORIES_KIND             = 0x00F,
    MyHVML_TAG_CATEGORIES_ORDINARY         = 0x010,
    MyHVML_TAG_CATEGORIES_SPECIAL          = 0x020,
    MyHVML_TAG_CATEGORIES_SCOPE            = 0x040,
    MyHVML_TAG_CATEGORIES_FORMATTING       = 0x080, /* VW: no use for HVML */
    MyHVML_TAG_CATEGORIES_SCOPE_LIST_ITEM  = 0x100, /* VW: no use for HVML */
    MyHVML_TAG_CATEGORIES_SCOPE_BUTTON     = 0x200, /* VW: no use for HVML */
    MyHVML_TAG_CATEGORIES_SCOPE_TABLE      = 0x400, /* VW: no use for HVML */
    MyHVML_TAG_CATEGORIES_SCOPE_SELECT     = 0x800, /* VW: no use for HVML */
};

typedef struct myhvml_tag_index_node myhvml_tag_index_node_t;
typedef struct myhvml_tag_index_entry myhvml_tag_index_entry_t;
typedef struct myhvml_tag_index myhvml_tag_index_t;

typedef size_t myhvml_tag_id_t;

typedef struct myhvml_tag myhvml_tag_t;

// stream
typedef struct myhvml_stream_buffer_entry myhvml_stream_buffer_entry_t;
typedef struct myhvml_stream_buffer myhvml_stream_buffer_t;

// parse
enum myhvml_tokenizer_state {
    MyHVML_TOKENIZER_STATE_DATA                                          = 0x000,
    MyHVML_TOKENIZER_STATE_CHARACTER_REFERENCE_IN_DATA                   = 0x001,
    MyHVML_TOKENIZER_STATE_RCDATA                                        = 0x002,
    MyHVML_TOKENIZER_STATE_CHARACTER_REFERENCE_IN_RCDATA                 = 0x003,
    MyHVML_TOKENIZER_STATE_RAWTEXT                                       = 0x004,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA                                   = 0x005,
    MyHVML_TOKENIZER_STATE_PLAINTEXT                                     = 0x006,
    MyHVML_TOKENIZER_STATE_TAG_OPEN                                      = 0x007,
    MyHVML_TOKENIZER_STATE_END_TAG_OPEN                                  = 0x008,
    MyHVML_TOKENIZER_STATE_TAG_NAME                                      = 0x009,
    MyHVML_TOKENIZER_STATE_RCDATA_LESS_THAN_SIGN                         = 0x00a,
    MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_OPEN                           = 0x00b,
    MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_NAME                           = 0x00c,
    MyHVML_TOKENIZER_STATE_RAWTEXT_LESS_THAN_SIGN                        = 0x00d,
    MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_OPEN                          = 0x00e,
    MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_NAME                          = 0x00f,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_LESS_THAN_SIGN                    = 0x010,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_OPEN                      = 0x011,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_NAME                      = 0x012,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START                      = 0x013,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START_DASH                 = 0x014,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED                           = 0x015,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH                      = 0x016,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH                 = 0x017,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN            = 0x018,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_OPEN              = 0x019,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_NAME              = 0x01a,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_START               = 0x01b,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED                    = 0x01c,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH               = 0x01d,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH          = 0x01e,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN     = 0x01f,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_END                 = 0x020,
    MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME                         = 0x021,
    MyHVML_TOKENIZER_STATE_ATTRIBUTE_NAME                                = 0x022,
    MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_NAME                          = 0x023,
    MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_VALUE                        = 0x024,
    MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED                 = 0x025,
    MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED                 = 0x026,
    MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_UNQUOTED                      = 0x027,
    MyHVML_TOKENIZER_STATE_CHARACTER_REFERENCE_IN_ATTRIBUTE_VALUE        = 0x028,
    MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED                  = 0x029,
    MyHVML_TOKENIZER_STATE_SELF_CLOSING_START_TAG                        = 0x02a,
    MyHVML_TOKENIZER_STATE_BOGUS_COMMENT                                 = 0x02b,
    MyHVML_TOKENIZER_STATE_MARKUP_DECLARATION_OPEN                       = 0x02c,
    MyHVML_TOKENIZER_STATE_COMMENT_START                                 = 0x02d,
    MyHVML_TOKENIZER_STATE_COMMENT_START_DASH                            = 0x02e,
    MyHVML_TOKENIZER_STATE_COMMENT                                       = 0x02f,
    MyHVML_TOKENIZER_STATE_COMMENT_END_DASH                              = 0x030,
    MyHVML_TOKENIZER_STATE_COMMENT_END                                   = 0x031,
    MyHVML_TOKENIZER_STATE_COMMENT_END_BANG                              = 0x032,
    MyHVML_TOKENIZER_STATE_DOCTYPE                                       = 0x033,
    MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_NAME                           = 0x034,
    MyHVML_TOKENIZER_STATE_DOCTYPE_NAME                                  = 0x035,
    MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_NAME                            = 0x036,
    MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_PUBLIC_KEYWORD                  = 0x037,
    MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER              = 0x038,
    MyHVML_TOKENIZER_STATE_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED       = 0x039,
    MyHVML_TOKENIZER_STATE_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED       = 0x03a,
    MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_PUBLIC_IDENTIFIER               = 0x03b,
    MyHVML_TOKENIZER_STATE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS = 0x03c,
    MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_SYSTEM_KEYWORD                  = 0x03d,
    MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER              = 0x03e,
    MyHVML_TOKENIZER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED       = 0x03f,
    MyHVML_TOKENIZER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED       = 0x040,
    MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_SYSTEM_IDENTIFIER               = 0x041,
    MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE                                 = 0x042,
    MyHVML_TOKENIZER_STATE_CDATA_SECTION                                 = 0x043,
    MyHVML_TOKENIZER_STATE_CUSTOM_AFTER_DOCTYPE_NAME_A_Z                 = 0x044,
    MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP                              = 0x045,
    MyHVML_TOKENIZER_STATE_FIRST_ENTRY                                   = MyHVML_TOKENIZER_STATE_DATA,
    MyHVML_TOKENIZER_STATE_LAST_ENTRY                                    = 0x046
};

enum myhvml_insertion_mode {
    MyHVML_INSERTION_MODE_INITIAL              = 0x000,
    MyHVML_INSERTION_MODE_BEFORE_HVML          = 0x001,
    MyHVML_INSERTION_MODE_BEFORE_HEAD          = 0x002,
    MyHVML_INSERTION_MODE_IN_HEAD              = 0x003,
    MyHVML_INSERTION_MODE_IN_HEAD_NOSCRIPT     = 0x004, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_AFTER_HEAD           = 0x005,
    MyHVML_INSERTION_MODE_IN_BODY              = 0x006,
    MyHVML_INSERTION_MODE_TEXT                 = 0x007,
    MyHVML_INSERTION_MODE_IN_TABLE             = 0x008, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_TABLE_TEXT        = 0x009, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_CAPTION           = 0x00a, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_COLUMN_GROUP      = 0x00b, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_TABLE_BODY        = 0x00c, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_ROW               = 0x00d, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_CELL              = 0x00e, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_SELECT            = 0x00f, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_SELECT_IN_TABLE   = 0x010, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_TEMPLATE          = 0x011,
    MyHVML_INSERTION_MODE_AFTER_BODY           = 0x012,
    MyHVML_INSERTION_MODE_IN_FRAMESET          = 0x013, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_AFTER_FRAMESET       = 0x014,
    MyHVML_INSERTION_MODE_AFTER_AFTER_BODY     = 0x015,
    MyHVML_INSERTION_MODE_AFTER_AFTER_FRAMESET = 0x016, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_LAST_ENTRY           = 0x017
};

typedef myhvml_token_attr_t myhvml_tree_attr_t;
typedef struct myhvml_collection myhvml_collection_t;
typedef struct myhvml myhvml_t;

#ifdef __cplusplus
extern "C" {
#endif

// parser state function
typedef size_t (*myhvml_tokenizer_state_f)(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

// parser insertion mode function
typedef bool (*myhvml_insertion_f)(myhvml_tree_t* tree, myhvml_token_node_t* token);

// char references state
typedef size_t (*myhvml_data_process_state_f)(myhvml_data_process_entry_t* charef, mycore_string_t* str, const char* data, size_t offset, size_t size);

// find attribute value functions
typedef bool (*myhvml_attribute_value_find_f)(mycore_string_t* str_key, const char* value, size_t value_len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

