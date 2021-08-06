/**
 * @file parser.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html parser.
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


#ifndef PCHTML_HTML_PARSER_H
#define PCHTML_HTML_PARSER_H

#include "config.h"

#include "html/base.h"
#include "html/tree.h"
#include "html/interfaces/document.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PCHTML_HTML_PARSER_STATE_BEGIN            = 0x00,
    PCHTML_HTML_PARSER_STATE_PROCESS          = 0x01,
    PCHTML_HTML_PARSER_STATE_END              = 0x02,
    PCHTML_HTML_PARSER_STATE_FRAGMENT_PROCESS = 0x03,
    PCHTML_HTML_PARSER_STATE_ERROR            = 0x04
}
pchtml_html_parser_state_t;

#if 0
typedef struct {
    pchtml_html_tokenizer_t    *tkz;
    pchtml_html_tree_t         *tree;
    pchtml_html_tree_t         *original_tree;

    pcedom_node_t          *root;
    pcedom_node_t          *form;

    pchtml_html_parser_state_t state;
    unsigned int            status;

    size_t                  ref_count;
}
pchtml_html_parser_t;
#endif
struct pchtml_html_parser {
    pchtml_html_tokenizer_t    *tkz;
    pchtml_html_tree_t         *tree;
    pchtml_html_tree_t         *original_tree;

    pcedom_node_t          *root;
    pcedom_node_t          *form;

    pchtml_html_parser_state_t state;
    unsigned int            status;

    size_t                  ref_count;
};

typedef struct pchtml_html_parser pchtml_html_parser_t;

// gengyue
#if 0
pchtml_html_parser_t *
pchtml_html_parser_create(void) WTF_INTERNAL;

unsigned int
pchtml_html_parser_init(pchtml_html_parser_t *parser) WTF_INTERNAL;

void
pchtml_html_parser_clean(pchtml_html_parser_t *parser) WTF_INTERNAL;

pchtml_html_parser_t *
pchtml_html_parser_destroy(pchtml_html_parser_t *parser) WTF_INTERNAL;

#endif

pchtml_html_parser_t *
pchtml_html_parser_ref(pchtml_html_parser_t *parser) WTF_INTERNAL;

pchtml_html_parser_t *
pchtml_html_parser_unref(pchtml_html_parser_t *parser) WTF_INTERNAL;

// gengyue
#if 0
pchtml_html_document_t *
pchtml_html_parse(pchtml_html_parser_t *parser,
                const unsigned char *html, size_t size) WTF_INTERNAL;
#endif

pcedom_node_t *
pchtml_html_parse_fragment(pchtml_html_parser_t *parser, pchtml_html_element_t *element,
                        const purc_rwstream_t html) WTF_INTERNAL;

pcedom_node_t *
pchtml_html_parse_fragment_by_tag_id(pchtml_html_parser_t *parser,
                pchtml_html_document_t *document,
                pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                const purc_rwstream_t html) WTF_INTERNAL;


pchtml_html_document_t *
pchtml_html_parse_chunk_begin(pchtml_html_parser_t *parser) WTF_INTERNAL;

unsigned int
pchtml_html_parse_chunk_process(pchtml_html_parser_t *parser,
                const purc_rwstream_t html) WTF_INTERNAL;

unsigned int
pchtml_html_parse_chunk_end(pchtml_html_parser_t *parser) WTF_INTERNAL;


unsigned int
pchtml_html_parse_fragment_chunk_begin(pchtml_html_parser_t *parser,
                pchtml_html_document_t *document,
                pchtml_tag_id_t tag_id, pchtml_ns_id_t ns) WTF_INTERNAL;

unsigned int
pchtml_html_parse_fragment_chunk_process(pchtml_html_parser_t *parser,
                const purc_rwstream_t html) WTF_INTERNAL;

pcedom_node_t *
pchtml_html_parse_fragment_chunk_end(pchtml_html_parser_t *parser) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_html_tokenizer_t *
pchtml_html_parser_tokenizer(pchtml_html_parser_t *parser)
{
    return parser->tkz;
}

static inline pchtml_html_tree_t *
pchtml_html_parser_tree(pchtml_html_parser_t *parser)
{
    return parser->tree;
}

static inline unsigned int
pchtml_html_parser_status(pchtml_html_parser_t *parser)
{
    return parser->status;
}

static inline unsigned int
pchtml_html_parser_state(pchtml_html_parser_t *parser)
{
    return parser->state;
}


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_PARSER_H */
