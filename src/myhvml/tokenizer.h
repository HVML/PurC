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

#ifndef MyHVML_TOKENIZER_H
#define MyHVML_TOKENIZER_H

#pragma once

#include "myosi.h"
#include "mycore/utils.h"
#include "mycore/mythread.h"
#include "myhvml_internals.h"
#include "tag.h"
#include "tokenizer_doctype.h"
#include "tokenizer_script.h"
#include "tokenizer_end.h"

#define myhvml_tokenizer_inc_hvml_offset(offset, size)   \
    offset++;                                            \
    if(offset >= size)                                   \
        return offset

#ifdef __cplusplus
extern "C" {
#endif

mystatus_t myhvml_tokenizer_begin(myhvml_tree_t* tree, const char* hvml, size_t hvml_length);
mystatus_t myhvml_tokenizer_chunk(myhvml_tree_t* tree, const char* hvml, size_t hvml_length);
mystatus_t myhvml_tokenizer_chunk_with_stream_buffer(myhvml_tree_t* tree, const char* hvml, size_t hvml_length);
mystatus_t myhvml_tokenizer_end(myhvml_tree_t* tree);
void myhvml_tokenizer_set_state(myhvml_tree_t* tree, myhvml_token_node_t* token_node);

void myhvml_tokenizer_calc_current_namespace(myhvml_tree_t* tree, myhvml_token_node_t* token_node);

myhvml_tree_node_t * myhvml_tokenizer_fragment_init(myhvml_tree_t* tree, myhvml_tag_id_t tag_idx, enum myhvml_namespace ns);

void myhvml_tokenizer_wait(myhvml_tree_t* tree);
void myhvml_tokenizer_post(myhvml_tree_t* tree);
void myhvml_tokenizer_pause(myhvml_tree_t* tree);

mystatus_t myhvml_tokenizer_state_init(myhvml_t* myhvml);
void myhvml_tokenizer_state_destroy(myhvml_t* myhvml);

myhvml_token_node_t * myhvml_tokenizer_queue_create_text_node_if_need(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t absolute_hvml_offset, enum myhvml_token_type type);
void myhvml_check_tag_parser(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset);

size_t myhvml_tokenizer_state_bogus_comment(myhvml_tree_t* tree, myhvml_token_node_t* token_node, const char* hvml, size_t hvml_offset, size_t hvml_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* MyHVML_TOKENIZER_H */
