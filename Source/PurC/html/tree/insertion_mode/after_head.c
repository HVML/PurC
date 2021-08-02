/**
 * @file after_head.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of pseudo after tag after head tag.
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


static bool
pchtml_html_tree_insertion_mode_after_head_open(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token);
static bool
pchtml_html_tree_insertion_mode_after_head_closed(pchtml_html_tree_t *tree,
                                               pchtml_html_token_t *token);

static inline bool
pchtml_html_tree_insertion_mode_after_head_anything_else(pchtml_html_tree_t *tree);

static inline pchtml_html_element_t *
pchtml_html_tree_insertion_mode_after_head_create_body(pchtml_html_tree_t *tree,
                                                    pchtml_html_token_t *token);


bool
pchtml_html_tree_insertion_mode_after_head(pchtml_html_tree_t *tree,
                                        pchtml_html_token_t *token)
{
    if (token->type & PCHTML_PARSER_TOKEN_TYPE_CLOSE) {
        return pchtml_html_tree_insertion_mode_after_head_closed(tree, token);
    }

    return pchtml_html_tree_insertion_mode_after_head_open(tree, token);
}

static bool
pchtml_html_tree_insertion_mode_after_head_open(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG__EM_COMMENT: {
            pcedom_comment_t *comment;

            comment = pchtml_html_tree_insert_comment(tree, token, NULL);
            if (comment == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG__EM_DOCTYPE:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_PARSER_RULES_ERROR_DOTOAFHEMO);
            break;

        case PCHTML_TAG_HTML:
            return pchtml_html_tree_insertion_mode_in_body(tree, token);

        case PCHTML_TAG_BODY: {
            pchtml_html_element_t *element;

            element = pchtml_html_tree_insertion_mode_after_head_create_body(tree,
                                                                          token);
            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            tree->frameset_ok = false;
            tree->mode = pchtml_html_tree_insertion_mode_in_body;

            break;
        }

        case PCHTML_TAG_FRAMESET: {
            pchtml_html_element_t *element;

            element = pchtml_html_tree_insert_html_element(tree, token);
            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            tree->mode = pchtml_html_tree_insertion_mode_in_frameset;

            break;
        }

        case PCHTML_TAG_BASE:
        case PCHTML_TAG_BASEFONT:
        case PCHTML_TAG_BGSOUND:
        case PCHTML_TAG_LINK:
        case PCHTML_TAG_META:
        case PCHTML_TAG_NOFRAMES:
        case PCHTML_TAG_SCRIPT:
        case PCHTML_TAG_STYLE:
        case PCHTML_TAG_TEMPLATE:
        case PCHTML_TAG_TITLE: {
            pcedom_node_t *head_node;

            head_node = pcedom_interface_node(tree->document->head);
            if (head_node == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR;

                return pchtml_html_tree_process_abort(tree);
            }

            pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNTO);

            tree->status = pchtml_html_tree_open_elements_push(tree, head_node);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            pchtml_html_tree_insertion_mode_in_head(tree, token);
            if (tree->status != PCHTML_STATUS_OK) {
                return pchtml_html_tree_process_abort(tree);
            }

            pchtml_html_tree_open_elements_remove_by_node(tree, head_node);

            break;
        }

        case PCHTML_TAG_HEAD:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_PARSER_RULES_ERROR_HETOAFHEMO);
            break;

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
            return pchtml_html_tree_insertion_mode_after_head_anything_else(tree);
    }

    return true;
}

static bool
pchtml_html_tree_insertion_mode_after_head_closed(pchtml_html_tree_t *tree,
                                               pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG_TEMPLATE:
            return pchtml_html_tree_insertion_mode_in_head(tree, token);

        case PCHTML_TAG_BODY:
        case PCHTML_TAG_HTML:
        case PCHTML_TAG_BR:
            return pchtml_html_tree_insertion_mode_after_head_anything_else(tree);

        default:
            pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

            break;
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_after_head_anything_else(pchtml_html_tree_t *tree)
{
    pchtml_html_element_t *element;
    pchtml_html_token_t fake_token = {0};

    fake_token.tag_id = PCHTML_TAG_BODY;

    element = pchtml_html_tree_insertion_mode_after_head_create_body(tree,
                                                                  &fake_token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_body;

    return false;
}

static inline pchtml_html_element_t *
pchtml_html_tree_insertion_mode_after_head_create_body(pchtml_html_tree_t *tree,
                                                    pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        return NULL;
    }

    tree->document->body = pchtml_html_interface_body(element);

    return element;
}
