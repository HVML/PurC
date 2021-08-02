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
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "config.h"

#include "purc-html-parser.h"
#include "purc-errors.h"
#include "purc-macros.h"

#include "private/debug.h"
#include "private/html.h"
#include "private/errors.h"
#include "interfaces/document.h"

struct purc_html_document {
    pchtml_html_document_t *doc;
};

static inline void
_html_document_release(purc_html_document_t doc)
{
    if (!doc->doc)
        return;
    pchtml_html_document_destroy(doc->doc);
    doc->doc = NULL;
}

static inline unsigned int
_html_parse_chunk(purc_html_document_t doc, purc_rwstream_t in)
{
    while (1) {
        char      utf8[16];
        wchar_t   wc;
        int       n;
        n = purc_rwstream_read_utf8_char(in, utf8, &wc);
        if (n<0) {
            // which specific PCHTML_STATUS_xxx to return?
            return PCHTML_STATUS_ERROR;
        }
        if (n==0) {
            return PCHTML_STATUS_OK;
        }
        PC_ASSERT((size_t)n<sizeof(utf8));

        unsigned int status;
        status = pchtml_html_document_parse_chunk(doc->doc,
                    (const unsigned char*)utf8, n);
        if (status != PCHTML_STATUS_OK) {
        PC_ASSERT(0);
            return status;
        }
    }
}

purc_html_document_t
purc_html_doc_load_from_stream(purc_rwstream_t in)
{
    if (!in) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        PC_ASSERT(0);
        return NULL;
    }

    purc_html_document_t doc;
    doc = (purc_html_document_t)calloc(1, sizeof(*doc));
    if (!doc) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        PC_ASSERT(0);
        return NULL;
    }

    do {
        doc->doc = pchtml_html_document_create();
        if (doc->doc == NULL) {
            // pchtml_html_document_create shall have already
            // set error code
        PC_ASSERT(0);
            break;
        }
        unsigned int  status = 0;
        status = pchtml_html_document_parse_chunk_begin(doc->doc);
        if (status != PCHTML_STATUS_OK) {
        PC_ASSERT(0);
            break;
        }

        status = _html_parse_chunk(doc, in);
        if (status != PCHTML_STATUS_OK) {
            // not necessary to call chunk_end
            PC_ASSERT(status==PCHTML_STATUS_ERROR);
            int err = purc_get_last_error();
            PC_ASSERT(err==PCRWSTREAM_ERROR_IO);
        }

        status = pchtml_html_document_parse_chunk_end(doc->doc);
        if (status != PCHTML_STATUS_OK) {
        PC_ASSERT(0);
            break;
        }

        return doc;
    } while (0);

    _html_document_release(doc);
    free(doc);
    return NULL;
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
purc_html_doc_write_to_stream(purc_html_document_t doc, purc_rwstream_t out)
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
purc_html_doc_destroy(purc_html_document_t doc)
{
    if (!doc || !doc->doc)
        return -1;

    _html_document_release(doc);
    free(doc);
    return 0;
}

