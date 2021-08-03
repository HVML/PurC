/**
 * @file in_table_body.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in table.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/tree/insertion_mode.h"
#include "html/tree/open_elements.h"


static void
pchtml_html_tree_clear_stack_back_to_table_body(pchtml_html_tree_t *tree)
{
    pcedom_node_t *current = pchtml_html_tree_current_node(tree);

    while ((current->local_name != PCHTML_TAG_TBODY
            && current->local_name != PCHTML_TAG_TFOOT
            && current->local_name != PCHTML_TAG_THEAD
            && current->local_name != PCHTML_TAG_TEMPLATE
            && current->local_name != PCHTML_TAG_HTML)
           || current->ns != PCHTML_NS_HTML)
    {
        pchtml_html_tree_open_elements_pop(tree);
        current = pchtml_html_tree_current_node(tree);
    }
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_body_tr(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    pchtml_html_tree_clear_stack_back_to_table_body(tree);

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_row;

    return true;
}

/*
 * "th", "td"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_table_body_thtd(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token)
{
    pchtml_html_token_t fake_token;
    pchtml_html_element_t *element;

    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNTO);

    pchtml_html_tree_clear_stack_back_to_table_body(tree);

    fake_token = *token;

    fake_token.tag_id = PCHTML_TAG_TR;
    fake_token.attr_first = NULL;
    fake_token.attr_last = NULL;

    element = pchtml_html_tree_insert_html_element(tree, &fake_token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_row;

    return false;
}

/*
 * "tbody", "tfoot", "thead"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_table_body_tbtfth_closed(pchtml_html_tree_t *tree,
                                                         pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, token->tag_id, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_MIELINSC);

        return true;
    }

    pchtml_html_tree_clear_stack_back_to_table_body(tree);
    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_table;

    return true;
}

/*
 * A start tag whose tag name is one of: "caption", "col", "colgroup", "tbody",
 * "tfoot", "thead"
 * An end tag whose tag name is "table"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_table_body_ct_open_closed(pchtml_html_tree_t *tree,
                                                          pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope_tbody_thead_tfoot(tree);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_MIELINSC);

        return true;
    }

    pchtml_html_tree_clear_stack_back_to_table_body(tree);
    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_table;

    return false;
}

/*
 * "body", "caption", "col", "colgroup", "html", "td", "th", "tr"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_table_body_bcht_closed(pchtml_html_tree_t *tree,
                                                       pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_body_anything_else(pchtml_html_tree_t *tree,
                                                         pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_table(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_body_anything_else_closed(pchtml_html_tree_t *tree,
                                                                pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_table_body_anything_else(tree,
                                                                    token);
}

bool
pchtml_html_tree_insertion_mode_in_table_body(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    if (token->type & PCHTML_PARSER_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_TFOOT:
            case PCHTML_TAG_THEAD:
                return pchtml_html_tree_insertion_mode_in_table_body_tbtfth_closed(tree,
                                                                                token);
            case PCHTML_TAG_TABLE:
                return pchtml_html_tree_insertion_mode_in_table_body_ct_open_closed(tree,
                                                                                 token);
            case PCHTML_TAG_BODY:
            case PCHTML_TAG_CAPTION:
            case PCHTML_TAG_COL:
            case PCHTML_TAG_COLGROUP:
            case PCHTML_TAG_HTML:
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TH:
            case PCHTML_TAG_TR:
                return pchtml_html_tree_insertion_mode_in_table_body_bcht_closed(tree,
                                                                              token);
            default:
                return pchtml_html_tree_insertion_mode_in_table_body_anything_else_closed(tree,
                                                                                       token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG_TR:
            return pchtml_html_tree_insertion_mode_in_table_body_tr(tree, token);

        case PCHTML_TAG_TH:
        case PCHTML_TAG_TD:
            return pchtml_html_tree_insertion_mode_in_table_body_thtd(tree, token);

        case PCHTML_TAG_CAPTION:
        case PCHTML_TAG_COL:
        case PCHTML_TAG_COLGROUP:
        case PCHTML_TAG_TBODY:
        case PCHTML_TAG_TFOOT:
        case PCHTML_TAG_THEAD:
            return pchtml_html_tree_insertion_mode_in_table_body_ct_open_closed(tree,
                                                                             token);
        default:
            return pchtml_html_tree_insertion_mode_in_table_body_anything_else(tree,
                                                                            token);
    }
}
