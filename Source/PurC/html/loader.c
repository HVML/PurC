/*
 * @file parser.c
 * @author Xu Xiaohong
 * @date 2021/07/30
 * @brief The API for html-parser
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

#include "config.h"

#include "purc-errors.h"
#include "purc-macros.h"

#include "private/debug.h"
#include "private/html.h"
#include "private/errors.h"
#include "interfaces/document.h"

struct pchtml_parser {
    pchtml_document_t       doc;
};

struct pchtml_document {
    pchtml_html_document_t *doc;
};

static inline void
_html_document_release(pchtml_document_t doc)
{
    if (!doc->doc)
        return;
    pchtml_html_document_destroy(doc->doc);
    doc->doc = NULL;
}

static inline int
_html_prepare_chunk(pchtml_document_t doc)
{
    doc->doc = pchtml_html_document_create();
    if (doc->doc == NULL) {
        // pchtml_html_document_create shall have already
        // set error code
        return -1;
    }

    unsigned int  status = 0;
    status = pchtml_html_document_parse_chunk_begin(doc->doc);
    if (status != PCHTML_STATUS_OK) {
        return -1;
    }

    return 0;
}

static inline int
_html_parse_chunk(pchtml_document_t doc, purc_rwstream_t in)
{
    while (1) {
        char      utf8[16];
        wchar_t   wc;
        int       n;
        n = purc_rwstream_read_utf8_char(in, utf8, &wc);
        if (n<0) {
            return -1;
        }
        if (n==0) {
            return 0;
        }
        PC_ASSERT((size_t)n<sizeof(utf8));

        unsigned int status;
        status = pchtml_html_document_parse_chunk(doc->doc,
                    (const unsigned char*)utf8, n);
        if (status != PURC_ERROR_OK) {
            pcinst_set_error(status);
            return -1;
        }
    }
}

static inline int
_html_parse_end(pchtml_document_t doc)
{
    unsigned int status;
    status = pchtml_html_document_parse_chunk_end(doc->doc);
    if (status != PURC_ERROR_OK) {
        pcinst_set_error(status);
        return -1;
    }
    return 0;
}

pchtml_document_t
pchtml_doc_load_from_stream(purc_rwstream_t in)
{
    if (!in) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    pchtml_document_t doc;
    doc = (pchtml_document_t)calloc(1, sizeof(*doc));
    if (!doc) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    do {
        int r;
        r = _html_prepare_chunk(doc);
        if (r) {
            break;
        }

        r = _html_parse_chunk(doc, in);
        if (r) {
            int err = purc_get_last_error();
            if (err!=PCRWSTREAM_ERROR_IO) {
                // not necessary to call chunk_end
                break;
            }
        }

        r = _html_parse_end(doc);
        if (r) {
            break;
        }

        return doc;
    } while (0);

    pchtml_doc_destroy(doc);
    doc = NULL;
    return NULL;
}

static inline void
_html_parser_release(pchtml_parser_t parser)
{
    if (!parser->doc)
        return;

    int r = pchtml_doc_destroy(parser->doc);
    PC_ASSERT(r==0);
    parser->doc = NULL;
}

pchtml_parser_t pchtml_parser_create(void)
{
    pchtml_parser_t parser;
    parser = (pchtml_parser_t)calloc(1, sizeof(*parser));
    if (!parser) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    do {
        parser->doc = (pchtml_document_t)calloc(1, sizeof(*parser->doc));
        if (!parser->doc) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
        if (_html_prepare_chunk(parser->doc)) {
            break;
        }

        return parser;
    } while (0);

    pchtml_parser_destroy(parser);
    parser = NULL;
    return NULL;
}

int pchtml_parser_parse_chunk(pchtml_parser_t parser, purc_rwstream_t in)
{
    if (!parser || !parser->doc || !in) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    int r;
    r = _html_parse_chunk(parser->doc, in);
    if (r) {
        int err = purc_get_last_error();
        if (err!=PCRWSTREAM_ERROR_IO) {
            // not necessary to call chunk_end
            return -1;
        }
    }
    return 0;
}

int pchtml_parser_parse_end(pchtml_parser_t parser, pchtml_document_t *doc)
{
    if (!parser || !parser->doc) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    int r;
    r = _html_parse_end(parser->doc);
    if (r) {
        return -1;
    }

    if (doc) {
        *doc = parser->doc;
        parser->doc = NULL;
    }

    return 0;
}

int pchtml_parser_reset(pchtml_parser_t parser)
{
    PC_ASSERT(parser);

    // fix me: reuse doc?
    pchtml_document_t doc;
    doc = (pchtml_document_t)calloc(1, sizeof(*doc));
    if (!doc) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    if (_html_prepare_chunk(doc)) {
        int r = pchtml_doc_destroy(doc);
        PC_ASSERT(r==0);
        doc = NULL;
        return -1;
    }

    if (parser->doc) {
        int r = pchtml_doc_destroy(doc);
        PC_ASSERT(r==0);
    }
    parser->doc = doc;

    return 0;
}

void pchtml_parser_destroy(pchtml_parser_t parser)
{
    if (!parser) {
        return;
    }
    _html_parser_release(parser);
    free(parser);
}

static inline unsigned int
serializer_callback(const unsigned char  *data, size_t len, void *ctx)
{
    purc_rwstream_t out = (purc_rwstream_t)ctx;
    static __thread char buf[1024*1024]; // big enough?
    int n = snprintf(buf, sizeof(buf), "%.*s", (int)len, (const char *)data);
    if (n<0) {
        // which err-code to set?
        pcinst_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        // which specific status-code to return?
        return PCHTML_STATUS_ERROR;
    }
    if ((size_t)n>=sizeof(buf)) {
        pcinst_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        return PCHTML_STATUS_ERROR_TOO_SMALL_SIZE;
    }

    ssize_t sz;
    sz = purc_rwstream_write(out, (const void*)buf, n);
    if (sz!=n) {
        // which specific status-code to return?
        return PCHTML_STATUS_ERROR;
    }

    return PCHTML_STATUS_OK;
}

int
pchtml_doc_write_to_stream(pchtml_document_t doc, purc_rwstream_t out)
{
    if (!doc || !doc->doc || !out) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    unsigned int status;
    status = pchtml_html_serialize_pretty_tree_cb((pcedom_node_t *)doc->doc,
                                          0x00, 0, serializer_callback, out);
    if (status!=PCHTML_STATUS_OK) {
        return -1;
    }
    return 0;
}

int
pchtml_doc_destroy(pchtml_document_t doc)
{
    if (!doc || !doc->doc)
        return -1;

    _html_document_release(doc);
    free(doc);
    return 0;
}

