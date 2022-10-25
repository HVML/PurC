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

#define pchtml_interface_document(obj) ((pchtml_html_document_t *) (obj))

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

static inline pchtml_html_document_t *
pchtml_html_parse_with_buf(pchtml_html_parser_t *parser,
        const unsigned char *data, size_t sz)
{
    purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)data, sz);
    if (rs == NULL)
        return NULL;

    pchtml_html_document_t *doc;
    doc = pchtml_html_parse(parser, rs);

    purc_rwstream_destroy(rs);

    return doc;
}

static inline pchtml_html_document_t*
pchmtl_html_load_document_with_buf(const unsigned char *data, size_t sz)
{
    pchtml_html_parser_t *parser;
    parser = pchtml_html_parser_create();
    if (!parser)
        return NULL;

    pchtml_html_document_t *doc = NULL;
    int r = 0;
    do {
        r = pchtml_html_parser_init(parser);
        if (r)
            break;
        doc = pchtml_html_parse_with_buf(parser, data, sz);
    } while (0);

    pchtml_html_parser_destroy(parser);

    return doc;
}

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

static inline pcdom_node_t *
pchtml_html_parse_fragment_with_buf(pchtml_html_parser_t *parser,
        pchtml_html_element_t *element,
        const unsigned char *data, size_t sz)
{
    purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)data, sz);
    if (rs == NULL)
        return NULL;

    pcdom_node_t *node;
    node = pchtml_html_parse_fragment(parser, element, rs);

    purc_rwstream_destroy(rs);

    return node;
}

pcdom_node_t *
pchtml_html_parse_fragment_by_tag_id(pchtml_html_parser_t *parser,
                pchtml_html_document_t *document,
                pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                const purc_rwstream_t html);

static inline pcdom_node_t *
pchtml_html_parse_fragment_by_tag_id_with_buf(pchtml_html_parser_t *parser,
        pchtml_html_document_t *document,
        pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
        const unsigned char *data, size_t sz)
{
    purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)data, sz);
    if (rs == NULL)
        return NULL;

    pcdom_node_t *node;
    node = pchtml_html_parse_fragment_by_tag_id(parser,
        document, tag_id, ns, rs);

    purc_rwstream_destroy(rs);

    return node;
}


unsigned int
pchtml_html_parse_fragment_chunk_begin(pchtml_html_parser_t *parser,
                pchtml_html_document_t *document,
                pchtml_tag_id_t tag_id, pchtml_ns_id_t ns);

unsigned int
pchtml_html_parse_fragment_chunk_process(pchtml_html_parser_t *parser,
                const unsigned char *data, size_t sz);

PCA_ATTRIBUTE_PRINTF(2, 3)
unsigned int
pchtml_html_parse_fragment_chunk_process_with_format(
        pchtml_html_parser_t *parser, const char *fmt, ...);

pcdom_node_t *
pchtml_html_parse_fragment_chunk_end(pchtml_html_parser_t *parser);

// API for node
bool
pchtml_html_node_is_void(pcdom_node_t *node);

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

static inline unsigned int
pchtml_html_document_parse_with_buf(pchtml_html_document_t *document,
        const unsigned char *data, size_t sz)
{
    purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)data, sz);
    if (rs == NULL)
        return -1;

    unsigned int r;
    r = pchtml_html_document_parse(document, rs);

    purc_rwstream_destroy(rs);

    return r;
}

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

static inline pcdom_node_t *
pchtml_html_document_parse_fragment_with_buf(pchtml_html_document_t *document,
        pcdom_element_t *element,
        const unsigned char *data, size_t sz)
{
    purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)data, sz);
    if (rs == NULL)
        return NULL;

    pcdom_node_t *node;
    node = pchtml_html_document_parse_fragment(document, element, rs);

    purc_rwstream_destroy(rs);

    return node;
}


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

static inline pchtml_html_element_t *
pchtml_html_element_inner_html_set_with_buf(pchtml_html_element_t *element,
        const unsigned char *data, size_t sz)
{
    purc_rwstream_t rs = purc_rwstream_new_from_mem((void*)data, sz);
    if (rs == NULL)
        return NULL;

    pchtml_html_element_t *elem;
    elem = pchtml_html_element_inner_html_set(element, rs);

    purc_rwstream_destroy(rs);

    return elem;
}


// for serialize
typedef unsigned int
(*pchtml_html_serialize_cb_f)(const unsigned char *data, size_t len, void *ctx);

unsigned int
pchtml_html_serialize_pretty_tree_cb(pcdom_node_t *node,
                int opt, size_t indent,
                pchtml_html_serialize_cb_f cb, void *ctx) ;

typedef int pchtml_html_serialize_opt_t;

enum pchtml_html_serialize_opt {
    PCHTML_HTML_SERIALIZE_OPT_UNDEF               = 0x00,
    PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES       = 0x01,
    PCHTML_HTML_SERIALIZE_OPT_SKIP_COMMENT        = 0x02,
    PCHTML_HTML_SERIALIZE_OPT_RAW                 = 0x04,
    PCHTML_HTML_SERIALIZE_OPT_WITHOUT_CLOSING     = 0x08,
    PCHTML_HTML_SERIALIZE_OPT_TAG_WITH_NS         = 0x10,
    PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT = 0x20,
    PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE        = 0x40,
    PCHTML_HTML_SERIALIZE_OPT_WITH_HVML_HANDLE    = 0x80
};

int
pchtml_doc_write_to_stream_ex(pchtml_html_document_t *doc,
        enum pchtml_html_serialize_opt opt, purc_rwstream_t out);

char*
pchtml_doc_snprintf_ex(pchtml_html_document_t *doc,
        enum pchtml_html_serialize_opt opt, char *buf, size_t *io_sz,
        const char *prefix);

static inline int
pchtml_doc_write_to_stream(pchtml_html_document_t *doc, purc_rwstream_t out)
{
    return pchtml_doc_write_to_stream_ex(doc,
            PCHTML_HTML_SERIALIZE_OPT_UNDEF, out);
}

#define pchtml_doc_snprintf(_doc, _buf, _pio_sz, _prefix, ...)          \
    pchtml_doc_snprintf_ex(_doc, PCHTML_HTML_SERIALIZE_OPT_UNDEF,       \
            _buf, _pio_sz, _prefix)

#define pchtml_doc_snprintf_plain(_doc, _buf, _pio_sz, _prefix, ...)    \
({                                                                      \
    int _opt = 0;                                                       \
    _opt |= PCHTML_HTML_SERIALIZE_OPT_UNDEF;                            \
    _opt |= PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;                    \
    _opt |= PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;              \
    _opt |= PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE;                     \
    pchtml_doc_snprintf_ex(_doc, (enum pchtml_html_serialize_opt)_opt,  \
            _buf, _pio_sz, _prefix);                                    \
})

int
pcdom_node_write_to_stream_ex(pcdom_node_t *node,
        enum pchtml_html_serialize_opt opt, purc_rwstream_t out);

char*
pcdom_node_snprintf_ex(pcdom_node_t *node,
        enum pchtml_html_serialize_opt opt, char *buf, size_t *io_sz,
        const char *prefix);

static inline int
pcdom_node_write_to_stream(pcdom_node_t *node, purc_rwstream_t out)
{
    return pcdom_node_write_to_stream_ex(node,
            PCHTML_HTML_SERIALIZE_OPT_UNDEF, out);
}

#define pcdom_node_snprintf(_node, _buf, _pio_sz, _prefix, ...)           \
    pcdom_node_snprintf_ex(_node, PCHTML_HTML_SERIALIZE_OPT_UNDEF,        \
            _buf, _pio_sz, _prefix)

#define pcdom_node_snprintf_plain(_node, _buf, _pio_sz, _prefix, ...)   \
({                                                                      \
    int _opt = 0;                                                       \
    _opt |= PCHTML_HTML_SERIALIZE_OPT_UNDEF;                            \
    _opt |= PCHTML_HTML_SERIALIZE_OPT_SKIP_WS_NODES;                    \
    _opt |= PCHTML_HTML_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;              \
    _opt |= PCHTML_HTML_SERIALIZE_OPT_FULL_DOCTYPE;                     \
    pcdom_node_snprintf_ex(_node, (enum pchtml_html_serialize_opt)_opt, \
            _buf, _pio_sz, _prefix);                                    \
})

struct pcdom_document*
pchtml_doc_get_document(pchtml_html_document_t *doc);

struct pcdom_element*
pchtml_doc_get_head(pchtml_html_document_t *doc);

struct pcdom_element*
pchtml_doc_get_body(pchtml_html_document_t *doc);

pchtml_html_parser_t*
pchtml_doc_get_parser(pchtml_html_document_t *doc);

PCA_EXTERN_C_END

#endif  /* PURC_PURC_HTML_H */

