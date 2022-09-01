/**
 * @file document_element.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html document element.
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

#ifndef PCHTML_HTML_DOCUMENT_H
#define PCHTML_HTML_DOCUMENT_H

#include "config.h"
#include "private/mraw.h"

#include "html/tag.h"
#include "html/html_interface.h"
#include "private/html.h"
#include "private/dom.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int pchtml_html_document_opt_t;


typedef enum {
    PCHTML_HTML_DOCUMENT_READY_STATE_UNDEF       = 0x00,
    PCHTML_HTML_DOCUMENT_READY_STATE_LOADING     = 0x01,
    PCHTML_HTML_DOCUMENT_READY_STATE_INTERACTIVE = 0x02,
    PCHTML_HTML_DOCUMENT_READY_STATE_COMPLETE    = 0x03,
}
pchtml_html_document_ready_state_t;

enum pchtml_html_document_opt {
    PCHTML_HTML_DOCUMENT_OPT_UNDEF     = 0x00,
    PCHTML_HTML_DOCUMENT_PARSE_WO_COPY = 0x01
};

struct pchtml_html_document {
    pcdom_document_t              dom_document;

    void                            *iframe_srcdoc;

    pchtml_html_head_element_t         *head;
    pchtml_html_body_element_t         *body;

    pchtml_html_document_ready_state_t ready_state;

    pchtml_html_document_opt_t         opt;
};

pchtml_html_document_t *
pchtml_html_document_interface_create(
                pchtml_html_document_t *document) WTF_INTERNAL;

pchtml_html_document_t *
pchtml_html_document_interface_destroy(
                pchtml_html_document_t *document) WTF_INTERNAL;


// gengyue
#if 0
pchtml_html_document_t *
pchtml_html_document_create(void) WTF_INTERNAL;

void
pchtml_html_document_clean(pchtml_html_document_t *document) WTF_INTERNAL;

pchtml_html_document_t *
pchtml_html_document_destroy(pchtml_html_document_t *document) WTF_INTERNAL;


unsigned int
pchtml_html_document_parse(pchtml_html_document_t *document,
                const unsigned char *html, size_t size) WTF_INTERNAL;

unsigned int
pchtml_html_document_parse_chunk_begin(
                pchtml_html_document_t *document) WTF_INTERNAL;

unsigned int
pchtml_html_document_parse_chunk(pchtml_html_document_t *document,
                const unsigned char *html, size_t size) WTF_INTERNAL;

unsigned int
pchtml_html_document_parse_chunk_end(
                pchtml_html_document_t *document) WTF_INTERNAL;

pcdom_node_t *
pchtml_html_document_parse_fragment(pchtml_html_document_t *document,
                pcdom_element_t *element,
                const unsigned char *html, size_t size) WTF_INTERNAL;

unsigned int
pchtml_html_document_parse_fragment_chunk_begin(
                pchtml_html_document_t *document,
                pcdom_element_t *element) WTF_INTERNAL;

unsigned int
pchtml_html_document_parse_fragment_chunk(pchtml_html_document_t *document,
                const unsigned char *html, size_t size) WTF_INTERNAL;

pcdom_node_t *
pchtml_html_document_parse_fragment_chunk_end(
                pchtml_html_document_t *document) WTF_INTERNAL;
#endif // gengyue


const unsigned char *
pchtml_html_document_title(
                pchtml_html_document_t *document, size_t *len) WTF_INTERNAL;

unsigned int
pchtml_html_document_title_set(pchtml_html_document_t *document,
                const unsigned char *title, size_t len) WTF_INTERNAL;

const unsigned char *
pchtml_html_document_title_raw(
                pchtml_html_document_t *document, size_t *len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_html_head_element_t *
pchtml_html_document_head_element(pchtml_html_document_t *document)
{
    return document->head;
}

static inline pchtml_html_body_element_t *
pchtml_html_document_body_element(pchtml_html_document_t *document)
{
    return document->body;
}

static inline pcdom_document_t *
pchtml_html_document_original_ref(pchtml_html_document_t *document)
{
    if (pcdom_interface_node(document)->owner_document
        != &document->dom_document)
    {
        return pcdom_interface_node(document)->owner_document;
    }

    return pcdom_interface_document(document);
}

static inline bool
pchtml_html_document_is_original(pchtml_html_document_t *document)
{
    return pcdom_interface_node(document)->owner_document
        == &document->dom_document;
}

static inline pcutils_mraw_t *
pchtml_html_document_mraw(pchtml_html_document_t *document)
{
    return (pcutils_mraw_t *) pcdom_interface_document(document)->mraw;
}

static inline pcutils_mraw_t *
pchtml_html_document_mraw_text(pchtml_html_document_t *document)
{
    return (pcutils_mraw_t *) pcdom_interface_document(document)->text;
}

static inline void
pchtml_html_document_opt_set(pchtml_html_document_t *document,
                          pchtml_html_document_opt_t opt)
{
    document->opt = opt;
}

static inline pchtml_html_document_opt_t
pchtml_html_document_opt(pchtml_html_document_t *document)
{
    return document->opt;
}

static inline pcutils_hash_t *
pchtml_html_document_tags(pchtml_html_document_t *document)
{
    return document->dom_document.tags;
}

static inline void *
pchtml_html_document_create_struct(pchtml_html_document_t *document,
                                size_t struct_size)
{
    return pcutils_mraw_calloc(pcdom_interface_document(document)->mraw,
                              struct_size);
}

static inline void *
pchtml_html_document_destroy_struct(pchtml_html_document_t *document, void *data)
{
    return pcutils_mraw_free(pcdom_interface_document(document)->mraw, data);
}

static inline pchtml_html_element_t *
pchtml_html_document_create_element(pchtml_html_document_t *document,
                                 const unsigned char *local_name, size_t lname_len,
                                 void *reserved_for_opt)
{
    return (pchtml_html_element_t *) pcdom_document_create_element(
            &document->dom_document, local_name, lname_len,
            reserved_for_opt, false);
}

static inline pcdom_element_t *
pchtml_html_document_destroy_element(pcdom_element_t *element)
{
    return pcdom_document_destroy_element(element);
}

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_DOCUMENT_H */
