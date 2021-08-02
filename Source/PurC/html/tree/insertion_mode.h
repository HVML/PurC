/**
 * @file insertion_mode.h 
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html insertion mode.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_PARSER_TREE_INSERTION_MODE_H
#define PCHTML_PARSER_TREE_INSERTION_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/token.h"
#include "html/tree.h"


bool
pchtml_html_tree_insertion_mode_initial(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_before_html(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_before_head(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_head(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_head_noscript(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_after_head(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_body(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_body_skip_new_line(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_body_skip_new_line_textarea(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

unsigned int
pchtml_html_tree_insertion_mode_in_body_text_append(pchtml_html_tree_t *tree,
                pchtml_str_t *str) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_text(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_table(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_table_anything_else(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_table_text(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_caption(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_column_group(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_table_body(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_row(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_cell(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_select(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_select_in_table(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_template(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_after_body(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_in_frameset(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_after_frameset(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_after_after_body(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_after_after_frameset(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;

bool
pchtml_html_tree_insertion_mode_foreign_content(pchtml_html_tree_t *tree,
                pchtml_html_token_t *token) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_PARSER_TREE_INSERTION_MODE_H */
