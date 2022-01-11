/**
 * @file in_head.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in head tag.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/tree/insertion_mode.h"
#include "html/tree/open_elements.h"
#include "html/tree/active_formatting.h"
#include "html/tree/template_insertion.h"
#include "html/interfaces/script_element.h"
#include "html/interfaces/template_element.h"
#include "html/tokenizer/state_script.h"


static bool
pchtml_html_tree_insertion_mode_in_head_open(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token);

static bool
pchtml_html_tree_insertion_mode_in_head_closed(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token);

static inline bool
pchtml_html_tree_insertion_mode_in_head_script(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token);

static inline bool
pchtml_html_tree_insertion_mode_in_head_template(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token);

static inline bool
pchtml_html_tree_insertion_mode_in_head_template_closed(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token);

static inline bool
pchtml_html_tree_insertion_mode_in_head_anything_else(pchtml_html_tree_t *tree);


bool
pchtml_html_tree_insertion_mode_in_head(pchtml_html_tree_t *tree,
                                     pchtml_html_token_t *token)
{
    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
        return pchtml_html_tree_insertion_mode_in_head_closed(tree, token);;
    }

    return pchtml_html_tree_insertion_mode_in_head_open(tree, token);
}

static bool
pchtml_html_tree_insertion_mode_in_head_open(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG__EM_COMMENT: {
            pcdom_comment_t *comment;

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
                                      PCHTML_HTML_RULES_ERROR_DOTOINHEMO);
            break;

        case PCHTML_TAG_HTML:
            return pchtml_html_tree_insertion_mode_in_body(tree, token);

        case PCHTML_TAG_BASE:
        case PCHTML_TAG_BASEFONT:
        case PCHTML_TAG_BGSOUND:
        case PCHTML_TAG_LINK: {
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

        case PCHTML_TAG_META: {
            pchtml_html_element_t *element;

            element = pchtml_html_tree_insert_html_element(tree, token);
            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            pchtml_html_tree_open_elements_pop(tree);
            pchtml_html_tree_acknowledge_token_self_closing(tree, token);

            /*
             * TODO: Check encoding: charset attribute or http-equiv attribute.
             */

            break;
        }

        case PCHTML_TAG_TITLE: {
            pchtml_html_element_t *element;

            element = pchtml_html_tree_generic_rcdata_parsing(tree, token);
            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG_NOSCRIPT: {
            pchtml_html_element_t *element;

            if (tree->document->dom_document.scripting) {
                element = pchtml_html_tree_generic_rawtext_parsing(tree, token);
            }
            else {
                element = pchtml_html_tree_insert_html_element(tree, token);
                tree->mode = pchtml_html_tree_insertion_mode_in_head_noscript;
            }

            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG_NOFRAMES:
        case PCHTML_TAG_STYLE: {
            pchtml_html_element_t *element;

            element = pchtml_html_tree_generic_rawtext_parsing(tree, token);
            if (element == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

                return pchtml_html_tree_process_abort(tree);
            }

            break;
        }

        case PCHTML_TAG_SCRIPT:
            return pchtml_html_tree_insertion_mode_in_head_script(tree, token);

        case PCHTML_TAG_TEMPLATE:
            return pchtml_html_tree_insertion_mode_in_head_template(tree, token);

        case PCHTML_TAG_HEAD:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_HETOINHEMO);
            break;

        /*
         * We can create function for this, but...
         *
         * The "in head noscript" insertion mode use this
         * is you change this code, please, change it in in head noscript" mode
         */
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
            return pchtml_html_tree_insertion_mode_in_head_anything_else(tree);
    }

    return true;
}

static bool
pchtml_html_tree_insertion_mode_in_head_closed(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    switch (token->tag_id) {
        case PCHTML_TAG_HEAD:
            pchtml_html_tree_open_elements_pop(tree);

            tree->mode = pchtml_html_tree_insertion_mode_after_head;

            break;

        case PCHTML_TAG_BODY:
        case PCHTML_TAG_HTML:
        case PCHTML_TAG_BR:
            return pchtml_html_tree_insertion_mode_in_head_anything_else(tree);

        case PCHTML_TAG_TEMPLATE:
            return pchtml_html_tree_insertion_mode_in_head_template_closed(tree,
                                                                        token);

        default:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_UNCLTOINHEMO);
            break;

    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_head_script(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pcdom_node_t *ap_node;
    pchtml_html_element_t *element;
    pchtml_html_tree_insertion_position_t ipos;

    ap_node = pchtml_html_tree_appropriate_place_inserting_node(tree, NULL, &ipos);
    if (ap_node == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR;

        return pchtml_html_tree_process_abort(tree);
    }

    if (ipos == PCHTML_HTML_TREE_INSERTION_POSITION_CHILD) {
        element = pchtml_html_tree_create_element_for_token(tree, token,
                                                         PCHTML_NS_HTML, ap_node);
    }
    else {
        element = pchtml_html_tree_create_element_for_token(tree, token,
                                                         PCHTML_NS_HTML,
                                                         ap_node->parent);
    }

    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    /* TODO: Need code for set flags for Script Element */

    tree->status = pchtml_html_tree_open_elements_push(tree,
                                                    pcdom_interface_node(element));
    if (tree->status != PCHTML_STATUS_OK) {
        pchtml_html_script_element_interface_destroy(pchtml_html_interface_script(element));

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tree_insert_node(ap_node, pcdom_interface_node(element), ipos);

    /*
     * Need for tokenizer state Script
     * See description for
     * 'pchtml_html_tokenizer_state_script_data_before' function
     */
    pchtml_html_tokenizer_tmp_tag_id_set(tree->tkz_ref, token->tag_id);
    pchtml_html_tokenizer_state_set(tree->tkz_ref,
                                 pchtml_html_tokenizer_state_script_data_before);

    tree->original_mode = tree->mode;
    tree->mode = pchtml_html_tree_insertion_mode_text;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_head_template(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->status = pchtml_html_tree_active_formatting_push_marker(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        pchtml_html_template_element_interface_destroy(pchtml_html_interface_template(element));

        return pchtml_html_tree_process_abort(tree);
    }

    tree->frameset_ok = false;
    tree->mode = pchtml_html_tree_insertion_mode_in_template;

    tree->status = pchtml_html_tree_template_insertion_push(tree,
                                      pchtml_html_tree_insertion_mode_in_template);
    if (tree->status != PCHTML_STATUS_OK) {
        pchtml_html_template_element_interface_destroy(pchtml_html_interface_template(element));

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_head_template_closed(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    pcdom_node_t *temp_node;

    temp_node = pchtml_html_tree_open_elements_find_reverse(tree, PCHTML_TAG_TEMPLATE,
                                                         PCHTML_NS_HTML, NULL);
    if (temp_node == NULL) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_TECLTOWIOPINHEMO);
        return true;
    }

    pchtml_html_tree_generate_all_implied_end_tags_thoroughly(tree, PCHTML_TAG__UNDEF,
                                                           PCHTML_NS__UNDEF);

    temp_node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(temp_node, PCHTML_TAG_TEMPLATE) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_TEELISNOCUINHEMO);
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_TEMPLATE,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_active_formatting_up_to_last_marker(tree);
    pchtml_html_tree_template_insertion_pop(tree);
    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_head_anything_else(pchtml_html_tree_t *tree)
{
    pchtml_html_tree_open_elements_pop(tree);

    tree->mode = pchtml_html_tree_insertion_mode_after_head;

    return false;
}
