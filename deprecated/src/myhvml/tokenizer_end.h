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

#ifndef MyHVML_TOKENIZER_END_H
#define MyHVML_TOKENIZER_END_H

#pragma once

#include "myosi.h"
#include "myhvml_internals.h"
#include "tokenizer.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t myhvml_tokenizer_end_state_data(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_self_closing_start_tag(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_markup_declaration_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_before_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_after_attribute_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_before_attribute_value(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_attribute_value_double_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_attribute_value_single_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_attribute_value_unquoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_after_attribute_value_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_comment_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_comment_start_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_comment(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_comment_end(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_comment_end_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_comment_end_bang(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_bogus_comment(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_cdata_section(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

size_t myhvml_tokenizer_end_state_rcdata(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_rcdata_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_rcdata_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_rcdata_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

size_t myhvml_tokenizer_end_state_rawtext(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_rawtext_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_rawtext_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_rawtext_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

size_t myhvml_tokenizer_end_state_plaintext(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

size_t myhvml_tokenizer_end_state_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_before_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_after_doctype_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_custom_after_doctype_name_a_z(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_before_doctype_system_identifier(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_doctype_system_identifier_double_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_doctype_system_identifier_single_quoted(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_after_doctype_system_identifier(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_bogus_doctype(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

size_t myhvml_tokenizer_end_state_script_data(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_escape_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_escape_start_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_escaped(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_escaped_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_escaped_dash_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_escaped_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_escaped_end_tag_open(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_escaped_end_tag_name(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_double_escape_start(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_double_escaped(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_double_escaped_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_double_escaped_dash_dash(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_double_escaped_less_than_sign(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_script_data_double_escape_end(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);
size_t myhvml_tokenizer_end_state_parse_error_stop(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_TOKENIZER_END_H */

