/**
 * @file after_after_body.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of pseudo after tag after body tag.
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

#include "html/html/tree/insertion_mode.h"


bool
pchtml_html_tree_insertion_mode_after_after_body(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG__EM_COMMENT: {
            pcedom_comment_t *comment;

            comment = pchtml_html_tree_insert_comment(tree, token,
                                                   pcedom_interface_node(tree->document));
            if (comment == NULL) {
                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG__EM_DOCTYPE:
        case PCHTML_TAG_HTML:
            return pchtml_html_tree_insertion_mode_in_body(tree, token);

        case PCHTML_TAG__END_OF_FILE:
            tree->status = pchtml_html_tree_stop_parsing(tree);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            break;

        case PCHTML_TAG__TEXT: {
            pchtml_html_token_t ws_token = *token;

            tree->status = pchtml_html_token_data_skip_ws_begin(&ws_token);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            if (ws_token.text_start == ws_token.text_end) {
                return pchtml_html_tree_insertion_mode_in_body(tree, token);
            }
        }
        /* fall through */

        default:
            pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

            tree->mode = pchtml_html_tree_insertion_mode_in_body;

            return false;
    }

    return true;
}
