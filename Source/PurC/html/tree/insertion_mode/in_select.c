/**
 * @file in_select.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in selection.
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


static inline bool
pchtml_html_tree_insertion_mode_in_select_text(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pchtml_str_t str;

    if (token->null_count != 0) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_NUCH);

        tree->status = pchtml_html_token_make_text_drop_null(token, &str,
                                                          tree->document->dom_document.text);
    }
    else {
        tree->status = pchtml_html_token_make_text(token, &str,
                                                tree->document->dom_document.text);
    }

    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    /* Can be zero only if all NULL are gone */
    if (str.length == 0) {
        pchtml_str_destroy(&str, tree->document->dom_document.text, false);

        return true;
    }

    tree->status = pchtml_html_tree_insert_character_for_data(tree, &str, NULL);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_comment(pchtml_html_tree_t *tree,
                                               pchtml_html_token_t *token)
{
    pcedom_comment_t *comment;

    comment = pchtml_html_tree_insert_comment(tree, token, NULL);
    if (comment == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_doctype(pchtml_html_tree_t *tree,
                                               pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_DOTOINSEMO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_html(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_body(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_option(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;
    pcedom_node_t *node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_OPTION)) {
        pchtml_html_tree_open_elements_pop(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_optgroup(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;
    pcedom_node_t *node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_OPTION)) {
        pchtml_html_tree_open_elements_pop(tree);
    }

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_OPTGROUP)) {
        pchtml_html_tree_open_elements_pop(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_optgroup_closed(pchtml_html_tree_t *tree,
                                                       pchtml_html_token_t *token)
{
    pcedom_node_t *node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_OPTION)
        && tree->open_elements->length > 1)
    {
        node = pchtml_html_tree_open_elements_get(tree,
                                               tree->open_elements->length - 2);
        if (node != NULL && pchtml_html_tree_node_is(node, PCHTML_TAG_OPTGROUP)) {
            pchtml_html_tree_open_elements_pop(tree);
        }
    }

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_OPTGROUP) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
        return true;
    }

    pchtml_html_tree_open_elements_pop(tree);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_option_closed(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    pcedom_node_t *node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_OPTION) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
        return true;
    }

    pchtml_html_tree_open_elements_pop(tree);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_select_closed(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_SELECT, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_SELECT);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNCLTO);

        return true;
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_SELECT,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_select(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_SELECT, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_SELECT);
    if (node == NULL) {
        return true;
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_SELECT,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return true;
}

/*
 * "input", "keygen", "textarea"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_select_ikt(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_SELECT, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_SELECT);
    if (node == NULL) {
        return true;
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_SELECT,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return false;
}

/*
 * A start tag whose tag name is one of: "script", "template"
 * An end tag whose tag name is "template"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_select_st_open_closed(pchtml_html_tree_t *tree,
                                                      pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_head(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_end_of_file(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_body(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_anything_else(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_select_anything_else_closed(pchtml_html_tree_t *tree,
                                                            pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNCLTO);

    return true;
}

bool
pchtml_html_tree_insertion_mode_in_select(pchtml_html_tree_t *tree,
                                       pchtml_html_token_t *token)
{
    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_OPTGROUP:
                return pchtml_html_tree_insertion_mode_in_select_optgroup_closed(tree,
                                                                              token);
            case PCHTML_TAG_OPTION:
                return pchtml_html_tree_insertion_mode_in_select_option_closed(tree,
                                                                            token);
            case PCHTML_TAG_SELECT:
                return pchtml_html_tree_insertion_mode_in_select_select_closed(tree,
                                                                            token);
            case PCHTML_TAG_TEMPLATE:
                return pchtml_html_tree_insertion_mode_in_select_st_open_closed(tree,
                                                                             token);
            default:
                return pchtml_html_tree_insertion_mode_in_select_anything_else_closed(tree,
                                                                                   token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG__TEXT:
            return pchtml_html_tree_insertion_mode_in_select_text(tree, token);

        case PCHTML_TAG__EM_COMMENT:
            return pchtml_html_tree_insertion_mode_in_select_comment(tree, token);

        case PCHTML_TAG__EM_DOCTYPE:
            return pchtml_html_tree_insertion_mode_in_select_doctype(tree, token);

        case PCHTML_TAG_HTML:
            return pchtml_html_tree_insertion_mode_in_select_html(tree, token);

        case PCHTML_TAG_OPTION:
            return pchtml_html_tree_insertion_mode_in_select_option(tree, token);

        case PCHTML_TAG_OPTGROUP:
            return pchtml_html_tree_insertion_mode_in_select_optgroup(tree, token);

        case PCHTML_TAG_SELECT:
            return pchtml_html_tree_insertion_mode_in_select_select(tree, token);

        case PCHTML_TAG_INPUT:
        case PCHTML_TAG_KEYGEN:
        case PCHTML_TAG_TEXTAREA:
            return pchtml_html_tree_insertion_mode_in_select_ikt(tree, token);

        case PCHTML_TAG_SCRIPT:
        case PCHTML_TAG_TEMPLATE:
            return pchtml_html_tree_insertion_mode_in_select_st_open_closed(tree,
                                                                         token);
        case PCHTML_TAG__END_OF_FILE:
            return pchtml_html_tree_insertion_mode_in_select_end_of_file(tree,
                                                                      token);
        default:
            return pchtml_html_tree_insertion_mode_in_select_anything_else(tree,
                                                                        token);
    }
}
