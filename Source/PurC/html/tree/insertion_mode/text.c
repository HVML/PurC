/**
 * @file text.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html text content.
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
#include "html/tree/open_elements.h"


bool
pchtml_html_tree_insertion_mode_text(pchtml_html_tree_t *tree,
                                  pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG__TEXT: {
            tree->status = pchtml_html_tree_insert_character(tree, token, NULL);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG__END_OF_FILE: {
            pcdom_node_t *node;

            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_UNENOFFI);

            node = pchtml_html_tree_current_node(tree);

            if (pchtml_html_tree_node_is(node, PCHTML_TAG_SCRIPT)) {
                /* TODO: mark the script element as "already started" */
            }

            pchtml_html_tree_open_elements_pop(tree);

            tree->mode = tree->original_mode;

            return false;
        }

        /* TODO: need to implement */
        case PCHTML_TAG_SCRIPT:
            pchtml_html_tree_open_elements_pop(tree);

            tree->mode = tree->original_mode;

            break;

        default:
            pchtml_html_tree_open_elements_pop(tree);

            tree->mode = tree->original_mode;

            break;
    }

    return true;
}
