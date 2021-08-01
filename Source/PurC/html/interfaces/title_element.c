/**
 * @file title_element.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html title element.
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
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#include "html/interfaces/title_element.h"
#include "html/interfaces/document.h"
#include "private/edom/text.h"


pchtml_html_title_element_t *
pchtml_html_title_element_interface_create(pchtml_html_document_t *document)
{
    pchtml_html_title_element_t *element;

    element = pchtml_mraw_calloc(document->dom_document.mraw,
                                 sizeof(pchtml_html_title_element_t));
    if (element == NULL) {
        return NULL;
    }

    pcedom_node_t *node = pcedom_interface_node(element);

    node->owner_document = pchtml_html_document_original_ref(document);
    node->type = PCEDOM_NODE_TYPE_ELEMENT;

    return element;
}

pchtml_html_title_element_t *
pchtml_html_title_element_interface_destroy(pchtml_html_title_element_t *title)
{
    pcedom_document_t *doc = pcedom_interface_node(title)->owner_document;

    if (title->strict_text != NULL) {
        pchtml_str_destroy(title->strict_text, doc->text, false);
        pcedom_document_destroy_struct(doc, title->strict_text);
    }

    return pchtml_mraw_free(doc->mraw, title);
}

const unsigned char *
pchtml_html_title_element_text(pchtml_html_title_element_t *title, size_t *len)
{
    if (pcedom_interface_node(title)->first_child == NULL) {
        goto failed;
    }

    if (pcedom_interface_node(title)->first_child->type != PCEDOM_NODE_TYPE_TEXT) {
        goto failed;
    }

    pcedom_text_t *text;

    text = pcedom_interface_text(pcedom_interface_node(title)->first_child);

    if (len != NULL) {
        *len = text->char_data.data.length;
    }

    return text->char_data.data.data;

failed:

    if (len != NULL) {
        *len = 0;
    }

    return NULL;
}

const unsigned char *
pchtml_html_title_element_strict_text(pchtml_html_title_element_t *title, size_t *len)
{
    const unsigned char *text;
    size_t text_len;

    pcedom_document_t *doc = pcedom_interface_node(title)->owner_document;

    text = pchtml_html_title_element_text(title, &text_len);
    if (text == NULL) {
        goto failed;
    }

    if (title->strict_text != NULL) {
        if (title->strict_text->length < text_len) {
            const unsigned char *data;

            data = pchtml_str_realloc(title->strict_text,
                                      doc->text, (text_len + 1));
            if (data == NULL) {
                goto failed;
            }
        }
    }
    else {
        title->strict_text = pcedom_document_create_struct(doc,
                                                            sizeof(pchtml_str_t));
        if (title->strict_text == NULL) {
            goto failed;
        }

        pchtml_str_init(title->strict_text, doc->text, text_len);
        if (title->strict_text->data == NULL) {
            title->strict_text = pcedom_document_destroy_struct(doc,
                                                                 title->strict_text);
            goto failed;
        }
    }

    memcpy(title->strict_text->data, text, sizeof(unsigned char) * text_len);

    title->strict_text->data[text_len] = 0x00;
    title->strict_text->length = text_len;

    pchtml_str_strip_collapse_whitespace(title->strict_text);

    if (len != NULL) {
        *len = title->strict_text->length;
    }

    return title->strict_text->data;

failed:

    if (len != NULL) {
        *len = 0;
    }

    return NULL;
}
