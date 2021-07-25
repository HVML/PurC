/**
 * @file before_html.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of pseudo before tag before html tag.
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
#include "html/html/tree/open_elements.h"
#include "html/html/interfaces/html_element.h"


static bool
pchtml_html_tree_insertion_mode_before_html_open(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token);

static bool
pchtml_html_tree_insertion_mode_before_html_closed(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token);

static inline bool
pchtml_html_tree_insertion_mode_before_html_anything_else(pchtml_html_tree_t *tree);

static inline unsigned int
pchtml_html_tree_insertion_mode_before_html_html(pchtml_html_tree_t *tree,
                                              pchtml_dom_node_t *node_html);


bool
pchtml_html_tree_insertion_mode_before_html(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
        return pchtml_html_tree_insertion_mode_before_html_closed(tree, token);;
    }

    return pchtml_html_tree_insertion_mode_before_html_open(tree, token);
}

static bool
pchtml_html_tree_insertion_mode_before_html_open(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG__EM_DOCTYPE:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_DOTOINBEHTMO);
            break;

        case PCHTML_TAG__EM_COMMENT: {
            pchtml_dom_comment_t *comment;

            comment = pchtml_html_tree_insert_comment(tree, token,
                                        pchtml_dom_interface_node(tree->document));
            if (comment == NULL) {
                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG_HTML: {
            pchtml_dom_node_t *node_html;
            pchtml_html_element_t *element;

            element = pchtml_html_tree_create_element_for_token(tree, token,
                                            PCHTML_NS_HTML,
                                            &tree->document->dom_document.node);
            if (element == NULL) {
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            node_html = pchtml_dom_interface_node(element);

            tree->status = pchtml_html_tree_insertion_mode_before_html_html(tree,
                                                                     node_html);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            tree->mode = pchtml_html_tree_insertion_mode_before_head;

            break;
        }

        case PCHTML_TAG__TEXT:
            tree->status = pchtml_html_token_data_skip_ws_begin(token);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            if (token->text_start == token->text_end) {
                return true;
            }
            /* fall through */

        default:
            return pchtml_html_tree_insertion_mode_before_html_anything_else(tree);
    }

    return true;
}

static bool
pchtml_html_tree_insertion_mode_before_html_closed(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG_HEAD:
        case PCHTML_TAG_BODY:
        case PCHTML_TAG_HTML:
        case PCHTML_TAG_BR:
            return pchtml_html_tree_insertion_mode_before_html_anything_else(tree);

        default:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_UNCLTOINBEHTMO);
            break;
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_before_html_anything_else(pchtml_html_tree_t *tree)
{
    pchtml_dom_node_t *node_html;

    node_html = pchtml_html_tree_create_node(tree, PCHTML_TAG_HTML, PCHTML_NS_HTML);
    if (node_html == NULL) {
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        return pchtml_html_tree_process_abort(tree);
    }

    tree->status = pchtml_html_tree_insertion_mode_before_html_html(tree,
                                                                 node_html);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_before_head;

    return false;
}

static inline unsigned int
pchtml_html_tree_insertion_mode_before_html_html(pchtml_html_tree_t *tree,
                                              pchtml_dom_node_t *node_html)
{
    unsigned int status;

    status = pchtml_html_tree_open_elements_push(tree, node_html);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    pchtml_html_tree_insert_node(pchtml_dom_interface_node(tree->document),
                              node_html,
                              PCHTML_HTML_TREE_INSERTION_POSITION_CHILD);

    pchtml_dom_document_attach_element(&tree->document->dom_document,
                                    pchtml_dom_interface_element(node_html));

    return PCHTML_STATUS_OK;
}
