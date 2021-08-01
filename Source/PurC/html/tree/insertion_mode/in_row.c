/**
 * @file in_row.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in table row.
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
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
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
#include "html/tree/active_formatting.h"


static void
pchtml_html_tree_clear_stack_back_to_table_row(pchtml_html_tree_t *tree)
{
    pcedom_node_t *current = pchtml_html_tree_current_node(tree);

    while ((current->local_name != PCHTML_TAG_TR
            && current->local_name != PCHTML_TAG_TEMPLATE
            && current->local_name != PCHTML_TAG_HTML)
           || current->ns != PCHTML_NS_HTML)
    {
        pchtml_html_tree_open_elements_pop(tree);
        current = pchtml_html_tree_current_node(tree);
    }
}

/*
 * "th", "td"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_row_thtd(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    pchtml_html_tree_clear_stack_back_to_table_row(tree);

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_cell;

    tree->status = pchtml_html_tree_active_formatting_push_marker(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_row_tr_closed(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_TR, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

        return true;
    }

    pchtml_html_tree_clear_stack_back_to_table_row(tree);
    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_table_body;

    return true;
}

/*
 * A start tag whose tag name is one of: "caption", "col", "colgroup", "tbody",
 * "tfoot", "thead", "tr"
 * An end tag whose tag name is "table"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_row_ct_open_closed(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_TR, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNTO);

        return true;
    }

    pchtml_html_tree_clear_stack_back_to_table_row(tree);
    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_table_body;

    return false;
}

/*
 * "tbody", "tfoot", "thead"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_row_tbtfth_closed(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, token->tag_id, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

        return true;
    }

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_TR, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        return true;
    }

    pchtml_html_tree_clear_stack_back_to_table_row(tree);
    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_table_body;

    return false;
}

/*
 * "body", "caption", "col", "colgroup", "html", "td", "th"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_row_bcht_closed(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_row_anything_else(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_table(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_row_anything_else_closed(pchtml_html_tree_t *tree,
                                                         pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_row_anything_else(tree, token);
}

bool
pchtml_html_tree_insertion_mode_in_row(pchtml_html_tree_t *tree,
                                    pchtml_html_token_t *token)
{
    if (token->type & PCHTML_PARSER_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_TR:
                return pchtml_html_tree_insertion_mode_in_row_tr_closed(tree,
                                                                     token);
            case PCHTML_TAG_TABLE:
                return pchtml_html_tree_insertion_mode_in_row_ct_open_closed(tree,
                                                                          token);
            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_TFOOT:
            case PCHTML_TAG_THEAD:
                return pchtml_html_tree_insertion_mode_in_row_tbtfth_closed(tree,
                                                                         token);
            case PCHTML_TAG_BODY:
            case PCHTML_TAG_CAPTION:
            case PCHTML_TAG_COL:
            case PCHTML_TAG_COLGROUP:
            case PCHTML_TAG_HTML:
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TH:
                return pchtml_html_tree_insertion_mode_in_row_bcht_closed(tree,
                                                                       token);
            default:
                return pchtml_html_tree_insertion_mode_in_row_anything_else_closed(tree,
                                                                                token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG_TH:
        case PCHTML_TAG_TD:
            return pchtml_html_tree_insertion_mode_in_row_thtd(tree, token);

        case PCHTML_TAG_CAPTION:
        case PCHTML_TAG_COL:
        case PCHTML_TAG_COLGROUP:
        case PCHTML_TAG_TBODY:
        case PCHTML_TAG_TFOOT:
        case PCHTML_TAG_THEAD:
        case PCHTML_TAG_TR:
            return pchtml_html_tree_insertion_mode_in_row_ct_open_closed(tree,
                                                                      token);
        default:
            return pchtml_html_tree_insertion_mode_in_row_anything_else(tree,
                                                                     token);
    }
}
