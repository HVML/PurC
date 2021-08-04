/**
 * @file in_template.c.
 * @author
 * @date 2021/07/02
 * @brief The complementation of parsing html in template tag.
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
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#include "private/errors.h" 

#include "html/tree/insertion_mode.h"
#include "html/tree/open_elements.h"
#include "html/tree/active_formatting.h"
#include "html/tree/template_insertion.h"


/*
 * "caption", "colgroup", "tbody", "tfoot", "thead"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_template_ct(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    UNUSED_PARAM(token);

    pchtml_html_tree_template_insertion_pop(tree);

    tree->status = pchtml_html_tree_template_insertion_push(tree,
                                                         pchtml_html_tree_insertion_mode_in_table);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_table;

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_template_col(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    UNUSED_PARAM(token);

    pchtml_html_tree_template_insertion_pop(tree);

    tree->status = pchtml_html_tree_template_insertion_push(tree,
                                                         pchtml_html_tree_insertion_mode_in_column_group);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_column_group;

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_template_tr(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    UNUSED_PARAM(token);

    pchtml_html_tree_template_insertion_pop(tree);

    tree->status = pchtml_html_tree_template_insertion_push(tree,
                                                         pchtml_html_tree_insertion_mode_in_table_body);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_table_body;

    return false;
}

/*
 * "td", "th"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_template_tdth(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    UNUSED_PARAM(token);

    pchtml_html_tree_template_insertion_pop(tree);

    tree->status = pchtml_html_tree_template_insertion_push(tree,
                                                         pchtml_html_tree_insertion_mode_in_row);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_row;

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_template_end_of_file(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_open_elements_find(tree, PCHTML_TAG_TEMPLATE, PCHTML_NS_HTML,
                                            NULL);
    if (node == NULL) {
        tree->status =  pchtml_html_tree_stop_parsing(tree);
        if (tree->status != PCHTML_STATUS_OK) {
            return pchtml_html_tree_process_abort(tree);
        }

        return true;
    }

    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNENOFFI);

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_TEMPLATE,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_active_formatting_up_to_last_marker(tree);
    pchtml_html_tree_template_insertion_pop(tree);
    pchtml_html_tree_reset_insertion_mode_appropriately(tree);

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_template_anything_else(pchtml_html_tree_t *tree,
                                                       pchtml_html_token_t *token)
{
    UNUSED_PARAM(token);

    pchtml_html_tree_template_insertion_pop(tree);

    tree->status = pchtml_html_tree_template_insertion_push(tree,
                                                         pchtml_html_tree_insertion_mode_in_body);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_body;

    return false;
}

bool
pchtml_html_tree_insertion_mode_in_template(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
        if (token->tag_id == PCHTML_TAG_TEMPLATE) {
            return pchtml_html_tree_insertion_mode_in_head(tree, token);
        }

        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNCLTO);

        return true;
    }

    switch (token->tag_id) {
        case PCHTML_TAG__TEXT:
        case PCHTML_TAG__EM_COMMENT:
        case PCHTML_TAG__EM_DOCTYPE:
            return pchtml_html_tree_insertion_mode_in_body(tree, token);

        case PCHTML_TAG_BASE:
        case PCHTML_TAG_BASEFONT:
        case PCHTML_TAG_BGSOUND:
        case PCHTML_TAG_LINK:
        case PCHTML_TAG_META:
        case PCHTML_TAG_NOFRAMES:
        case PCHTML_TAG_SCRIPT:
        case PCHTML_TAG_STYLE:
        case PCHTML_TAG_TEMPLATE:
        case PCHTML_TAG_TITLE:
            return pchtml_html_tree_insertion_mode_in_head(tree, token);

        case PCHTML_TAG_CAPTION:
        case PCHTML_TAG_COLGROUP:
        case PCHTML_TAG_TBODY:
        case PCHTML_TAG_TFOOT:
        case PCHTML_TAG_THEAD:
            return pchtml_html_tree_insertion_mode_in_template_ct(tree, token);

        case PCHTML_TAG_COL:
            return pchtml_html_tree_insertion_mode_in_template_col(tree, token);

        case PCHTML_TAG_TR:
            return pchtml_html_tree_insertion_mode_in_template_tr(tree, token);

        case PCHTML_TAG_TD:
        case PCHTML_TAG_TH:
            return pchtml_html_tree_insertion_mode_in_template_tdth(tree, token);

        case PCHTML_TAG__END_OF_FILE:
            return pchtml_html_tree_insertion_mode_in_template_end_of_file(tree,
                                                                        token);
        default:
            return pchtml_html_tree_insertion_mode_in_template_anything_else(tree,
                                                                          token);
    }
}
