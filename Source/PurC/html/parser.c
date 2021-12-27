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

    pchtml_html_parse_chunk_process(parser, html);
    if (parser->status != PCHTML_STATUS_OK) {
        goto failed;
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

pcedom_node_t *
pchtml_html_parse_fragment(pchtml_html_parser_t *parser, pchtml_html_element_t *element,
                        const purc_rwstream_t html)
{
    return pchtml_html_parse_fragment_by_tag_id(parser,
                                             parser->tree->document,
                                             element->element.node.local_name,
                                             element->element.node.ns,
                                             html);
}

pcedom_node_t *
pchtml_html_parse_fragment_by_tag_id(pchtml_html_parser_t *parser,
                                  pchtml_html_document_t *document,
                                  pchtml_tag_id_t tag_id, pchtml_ns_id_t ns,
                                  const purc_rwstream_t html)
{
    pchtml_html_parse_fragment_chunk_begin(parser, document, tag_id, ns);
    if (parser->status != PCHTML_STATUS_OK) {
        return NULL;
    }

    pchtml_html_parse_fragment_chunk_process(parser, html);
    if (parser->status != PCHTML_STATUS_OK) {
        return NULL;
    }

    return pchtml_html_parse_fragment_chunk_end(parser);
}

unsigned int
pchtml_html_parse_fragment_chunk_begin(pchtml_html_parser_t *parser,
                                    pchtml_html_document_t *document,
                                    pchtml_tag_id_t tag_id, pchtml_ns_id_t ns)
{
    pcedom_document_t *doc;
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

    doc = pcedom_interface_document(new_doc);

    if (document == NULL) {
        doc->scripting = parser->tree->scripting;
        doc->compat_mode = PCEDOM_DOCUMENT_CMODE_NO_QUIRKS;
    }

    pchtml_html_tokenizer_set_state_by_tag(parser->tkz, doc->scripting, tag_id, ns);

    parser->root = pchtml_html_interface_create(new_doc, PCHTML_TAG_HTML, PCHTML_NS_HTML);
    if (parser->root == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        parser->status = PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;

        goto done;
    }

    pcedom_node_insert_child(pcedom_interface_node(new_doc), parser->root);
    pcedom_document_attach_element(doc, pcedom_interface_element(parser->root));

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
                                      const purc_rwstream_t html)
{
    if (parser->state != PCHTML_HTML_PARSER_STATE_FRAGMENT_PROCESS) {
        pcinst_set_error (PURC_ERROR_WRONG_STAGE);
        return PCHTML_STATUS_ERROR_WRONG_STAGE;
    }

    parser->status = pchtml_html_tree_chunk(parser->tree, html);
    if (parser->status != PCHTML_STATUS_OK) {
        pchtml_html_html_element_interface_destroy(pchtml_html_interface_html(parser->root));

        parser->state = PCHTML_HTML_PARSER_STATE_ERROR;
        parser->root = NULL;

        pchtml_html_parse_fragment_chunk_destroy(parser);
    }

    return parser->status;
}

pcedom_node_t *
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
    pcedom_document_t *doc;

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
            doc = pcedom_interface_node(parser->tree->document)->owner_document;
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
                             const purc_rwstream_t html)
{
    if (parser->state != PCHTML_HTML_PARSER_STATE_PROCESS) {
        pcinst_set_error (PURC_ERROR_WRONG_STAGE);
        return PCHTML_STATUS_ERROR_WRONG_STAGE;
    }

    parser->status = pchtml_html_tree_chunk(parser->tree, html);
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
pchtml_doc_write_to_stream(pchtml_html_document_t *doc, purc_rwstream_t out)
{
    if (!doc || !out) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    unsigned int status;
    status = pchtml_html_serialize_pretty_tree_cb((pcedom_node_t *)doc,
                                          0x00, 0, serializer_callback, out);
    if (status!=PCHTML_STATUS_OK) {
        return -1;
    }
    return 0;
}

struct pcedom_document*
pchtml_doc_get_document(pchtml_html_document_t *doc)
{
    return &doc->dom_document;
}

static pcedom_node_t * get_node (pcedom_node_t *node, unsigned int tag, int *index)
{
    if (node && node->local_name == tag) {
        (*index)--;
        if ((*index) < 0)
            return node;
    }

    node = node->first_child;
    pcedom_node_t *return_node = NULL;

    while (node != NULL) {
        return_node = get_node (node, tag, index);
        if (return_node)
            return return_node;

        node = node->next;
    }

    return return_node;
}

pcedom_node_t * pchtml_edom_document_parse_fragment (pchtml_html_document_t *document,
                pcedom_node_t *node, purc_rwstream_t html)
{
    int index = 0;
    unsigned int status = PCHTML_STATUS_OK;
    if (node == NULL) {
        node = get_node (&(document->dom_document.node), PCHTML_TAG_BODY, &index);
    }
    pcedom_element_t *root_element = pcedom_interface_element (node);

    status = pchtml_html_document_parse_fragment_chunk_begin (document, root_element);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to start parse HTML chunk");
        return NULL;
    }

    status = pchtml_html_document_parse_fragment_chunk (document, html);
    if (status != PCHTML_STATUS_OK) {
        printf ("Failed to parse HTML chunk");
        return NULL;
    }

    pcedom_node_t *fragment_root = pchtml_html_document_parse_fragment_chunk_end (document);
    if (fragment_root == NULL) {
        printf ("Failed to parse HTML");
        status = PCHTML_STATUS_ERROR;
        return NULL;
    }

    return fragment_root;
}

bool pchtml_edom_insert_node(pcedom_node_t *node, pcedom_node_t *fragment_root,
        purc_atom_t op)
{
    bool ret = true;
    pcedom_node_t *child = NULL;

    if (op == pcvariant_atom_append) {
        while (node->first_child != NULL) {
            pcedom_node_destroy_deep(node->first_child);
        }

        while (fragment_root->first_child != NULL) {
            child = fragment_root->first_child;

            pcedom_node_remove(child);
            pcedom_node_insert_child(node, child);
        }
        pcedom_node_destroy(fragment_root);
    }
    else if (op == pcvariant_atom_prepend) {
        ret = true;
    }
    else if (op == pcvariant_atom_insertBefore) {
        while (fragment_root->first_child != NULL) {
            child = fragment_root->first_child;

            pcedom_node_remove(child);
            pcedom_node_insert_before (node, child);
        }
        pcedom_node_destroy(fragment_root);
    }
    else if (op == pcvariant_atom_insertAfter) {
        while (fragment_root->first_child != NULL) {
            child = fragment_root->first_child;

            pcedom_node_remove(child);
            pcedom_node_insert_after (node, child);
            node = node->next;
        }
        pcedom_node_destroy(fragment_root);
    }
    return ret;
}
