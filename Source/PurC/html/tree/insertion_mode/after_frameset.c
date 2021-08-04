/**
 * @file after_frameset.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of pseudo after tag after frameset tag.
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


#include "html/tree/insertion_mode.h"


bool
pchtml_html_tree_insertion_mode_after_frameset(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
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
                                      PCHTML_HTML_RULES_ERROR_DOTOAFFRMO);
            break;

        case PCHTML_TAG_HTML:
            if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
                tree->mode = pchtml_html_tree_insertion_mode_after_after_frameset;

                return true;
            }

            return pchtml_html_tree_insertion_mode_in_body(tree, token);

        case PCHTML_TAG_NOFRAMES:
            return pchtml_html_tree_insertion_mode_in_head(tree, token);

        case PCHTML_TAG__END_OF_FILE: {
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
