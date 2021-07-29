/**
 * @file in_cell.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in table cell.
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
 */

#include "html/parser/tree/insertion_mode.h"
#include "html/parser/tree/open_elements.h"
#include "html/parser/tree/active_formatting.h"


static void
pchtml_html_tree_close_cell(pchtml_html_tree_t *tree, pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                            PCHTML_NS__UNDEF);

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_TD) == false
        && pchtml_html_tree_node_is(node, PCHTML_TAG_TH) == false)
    {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_PARSER_RULES_ERROR_MIELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_td_th(tree);
    pchtml_html_tree_active_formatting_up_to_last_marker(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_row;
}

/*
 * "td", "th"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_cell_tdth_closed(pchtml_html_tree_t *tree,
                                                 pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, token->tag_id, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

        return true;
    }

    pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                            PCHTML_NS__UNDEF);

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, token->tag_id) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_PARSER_RULES_ERROR_MIELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, token->tag_id,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_active_formatting_up_to_last_marker(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_row;

    return true;
}

/*
 * "caption", "col", "colgroup", "tbody", "td", "tfoot", "th", "thead", "tr"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_cell_ct(pchtml_html_tree_t *tree,
                                        pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope_td_th(tree);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_MIELINSC);

        return true;
    }

    pchtml_html_tree_close_cell(tree, token);

    return false;
}

/*
 * "body", "caption", "col", "colgroup", "html"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_cell_bch_closed(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

    return true;
}

/*
 * "table", "tbody", "tfoot", "thead", "tr"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_cell_t_closed(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, token->tag_id, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

        return true;
    }

    pchtml_html_tree_close_cell(tree, token);

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_cell_anything_else(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_body(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_cell_anything_else_closed(pchtml_html_tree_t *tree,
                                                          pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_cell_anything_else(tree, token);
}

bool
pchtml_html_tree_insertion_mode_in_cell(pchtml_html_tree_t *tree,
                                     pchtml_html_token_t *token)
{
    if (token->type & PCHTML_PARSER_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TH:
                return pchtml_html_tree_insertion_mode_in_cell_tdth_closed(tree,
                                                                        token);
            case PCHTML_TAG_BODY:
            case PCHTML_TAG_CAPTION:
            case PCHTML_TAG_COL:
            case PCHTML_TAG_COLGROUP:
            case PCHTML_TAG_HTML:
                return pchtml_html_tree_insertion_mode_in_cell_bch_closed(tree,
                                                                       token);
            case PCHTML_TAG_TABLE:
            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_TFOOT:
            case PCHTML_TAG_THEAD:
            case PCHTML_TAG_TR:
                return pchtml_html_tree_insertion_mode_in_cell_t_closed(tree,
                                                                     token);
            default:
                return pchtml_html_tree_insertion_mode_in_cell_anything_else_closed(tree,
                                                                                 token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG_CAPTION:
        case PCHTML_TAG_COL:
        case PCHTML_TAG_COLGROUP:
        case PCHTML_TAG_TBODY:
        case PCHTML_TAG_TD:
        case PCHTML_TAG_TFOOT:
        case PCHTML_TAG_TH:
        case PCHTML_TAG_THEAD:
        case PCHTML_TAG_TR:
            return pchtml_html_tree_insertion_mode_in_cell_ct(tree, token);

        default:
            return pchtml_html_tree_insertion_mode_in_cell_anything_else(tree,
                                                                      token);
    }
}
