/**
 * @file in_table.c.
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in table.
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

#include "html/tree/insertion_mode.h"
#include "html/tree/open_elements.h"
#include "html/tree/active_formatting.h"


static inline void
pchtml_html_tree_clear_stack_back_to_table_context(pchtml_html_tree_t *tree)
{
    pcedom_node_t *current = pchtml_html_tree_current_node(tree);

    while ((current->local_name != PCHTML_TAG_TABLE
            && current->local_name != PCHTML_TAG_TEMPLATE
            && current->local_name != PCHTML_TAG_HTML)
           || current->ns != PCHTML_NS_HTML)
    {
        pchtml_html_tree_open_elements_pop(tree);
        current = pchtml_html_tree_current_node(tree);
    }
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_text_open(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token)
{
    pcedom_node_t *node = pchtml_html_tree_current_node(tree);

    if (node->ns == PCHTML_NS_HTML &&
        (node->local_name == PCHTML_TAG_TABLE
         || node->local_name == PCHTML_TAG_TBODY
         || node->local_name == PCHTML_TAG_TFOOT
         || node->local_name == PCHTML_TAG_THEAD
         || node->local_name == PCHTML_TAG_TR))
    {
        tree->pending_table.text_list->length = 0;
        tree->pending_table.have_non_ws = false;

        tree->original_mode = tree->mode;
        tree->mode = pchtml_html_tree_insertion_mode_in_table_text;

        return false;
    }

    return pchtml_html_tree_insertion_mode_in_table_anything_else(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_comment(pchtml_html_tree_t *tree,
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
pchtml_html_tree_insertion_mode_in_table_doctype(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_DOTOINTAMO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_caption(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    pchtml_html_tree_clear_stack_back_to_table_context(tree);

    tree->status = pchtml_html_tree_active_formatting_push_marker(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_caption;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_colgroup(pchtml_html_tree_t *tree,
                                               pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    pchtml_html_tree_clear_stack_back_to_table_context(tree);

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_column_group;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_col(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    UNUSED_PARAM(token);

    pchtml_html_element_t *element;
    pchtml_html_token_t fake_token = {0};

    pchtml_html_tree_clear_stack_back_to_table_context(tree);

    fake_token.tag_id = PCHTML_TAG_COLGROUP;
    fake_token.attr_first = NULL;
    fake_token.attr_last = NULL;

    element = pchtml_html_tree_insert_html_element(tree, &fake_token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_column_group;

    return false;
}

/*
 * "tbody", "tfoot", "thead"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_table_tbtfth(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    pchtml_html_tree_clear_stack_back_to_table_context(tree);

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_table_body;

    return true;
}

/*
 * "td", "th", "tr"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_table_tdthtr(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    UNUSED_PARAM(token);

    pchtml_html_element_t *element;
    pchtml_html_token_t fake_token = {0};

    pchtml_html_tree_clear_stack_back_to_table_context(tree);

    fake_token.tag_id = PCHTML_TAG_TBODY;
    fake_token.attr_first = NULL;
    fake_token.attr_last = NULL;

    element = pchtml_html_tree_insert_html_element(tree, &fake_token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_table_body;

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_table(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNTO);

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_TABLE, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        return true;
    }

    pchtml_html_tree_open_elements_pop_until_node(tree, node, true);
    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_table_closed(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_TABLE, PCHTML_NS_HTML,
                                          PCHTML_PARSER_TAG_CATEGORY_SCOPE_TABLE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

        return true;
    }

    pchtml_html_tree_open_elements_pop_until_node(tree, node, true);
    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return true;
}

/*
 * "body", "caption", "col", "colgroup", "html", "tbody", "td", "tfoot", "th",
 * "thead", "tr"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_table_bcht_closed(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNCLTO);

    return true;
}

/*
 * A start tag whose tag name is one of: "style", "script", "template"
 * An end tag whose tag name is "template"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_table_st_open_closed(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_head(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_input(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;
    pchtml_html_token_attr_t *attr = token->attr_first;

    while (attr != NULL) {

        /* Name == "type" and value == "hidden" */
        if (attr->name != NULL && attr->name->attr_id == PCEDOM_ATTR_TYPE) {
            if (attr->value_size == 6
                && pchtml_str_data_ncasecmp(attr->value,
                                            (const unsigned char *) "hidden", 6))
            {
                goto have_hidden;
            }
        }

        attr = attr->next;
    }

    return pchtml_html_tree_insertion_mode_in_table_anything_else(tree, token);

have_hidden:

    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNTO);

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tree_open_elements_pop_until_node(tree,
                                               pcedom_interface_node(element),
                                               true);

    pchtml_html_tree_acknowledge_token_self_closing(tree, token);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_form(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    pchtml_html_tree_parse_error(tree, token, PCHTML_PARSER_RULES_ERROR_UNTO);

    if (tree->form != NULL) {
        return true;
    }

    node = pchtml_html_tree_open_elements_find_reverse(tree, PCHTML_TAG_TEMPLATE,
                                                    PCHTML_NS_HTML, NULL);
    if (node != NULL) {
        return true;
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->form = pchtml_html_interface_form(element);

    pchtml_html_tree_open_elements_pop_until_node(tree,
                                               pcedom_interface_node(element),
                                               true);
    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_end_of_file(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_body(tree, token);
}

bool
pchtml_html_tree_insertion_mode_in_table_anything_else(pchtml_html_tree_t *tree,
                                                    pchtml_html_token_t *token)
{
    tree->foster_parenting = true;

    pchtml_html_tree_insertion_mode_in_body(tree, token);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->foster_parenting = false;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_table_anything_else_closed(pchtml_html_tree_t *tree,
                                                           pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_table_anything_else(tree, token);
}

bool
pchtml_html_tree_insertion_mode_in_table(pchtml_html_tree_t *tree,
                                      pchtml_html_token_t *token)
{
    if (token->type & PCHTML_PARSER_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_TABLE:
                return pchtml_html_tree_insertion_mode_in_table_table_closed(tree,
                                                                          token);
            case PCHTML_TAG_BODY:
            case PCHTML_TAG_CAPTION:
            case PCHTML_TAG_COL:
            case PCHTML_TAG_COLGROUP:
            case PCHTML_TAG_HTML:
            case PCHTML_TAG_TBODY:
            case PCHTML_TAG_TD:
            case PCHTML_TAG_TFOOT:
            case PCHTML_TAG_TH:
            case PCHTML_TAG_THEAD:
            case PCHTML_TAG_TR:
                return pchtml_html_tree_insertion_mode_in_table_bcht_closed(tree,
                                                                         token);
            case PCHTML_TAG_TEMPLATE:
                return pchtml_html_tree_insertion_mode_in_table_st_open_closed(tree,
                                                                            token);
            default:
                return pchtml_html_tree_insertion_mode_in_table_anything_else_closed(tree,
                                                                                  token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG__TEXT:
            return pchtml_html_tree_insertion_mode_in_table_text_open(tree, token);

        case PCHTML_TAG__EM_COMMENT:
            return pchtml_html_tree_insertion_mode_in_table_comment(tree, token);

        case PCHTML_TAG__EM_DOCTYPE:
            return pchtml_html_tree_insertion_mode_in_table_doctype(tree, token);

        case PCHTML_TAG_CAPTION:
            return pchtml_html_tree_insertion_mode_in_table_caption(tree, token);

        case PCHTML_TAG_COLGROUP:
            return pchtml_html_tree_insertion_mode_in_table_colgroup(tree, token);

        case PCHTML_TAG_COL:
            return pchtml_html_tree_insertion_mode_in_table_col(tree, token);

        case PCHTML_TAG_TBODY:
        case PCHTML_TAG_TFOOT:
        case PCHTML_TAG_THEAD:
            return pchtml_html_tree_insertion_mode_in_table_tbtfth(tree, token);

        case PCHTML_TAG_TD:
        case PCHTML_TAG_TH:
        case PCHTML_TAG_TR:
            return pchtml_html_tree_insertion_mode_in_table_tdthtr(tree, token);

        case PCHTML_TAG_TABLE:
            return pchtml_html_tree_insertion_mode_in_table_table(tree, token);

        case PCHTML_TAG_STYLE:
        case PCHTML_TAG_SCRIPT:
        case PCHTML_TAG_TEMPLATE:
            return pchtml_html_tree_insertion_mode_in_table_st_open_closed(tree,
                                                                        token);
        case PCHTML_TAG_INPUT:
            return pchtml_html_tree_insertion_mode_in_table_input(tree, token);

        case PCHTML_TAG_FORM:
            return pchtml_html_tree_insertion_mode_in_table_form(tree, token);

        case PCHTML_TAG__END_OF_FILE:
            return pchtml_html_tree_insertion_mode_in_table_end_of_file(tree,
                                                                     token);
        default:
            return pchtml_html_tree_insertion_mode_in_table_anything_else(tree,
                                                                       token);
    }
}
