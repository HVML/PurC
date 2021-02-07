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

#ifndef MyHVML_TOKENIZER_SCRIPT_H
#define MyHVML_TOKENIZER_SCRIPT_H

#pragma once

#include "myosi.h"
#include "mycore/utils.h"
#include "myhvml_internals.h"
#include "tokenizer.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t myhvml_tokenizer_state_script_data(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_escape_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_escape_start_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_escaped(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_escaped_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_escaped_dash_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_escaped_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_escaped_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_escaped_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_double_escape_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_double_escaped(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_double_escaped_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_double_escaped_dash_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_double_escaped_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_script_data_double_escape_end(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_TOKENIZER_SCRIPT_H */
