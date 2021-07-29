/**
 * @file in_head_noscript.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in head tag without script.
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


static bool
pchtml_html_tree_insertion_mode_in_head_noscript_open(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token);

static bool
pchtml_html_tree_insertion_mode_in_head_noscript_closed(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token);


bool
pchtml_html_tree_insertion_mode_in_head_noscript(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    if (token->type & PCHTML_PARSER_TOKEN_TYPE_CLOSE) {
       return pchtml_html_tree_insertion_mode_in_head_noscript_closed(tree, token);
    }

    return pchtml_html_tree_insertion_mode_in_head_noscript_open(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_head_noscript_anything_else(pchtml_html_tree_t *tree,
                                                            pchtml_html_token_t *token);


static bool
pchtml_html_tree_insertion_mode_in_head_noscript_open(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG__EM_DOCTYPE:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_PARSER_RULES_ERROR_DOTOINHENOMO);
            break;

        case PCHTML_TAG_HTML:
            return pchtml_html_tree_insertion_mode_in_body(tree, token);

        case PCHTML_TAG__EM_COMMENT:
        case PCHTML_TAG_BASEFONT:
        case PCHTML_TAG_BGSOUND:
        case PCHTML_TAG_LINK:
        case PCHTML_TAG_META:
        case PCHTML_TAG_NOFRAMES:
        case PCHTML_TAG_STYLE:
            return pchtml_html_tree_insertion_mode_in_head(tree, token);

        case PCHTML_TAG_HEAD:
        case PCHTML_TAG_NOSCRIPT:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_PARSER_RULES_ERROR_UNTO);
            break;

        /* CopyPast from "in head" insertion mode */
        case PCHTML_TAG__TEXT: {
            pchtml_html_token_t ws_token = {0};

            tree->status = pchtml_html_token_data_split_ws_begin(token, &ws_token);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            if (ws_token.text_start != ws_token.text_end) {
                tree->status = pchtml_html_tree_insert_character(tree, &ws_token,
                                                              NULL);
                if (tree->status != PCHTML_STATUS_OK) {
                    return pchtml_html_tree_process_abort(tree);
                }
            }

            if (token->text_start == token->text_end) {
                return true;
            }
        }
        /* fall through */

        default:
            return pchtml_html_tree_insertion_mode_in_head_noscript_anything_else(tree,
                                                                               token);
    }

    return true;
}

static bool
pchtml_html_tree_insertion_mode_in_head_noscript_closed(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    if(token->tag_id == PCHTML_TAG_BR) {
        return pchtml_html_tree_insertion_mode_in_head_noscript_anything_else(tree,
                                                                            token);
    }

    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNTO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_head_noscript_anything_else(pchtml_html_tree_t *tree,
                                                            pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token,
                              PCHTML_PARSER_RULES_ERROR_UNTO);

    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_in_head;

    return false;
}
