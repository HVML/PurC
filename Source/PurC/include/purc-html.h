/**
 * @file purc-html.h
 * @author Vincent Wei
 * @date 2022/01/02
 * @brief The API for HTML parser.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PURC_HTML_H
#define PURC_PURC_HTML_H

#include "purc-dom.h"
#include "purc-rwstream.h"

typedef enum {
    PCHTML_STATUS_OK                       = PURC_ERROR_OK,
    PCHTML_STATUS_ERROR                    = PURC_ERROR_INVALID_VALUE,
    PCHTML_STATUS_ERROR_MEMORY_ALLOCATION  = PURC_ERROR_OUT_OF_MEMORY,
    PCHTML_STATUS_ERROR_OBJECT_IS_NULL     = PURC_ERROR_NULL_OBJECT,
    PCHTML_STATUS_ERROR_SMALL_BUFFER       = PURC_ERROR_TOO_SMALL_BUFF,
    PCHTML_STATUS_ERROR_TOO_SMALL_SIZE     = PURC_ERROR_TOO_SMALL_SIZE,
    PCHTML_STATUS_ERROR_INCOMPLETE_OBJECT  = PURC_ERROR_INCOMPLETE_OBJECT,
    PCHTML_STATUS_ERROR_NO_FREE_SLOT       = PURC_ERROR_NO_FREE_SLOT,
    PCHTML_STATUS_ERROR_NOT_EXISTS         = PURC_ERROR_NOT_EXISTS,
    PCHTML_STATUS_ERROR_WRONG_ARGS         = PURC_ERROR_ARGUMENT_MISSED,
    PCHTML_STATUS_ERROR_WRONG_STAGE        = PURC_ERROR_WRONG_STAGE,
    PCHTML_STATUS_ERROR_OVERFLOW           = PURC_ERROR_OVERFLOW,
    PCHTML_STATUS_CONTINUE                 = PURC_ERROR_FIRST_HTML,
    PCHTML_STATUS_SMALL_BUFFER,
    PCHTML_STATUS_ABORTED,
    PCHTML_STATUS_STOPPED,
    PCHTML_STATUS_NEXT,
    PCHTML_STATUS_STOP,
} pchtml_status_t;

struct pchtml_html_document;
typedef struct pchtml_html_document pchtml_html_document_t;

struct pchtml_html_element;
typedef struct pchtml_html_element pchtml_html_element_t;

struct pchtml_html_body_element;
typedef struct pchtml_html_body_element pchtml_html_body_element_t;

struct pchtml_html_parser;
typedef struct pchtml_html_parser pchtml_html_parser_t;

PCA_EXTERN_C_BEGIN

// API for parser
pchtml_html_parser_t *
pchtml_html_parser_create(void);

unsigned int
pchtml_html_parser_init(pchtml_html_parser_t *parser);

void
pchtml_html_parser_clean(pchtml_html_parser_t *parser);

pchtml_html_parser_t *
pchtml_html_parser_destroy(pchtml_html_parser_t *parser);

pchtml_html_document_t *
pchtml_html_parse(pchtml_html_parser_t *parser, const purc_rwstream_t html);

pchtml_html_document_t *
pchtml_html_parse_chunk_begin(pchtml_html_parser_t *parser);

unsigned int
pchtml_html_parse_chunk_process(pchtml_html_parser_t *parser,
                const unsigned char *data, size_t sz);

unsigned int
pchtml_html_parse_chunk_end(pchtml_html_parser_t *parser);

pcdom_node_t *
pchtml_html_parse_fragment(pchtml_html_parser_t *parser,
                        pchtml_html_element_t *element,
                        const purc_rwstream_t html);

pcdom_node_t *
pchtml_html_parse_fragment_by_tag_id(pchtml_html_parser_t *parser,
                pchtml_html_document_t *document,
                pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                const purc_rwstream_t html);


unsigned int
pchtml_html_parse_fragment_chunk_begin(pchtml_html_parser_t *parser,
                pchtml_html_document_t *document,
                pchtml_tag_id_t tag_id, pchtml_ns_id_t ns);

unsigned int
pchtml_html_parse_fragment_chunk_process(pchtml_html_parser_t *parser,
                const unsigned char *data, size_t sz);

pcdom_node_t *
pchtml_html_parse_fragment_chunk_end(pchtml_html_parser_t *parser);

// API for document
pchtml_html_document_t *
pchtml_html_document_create(void);

void
pchtml_html_document_clean(pchtml_html_document_t *document);

pchtml_html_document_t *
pchtml_html_document_destroy(pchtml_html_document_t *document);


// API for parse document
unsigned int
pchtml_html_document_parse(pchtml_html_document_t *document,
                const purc_rwstream_t html) ;

unsigned int
pchtml_html_document_parse_chunk_begin(
                pchtml_html_document_t *document) ;

unsigned int
pchtml_html_document_parse_chunk(pchtml_html_document_t *document,
                const unsigned char *data, size_t sz);

unsigned int
pchtml_html_document_parse_chunk_end(
                pchtml_html_document_t *document) ;

// API for parsing fragment 
pcdom_node_t *
pchtml_html_document_parse_fragment(pchtml_html_document_t *document,
                pcdom_element_t *element,
                const purc_rwstream_t html) ;

unsigned int
pchtml_html_document_parse_fragment_chunk_begin(
                pchtml_html_document_t *document,
                pcdom_element_t *element) ;

unsigned int
pchtml_html_document_parse_fragment_chunk(pchtml_html_document_t *document,
                const unsigned char *data, size_t sz);

pcdom_node_t *
pchtml_html_document_parse_fragment_chunk_end(
                pchtml_html_document_t *document) ;

pchtml_html_element_t *
pchtml_html_element_inner_html_set(pchtml_html_element_t *element,
                const purc_rwstream_t html);

// for serialize
typedef unsigned int
(*pchtml_html_serialize_cb_f)(const unsigned char *data, size_t len, void *ctx);

unsigned int
pchtml_html_serialize_pretty_tree_cb(pcdom_node_t *node,
                int opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) ;

int
pchtml_doc_write_to_stream(pchtml_html_document_t *doc, purc_rwstream_t out);

struct pcdom_document*
pchtml_doc_get_document(pchtml_html_document_t *doc);

PCA_EXTERN_C_END

#endif  /* PURC_PURC_HTML_H */

