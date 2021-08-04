/**
 * @file in_table_text.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in table text.
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
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache 
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/tree/insertion_mode.h"
#include "html/token.h"


static void
pchtml_html_tree_insertion_mode_in_table_text_erase(pchtml_html_tree_t *tree);


bool
pchtml_html_tree_insertion_mode_in_table_text(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    unsigned int status;
    pchtml_str_t *text;
    pcutils_array_obj_t *pt_list = tree->pending_table.text_list;

    if (token->tag_id == PCHTML_TAG__TEXT) {
        if (token->null_count != 0) {
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_NUCH);
        }

        text = pcutils_array_obj_push(pt_list);
        if (text == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

            pchtml_html_tree_insertion_mode_in_table_text_erase(tree);

            return pchtml_html_tree_process_abort(tree);
        }

        if (token->null_count != 0) {
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_NUCH);

            tree->status = pchtml_html_token_make_text_drop_null(token, text,
                                                              tree->document->dom_document.text);
        }
        else {
            tree->status = pchtml_html_token_make_text(token, text,
                                                    tree->document->dom_document.text);
        }

        if (tree->status != PCHTML_STATUS_OK) {
            pchtml_html_tree_insertion_mode_in_table_text_erase(tree);

            return pchtml_html_tree_process_abort(tree);
        }

        if (text->length == 0) {
            pcutils_array_obj_pop(pt_list);
            pchtml_str_destroy(text, tree->document->dom_document.text, false);

            return true;
        }

        /*
         * The pchtml_html_token_data_skip_ws_begin function
         * can change token->text_start value.
         */
        size_t i_pos = pchtml_str_whitespace_from_begin(text);

        if (i_pos != text->length) {
            if (!tree->pending_table.have_non_ws) {
                tree->pending_table.have_non_ws = true;
            }
        }

        return true;
    }

    if (tree->pending_table.have_non_ws) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_CHINTATE);

        tree->foster_parenting = true;

        for (size_t i = 0; i < pcutils_array_obj_length(pt_list); i++) {
            text = pcutils_array_obj_get(pt_list, i);

            status = pchtml_html_tree_insertion_mode_in_body_text_append(tree,
                                                                      text);
            if (status != PCHTML_STATUS_OK) {
                pchtml_html_tree_insertion_mode_in_table_text_erase(tree);

                return pchtml_html_tree_process_abort(tree);
            }
        }

        tree->foster_parenting = false;
    }
    else {
        for (size_t i = 0; i < pcutils_array_obj_length(pt_list); i++) {
            text = pcutils_array_obj_get(pt_list, i);

            tree->status = pchtml_html_tree_insert_character_for_data(tree, text,
                                                                   NULL);
            if (tree->status != PCHTML_STATUS_OK) {
                pchtml_html_tree_insertion_mode_in_table_text_erase(tree);

                return pchtml_html_tree_process_abort(tree);
            }
        }
    }

    tree->mode = tree->original_mode;

    return false;
}

static void
pchtml_html_tree_insertion_mode_in_table_text_erase(pchtml_html_tree_t *tree)
{
    pchtml_str_t *text;
    pcutils_array_obj_t *pt_list = tree->pending_table.text_list;

    for (size_t i = 0; i < pcutils_array_obj_length(pt_list); i++) {
        text = pcutils_array_obj_get(pt_list, i);

        pchtml_str_destroy(text, tree->document->dom_document.text, false);
    }
}
