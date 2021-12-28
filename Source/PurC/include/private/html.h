/**
 * @file html.h
 * @author 
 * @date 2021/07/02
 * @brief The internal interfaces for html parser.
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

#ifndef PURC_PRIVATE_HTML_H
#define PURC_PRIVATE_HTML_H

#include "config.h"
#include "private/edom.h"
#include "purc-rwstream.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// initialize html module (once)
void pchtml_init_once(void) WTF_INTERNAL;

struct pcinst;

// initialize the html module for a PurC instance.
void pchtml_init_instance(struct pcinst* inst) WTF_INTERNAL;
// clean up the html module for a PurC instance.
void pchtml_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

struct pchtml_html_document;
typedef struct pchtml_html_document pchtml_html_document_t;

struct pchtml_html_element;
typedef struct pchtml_html_element pchtml_html_element_t;

struct pchtml_html_body_element;
typedef struct pchtml_html_body_element pchtml_html_body_element_t;

struct pchtml_html_parser;
typedef struct pchtml_html_parser pchtml_html_parser_t;

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
                const purc_rwstream_t html);

unsigned int
pchtml_html_parse_chunk_end(pchtml_html_parser_t *parser);

pcedom_node_t *
pchtml_html_parse_fragment(pchtml_html_parser_t *parser, 
                        pchtml_html_element_t *element,
                        const purc_rwstream_t html);

pcedom_node_t *
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
                const purc_rwstream_t html);

pcedom_node_t *
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
                const purc_rwstream_t html) ;

unsigned int
pchtml_html_document_parse_chunk_end(
                pchtml_html_document_t *document) ;

// API for parse fragment 
pcedom_node_t *
pchtml_html_document_parse_fragment(pchtml_html_document_t *document,
                pcedom_element_t *element,
                const purc_rwstream_t html) ;

unsigned int
pchtml_html_document_parse_fragment_chunk_begin(
                pchtml_html_document_t *document,
                pcedom_element_t *element) ;

unsigned int
pchtml_html_document_parse_fragment_chunk(pchtml_html_document_t *document,
                const purc_rwstream_t html) ;

pcedom_node_t *
pchtml_html_document_parse_fragment_chunk_end(
                pchtml_html_document_t *document) ;

pchtml_html_element_t *
pchtml_html_element_inner_html_set(pchtml_html_element_t *element,
                const purc_rwstream_t html);

// for serialize
typedef unsigned int
(*pchtml_html_serialize_cb_f)(const unsigned char *data, size_t len, void *ctx);

unsigned int
pchtml_html_serialize_pretty_tree_cb(pcedom_node_t *node,
                int opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) ;

int
pchtml_doc_write_to_stream(pchtml_html_document_t *doc, purc_rwstream_t out);

struct pcedom_document*
pchtml_doc_get_document(pchtml_html_document_t *doc);


// for html command
enum {
    ID_HTML_CMD_APPEND = 0,
    ID_HTML_CMD_PREPEND,
    ID_HTML_CMD_INSERTBEFORE,
    ID_HTML_CMD_INSERTAFTER,
};

purc_atom_t get_html_cmd_atom(size_t id);


// doc:  html document root
// node: the node fragment associated.If it is NULL, <body> will be used;
// html: html stream
pcedom_node_t * pchtml_edom_document_parse_fragment (pchtml_html_document_t *doc,
                pcedom_node_t *node, purc_rwstream_t html);

// node: the position will be used;
// fragment_root: the return value from pchtml_edom_document_parse_fragment
// op: atom value, which can be one of append / prepend / insertBefore /insertAfter
bool pchtml_edom_insert_node(pcedom_node_t *node, pcedom_node_t *fragment_root,
        purc_atom_t op);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_HTML_H */

