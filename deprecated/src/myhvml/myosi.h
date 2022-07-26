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
    MyHVML_TOKEN_TYPE_OPEN             = 0x0000,
    MyHVML_TOKEN_TYPE_CLOSE            = 0x0001,
    MyHVML_TOKEN_TYPE_CLOSE_SELF       = 0x0002,
    MyHVML_TOKEN_TYPE_DONE             = 0x0004,
    MyHVML_TOKEN_TYPE_WHITESPACE       = 0x0008,
    MyHVML_TOKEN_TYPE_RCDATA           = 0x0010,
    MyHVML_TOKEN_TYPE_RAWTEXT          = 0x0020,
    MyHVML_TOKEN_TYPE_SCRIPT           = 0x0040,
    MyHVML_TOKEN_TYPE_PLAINTEXT        = 0x0080,
    MyHVML_TOKEN_TYPE_CDATA            = 0x0100,
    MyHVML_TOKEN_TYPE_DATA             = 0x0200,
    MyHVML_TOKEN_TYPE_COMMENT          = 0x0400,
    MyHVML_TOKEN_TYPE_NULL             = 0x0800,
    MyHVML_TOKEN_TYPE_JSONEE           = 0x1000,
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

// attributes
enum myhvml_attr_types {
    MyHVML_ATTR_TYPE_ORDINARY = 0,
    MyHVML_ATTR_TYPE_ADVERB_ASC,
    MyHVML_ATTR_TYPE_ADVERB_ASYNC,
    MyHVML_ATTR_TYPE_ADVERB_DESC,
    MyHVML_ATTR_TYPE_ADVERB_EXCL,
    MyHVML_ATTR_TYPE_ADVERB_SYNC,
    MyHVML_ATTR_TYPE_ADVERB_UNIQ,
    MyHVML_ATTR_TYPE_PREP_AS,
    MyHVML_ATTR_TYPE_PREP_AT,
    MyHVML_ATTR_TYPE_PREP_BY,
    MyHVML_ATTR_TYPE_PREP_EXCEPT,
    MyHVML_ATTR_TYPE_PREP_FOR,
    MyHVML_ATTR_TYPE_PREP_FROM,
    MyHVML_ATTR_TYPE_PREP_IN,
    MyHVML_ATTR_TYPE_PREP_ON,
    MyHVML_ATTR_TYPE_PREP_TO,
    MyHVML_ATTR_TYPE_PREP_WITH,
    MyHVML_ATTR_TYPE_UPDATE_ARRAY,
    MyHVML_ATTR_TYPE_UPDATE_ATTR,
    MyHVML_ATTR_TYPE_UPDATE_KEY,
    MyHVML_ATTR_TYPE_UPDATE_STYLE,
    MyHVML_ATTR_TYPE_UPDATE_TEXT,
    MyHVML_ATTR_TYPE_UPDATE_VALUE,
    MyHVML_ATTR_TYPE_FIRST_ENTRY = MyHVML_ATTR_TYPE_ADVERB_ASC,
    MyHVML_ATTR_TYPE_LAST_ENTRY  = MyHVML_ATTR_TYPE_UPDATE_VALUE + 1,
    MyHVML_ATTR_TYPE_ADVERB_FIRST = MyHVML_ATTR_TYPE_ADVERB_ASC,
    MyHVML_ATTR_TYPE_ADVERB_LAST = MyHVML_ATTR_TYPE_ADVERB_UNIQ,
    MyHVML_ATTR_TYPE_PREP_FIRST = MyHVML_ATTR_TYPE_PREP_AS,
    MyHVML_ATTR_TYPE_PREP_LAST = MyHVML_ATTR_TYPE_PREP_WITH,
    MyHVML_ATTR_TYPE_UPDATE_FIRST = MyHVML_ATTR_TYPE_UPDATE_ARRAY,
    MyHVML_ATTR_TYPE_UPDATE_LAST = MyHVML_ATTR_TYPE_UPDATE_VALUE,
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
    MyHVML_TOKENIZER_STATE_DATA = 0x000,
    MyHVML_TOKENIZER_STATE_CHARACTER_REFERENCE_IN_DATA,
    MyHVML_TOKENIZER_STATE_RCDATA,
    MyHVML_TOKENIZER_STATE_CHARACTER_REFERENCE_IN_RCDATA,
    MyHVML_TOKENIZER_STATE_RAWTEXT,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA,
    MyHVML_TOKENIZER_STATE_PLAINTEXT,
    MyHVML_TOKENIZER_STATE_TAG_OPEN,
    MyHVML_TOKENIZER_STATE_END_TAG_OPEN,
    MyHVML_TOKENIZER_STATE_TAG_NAME,
    MyHVML_TOKENIZER_STATE_RCDATA_LESS_THAN_SIGN,
    MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_OPEN,
    MyHVML_TOKENIZER_STATE_RCDATA_END_TAG_NAME,
    MyHVML_TOKENIZER_STATE_RAWTEXT_LESS_THAN_SIGN,
    MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_OPEN,
    MyHVML_TOKENIZER_STATE_RAWTEXT_END_TAG_NAME,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_LESS_THAN_SIGN,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_OPEN,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_END_TAG_NAME,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPE_START_DASH,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_OPEN,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_NAME,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_START,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN,
    MyHVML_TOKENIZER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_END,
    MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_NAME,
    MyHVML_TOKENIZER_STATE_ATTRIBUTE_NAME,
    MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_NAME,
    MyHVML_TOKENIZER_STATE_BEFORE_ATTRIBUTE_VALUE,
    MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED,
    MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED,
    MyHVML_TOKENIZER_STATE_ATTRIBUTE_VALUE_UNQUOTED,
    MyHVML_TOKENIZER_STATE_CHARACTER_REFERENCE_IN_ATTRIBUTE_VALUE,
    MyHVML_TOKENIZER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED,
    MyHVML_TOKENIZER_STATE_SELF_CLOSING_START_TAG,
    MyHVML_TOKENIZER_STATE_BOGUS_COMMENT,
    MyHVML_TOKENIZER_STATE_MARKUP_DECLARATION_OPEN,
    MyHVML_TOKENIZER_STATE_COMMENT_START,
    MyHVML_TOKENIZER_STATE_COMMENT_START_DASH,
    MyHVML_TOKENIZER_STATE_COMMENT,
    MyHVML_TOKENIZER_STATE_COMMENT_END_DASH,
    MyHVML_TOKENIZER_STATE_COMMENT_END,
    MyHVML_TOKENIZER_STATE_COMMENT_END_BANG,

    MyHVML_TOKENIZER_STATE_DOCTYPE,
    MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_NAME,
    MyHVML_TOKENIZER_STATE_DOCTYPE_NAME,
    MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_NAME,
    MyHVML_TOKENIZER_STATE_CUSTOM_AFTER_DOCTYPE_NAME_A_Z,
    MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_SYSTEM_KEYWORD,
    MyHVML_TOKENIZER_STATE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER,
    MyHVML_TOKENIZER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED,
    MyHVML_TOKENIZER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED,
    MyHVML_TOKENIZER_STATE_AFTER_DOCTYPE_SYSTEM_IDENTIFIER,
    MyHVML_TOKENIZER_STATE_BOGUS_DOCTYPE,

    MyHVML_TOKENIZER_STATE_CDATA_SECTION,
    MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP,
    MyHVML_TOKENIZER_STATE_FIRST_ENTRY = MyHVML_TOKENIZER_STATE_DATA,
    MyHVML_TOKENIZER_STATE_LAST_ENTRY  = MyHVML_TOKENIZER_STATE_PARSE_ERROR_STOP + 1,
};

enum myhvml_insertion_mode {
    MyHVML_INSERTION_MODE_INITIAL = 0x000,
    MyHVML_INSERTION_MODE_BEFORE_HVML,
    MyHVML_INSERTION_MODE_BEFORE_HEAD,
    MyHVML_INSERTION_MODE_IN_HEAD,
    MyHVML_INSERTION_MODE_AFTER_HEAD,
    MyHVML_INSERTION_MODE_IN_BODY,
    MyHVML_INSERTION_MODE_TEXT,
    MyHVML_INSERTION_MODE_IN_TEMPLATE,
    MyHVML_INSERTION_MODE_AFTER_BODY,
    MyHVML_INSERTION_MODE_AFTER_FRAMESET,
    MyHVML_INSERTION_MODE_AFTER_AFTER_BODY,
    MyHVML_INSERTION_MODE_LAST_ENTRY = MyHVML_INSERTION_MODE_AFTER_AFTER_BODY + 1,

    MyHVML_INSERTION_MODE_IN_HEAD_NOSCRIPT, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_TABLE,         /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_TABLE_TEXT,    /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_CAPTION,       /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_COLUMN_GROUP,  /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_TABLE_BODY,    /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_ROW,           /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_CELL,          /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_SELECT,        /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_SELECT_IN_TABLE, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_IN_FRAMESET, /* VW: no use for HVML */
    MyHVML_INSERTION_MODE_AFTER_AFTER_FRAMESET, /* VW: no use for HVML */
};

typedef myhvml_token_attr_t myhvml_tree_attr_t;
typedef struct myhvml_collection myhvml_collection_t;
typedef struct myhvml myhvml_t;

#ifdef __cplusplus
extern "C" {
#endif

// parser state function
typedef size_t (*myhvml_tokenizer_state_f)(
        myhvml_tree_t* tree, myhvml_token_node_t* token_node,
        const char* hvml, size_t hvml_offset, size_t hvml_size);

// parser insertion mode function
typedef bool (*myhvml_insertion_f)(myhvml_tree_t* tree, myhvml_token_node_t* token);

// char references state
typedef size_t (*myhvml_data_process_state_f)(
        myhvml_data_process_entry_t* charef, mycore_string_t* str,
        const char* data, size_t offset, size_t size);

// find attribute value functions
typedef bool (*myhvml_attribute_value_find_f)(
        mycore_string_t* str_key, const char* value, size_t value_len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

