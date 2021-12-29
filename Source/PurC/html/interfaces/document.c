/**
 * @file document_element.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html document element.
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

#include "private/str.h"

#include "html/interfaces/document.h"
#include "html/interfaces/title_element.h"
#include "html/node.h"
#include "html/parser.h"

#include "html/tag.h"

#define PCHTML_HTML_TAG_RES_DATA
#define PCHTML_HTML_TAG_RES_SHS_DATA
#include "html_tag_res.h"

#include "private/edom.h"

unsigned int
pchtml_html_parse_chunk_prepare(pchtml_html_parser_t *parser,
                             pchtml_html_document_t *document);

static inline unsigned int
pchtml_html_document_parser_prepare(pchtml_html_document_t *document);

static pchtml_action_t
pchtml_html_document_title_walker(pcedom_node_t *node, void *ctx);


pchtml_html_document_t *
pchtml_html_document_interface_create(pchtml_html_document_t *document)
{
    unsigned int status;
    pcedom_document_t *doc;
    pcedom_interface_create_f icreator;

    if (document != NULL) {
        doc = pcutils_mraw_calloc(pchtml_html_document_mraw(document),
                                 sizeof(pchtml_html_document_t));
    }
    else {
        doc = pcutils_calloc(1, sizeof(pchtml_html_document_t));
    }

    if (doc == NULL) {
        return NULL;
    }

    icreator = (pcedom_interface_create_f) pchtml_html_interface_create;

    status = pcedom_document_init(doc, pcedom_interface_document(document),
                                   icreator, pchtml_html_interface_destroy,
                                   PCEDOM_DOCUMENT_DTYPE_HTML, PCHTML_NS_HTML);
    if (status != PCHTML_STATUS_OK) {
        (void) pcedom_document_destroy(doc);
        return NULL;
    }

    return pchtml_html_interface_document(doc);
}

pchtml_html_document_t *
pchtml_html_document_interface_destroy(pchtml_html_document_t *document)
{
    pcedom_document_t *doc;

    if (document == NULL) {
        return NULL;
    }

    doc = pcedom_interface_document(document);

    if (doc->node.owner_document == doc) {
        (void) pchtml_html_parser_unref(doc->parser);
    }

    (void) pcedom_document_destroy(doc);

    return NULL;
}

pchtml_html_document_t *
pchtml_html_document_create(void)
{
    return pchtml_html_document_interface_create(NULL);
}

void
pchtml_html_document_clean(pchtml_html_document_t *document)
{
    document->body = NULL;
    document->head = NULL;
    document->iframe_srcdoc = NULL;
    document->ready_state = PCHTML_HTML_DOCUMENT_READY_STATE_UNDEF;

    pcedom_document_clean(pcedom_interface_document(document));
}

pchtml_html_document_t *
pchtml_html_document_destroy(pchtml_html_document_t *document)
{
    return pchtml_html_document_interface_destroy(document);
}

unsigned int
pchtml_html_document_parse(pchtml_html_document_t *document,
                        const purc_rwstream_t html)
{
    unsigned int status;
    pcedom_document_t *doc;
    pchtml_html_document_opt_t opt;

    if (document->ready_state != PCHTML_HTML_DOCUMENT_READY_STATE_UNDEF
        && document->ready_state != PCHTML_HTML_DOCUMENT_READY_STATE_LOADING)
    {
        pchtml_html_document_clean(document);
    }

    opt = document->opt;
    doc = pcedom_interface_document(document);

    status = pchtml_html_document_parser_prepare(document);
    if (status != PCHTML_STATUS_OK) {
        goto failed;
    }

    status = pchtml_html_parse_chunk_prepare(doc->parser, document);
    if (status != PCHTML_STATUS_OK) {
        goto failed;
    }

    ssize_t sz;
    while (1) {
        char buf[1024];
        sz = purc_rwstream_read(html, buf, sizeof(buf));
        if (sz <= 0)
            break;

        status = pchtml_html_parse_chunk_process(doc->parser,
                (unsigned char*)buf, sz);
        if (status != PCHTML_STATUS_OK) {
            goto failed;
        }
    }

    document->opt = opt;

    return pchtml_html_parse_chunk_end(doc->parser);

failed:

    document->opt = opt;

    return status;
}

unsigned int
pchtml_html_document_parse_chunk_begin(pchtml_html_document_t *document)
{
    if (document->ready_state != PCHTML_HTML_DOCUMENT_READY_STATE_UNDEF
        && document->ready_state != PCHTML_HTML_DOCUMENT_READY_STATE_LOADING)
    {
        pchtml_html_document_clean(document);
    }

    unsigned int status = pchtml_html_document_parser_prepare(document);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    return pchtml_html_parse_chunk_prepare(document->dom_document.parser,
                                        document);
}

unsigned int
pchtml_html_document_parse_chunk(pchtml_html_document_t *document,
                const unsigned char *data, size_t sz)
{
    return pchtml_html_parse_chunk_process(document->dom_document.parser,
                                        data, sz);
}

unsigned int
pchtml_html_document_parse_chunk_end(pchtml_html_document_t *document)
{
    return pchtml_html_parse_chunk_end(document->dom_document.parser);
}

pcedom_node_t *
pchtml_html_document_parse_fragment(pchtml_html_document_t *document,
                                 pcedom_element_t *element,
                                 const purc_rwstream_t html)
{
    unsigned int status;
    pchtml_html_parser_t *parser;
    pchtml_html_document_opt_t opt = document->opt;

    status = pchtml_html_document_parser_prepare(document);
    if (status != PCHTML_STATUS_OK) {
        goto failed;
    }

    parser = document->dom_document.parser;

    status = pchtml_html_parse_fragment_chunk_begin(parser, document,
                                                 element->node.local_name,
                                                 element->node.ns);
    if (status != PCHTML_STATUS_OK) {
        goto failed;
    }

    ssize_t sz;
    while (1) {
        char buf[1024];
        sz = purc_rwstream_read(html, buf, sizeof(buf));
        if (sz <= 0)
            break;

        status = pchtml_html_parse_fragment_chunk_process(parser,
                (unsigned char*)buf, sz);
        if (status != PCHTML_STATUS_OK) {
            goto failed;
        }
    }

    document->opt = opt;

    return pchtml_html_parse_fragment_chunk_end(parser);

failed:

    document->opt = opt;

    return NULL;
}

unsigned int
pchtml_html_document_parse_fragment_chunk_begin(pchtml_html_document_t *document,
                                             pcedom_element_t *element)
{
    unsigned int status;
    pchtml_html_parser_t *parser = document->dom_document.parser;

    status = pchtml_html_document_parser_prepare(document);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    return pchtml_html_parse_fragment_chunk_begin(parser, document,
                                               element->node.local_name,
                                               element->node.ns);
}

unsigned int
pchtml_html_document_parse_fragment_chunk(pchtml_html_document_t *document,
                const unsigned char *data, size_t sz)
{
    return pchtml_html_parse_fragment_chunk_process(document->dom_document.parser,
                                                 data, sz);
}

pcedom_node_t *
pchtml_html_document_parse_fragment_chunk_end(pchtml_html_document_t *document)
{
    return pchtml_html_parse_fragment_chunk_end(document->dom_document.parser);
}

static inline unsigned int
pchtml_html_document_parser_prepare(pchtml_html_document_t *document)
{
    unsigned int status;
    pcedom_document_t *doc;

    doc = pcedom_interface_document(document);

    if (doc->parser == NULL) {
        doc->parser = pchtml_html_parser_create();
        status = pchtml_html_parser_init(doc->parser);

        if (status != PCHTML_STATUS_OK) {
            pchtml_html_parser_destroy(doc->parser);
            return status;
        }
    }
    else if (pchtml_html_parser_state(doc->parser) != PCHTML_HTML_PARSER_STATE_BEGIN) {
        pchtml_html_parser_clean(doc->parser);
    }

    return PCHTML_STATUS_OK;
}

const unsigned char *
pchtml_html_document_title(pchtml_html_document_t *document, size_t *len)
{
    pchtml_html_title_element_t *title = NULL;

    pcedom_node_simple_walk(pcedom_interface_node(document),
                             pchtml_html_document_title_walker, &title);
    if (title == NULL) {
        return NULL;
    }

    return pchtml_html_title_element_strict_text(title, len);
}

unsigned int
pchtml_html_document_title_set(pchtml_html_document_t *document,
                            const unsigned char *title, size_t len)
{
    unsigned int status;

    /* TODO: If the document element is an SVG svg element */

    /* If the document element is in the HTML namespace */
    if (document->head == NULL) {
        return PCHTML_STATUS_OK;
    }

    pchtml_html_title_element_t *el_title = NULL;

    pcedom_node_simple_walk(pcedom_interface_node(document),
                             pchtml_html_document_title_walker, &el_title);
    if (el_title == NULL) {
        el_title = (void *) pchtml_html_document_create_element(document,
                                         (const unsigned char *) "title", 5, NULL);
        if (el_title == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        pcedom_node_insert_child(pcedom_interface_node(document->head),
                                  pcedom_interface_node(el_title));
    }

    status = pcedom_node_text_content_set(pcedom_interface_node(el_title),
                                           title, len);
    if (status != PCHTML_STATUS_OK) {
        pchtml_html_document_destroy_element(&el_title->element.element);

        return status;
    }

    return PCHTML_STATUS_OK;
}

const unsigned char *
pchtml_html_document_title_raw(pchtml_html_document_t *document, size_t *len)
{
    pchtml_html_title_element_t *title = NULL;

    pcedom_node_simple_walk(pcedom_interface_node(document),
                             pchtml_html_document_title_walker, &title);
    if (title == NULL) {
        return NULL;
    }

    return pchtml_html_title_element_text(title, len);
}

static pchtml_action_t
pchtml_html_document_title_walker(pcedom_node_t *node, void *ctx)
{
    if (node->local_name == PCHTML_TAG_TITLE) {
        *((void **) ctx) = node;

        return PCHTML_ACTION_STOP;
    }

    return PCHTML_ACTION_OK;
}
