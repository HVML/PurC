/*
** Copyright (C) 2021 FMSoft (https://www.fmsoft.cn)
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
** Author: Vincent Wei (https://github.com/VincentWei)
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef MyHTML_TOKENIZER_SCRIPT_H
#define MyHTML_TOKENIZER_SCRIPT_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "myosi.h"
#include "mycore/utils.h"
#include "myhtml_internal.h"
#include "tokenizer.h"

size_t myhtml_tokenizer_state_script_data(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_less_than_sign(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_end_tag_open(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_end_tag_name(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_escape_start(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_escape_start_dash(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_escaped(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_escaped_dash(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_escaped_dash_dash(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_escaped_less_than_sign(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_escaped_end_tag_open(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_escaped_end_tag_name(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_double_escape_start(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_double_escaped(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_double_escaped_dash(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_double_escaped_dash_dash(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_double_escaped_less_than_sign(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);
size_t myhtml_tokenizer_state_script_data_double_escape_end(myhtml_tree_t* tree, myhtml_token_node_t* token_node, const char* html, size_t html_offset, size_t html_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* myhtml_tokenizer_script_h */
