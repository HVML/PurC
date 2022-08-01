/**
 * @file parser.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html parser.
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

#include "html/parser.h"
#include "html/node.h"
#include "html/tree/open_elements.h"
#include "html/interfaces/element.h"
#include "html/interfaces/html_element.h"
#include "html/interfaces/form_element.h"
#include "html/tree/template_insertion.h"
#include "html/tree/insertion_mode.h"
#include "html/interfaces/document.h"

#define PCHTML_HTML_TAG_RES_DATA
#define PCHTML_HTML_TAG_RES_SHS_DATA
#include "html_tag_res_ext.h"


static void
pchtml_html_parse_fragment_chunk_destroy(pchtml_html_parser_t *parser);


pchtml_html_parser_t *
pchtml_html_parser_create(void)
{
    return pcutils_calloc(1, sizeof(pchtml_html_parser_t));
}

unsigned int
pchtml_html_parser_init(pchtml_html_parser_t *parser)
{
    if (parser == NULL) {
        pcinst_set_error (PURC_ERROR_NULL_OBJECT);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    /* Tokenizer */
    parser->tkz = pchtml_html_tokenizer_create();
    unsigned int status = pchtml_html_tokenizer_init(parser->tkz);

    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    /* Tree */
    parser->tree = pchtml_html_tree_create();
    status = pchtml_html_tree_init(parser->tree, parser->tkz);

    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    parser->original_tree = NULL;
    parser->form = NULL;
    parser->root = NULL;

    parser->state = PCHTML_HTML_PARSER_STATE_BEGIN;

    parser->ref_count = 1;

    return PCHTML_STATUS_OK;
}

void
pchtml_html_parser_clean(pchtml_html_parser_t *parser)
{
    parser->original_tree = NULL;
    parser->form = NULL;
    parser->root = NULL;

    parser->state = PCHTML_HTML_PARSER_STATE_BEGIN;

    pchtml_html_tokenizer_clean(parser->tkz);
    pchtml_html_tree_clean(parser->tree);
}

pchtml_html_parser_t *
pchtml_html_parser_destroy(pchtml_html_parser_t *parser)
{
    if (parser == NULL) {
        return NULL;
    }

    parser->tkz = pchtml_html_tokenizer_unref(parser->tkz);
    parser->tree = pchtml_html_tree_unref(parser->tree);

    return pcutils_free(parser);
}

pchtml_html_parser_t *
pchtml_html_parser_ref(pchtml_html_parser_t *parser)
{
    if (parser == NULL) {
        return NULL;
    }

    parser->ref_count++;

    return parser;
}

pchtml_html_parser_t *
pchtml_html_parser_unref(pchtml_html_parser_t *parser)
{
    if (parser == NULL || parser->ref_count == 0) {
        return NULL;
    }

    parser->ref_count--;

    if (parser->ref_count == 0) {
        pchtml_html_parser_destroy(parser);
    }

    return NULL;
}


pchtml_html_document_t *
pchtml_html_parse(pchtml_html_parser_t *parser,
    const purc_rwstream_t html)
{
    pchtml_html_document_t *document = pchtml_html_parse_chunk_begin(parser);
    if (document == NULL) {
        return NULL;
    }

    ssize_t sz;
    while (1) {
        char buf[1024];
        sz = purc_rwstream_read(html, buf, sizeof(buf));
        if (sz <= 0)
            break;

        pchtml_html_parse_chunk_process(parser, (unsigned char*)buf, sz);
        if (parser->status != PCHTML_STATUS_OK) {
            goto failed;
        }
    }

    pchtml_html_parse_chunk_end(parser);
    if (parser->status != PCHTML_STATUS_OK) {
        goto failed;
    }

    return document;

failed:

    pchtml_html_document_interface_destroy(document);

    return NULL;
}

pcdom_node_t *
pchtml_html_parse_fragment(pchtml_html_parser_t *parser, pchtml_html_element_t *element,
                        const purc_rwstream_t html)
{
    return pchtml_html_parse_fragment_by_tag_id(parser,
                                             parser->tree->document,
                                             element->element.node.local_name,
                                             element->element.node.ns,
                                             html);
}

pcdom_node_t *
pchtml_html_parse_fragment_by_tag_id(pchtml_html_parser_t *parser,
                                  pchtml_html_document_t *document,
                                  pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                                  const purc_rwstream_t html)
{
    pchtml_html_parse_fragment_chunk_begin(parser, document, tag_id, ns);
    if (parser->status != PCHTML_STATUS_OK) {
        return NULL;
    }

    ssize_t sz;
    while (1) {
        char buf[1024];
        sz = purc_rwstream_read(html, buf, sizeof(buf));
        if (sz <= 0)
            break;

        pchtml_html_parse_fragment_chunk_process(parser,
                (unsigned char*)buf, sz);
        if (parser->status != PCHTML_STATUS_OK) {
            return NULL;
        }
    }

    return pchtml_html_parse_fragment_chunk_end(parser);
}

unsigned int
pchtml_html_parse_fragment_chunk_begin(pchtml_html_parser_t *parser,
                                    pchtml_html_document_t *document,
                                    pchtml_tag_id_t tag_id, pchtml_ns_id_t ns)
{
    pcdom_document_t *doc;
    pchtml_html_document_t *new_doc;

    if (parser->state != PCHTML_HTML_PARSER_STATE_BEGIN) {
        pchtml_html_parser_clean(parser);
    }

    parser->state = PCHTML_HTML_PARSER_STATE_FRAGMENT_PROCESS;

    new_doc = pchtml_html_document_interface_create(document);
    if (new_doc == NULL) {
        parser->state = PCHTML_HTML_PARSER_STATE_ERROR;
        return parser->status;
    }

    doc = pcdom_interface_document(new_doc);

    if (document == NULL) {
        doc->scripting = parser->tree->scripting;
        doc->compat_mode = PCDOM_DOCUMENT_CMODE_NO_QUIRKS;
    }

    pchtml_html_tokenizer_set_state_by_tag(parser->tkz, doc->scripting, tag_id, ns);

    parser->root = pchtml_html_interface_create(new_doc, PCHTML_TAG_HTML, PCHTML_NS_HTML);
    if (parser->root == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        parser->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        goto done;
    }

    pcdom_node_append_child(pcdom_interface_node(new_doc), parser->root);
    pcdom_document_attach_element(doc, pcdom_interface_element(parser->root));

    parser->tree->fragment = pchtml_html_interface_create(new_doc, tag_id, ns);
    if (parser->tree->fragment == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        parser->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        goto done;
    }

    /* Contains just the single element root */
    parser->status = pchtml_html_tree_open_elements_push(parser->tree, parser->root);
    if (parser->status != PCHTML_STATUS_OK) {
        goto done;
    }

    if (tag_id == PCHTML_TAG_TEMPLATE && ns == PCHTML_NS_HTML) {
        parser->status = pchtml_html_tree_template_insertion_push(parser->tree,
                                      pchtml_html_tree_insertion_mode_in_template);
        if (parser->status != PCHTML_STATUS_OK) {
            goto done;
        }
    }

    pchtml_html_tree_attach_document(parser->tree, new_doc);
    pchtml_html_tree_reset_insertion_mode_appropriately(parser->tree);

    if (tag_id == PCHTML_TAG_FORM && ns == PCHTML_NS_HTML) {
        parser->form = pchtml_html_interface_create(new_doc,
                                                 PCHTML_TAG_FORM, PCHTML_NS_HTML);
        if (parser->form == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            parser->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

            goto done;
        }

        parser->tree->form = pchtml_html_interface_form(parser->form);
    }

    parser->original_tree = pchtml_html_tokenizer_tree(parser->tkz);
    pchtml_html_tokenizer_tree_set(parser->tkz, parser->tree);

    pchtml_html_tokenizer_tags_set(parser->tkz, doc->tags);
    pchtml_html_tokenizer_attrs_set(parser->tkz, doc->attrs);
    pchtml_html_tokenizer_attrs_mraw_set(parser->tkz, doc->text);

    parser->status = pchtml_html_tree_begin(parser->tree, new_doc);

done:

    if (parser->status != PCHTML_STATUS_OK) {
        if (parser->root != NULL) {
            pchtml_html_html_element_interface_destroy(pchtml_html_interface_html(parser->root));
        }

        parser->state = PCHTML_HTML_PARSER_STATE_ERROR;
        parser->root = NULL;

        pchtml_html_parse_fragment_chunk_destroy(parser);
    }

    return parser->status;
}

unsigned int
pchtml_html_parse_fragment_chunk_process(pchtml_html_parser_t *parser,
                const unsigned char *data, size_t sz)
{
    if (parser->state != PCHTML_HTML_PARSER_STATE_FRAGMENT_PROCESS) {
        pcinst_set_error (PURC_ERROR_WRONG_STAGE);
        return PCHTML_STATUS_ERROR_WRONG_STAGE;
    }

    parser->status = pchtml_html_tree_chunk(parser->tree, data, sz);
    if (parser->status != PCHTML_STATUS_OK) {
        pchtml_html_html_element_interface_destroy(pchtml_html_interface_html(parser->root));

        parser->state = PCHTML_HTML_PARSER_STATE_ERROR;
        parser->root = NULL;

        pchtml_html_parse_fragment_chunk_destroy(parser);
    }

    return parser->status;
}

unsigned int
pchtml_html_parse_fragment_chunk_process_with_format(
        pchtml_html_parser_t *parser, const char *fmt, ...)
{
    char buf[1024];
    size_t nr = sizeof(buf);
    char *p = NULL;

    va_list ap;
    va_start(ap, fmt);
    p = pcutils_vsnprintf(buf, &nr, fmt, ap);
    va_end(ap);

    if (!p)
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

    unsigned int r;
    r = pchtml_html_parse_fragment_chunk_process(parser,
            (const unsigned char*)p, nr);
    if (p != buf)
        free(p);

    return r;
}

pcdom_node_t *
pchtml_html_parse_fragment_chunk_end(pchtml_html_parser_t *parser)
{
    if (parser->state != PCHTML_HTML_PARSER_STATE_FRAGMENT_PROCESS) {
        pcinst_set_error (PURC_ERROR_WRONG_STAGE);
        parser->status = PCHTML_STATUS_ERROR_WRONG_STAGE;

        return NULL;
    }

    parser->status = pchtml_html_tree_end(parser->tree);
    if (parser->status != PCHTML_STATUS_OK) {
        pchtml_html_html_element_interface_destroy(pchtml_html_interface_html(parser->root));

        parser->root = NULL;
    }

    pchtml_html_parse_fragment_chunk_destroy(parser);

    pchtml_html_tokenizer_tree_set(parser->tkz, parser->original_tree);

    parser->state = PCHTML_HTML_PARSER_STATE_END;

    return parser->root;
}

static void
pchtml_html_parse_fragment_chunk_destroy(pchtml_html_parser_t *parser)
{
    pcdom_document_t *doc;

    if (parser->form != NULL) {
        pchtml_html_form_element_interface_destroy(pchtml_html_interface_form(parser->form));

        parser->form = NULL;
    }

    if (parser->tree->fragment != NULL) {
        pchtml_html_interface_destroy(parser->tree->fragment);

        parser->tree->fragment = NULL;
    }

    if (pchtml_html_document_is_original(parser->tree->document) == false) {
        if (parser->root != NULL) {
            doc = pcdom_interface_node(parser->tree->document)->owner_document;
            parser->root->parent = &doc->node;
        }

        pchtml_html_document_interface_destroy(parser->tree->document);

        parser->tree->document = NULL;
    }
}

unsigned int
pchtml_html_parse_chunk_prepare(pchtml_html_parser_t *parser,
                             pchtml_html_document_t *document)
{
    parser->state = PCHTML_HTML_PARSER_STATE_PROCESS;

    parser->original_tree = pchtml_html_tokenizer_tree(parser->tkz);
    pchtml_html_tokenizer_tree_set(parser->tkz, parser->tree);

    pchtml_html_tokenizer_tags_set(parser->tkz, document->dom_document.tags);
    pchtml_html_tokenizer_attrs_set(parser->tkz, document->dom_document.attrs);
    pchtml_html_tokenizer_attrs_mraw_set(parser->tkz, document->dom_document.text);

    parser->status = pchtml_html_tree_begin(parser->tree, document);
    if (parser->status != PCHTML_STATUS_OK) {
        parser->state = PCHTML_HTML_PARSER_STATE_ERROR;
    }

    return parser->status;
}

pchtml_html_document_t *
pchtml_html_parse_chunk_begin(pchtml_html_parser_t *parser)
{
    pchtml_html_document_t *document;

    if (parser->state != PCHTML_HTML_PARSER_STATE_BEGIN) {
        pchtml_html_parser_clean(parser);
    }

    document = pchtml_html_document_interface_create(NULL);
    if (document == NULL) {
        parser->state = PCHTML_HTML_PARSER_STATE_ERROR;
        parser->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);

        return pchtml_html_document_destroy(document);
    }

    document->dom_document.scripting = parser->tree->scripting;

    parser->status = pchtml_html_parse_chunk_prepare(parser, document);
    if (parser->status != PCHTML_STATUS_OK) {
        return pchtml_html_document_destroy(document);
    }

    return document;
}

unsigned int
pchtml_html_parse_chunk_process(pchtml_html_parser_t *parser,
                const unsigned char *data, size_t sz)
{
    if (parser->state != PCHTML_HTML_PARSER_STATE_PROCESS) {
        pcinst_set_error (PURC_ERROR_WRONG_STAGE);
        return PCHTML_STATUS_ERROR_WRONG_STAGE;
    }

    parser->status = pchtml_html_tree_chunk(parser->tree, data, sz);
    if (parser->status != PCHTML_STATUS_OK) {
        parser->state = PCHTML_HTML_PARSER_STATE_ERROR;
    }

    return parser->status;
}

unsigned int
pchtml_html_parse_chunk_end(pchtml_html_parser_t *parser)
{
    if (parser->state != PCHTML_HTML_PARSER_STATE_PROCESS) {
        pcinst_set_error (PURC_ERROR_WRONG_STAGE);
        return PCHTML_STATUS_ERROR_WRONG_STAGE;
    }

    parser->status = pchtml_html_tree_end(parser->tree);

    pchtml_html_tokenizer_tree_set(parser->tkz, parser->original_tree);

    parser->state = PCHTML_HTML_PARSER_STATE_END;

    return parser->status;
}

struct serializer_data {
    size_t       nr;
    void        *ctxt;
    int (*writer)(const char *buf, size_t nr, int oom, void *ctxt);

    int oom; // -1: out of memory
};

static inline int
rwstream_writer(const char *data, size_t nr, int oom, void *ctxt)
{
    if (oom)
        return oom;

    purc_rwstream_t out = (purc_rwstream_t)ctxt;

    ssize_t sz = purc_rwstream_write(out, data, nr);
    // TODO: check ret value
    if (sz < 0 || (size_t)sz != nr)
        return -1;

    return 0;
}

static inline unsigned int
serializer_callback(const unsigned char  *data, size_t len, void *ctxt)
{
    struct serializer_data *ud = (struct serializer_data*)ctxt;

    char buf[1024];
    size_t nr = sizeof(buf);
    buf[0] = '\0';
    char *p = pcutils_snprintf(buf, &nr, "%.*s", (int)len, (const char *)data);
    PC_ASSERT((int64_t)nr>=0);
    ud->nr += nr;

    if (p == NULL) {
        if (ud->oom == 0) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        }
        ud->oom = -1;
        return PCHTML_STATUS_OK;
    }

    ud->oom = ud->writer(p, nr, ud->oom, ud->ctxt);

    if (p != buf)
        free(p);

    return PCHTML_STATUS_OK;
}

int
pchtml_doc_write_to_stream_ex(pchtml_html_document_t *doc,
    enum pchtml_html_serialize_opt opt, purc_rwstream_t out)
{
    struct serializer_data ud = {
        .nr             = 0,
        .ctxt           = out,
        .writer         = rwstream_writer,
        .oom            = 0,
    };
    unsigned int status;
    status = pchtml_html_serialize_pretty_tree_cb((pcdom_node_t *)doc,
            opt, 0, serializer_callback, &ud);
    PC_ASSERT(status==PCHTML_STATUS_OK);

    return ud.oom ? -1 : 0;
}

struct buffer_data {
    char         *orig_buf;
    size_t        orig_sz;

    char         *buf;
    size_t        sz;

    size_t        pos;

    size_t        nr;
};

static int
buffer_writer(const char *data, size_t nr, int oom, void *ctxt)
{
    struct buffer_data *bd = (struct buffer_data*)ctxt;

    bd->nr += nr;
    if (oom)
        return oom;

    if (bd->pos + nr + 1 >= bd->sz) {
        size_t align_sz = bd->pos + nr + 1;
        align_sz = (align_sz + 63) / 64 * 64; // bit operation?
        char *buf;
        if (bd->buf == bd->orig_buf) {
            buf = (char*)malloc(align_sz);
            if (!buf)
                return oom;
            strncpy(buf, bd->buf, bd->pos);
            bd->buf = buf;
            bd->sz  = align_sz;
        }
        else {
            buf = (char*)realloc(bd->buf, align_sz);
            if (!buf)
                return oom;
            bd->buf = buf;
            bd->sz  = align_sz;
        }
    }

    // XXX: warning on gcc 9.4.0 : output truncated before terminating nul
    // copying as many bytes from a string as its length
    // strncpy(bd->buf + bd->pos, data, nr);
    memcpy(bd->buf + bd->pos, data, nr);
    bd->pos += nr;
    bd->buf[bd->pos] = 0;

    return 0;
}

char*
pchtml_doc_snprintf_ex(pchtml_html_document_t *doc,
        enum pchtml_html_serialize_opt opt, char *buf, size_t *io_sz,
        const char *prefix)
{
    struct buffer_data bd = {
        .orig_buf       = buf,
        .orig_sz        = *io_sz,
        .buf            = buf,
        .sz             = *io_sz,
        .pos            = 0,
    };
    struct serializer_data ud = {
        .nr             = 0,
        .ctxt           = &bd,
        .writer         = buffer_writer,
        .oom            = 0,
    };
    ud.oom = buffer_writer(prefix, strlen(prefix), ud.oom, &bd);

    unsigned int status;
    status = pchtml_html_serialize_pretty_tree_cb((pcdom_node_t *)doc,
            opt, 0, serializer_callback, &ud);
    PC_ASSERT(status==PCHTML_STATUS_OK);
    PC_ASSERT(bd.pos < bd.sz);
    PC_ASSERT(bd.buf);
    bd.buf[bd.pos] = '\0';

    *io_sz = bd.nr;

    return ud.oom ? NULL : bd.buf;
}

int
pcdom_node_write_to_stream_ex(pcdom_node_t *node,
    enum pchtml_html_serialize_opt opt, purc_rwstream_t out)
{
    struct serializer_data ud = {
        .nr             = 0,
        .ctxt           = out,
        .writer         = rwstream_writer,
        .oom            = 0,
    };
    unsigned int status;
    status = pchtml_html_serialize_pretty_tree_cb(node,
            opt, 0, serializer_callback, &ud);
    if (status!=PCHTML_STATUS_OK) {
        return -1;
    }
    return 0;
}

char*
pcdom_node_snprintf_ex(pcdom_node_t *node,
        enum pchtml_html_serialize_opt opt, char *buf, size_t *io_sz,
        const char *prefix)
{
    struct buffer_data bd = {
        .orig_buf       = buf,
        .orig_sz        = *io_sz,
        .buf            = buf,
        .sz             = *io_sz,
        .pos            = 0,
    };
    struct serializer_data ud = {
        .nr             = 0,
        .ctxt           = &bd,
        .writer         = buffer_writer,
        .oom            = 0,
    };
    ud.oom = buffer_writer(prefix, strlen(prefix), ud.oom, &bd);

    unsigned int status;
    status = pchtml_html_serialize_pretty_tree_cb(node,
            opt, 0, serializer_callback, &ud);
    PC_ASSERT(status==PCHTML_STATUS_OK);
    PC_ASSERT(bd.pos < bd.sz);
    PC_ASSERT(bd.buf);
    bd.buf[bd.pos] = '\0';

    *io_sz = bd.nr;

    return ud.oom ? NULL : bd.buf;
}


struct pcdom_document*
pchtml_doc_get_document(pchtml_html_document_t *doc)
{
    return &doc->dom_document;
}

struct pcdom_element*
pchtml_doc_get_head(pchtml_html_document_t *doc)
{
    pchtml_html_head_element_t *head = doc->head;
    return (pcdom_element_t*)head;
}

struct pcdom_element*
pchtml_doc_get_body(pchtml_html_document_t *doc)
{
    pchtml_html_body_element_t *body = doc->body;
    return (pcdom_element_t*)body;
}

pchtml_html_parser_t*
pchtml_doc_get_parser(pchtml_html_document_t *doc)
{
    pcdom_document_t *dom_doc;
    dom_doc = pcdom_interface_document(doc);
    return dom_doc->parser;
}

