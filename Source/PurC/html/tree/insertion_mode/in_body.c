/**
 * @file in_body.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of parsing html in body tag.
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



#define PCHTML_TOKENIZER_CHARS_MAP
#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"

#include "str_res.h"

#include "html/tree/insertion_mode.h"
#include "html/tree/open_elements.h"
#include "html/tree/active_formatting.h"
#include "html/interfaces/head_element.h"
#include "html/tokenizer/state.h"
#include "html/tokenizer/state_rcdata.h"


/*
 * User case insertion mode.
 *
 * After "pre" and "listing" tag we need skip one newline in text tag.
 * Since we have a stream of tokens,
 * we can "look into the future" only in this way.
 */
bool
pchtml_html_tree_insertion_mode_in_body_skip_new_line(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token)
{
    tree->mode = tree->original_mode;

    if (token->tag_id != PCHTML_TAG__TEXT) {
        return false;
    }

    tree->status = pchtml_html_token_data_skip_one_newline_begin(token);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    if (token->text_start == token->text_end) {
        return true;
    }

    return false;
}

/*
 * User case insertion mode.
 *
 * After "textarea" tag we need skip one newline in text tag.
 */
bool
pchtml_html_tree_insertion_mode_in_body_skip_new_line_textarea(pchtml_html_tree_t *tree,
                                                            pchtml_html_token_t *token)
{
    tree->mode = tree->original_mode;

    if (token->tag_id != PCHTML_TAG__TEXT) {
        return false;
    }

    tree->status = pchtml_html_token_data_skip_one_newline_begin(token);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    if (token->text_start == token->text_end) {
        return true;
    }

    return false;
}

/* Open */
static inline bool
pchtml_html_tree_insertion_mode_in_body_text(pchtml_html_tree_t *tree,
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

    pchtml_html_tree_insertion_mode_in_body_text_append(tree, &str);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

unsigned int
pchtml_html_tree_insertion_mode_in_body_text_append(pchtml_html_tree_t *tree,
                                                 pchtml_str_t *str)
{
    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return tree->status;
    }

    if (tree->frameset_ok) {
        const unsigned char *pos = str->data;
        const unsigned char *end = str->data + str->length;

        while (pos != end) {
            if (pchtml_tokenizer_chars_map[*pos]
                != PCHTML_STR_RES_MAP_CHAR_WHITESPACE)
            {
                tree->frameset_ok = false;
                break;
            }

            pos++;
        }
    }

    tree->status = pchtml_html_tree_insert_character_for_data(tree, str, NULL);
    if (tree->status != PCHTML_STATUS_OK) {
        return tree->status;
    }

    return PCHTML_STATUS_OK;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_comment(pchtml_html_tree_t *tree,
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
pchtml_html_tree_insertion_mode_in_body_doctype(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_DOTOINBOMO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_html(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    pcedom_node_t *temp_node;
    pcedom_element_t *html;

    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

    temp_node = pchtml_html_tree_open_elements_find(tree, PCHTML_TAG_TEMPLATE,
                                                 PCHTML_NS_HTML, NULL);
    if (temp_node != NULL) {
        return true;
    }

    html = pcedom_interface_element(pchtml_html_tree_open_elements_first(tree));

    tree->status = pchtml_html_tree_append_attributes(tree, html, token,
                                                   html->node.ns);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

/*
 * Start tag:
 *      "base", "basefont", "bgsound", "link", "meta", "noframes",
 *      "script", "style", "template", "title"
 * End Tag:
 *      "template"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_blmnst(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    return pchtml_html_tree_insertion_mode_in_head(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_body(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    pcedom_node_t *node, *temp;
    pcedom_element_t *body;

    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

    node = pchtml_html_tree_open_elements_get(tree, 1);
    if (node == NULL || node->local_name != PCHTML_TAG_BODY) {
        return true;
    }

    temp = pchtml_html_tree_open_elements_find_reverse(tree, PCHTML_TAG_TEMPLATE,
                                                    PCHTML_NS_HTML, NULL);
    if (temp != NULL) {
        return true;
    }

    tree->frameset_ok = false;

    body = pcedom_interface_element(node);

    tree->status = pchtml_html_tree_append_attributes(tree, body, token, node->ns);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_frameset(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

    node = pchtml_html_tree_open_elements_get(tree, 1);
    if (node == NULL || node->local_name != PCHTML_TAG_BODY) {
        return true;
    }

    if (tree->frameset_ok == false) {
        return true;
    }

    pchtml_html_tree_node_delete_deep(tree, node);

    /* node is HTML */
    node = pchtml_html_tree_open_elements_get(tree, 0);
    pchtml_html_tree_open_elements_pop_until_node(tree, node, false);

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->mode = pchtml_html_tree_insertion_mode_in_frameset;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_eof(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    if (pcutils_array_obj_length(tree->template_insertion_modes) != 0) {
        return pchtml_html_tree_insertion_mode_in_template(tree, token);
    }

    bool it_is = pchtml_html_tree_check_scope_element(tree);
    if (it_is == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_BAENOPELISWR);
    }

    tree->status = pchtml_html_tree_stop_parsing(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_body_closed(pchtml_html_tree_t *tree,
                                                 pchtml_html_token_t *token)
{
    pcedom_node_t *body_node;

    body_node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_BODY, PCHTML_NS_HTML,
                                               PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (body_node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_NOBOELINSC);

        return true;
    }

    bool it_is = pchtml_html_tree_check_scope_element(tree);
    if (it_is == false) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_OPELISWR);
    }

    tree->mode = pchtml_html_tree_insertion_mode_after_body;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_html_closed(pchtml_html_tree_t *tree,
                                                 pchtml_html_token_t *token)
{
    pcedom_node_t *body_node;

    body_node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_BODY, PCHTML_NS_HTML,
                                               PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (body_node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_NOBOELINSC);

        return true;
    }

    bool it_is = pchtml_html_tree_check_scope_element(tree);
    if (it_is == false) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_OPELISWR);
    }

    tree->mode = pchtml_html_tree_insertion_mode_after_body;

    return false;
}

/*
 * "address", "article", "aside", "blockquote", "center", "details", "dialog",
 * "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "header",
 * "hgroup", "main", "menu", "nav", "ol", "p", "section", "summary", "ul"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_abcdfhmnopsu(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    pcedom_node_t *body_node;
    pchtml_html_element_t *element;

    body_node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                               PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (body_node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

/*
 * "h1", "h2", "h3", "h4", "h5", "h6"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_h123456(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
    }

    node = pchtml_html_tree_current_node(tree);

    switch (node->local_name) {
        case PCHTML_TAG_H1:
        case PCHTML_TAG_H2:
        case PCHTML_TAG_H3:
        case PCHTML_TAG_H4:
        case PCHTML_TAG_H5:
        case PCHTML_TAG_H6:
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_UNELINOPELST);

            pchtml_html_tree_open_elements_pop(tree);
            break;

        default:
            break;
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

/*
 * "pre", "listing"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_pre_listing(pchtml_html_tree_t *tree,
                                                 pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->original_mode = tree->mode;
    tree->mode = pchtml_html_tree_insertion_mode_in_body_skip_new_line;
    tree->frameset_ok = false;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_form(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    pcedom_node_t *node, *temp;
    pchtml_html_element_t *element;

    temp = pchtml_html_tree_open_elements_find_reverse(tree, PCHTML_TAG_TEMPLATE,
                                                    PCHTML_NS_HTML, NULL);

    if (tree->form != NULL && temp == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

        return true;
    }

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    if (temp == NULL) {
        tree->form = pchtml_html_interface_form(element);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_li(pchtml_html_tree_t *tree,
                                        pchtml_html_token_t *token)
{
    bool is_special;
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    void **list = tree->open_elements->list;
    size_t idx = tree->open_elements->length;

    tree->frameset_ok = false;

    while (idx != 0) {
        idx--;

        node = list[idx];

        if (pchtml_html_tree_node_is(node, PCHTML_TAG_LI)) {
            pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG_LI,
                                                    PCHTML_NS_HTML);

            node = pchtml_html_tree_current_node(tree);

            if (pchtml_html_tree_node_is(node, PCHTML_TAG_LI) == false) {
                pchtml_html_tree_parse_error(tree, token,
                                          PCHTML_HTML_RULES_ERROR_UNELINOPELST);
            }

            pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_LI,
                                                         PCHTML_NS_HTML, true);
            break;
        }

        is_special = pchtml_html_tag_is_category(node->local_name, node->ns,
                                              PCHTML_HTML_TAG_CATEGORY_SPECIAL);
        if (is_special
            && pchtml_html_tree_node_is(node, PCHTML_TAG_ADDRESS) == false
            && pchtml_html_tree_node_is(node, PCHTML_TAG_DIV) == false
            && pchtml_html_tree_node_is(node, PCHTML_TAG_P) == false)
        {
            break;
        }
    }

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

/*
 * "dd", "dt"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_dd_dt(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    bool is_special;
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    void **list = tree->open_elements->list;
    size_t idx = tree->open_elements->length;

    tree->frameset_ok = false;

    while (idx != 0) {
        idx--;

        node = list[idx];

        if (pchtml_html_tree_node_is(node, PCHTML_TAG_DD)) {
            pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG_DD,
                                                    PCHTML_NS_HTML);

            node = pchtml_html_tree_current_node(tree);

            if (pchtml_html_tree_node_is(node, PCHTML_TAG_DD) == false) {
                pchtml_html_tree_parse_error(tree, token,
                                          PCHTML_HTML_RULES_ERROR_UNELINOPELST);
            }

            pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_DD,
                                                         PCHTML_NS_HTML, true);
            break;
        }

        if (pchtml_html_tree_node_is(node, PCHTML_TAG_DT)) {
            pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG_DT,
                                                    PCHTML_NS_HTML);

            node = pchtml_html_tree_current_node(tree);

            if (pchtml_html_tree_node_is(node, PCHTML_TAG_DT) == false) {
                pchtml_html_tree_parse_error(tree, token,
                                          PCHTML_HTML_RULES_ERROR_UNELINOPELST);
            }

            pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_DT,
                                                         PCHTML_NS_HTML, true);
            break;
        }

        is_special = pchtml_html_tag_is_category(node->local_name, node->ns,
                                              PCHTML_HTML_TAG_CATEGORY_SPECIAL);
        if (is_special
            && pchtml_html_tree_node_is(node, PCHTML_TAG_ADDRESS) == false
            && pchtml_html_tree_node_is(node, PCHTML_TAG_DIV) == false
            && pchtml_html_tree_node_is(node, PCHTML_TAG_P) == false)
        {
            break;
        }
    }

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
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
pchtml_html_tree_insertion_mode_in_body_plaintext(pchtml_html_tree_t *tree,
                                               pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tokenizer_state_set(tree->tkz_ref,
                                 pchtml_html_tokenizer_state_plaintext_before);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_button(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_BUTTON, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (node != NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

        pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                                PCHTML_NS__UNDEF);

        pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_BUTTON,
                                                     PCHTML_NS_HTML, true);
    }

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->frameset_ok = false;

    return true;
}

/*
 * "address", "article", "aside", "blockquote", "button",  "center", "details",
 * "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer",
 * "header", "hgroup", "listing", "main", "menu", "nav", "ol", "pre", "section",
 * "summary", "ul"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_abcdfhlmnopsu_closed(pchtml_html_tree_t *tree,
                                                          pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, token->tag_id,
                                          PCHTML_NS_HTML, PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNCLTO);

        return true;
    }

    pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                            PCHTML_NS__UNDEF);

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, token->tag_id) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, token->tag_id,
                                                 PCHTML_NS_HTML, true);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_form_closed(pchtml_html_tree_t *tree,
                                                 pchtml_html_token_t *token)
{
    pcedom_node_t *node, *current;

    node = pchtml_html_tree_open_elements_find_reverse(tree, PCHTML_TAG_TEMPLATE,
                                                    PCHTML_NS_HTML, NULL);
    if (node == NULL) {
        node = pcedom_interface_node(tree->form);

        tree->form = NULL;

        if (node == NULL) {
            pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNCLTO);

            return true;
        }

        node = pchtml_html_tree_element_in_scope_by_node(tree, node,
                                                      PCHTML_HTML_TAG_CATEGORY_SCOPE);
        if (node == NULL) {
            pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNCLTO);

            return true;
        }

        pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                                PCHTML_NS__UNDEF);

        current = pchtml_html_tree_current_node(tree);

        if (current != node) {
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_UNELINOPELST);
        }

        pchtml_html_tree_open_elements_remove_by_node(tree, node);

        return true;
    }

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_FORM, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNCLTO);

        return true;
    }

    pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                            PCHTML_NS__UNDEF);

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_FORM) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_FORM,
                                                 PCHTML_NS_HTML, true);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_p_closed(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node == NULL) {
        pchtml_html_token_t fake_token = {0};
        pchtml_html_element_t *element;

        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNCLTO);

        fake_token.tag_id = PCHTML_TAG_P;

        element = pchtml_html_tree_insert_html_element(tree, &fake_token);
        if (element == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

            return pchtml_html_tree_process_abort(tree);
        }
    }

    pchtml_html_tree_close_p_element(tree, token);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_li_closed(pchtml_html_tree_t *tree,
                                               pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_LI, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_LIST_ITEM);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNCLTO);
        return true;
    }

    pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG_LI, PCHTML_NS_HTML);

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_LI) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, PCHTML_TAG_LI, PCHTML_NS_HTML,
                                                 true);

    return true;
}

/*
 * "dd", "dt"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_dd_dt_closed(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, token->tag_id, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNCLTO);
        return true;
    }

    pchtml_html_tree_generate_implied_end_tags(tree, token->tag_id, PCHTML_NS_HTML);

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, token->tag_id) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, token->tag_id,
                                                 PCHTML_NS_HTML, true);

    return true;
}

/*
 * "h1", "h2", "h3", "h4", "h5", "h6"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_h123456_closed(pchtml_html_tree_t *tree,
                                                    pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope_h123456(tree);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNCLTO);
        return true;
    }

    pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                            PCHTML_NS__UNDEF);

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, token->tag_id) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_h123456(tree);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_a(pchtml_html_tree_t *tree,
                                       pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_active_formatting_between_last_marker(tree,
                                                               token->tag_id,
                                                               NULL);
    if (node != NULL) {
        /* bool is; */

        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINACFOST);

        pchtml_html_tree_adoption_agency_algorithm(tree, token,
                                                &tree->status);
        if (tree->status != PCHTML_STATUS_OK) {
            return pchtml_html_tree_process_abort(tree);
        }

/*
        if (is) {
            return pchtml_html_tree_insertion_mode_in_body_anything_else_closed(tree,
                                                                             token);
        }
*/

        pchtml_html_tree_active_formatting_remove_by_node(tree, node);
        pchtml_html_tree_open_elements_remove_by_node(tree, node);
    }

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    node = pcedom_interface_node(element);

    pchtml_html_tree_active_formatting_push_with_check_dupl(tree, node);

    return true;
}

/*
 * "b", "big", "code", "em", "font", "i", "s", "small", "strike", "strong",
 * "tt", "u"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_bcefistu(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    node = pcedom_interface_node(element);

    pchtml_html_tree_active_formatting_push_with_check_dupl(tree, node);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_nobr(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_NOBR, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (node != NULL) {
        /* bool is; */

        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINSC);

        pchtml_html_tree_adoption_agency_algorithm(tree, token, &tree->status);
        if (tree->status != PCHTML_STATUS_OK) {
            return pchtml_html_tree_process_abort(tree);
        }
/*
        if (is) {
            return pchtml_html_tree_insertion_mode_in_body_anything_else_closed(tree,
                                                                             token);
        }
*/
        tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
        if (tree->status != PCHTML_STATUS_OK) {
            return pchtml_html_tree_process_abort(tree);
        }
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    node = pcedom_interface_node(element);

    pchtml_html_tree_active_formatting_push_with_check_dupl(tree, node);

    return true;
}

/*
 * "a", "b", "big", "code", "em", "font", "i", "nobr", "s", "small", "strike",
 * "strong", "tt", "u"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_abcefinstu_closed(pchtml_html_tree_t *tree,
                                                       pchtml_html_token_t *token)
{
    /* bool is; */

    pchtml_html_tree_adoption_agency_algorithm(tree, token, &tree->status);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

/*
    if (is) {
        return pchtml_html_tree_insertion_mode_in_body_anything_else_closed(tree,
                                                                         token);
    }
*/

    return true;
}

/*
 * "applet", "marquee", "object"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_amo(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->status = pchtml_html_tree_active_formatting_push_marker(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->frameset_ok = false;

    return true;
}

/*
 * "applet", "marquee", "object"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_amo_closed(pchtml_html_tree_t *tree,
                                                pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    node = pchtml_html_tree_element_in_scope(tree, token->tag_id, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (node == NULL) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNCLTO);
        return true;
    }

    pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                            PCHTML_NS__UNDEF);

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, token->tag_id) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
    }

    pchtml_html_tree_open_elements_pop_until_tag_id(tree, token->tag_id,
                                                 PCHTML_NS_HTML, true);

    pchtml_html_tree_active_formatting_up_to_last_marker(tree);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_table(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    if (pcedom_interface_document(tree->document)->compat_mode
        != PCEDOM_DOCUMENT_CMODE_QUIRKS)
    {
        node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                              PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
        if (node != NULL) {
            pchtml_html_tree_close_p_element(tree, token);
        }
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->frameset_ok = false;
    tree->mode = pchtml_html_tree_insertion_mode_in_table;

    return true;
}

/*
 * "area", "br", "embed", "img", "keygen", "wbr"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_abeikw(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tree_open_elements_pop(tree);
    pchtml_html_tree_acknowledge_token_self_closing(tree, token);

    tree->frameset_ok = false;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_br_closed(pchtml_html_tree_t *tree,
                                               pchtml_html_token_t *token)
{
    token->type ^= (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE);
    token->attr_first = NULL;
    token->attr_last = NULL;

    return pchtml_html_tree_insertion_mode_in_body_abeikw(tree, token);
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_input(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    pcedom_attr_t *attr;
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tree_open_elements_pop(tree);
    pchtml_html_tree_acknowledge_token_self_closing(tree, token);

    attr = pcedom_element_attr_is_exist(pcedom_interface_element(element),
                                         (unsigned char *) "type", 4);
    if (attr != NULL) {
        if (attr->value == NULL || attr->value->length != 6
            || pchtml_str_data_cmp(attr->value->data, (unsigned char *) "hidden") == false)
        {
            tree->frameset_ok = false;
        }
    }
    else {
        tree->frameset_ok = false;
    }

    return true;
}

/*
 * "param", "source", "track"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_pst(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tree_open_elements_pop(tree);
    pchtml_html_tree_acknowledge_token_self_closing(tree, token);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_hr(pchtml_html_tree_t *tree,
                                        pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tree_open_elements_pop(tree);
    pchtml_html_tree_acknowledge_token_self_closing(tree, token);

    tree->frameset_ok = false;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_image(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

    token->tag_id = PCHTML_TAG_IMG;

    return false;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_textarea(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    pchtml_html_tokenizer_tmp_tag_id_set(tree->tkz_ref, PCHTML_TAG_TEXTAREA);
    pchtml_html_tokenizer_state_set(tree->tkz_ref,
                                 pchtml_html_tokenizer_state_rcdata_before);

    tree->original_mode = tree->mode;

    tree->frameset_ok = false;

    tree->original_mode = tree->mode;
    tree->mode = pchtml_html_tree_insertion_mode_in_body_skip_new_line_textarea;

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_xmp(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_P, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE_BUTTON);
    if (node != NULL) {
        pchtml_html_tree_close_p_element(tree, token);
    }

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->frameset_ok = false;

    element = pchtml_html_tree_generic_rawtext_parsing(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_iframe(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    tree->frameset_ok = false;

    element = pchtml_html_tree_generic_rawtext_parsing(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_noembed(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    element = pchtml_html_tree_generic_rawtext_parsing(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_select(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->frameset_ok = false;

    if (tree->mode == pchtml_html_tree_insertion_mode_in_table
        || tree->mode == pchtml_html_tree_insertion_mode_in_caption
        || tree->mode == pchtml_html_tree_insertion_mode_in_table_body
        || tree->mode == pchtml_html_tree_insertion_mode_in_row
        || tree->mode == pchtml_html_tree_insertion_mode_in_cell)
    {
        tree->mode = pchtml_html_tree_insertion_mode_in_select_in_table;
    }
    else {
        tree->mode = pchtml_html_tree_insertion_mode_in_select;
    }

    return true;
}

/*
 * "optgroup", "option"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_optopt(pchtml_html_tree_t *tree,
                                            pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_current_node(tree);
    if (pchtml_html_tree_node_is(node, PCHTML_TAG_OPTION)) {
        pchtml_html_tree_open_elements_pop(tree);
    }

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

/*
 * "rb", "rtc"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_rbrtc(pchtml_html_tree_t *tree,
                                           pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_RUBY, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (node != NULL) {
        pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG__UNDEF,
                                                PCHTML_NS__UNDEF);
    }

    node = pchtml_html_tree_current_node(tree);
    if (pchtml_html_tree_node_is(node, PCHTML_TAG_RUBY) == false) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_MIELINOPELST);
    }

    element = pchtml_html_tree_insert_html_element(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

/*
 * "rp", "rt"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_rprt(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    pcedom_node_t *node;
    pchtml_html_element_t *element;

    node = pchtml_html_tree_element_in_scope(tree, PCHTML_TAG_RUBY, PCHTML_NS_HTML,
                                          PCHTML_HTML_TAG_CATEGORY_SCOPE);
    if (node != NULL) {
        pchtml_html_tree_generate_implied_end_tags(tree, PCHTML_TAG_RTC,
                                                PCHTML_NS_HTML);
    }

    node = pchtml_html_tree_current_node(tree);

    if (pchtml_html_tree_node_is(node, PCHTML_TAG_RTC) == false
        || pchtml_html_tree_node_is(node, PCHTML_TAG_RUBY) == false)
    {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_MIELINOPELST);
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
pchtml_html_tree_insertion_mode_in_body_math(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->before_append_attr = pchtml_html_tree_adjust_attributes_mathml;

    element = pchtml_html_tree_insert_foreign_element(tree, token, PCHTML_NS_MATH);
    if (element == NULL) {
        tree->before_append_attr = NULL;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->before_append_attr = NULL;

    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE_SELF) {
        pchtml_html_tree_open_elements_pop(tree);
        pchtml_html_tree_acknowledge_token_self_closing(tree, token);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_svg(pchtml_html_tree_t *tree,
                                         pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    tree->before_append_attr = pchtml_html_tree_adjust_attributes_svg;

    element = pchtml_html_tree_insert_foreign_element(tree, token, PCHTML_NS_SVG);
    if (element == NULL) {
        tree->before_append_attr = NULL;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    tree->before_append_attr = NULL;

    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE_SELF) {
        pchtml_html_tree_open_elements_pop(tree);
        pchtml_html_tree_acknowledge_token_self_closing(tree, token);
    }

    return true;
}

/*
 * "caption", "col", "colgroup", "frame", "head", "tbody", "td", "tfoot", "th",
 * "thead", "tr"
 */
static inline bool
pchtml_html_tree_insertion_mode_in_body_cfht(pchtml_html_tree_t *tree,
                                          pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token,
                              PCHTML_HTML_RULES_ERROR_UNTO);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_anything_else(pchtml_html_tree_t *tree,
                                                   pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;

    tree->status = pchtml_html_tree_active_formatting_reconstruct_elements(tree);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
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
pchtml_html_tree_insertion_mode_in_body_noscript(pchtml_html_tree_t *tree,
                                              pchtml_html_token_t *token)
{
    if (tree->document->dom_document.scripting == false) {
        return pchtml_html_tree_insertion_mode_in_body_anything_else(tree, token);
    }

    pchtml_html_element_t *element;

    element = pchtml_html_tree_generic_rawtext_parsing(tree, token);
    if (element == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_in_body_anything_else_closed(pchtml_html_tree_t *tree,
                                                          pchtml_html_token_t *token)
{
    bool is;
    pcedom_node_t **list = (pcedom_node_t **) tree->open_elements->list;
    size_t len = tree->open_elements->length;

    while (len != 0) {
        len--;

        if (pchtml_html_tree_node_is(list[len], token->tag_id)) {
            pchtml_html_tree_generate_implied_end_tags(tree, token->tag_id,
                                                    PCHTML_NS_HTML);

            if (list[len] != pchtml_html_tree_current_node(tree)) {
                pchtml_html_tree_parse_error(tree, token,
                                          PCHTML_HTML_RULES_ERROR_UNELINOPELST);
            }

            pchtml_html_tree_open_elements_pop_until_node(tree, list[len], true);

            return true;
        }

        is = pchtml_html_tag_is_category(list[len]->local_name, list[len]->ns,
                                      PCHTML_HTML_TAG_CATEGORY_SPECIAL);
        if (is) {
            pchtml_html_tree_parse_error(tree, token,
                                      PCHTML_HTML_RULES_ERROR_UNCLTO);
            return true;
        }
    }

    return true;
}


bool
pchtml_html_tree_insertion_mode_in_body(pchtml_html_tree_t *tree,
                                     pchtml_html_token_t *token)
{
    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_TEMPLATE:
                return pchtml_html_tree_insertion_mode_in_body_blmnst(tree, token);

            case PCHTML_TAG_BODY:
                return pchtml_html_tree_insertion_mode_in_body_body_closed(tree,
                                                                        token);
            case PCHTML_TAG_HTML:
                return pchtml_html_tree_insertion_mode_in_body_html_closed(tree,
                                                                        token);
            case PCHTML_TAG_ADDRESS:
            case PCHTML_TAG_ARTICLE:
            case PCHTML_TAG_ASIDE:
            case PCHTML_TAG_BLOCKQUOTE:
            case PCHTML_TAG_BUTTON:
            case PCHTML_TAG_CENTER:
            case PCHTML_TAG_DETAILS:
            case PCHTML_TAG_DIALOG:
            case PCHTML_TAG_DIR:
            case PCHTML_TAG_DIV:
            case PCHTML_TAG_DL:
            case PCHTML_TAG_FIELDSET:
            case PCHTML_TAG_FIGCAPTION:
            case PCHTML_TAG_FIGURE:
            case PCHTML_TAG_FOOTER:
            case PCHTML_TAG_HEADER:
            case PCHTML_TAG_HGROUP:
            case PCHTML_TAG_LISTING:
            case PCHTML_TAG_MAIN:
            case PCHTML_TAG_MENU:
            case PCHTML_TAG_NAV:
            case PCHTML_TAG_OL:
            case PCHTML_TAG_PRE:
            case PCHTML_TAG_SECTION:
            case PCHTML_TAG_SUMMARY:
            case PCHTML_TAG_UL:
                return pchtml_html_tree_insertion_mode_in_body_abcdfhlmnopsu_closed(tree,
                                                                                 token);
            case PCHTML_TAG_FORM:
                return pchtml_html_tree_insertion_mode_in_body_form_closed(tree,
                                                                        token);
            case PCHTML_TAG_P:
                return pchtml_html_tree_insertion_mode_in_body_p_closed(tree,
                                                                     token);
            case PCHTML_TAG_LI:
                return pchtml_html_tree_insertion_mode_in_body_li_closed(tree,
                                                                      token);
            case PCHTML_TAG_DD:
            case PCHTML_TAG_DT:
                return pchtml_html_tree_insertion_mode_in_body_dd_dt_closed(tree,
                                                                         token);
            case PCHTML_TAG_H1:
            case PCHTML_TAG_H2:
            case PCHTML_TAG_H3:
            case PCHTML_TAG_H4:
            case PCHTML_TAG_H5:
            case PCHTML_TAG_H6:
                return pchtml_html_tree_insertion_mode_in_body_h123456_closed(tree,
                                                                           token);
            case PCHTML_TAG_A:
            case PCHTML_TAG_B:
            case PCHTML_TAG_BIG:
            case PCHTML_TAG_CODE:
            case PCHTML_TAG_EM:
            case PCHTML_TAG_FONT:
            case PCHTML_TAG_I:
            case PCHTML_TAG_NOBR:
            case PCHTML_TAG_S:
            case PCHTML_TAG_SMALL:
            case PCHTML_TAG_STRIKE:
            case PCHTML_TAG_STRONG:
            case PCHTML_TAG_TT:
            case PCHTML_TAG_U:
                return pchtml_html_tree_insertion_mode_in_body_abcefinstu_closed(tree,
                                                                              token);
            case PCHTML_TAG_APPLET:
            case PCHTML_TAG_MARQUEE:
            case PCHTML_TAG_OBJECT:
                return pchtml_html_tree_insertion_mode_in_body_amo_closed(tree,
                                                                       token);
            case PCHTML_TAG_BR:
                return pchtml_html_tree_insertion_mode_in_body_br_closed(tree,
                                                                      token);

            default:
                return pchtml_html_tree_insertion_mode_in_body_anything_else_closed(tree,
                                                                                 token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG__TEXT:
            return pchtml_html_tree_insertion_mode_in_body_text(tree, token);

        case PCHTML_TAG__EM_COMMENT:
            return pchtml_html_tree_insertion_mode_in_body_comment(tree, token);

        case PCHTML_TAG__EM_DOCTYPE:
            return pchtml_html_tree_insertion_mode_in_body_doctype(tree, token);

        case PCHTML_TAG_HTML:
            return pchtml_html_tree_insertion_mode_in_body_html(tree, token);

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
            return pchtml_html_tree_insertion_mode_in_body_blmnst(tree, token);

        case PCHTML_TAG_BODY:
            return pchtml_html_tree_insertion_mode_in_body_body(tree, token);

        case PCHTML_TAG_FRAMESET:
            return pchtml_html_tree_insertion_mode_in_body_frameset(tree, token);

        case PCHTML_TAG__END_OF_FILE:
            return pchtml_html_tree_insertion_mode_in_body_eof(tree, token);

        case PCHTML_TAG_ADDRESS:
        case PCHTML_TAG_ARTICLE:
        case PCHTML_TAG_ASIDE:
        case PCHTML_TAG_BLOCKQUOTE:
        case PCHTML_TAG_CENTER:
        case PCHTML_TAG_DETAILS:
        case PCHTML_TAG_DIALOG:
        case PCHTML_TAG_DIR:
        case PCHTML_TAG_DIV:
        case PCHTML_TAG_DL:
        case PCHTML_TAG_FIELDSET:
        case PCHTML_TAG_FIGCAPTION:
        case PCHTML_TAG_FIGURE:
        case PCHTML_TAG_FOOTER:
        case PCHTML_TAG_HEADER:
        case PCHTML_TAG_HGROUP:
        case PCHTML_TAG_MAIN:
        case PCHTML_TAG_MENU:
        case PCHTML_TAG_NAV:
        case PCHTML_TAG_OL:
        case PCHTML_TAG_P:
        case PCHTML_TAG_SECTION:
        case PCHTML_TAG_SUMMARY:
        case PCHTML_TAG_UL:
            return pchtml_html_tree_insertion_mode_in_body_abcdfhmnopsu(tree,
                                                                     token);

        case PCHTML_TAG_H1:
        case PCHTML_TAG_H2:
        case PCHTML_TAG_H3:
        case PCHTML_TAG_H4:
        case PCHTML_TAG_H5:
        case PCHTML_TAG_H6:
            return pchtml_html_tree_insertion_mode_in_body_h123456(tree, token);

        case PCHTML_TAG_PRE:
        case PCHTML_TAG_LISTING:
            return pchtml_html_tree_insertion_mode_in_body_pre_listing(tree,
                                                                    token);

        case PCHTML_TAG_FORM:
            return pchtml_html_tree_insertion_mode_in_body_form(tree, token);

        case PCHTML_TAG_LI:
            return pchtml_html_tree_insertion_mode_in_body_li(tree, token);

        case PCHTML_TAG_DD:
        case PCHTML_TAG_DT:
            return pchtml_html_tree_insertion_mode_in_body_dd_dt(tree, token);

        case PCHTML_TAG_PLAINTEXT:
            return pchtml_html_tree_insertion_mode_in_body_plaintext(tree, token);

        case PCHTML_TAG_BUTTON:
            return pchtml_html_tree_insertion_mode_in_body_button(tree, token);

        case PCHTML_TAG_A:
            return pchtml_html_tree_insertion_mode_in_body_a(tree, token);

        case PCHTML_TAG_B:
        case PCHTML_TAG_BIG:
        case PCHTML_TAG_CODE:
        case PCHTML_TAG_EM:
        case PCHTML_TAG_FONT:
        case PCHTML_TAG_I:
        case PCHTML_TAG_S:
        case PCHTML_TAG_SMALL:
        case PCHTML_TAG_STRIKE:
        case PCHTML_TAG_STRONG:
        case PCHTML_TAG_TT:
        case PCHTML_TAG_U:
            return pchtml_html_tree_insertion_mode_in_body_bcefistu(tree, token);

        case PCHTML_TAG_NOBR:
            return pchtml_html_tree_insertion_mode_in_body_nobr(tree, token);

        case PCHTML_TAG_APPLET:
        case PCHTML_TAG_MARQUEE:
        case PCHTML_TAG_OBJECT:
            return pchtml_html_tree_insertion_mode_in_body_amo(tree, token);

        case PCHTML_TAG_TABLE:
            return pchtml_html_tree_insertion_mode_in_body_table(tree, token);

        case PCHTML_TAG_AREA:
        case PCHTML_TAG_BR:
        case PCHTML_TAG_EMBED:
        case PCHTML_TAG_IMG:
        case PCHTML_TAG_KEYGEN:
        case PCHTML_TAG_WBR:
            return pchtml_html_tree_insertion_mode_in_body_abeikw(tree, token);

        case PCHTML_TAG_INPUT:
            return pchtml_html_tree_insertion_mode_in_body_input(tree, token);

        case PCHTML_TAG_PARAM:
        case PCHTML_TAG_SOURCE:
        case PCHTML_TAG_TRACK:
            return pchtml_html_tree_insertion_mode_in_body_pst(tree, token);

        case PCHTML_TAG_HR:
            return pchtml_html_tree_insertion_mode_in_body_hr(tree, token);

        case PCHTML_TAG_IMAGE:
            return pchtml_html_tree_insertion_mode_in_body_image(tree, token);

        case PCHTML_TAG_TEXTAREA:
            return pchtml_html_tree_insertion_mode_in_body_textarea(tree, token);

        case PCHTML_TAG_XMP:
            return pchtml_html_tree_insertion_mode_in_body_xmp(tree, token);

        case PCHTML_TAG_IFRAME:
            return pchtml_html_tree_insertion_mode_in_body_iframe(tree, token);

        case PCHTML_TAG_NOEMBED:
            return pchtml_html_tree_insertion_mode_in_body_noembed(tree, token);

        case PCHTML_TAG_NOSCRIPT:
            return pchtml_html_tree_insertion_mode_in_body_noscript(tree, token);

        case PCHTML_TAG_SELECT:
            return pchtml_html_tree_insertion_mode_in_body_select(tree, token);

        case PCHTML_TAG_OPTGROUP:
        case PCHTML_TAG_OPTION:
            return pchtml_html_tree_insertion_mode_in_body_optopt(tree, token);

        case PCHTML_TAG_RB:
        case PCHTML_TAG_RTC:
            return pchtml_html_tree_insertion_mode_in_body_rbrtc(tree, token);

        case PCHTML_TAG_RP:
        case PCHTML_TAG_RT:
            return pchtml_html_tree_insertion_mode_in_body_rprt(tree, token);

        case PCHTML_TAG_MATH:
            return pchtml_html_tree_insertion_mode_in_body_math(tree, token);

        case PCHTML_TAG_SVG:
            return pchtml_html_tree_insertion_mode_in_body_svg(tree, token);

        case PCHTML_TAG_CAPTION:
        case PCHTML_TAG_COL:
        case PCHTML_TAG_COLGROUP:
        case PCHTML_TAG_FRAME:
        case PCHTML_TAG_HEAD:
        case PCHTML_TAG_TBODY:
        case PCHTML_TAG_TD:
        case PCHTML_TAG_TFOOT:
        case PCHTML_TAG_TH:
        case PCHTML_TAG_THEAD:
        case PCHTML_TAG_TR:
            return pchtml_html_tree_insertion_mode_in_body_cfht(tree, token);

        default:
            return pchtml_html_tree_insertion_mode_in_body_anything_else(tree,
                                                                      token);
    }
}
