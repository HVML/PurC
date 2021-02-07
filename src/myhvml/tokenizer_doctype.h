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

#ifndef MyHVML_TOKENIZER_DOCTYPE_H
#define MyHVML_TOKENIZER_DOCTYPE_H

#pragma once

#include "myosi.h"
#include "mycore/utils.h"
#include "myhvml_internals.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t myhvml_tokenizer_state_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_before_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_after_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_custom_after_doctype_name_a_z(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_before_doctype_public_identifier(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_doctype_public_identifier_double_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_doctype_public_identifier_single_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_after_doctype_public_identifier(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_doctype_system_identifier_double_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_doctype_system_identifier_single_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_after_doctype_system_identifier(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_state_bogus_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_TOKENIZER_DOCTYPE_H */
