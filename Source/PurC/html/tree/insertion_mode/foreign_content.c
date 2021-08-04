/**
 * @file foreign_content.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of foreign content.
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
#include "html/interfaces/element.h"

#define PCHTML_TOKENIZER_CHARS_MAP
#define PCHTML_STR_RES_ANSI_REPLACEMENT_CHARACTER
#include "html/str_res.h"


unsigned int
pcedom_element_qualified_name_set(pcedom_element_t *element,
                                   const unsigned char *prefix, size_t prefix_len,
                                   const unsigned char *lname, size_t lname_len);


static inline bool
pchtml_html_tree_insertion_mode_foreign_content_anything_else_closed(pchtml_html_tree_t *tree,
                                                                  pchtml_html_token_t *token)
{
    if (tree->open_elements->length == 0) {
        return tree->mode(tree, token);
    }

    pcedom_node_t **list = (pcedom_node_t **) tree->open_elements->list;

    size_t idx = tree->open_elements->length - 1;

    if (idx > 0 && list[idx]->local_name != token->tag_id) {
        pchtml_html_tree_parse_error(tree, token,
                                  PCHTML_HTML_RULES_ERROR_UNELINOPELST);
    }

    while (idx != 0) {
        if (list[idx]->local_name == token->tag_id) {
            pchtml_html_tree_open_elements_pop_until_node(tree, list[idx], true);

            return true;
        }

        idx--;

        if (list[idx]->ns == PCHTML_NS_HTML) {
            break;
        }
    }

    return tree->mode(tree, token);
}

/*
 * TODO: Need to process script
 */
static inline bool
pchtml_html_tree_insertion_mode_foreign_content_script_closed(pchtml_html_tree_t *tree,
                                                           pchtml_html_token_t *token)
{
    pcedom_node_t *node = pchtml_html_tree_current_node(tree);

    if (node->local_name != PCHTML_TAG_SCRIPT || node->ns != PCHTML_NS_SVG) {
        return pchtml_html_tree_insertion_mode_foreign_content_anything_else_closed(tree,
                                                                                 token);
    }

    pchtml_html_tree_open_elements_pop(tree);

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_foreign_content_anything_else(pchtml_html_tree_t *tree,
                                                           pchtml_html_token_t *token)
{
    pchtml_html_element_t *element;
    const pchtml_html_tag_fixname_t *fixname_svg;
    pcedom_node_t *node = pchtml_html_tree_adjusted_current_node(tree);

    if (node->ns == PCHTML_NS_MATH) {
        tree->before_append_attr = pchtml_html_tree_adjust_attributes_mathml;
    }
    else if (node->ns == PCHTML_NS_SVG) {
        tree->before_append_attr = pchtml_html_tree_adjust_attributes_svg;
    }

    element = pchtml_html_tree_insert_foreign_element(tree, token, node->ns);
    if (element == NULL) {
        tree->before_append_attr = NULL;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        tree->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        return pchtml_html_tree_process_abort(tree);
    }

    if (node->ns == PCHTML_NS_SVG) {
        fixname_svg = pchtml_html_tag_fixname_svg(element->element.node.local_name);
        if (fixname_svg != NULL && fixname_svg->name != NULL) {
            pcedom_element_qualified_name_set(&element->element, NULL, 0,
                                               fixname_svg->name,
                                               (size_t) fixname_svg->len);
        }
    }

    tree->before_append_attr = NULL;

    if ((token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE_SELF) == 0) {
        return true;
    }

    node = pchtml_html_tree_current_node(tree);

    if (token->tag_id == PCHTML_TAG_SCRIPT && node->ns == PCHTML_NS_SVG) {
        pchtml_html_tree_acknowledge_token_self_closing(tree, token);
        return pchtml_html_tree_insertion_mode_foreign_content_script_closed(tree, token);
    }
    else {
        pchtml_html_tree_open_elements_pop(tree);
        pchtml_html_tree_acknowledge_token_self_closing(tree, token);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_foreign_content_text(pchtml_html_tree_t *tree,
                                                  pchtml_html_token_t *token)
{
    pchtml_str_t str;

    if (token->null_count != 0) {
        pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_NUCH);

        tree->status = pchtml_html_token_make_text_replace_null(token, &str,
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

    if (tree->frameset_ok) {
        const unsigned char *pos = str.data;
        const unsigned char *end = str.data + str.length;

        static const unsigned char *rep = pchtml_str_res_ansi_replacement_character;
        static const unsigned rep_len = sizeof(pchtml_str_res_ansi_replacement_character) - 1;

        while (pos != end) {
            /* Need skip U+FFFD REPLACEMENT CHARACTER */
            if (*pos == *rep) {
                if ((end - pos) < rep_len) {
                    tree->frameset_ok = false;

                    break;
                }

                if (memcmp(pos, rep, sizeof(unsigned char) * rep_len) != 0) {
                    tree->frameset_ok = false;

                    break;
                }

                pos = pos + rep_len;

                continue;
            }

            if (pchtml_tokenizer_chars_map[*pos]
                != PCHTML_STR_RES_MAP_CHAR_WHITESPACE)
            {
                tree->frameset_ok = false;

                break;
            }

            pos++;
        }
    }

    tree->status = pchtml_html_tree_insert_character_for_data(tree, &str, NULL);
    if (tree->status != PCHTML_STATUS_OK) {
        return pchtml_html_tree_process_abort(tree);
    }

    return true;
}

static inline bool
pchtml_html_tree_insertion_mode_foreign_content_comment(pchtml_html_tree_t *tree,
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
pchtml_html_tree_insertion_mode_foreign_content_doctype(pchtml_html_tree_t *tree,
                                                     pchtml_html_token_t *token)
{
    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_DOTOFOCOMO);

    return true;
}

/*
 * "b", "big", "blockquote", "body", "br", "center", "code", "dd", "div", "dl",
 * "dt", "em", "embed", "h1", "h2", "h3", "h4", "h5", "h6", "head", "hr", "i",
 * "img", "li", "listing", "menu", "meta", "nobr", "ol", "p", "pre", "ruby",
 * "s", "small", "span", "strong", "strike", "sub", "sup", "table", "tt", "u",
 * "ul", "var"
 * "font", if the token has any attributes named "color", "face", or "size"
 */
static inline bool
pchtml_html_tree_insertion_mode_foreign_content_all(pchtml_html_tree_t *tree,
                                                 pchtml_html_token_t *token)
{
    pcedom_node_t *node;

    if (token->tag_id == PCHTML_TAG_FONT) {
        pchtml_html_token_attr_t *attr = token->attr_first;

        while (attr != NULL) {
            if (attr->name != NULL
                && (attr->name->attr_id == PCEDOM_ATTR_COLOR
                || attr->name->attr_id == PCEDOM_ATTR_FACE
                || attr->name->attr_id == PCEDOM_ATTR_SIZE))
            {
                goto go_next;
            }

            attr = attr->next;
        }

        return pchtml_html_tree_insertion_mode_foreign_content_anything_else(tree,
                                                                          token);
    }

go_next:

    pchtml_html_tree_parse_error(tree, token, PCHTML_HTML_RULES_ERROR_UNTO);

    if (tree->fragment != NULL) {
        return pchtml_html_tree_insertion_mode_foreign_content_anything_else(tree,
                                                                          token);
    }

    do {
        pchtml_html_tree_open_elements_pop(tree);

        node = pchtml_html_tree_current_node(tree);
    }
    while (node &&
           !(pchtml_html_tree_mathml_text_integration_point(node)
            || pchtml_html_tree_html_integration_point(node)
            || node->ns == PCHTML_NS_HTML));

    return false;
}

bool
pchtml_html_tree_insertion_mode_foreign_content(pchtml_html_tree_t *tree,
                                             pchtml_html_token_t *token)
{
    if (token->type & PCHTML_HTML_TOKEN_TYPE_CLOSE) {
        switch (token->tag_id) {
            case PCHTML_TAG_SCRIPT:
                return pchtml_html_tree_insertion_mode_foreign_content_script_closed(tree,
                                                                                  token);
            default:
                return pchtml_html_tree_insertion_mode_foreign_content_anything_else_closed(tree,
                                                                                         token);
        }
    }

    switch (token->tag_id) {
        case PCHTML_TAG__TEXT:
            return pchtml_html_tree_insertion_mode_foreign_content_text(tree,
                                                                     token);
        case PCHTML_TAG__EM_COMMENT:
            return pchtml_html_tree_insertion_mode_foreign_content_comment(tree,
                                                                        token);
        case PCHTML_TAG__EM_DOCTYPE:
            return pchtml_html_tree_insertion_mode_foreign_content_doctype(tree,
                                                                        token);

        case PCHTML_TAG_B:
        case PCHTML_TAG_BIG:
        case PCHTML_TAG_BLOCKQUOTE:
        case PCHTML_TAG_BODY:
        case PCHTML_TAG_BR:
        case PCHTML_TAG_CENTER:
        case PCHTML_TAG_CODE:
        case PCHTML_TAG_DD:
        case PCHTML_TAG_DIV:
        case PCHTML_TAG_DL:
        case PCHTML_TAG_DT:
        case PCHTML_TAG_EM:
        case PCHTML_TAG_EMBED:
        case PCHTML_TAG_H1:
        case PCHTML_TAG_H2:
        case PCHTML_TAG_H3:
        case PCHTML_TAG_H4:
        case PCHTML_TAG_H5:
        case PCHTML_TAG_H6:
        case PCHTML_TAG_HEAD:
        case PCHTML_TAG_HR:
        case PCHTML_TAG_I:
        case PCHTML_TAG_IMG:
        case PCHTML_TAG_LI:
        case PCHTML_TAG_LISTING:
        case PCHTML_TAG_MENU:
        case PCHTML_TAG_META:
        case PCHTML_TAG_NOBR:
        case PCHTML_TAG_OL:
        case PCHTML_TAG_P:
        case PCHTML_TAG_PRE:
        case PCHTML_TAG_RUBY:
        case PCHTML_TAG_S:
        case PCHTML_TAG_SMALL:
        case PCHTML_TAG_SPAN:
        case PCHTML_TAG_STRONG:
        case PCHTML_TAG_STRIKE:
        case PCHTML_TAG_SUB:
        case PCHTML_TAG_TABLE:
        case PCHTML_TAG_TT:
        case PCHTML_TAG_U:
        case PCHTML_TAG_UL:
        case PCHTML_TAG_VAR:
        case PCHTML_TAG_FONT:
            return pchtml_html_tree_insertion_mode_foreign_content_all(tree,
                                                                    token);
        default:
            return pchtml_html_tree_insertion_mode_foreign_content_anything_else(tree,
                                                                              token);
    }
}
