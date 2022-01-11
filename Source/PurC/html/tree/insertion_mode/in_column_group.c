/**
 * @file in_column_group_.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in table column group.
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
 * License, Version 2.0:
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
#include "html/tree/open_elements.h"


static inline bool
pchtml_html_tree_insertion_mode_in_column_group_anything_else(pchtml_html_tree_t *tree,
                                                           pchtml_html_token_t *token)
{
    pcdom_node_t *node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_COLGROUP) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_MIELINOPELST);

        return true;
    }

    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_table;

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_text(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    pchtml_html_token_t ws_token = {0};

    tree->status = pchtml_html_token_data_split_ws_begin(token, &ws_token);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    if (ws_token.text_start != ws_token.text_end) {
        tree->status = pchtml_html_tree_insert_character(tree, &ws_token, NULL);
        if (tree->status != PCHTML_STATUS_OK) {
            return pchtml_html_tree_process_abort(tree);
        }
    }

    if (token->text_start == token->text_end) {
        return true;
    }

    return pchtml_html_tree_insertion_mode_in_column_group_anything_else(tree,
                                                                      token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_comment(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    pcdom_comment_t *comment;

    comment = pchtml_html_tree_insert_comment(tree, token, NULL);
    if (comment == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_html(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_body(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_col(pchtml_html_tree_t *tree,
                                                 pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tree_open_elements_pop(tree);
    pchtml_html_tree_acknowledge_token_self_closing(tree, token);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_colgroup_closed(pchtml_html_tree_t *tree,
                                                             pchtml_html_token_t *token)
{
    pcdom_node_t *node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_COLGROUP) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_MIELINOPELST);

        return true;
    }

    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_table;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_col_closed(pchtml_html_tree_t *tree,
                                                        pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNCLTO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_template_open_closed(pchtml_html_tree_t *tree,
                                                                  pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_head(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_end_of_file(pchtml_html_tree_t *tree,
                                                         pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_body(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_column_group_anything_else_closed(pchtml_html_tree_t *tree,
                                                                  pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_column_group_anything_else(tree, token);
}

bool
pchtml_html_tree_insertion_mode_in_column_group(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_COLGROUP:
                return pchtml_html_tree_insertion_mode_in_column_group_colgroup_closed(tree,
                                                                                    token);
            case PCHTML_TAG_COL:
                return pchtml_html_tree_insertion_mode_in_column_group_col_closed(tree,
                                                                               token);
            case PCHTML_TAG_TEMPLATE:
                return pchtml_html_tree_insertion_mode_in_column_group_template_open_closed(tree,
                                                                                         token);
            default:
                return pchtml_html_tree_insertion_mode_in_column_group_anything_else_closed(tree,
                                                                                         token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG__TEXT:
            return pchtml_html_tree_insertion_mode_in_column_group_text(tree,
                                                                     token);
        case PCHTML_TAG__EM_COMMENT:
            return pchtml_html_tree_insertion_mode_in_column_group_comment(tree,
                                                                        token);
        case PCHTML_TAG_HTML:
            return pchtml_html_tree_insertion_mode_in_column_group_html(tree,
                                                                     token);
        case PCHTML_TAG_COL:
            return pchtml_html_tree_insertion_mode_in_column_group_col(tree,
                                                                    token);
        case PCHTML_TAG_TEMPLATE:
            return pchtml_html_tree_insertion_mode_in_column_group_template_open_closed(tree,
                                                                                     token);

        case PCHTML_TAG__END_OF_FILE:
            return pchtml_html_tree_insertion_mode_in_column_group_end_of_file(tree,
                                                                            token);
        default:
            return pchtml_html_tree_insertion_mode_in_column_group_anything_else(tree,
                                                                              token);
    }
}
