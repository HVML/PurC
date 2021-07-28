/**
 * @file in_frameset.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in frameset tag.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/html/tree/insertion_mode.h"
#include "html/html/tree/open_elements.h"


bool
pchtml_html_tree_insertion_mode_in_frameset(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
        if (token->tag_id == PCHTML_TAG_FRAMESET)
        {
            pcedom_node_t *node;
            node = pchtml_html_tree_current_node(tree);

            if (node == pchtml_html_tree_open_elements_first(tree)) {
                pchtml_html_tree_parse_error(tree, token,
                                          PCHTML_HTML_RULES_ERROR_UNELINOPELST);
                return true;
            }

            pchtml_html_tree_open_elements_pop(tree);

            node = pchtml_html_tree_current_node(tree);

            if (tree->fragment == NULL
                && pchtml_html_tree_node_is(node, PCHTML_TAG_FRAMESET) == false)
            {
                tree->mode = pchtml_html_tree_insertion_mode_after_frameset;
            }

            return true;
        }

        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

        return true;
    }

    switch (token->tag_id) {
        case PCHTML_TAG__EM_COMMENT: {
            pcedom_comment_t *comment;

            comment = pchtml_html_tree_insert_comment(tree, token, NULL);
            if (comment == NULL) {
                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG__EM_DOCTYPE:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_DOTOINFRMO);
            break;

        case PCHTML_TAG_HTML:
            return pchtml_html_tree_insertion_mode_in_body(tree, token);

        case PCHTML_TAG_FRAMESET: {
            pchtml_html_element_t *element;

            element = pchtml_html_tree_insert_html_element(tree, token);
            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG_FRAME: {
            pchtml_html_element_t *element;

            element = pchtml_html_tree_insert_html_element(tree, token);
            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            pchtml_html_tree_open_elements_pop(tree);
            pchtml_html_tree_acknowledge_token_self_closing(tree, token);

            break;
        }

        case PCHTML_TAG_NOFRAMES:
            return pchtml_html_tree_insertion_mode_in_head(tree, token);

        case PCHTML_TAG__END_OF_FILE: {
            pcedom_node_t *node = pchtml_html_tree_current_node(tree);

            if (node != pchtml_html_tree_open_elements_first(tree)) {
                pchtml_html_tree_parse_error(tree, token,
                                          PCHTML_HTML_RULES_ERROR_UNELINOPELST);
            }

            tree->status = pchtml_html_tree_stop_parsing(tree);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG__TEXT: {
            size_t cur_len;
            pchtml_str_t str;

            tree->status = pchtml_html_token_make_text(token, &str,
                                                    tree->document->dom_document.text);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            cur_len = str.length;

            pchtml_str_stay_only_whitespace(&str);

            if (str.length != 0) {
                tree->status = pchtml_html_tree_insert_character_for_data(tree,
                                                                       &str,
                                                                       NULL);
                if (tree->status != PCHTML_STATUS_OK) {
                    return pchtml_html_tree_process_abort(tree);
                }
            }

            if (str.length == cur_len) {
                return true;
            }
        }
        /* fall through */

        default:
            pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

            break;
    }

    return true;
}
