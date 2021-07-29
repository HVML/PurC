/**
 * @file in_select_in_table.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html with table selection.
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


/*
 * "caption", "table", "tbody", "tfoot", "thead", "tr", "td", "th"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_select_in_table_ct(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNTO);

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_SELECT,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return false;
}

/*
 * "caption", "table", "tbody", "tfoot", "thead", "tr", "td", "th"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_select_in_table_ct_closed(pchtml_html_tree_t *tree,
                                                          pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

    node = pchtml_html_tree_element_in_scope(tree, token->tag_id, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        return true;
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_SELECT,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_in_table_anything_else(pchtml_html_tree_t *tree,
                                                              pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_select(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_in_table_anything_else_closed(pchtml_html_tree_t *tree,
                                                                     pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_select_in_table_anything_else(tree,
                                                                         token);
}

bool
pchtml_html_tree_insertion_mode_in_select_in_table(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token)
{
    if (token->tag_id >= PCHTML_TAG__LAST_ENTRY) {
        if (token->type & PCHTML_PARSER_TOKEN_TYPE_CLOSE) {
            return pchtml_html_tree_insertion_mode_in_select_in_table_anything_else_closed(tree,
                                                                                        token);
        }

        return pchtml_html_tree_insertion_mode_in_select_in_table_anything_else(tree,
                                                                             token);
    }

    if (token->type & PCHTML_PARSER_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_CAPTION:
            case PCHTML_TAG_TABLE:
            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_TFOOT:
            case PCHTML_TAG_THEAD:
            case PCHTML_TAG_TR:
            case PCHTML_TAG_TH:
            case PCHTML_TAG_TD:
                return pchtml_html_tree_insertion_mode_in_select_in_table_ct_closed(tree,
                                                                                 token);
            default:
                return pchtml_html_tree_insertion_mode_in_select_in_table_anything_else_closed(tree,
                                                                                            token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG_CAPTION:
        case PCHTML_TAG_TABLE:
        case PCHTML_TAG_TBODY:
        case PCHTML_TAG_TFOOT:
        case PCHTML_TAG_THEAD:
        case PCHTML_TAG_TR:
        case PCHTML_TAG_TH:
        case PCHTML_TAG_TD:
            return pchtml_html_tree_insertion_mode_in_select_in_table_ct(tree,
                                                                      token);
        default:
            return pchtml_html_tree_insertion_mode_in_select_in_table_anything_else(tree,
                                                                                 token);
    }
}
