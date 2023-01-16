/**
 * @file document.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of document.
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
#include "private/dom.h"

pcdom_document_t *
pcdom_document_interface_create(pcdom_document_t *document)
{
    pcdom_document_t *doc;

    doc = pcutils_mraw_calloc(document->mraw, sizeof(pcdom_document_t));
    if (doc == NULL) {
        return NULL;
    }

    (void) pcdom_document_init(doc, document, pcdom_interface_create,
                    pcdom_interface_destroy, PCDOM_DOCUMENT_DTYPE_UNDEF, 0);

    return doc;
}

pcdom_document_t *
pcdom_document_interface_destroy(pcdom_document_t *document)
{
    return pcutils_mraw_free(
        pcdom_interface_node(document)->owner_document->mraw,
        document);
}

pcdom_document_t *
pcdom_document_create(pcdom_document_t *owner)
{
    if (owner != NULL) {
        return pcutils_mraw_calloc(owner->mraw, sizeof(pcdom_document_t));
    }

    return pcutils_calloc(1, sizeof(pcdom_document_t));
}

unsigned int
pcdom_document_init(pcdom_document_t *document, pcdom_document_t *owner,
                      pcdom_interface_create_f create_interface,
                      pcdom_interface_destroy_f destroy_interface,
                      pcdom_document_dtype_t type, unsigned int ns)
{
    unsigned int status;
    pcdom_node_t *node;

    if (document == NULL) {
        return PURC_ERROR_NULL_OBJECT;
    }

    document->type = type;
    document->create_interface = create_interface;
    document->destroy_interface = destroy_interface;

    node = pcdom_interface_node(document);

    node->type = PCDOM_NODE_TYPE_DOCUMENT;
    node->local_name = PCHTML_TAG__DOCUMENT;
    node->ns = ns;

    if (owner != NULL) {
        document->mraw = owner->mraw;
        document->text = owner->text;
        document->tags = owner->tags;
        document->ns = owner->ns;
        document->prefix = owner->prefix;
        document->attrs = owner->attrs;
        document->parser = owner->parser;
        document->user = owner->user;
        document->scripting = owner->scripting;
        document->compat_mode = owner->compat_mode;

        document->tags_inherited = true;
        document->ns_inherited = true;

        node->owner_document = owner;

        return PURC_ERROR_OK;
    }

    /* For nodes */
    document->mraw = pcutils_mraw_create();
    status = pcutils_mraw_init(document->mraw, (4096 * 8));

    if (status != PURC_ERROR_OK) {
        goto failed;
    }

    /* For text */
    document->text = pcutils_mraw_create();
    status = pcutils_mraw_init(document->text, (4096 * 12));

    if (status != PURC_ERROR_OK) {
        goto failed;
    }

    document->tags = pcutils_hash_create();
    status = pcutils_hash_init(document->tags, 128, sizeof(pchtml_tag_data_t));
    if (status != PURC_ERROR_OK) {
        goto failed;
    }

    document->ns = pcutils_hash_create();
    status = pcutils_hash_init(document->ns, 128, sizeof(pchtml_ns_data_t));
    if (status != PURC_ERROR_OK) {
        goto failed;
    }

    document->prefix = pcutils_hash_create();
    status = pcutils_hash_init(document->prefix, 128,
                              sizeof(pcdom_attr_data_t));
    if (status != PURC_ERROR_OK) {
        goto failed;
    }

    document->attrs = pcutils_hash_create();
    status = pcutils_hash_init(document->attrs, 128,
                              sizeof(pcdom_attr_data_t));
    if (status != PURC_ERROR_OK) {
        goto failed;
    }

    document->id_elem= pcutils_hash_create();
    status = pcutils_hash_init(document->id_elem, 128,
                              sizeof(pchtml_id_elem_data_t));
    if (status != PURC_ERROR_OK) {
        goto failed;
    }

    node->owner_document = document;

    return PURC_ERROR_OK;

failed:

    pcutils_mraw_destroy(document->mraw, true);
    pcutils_mraw_destroy(document->text, true);
    pcutils_hash_destroy(document->tags, true);
    pcutils_hash_destroy(document->ns, true);
    pcutils_hash_destroy(document->attrs, true);
    pcutils_hash_destroy(document->prefix, true);
    pcutils_hash_destroy(document->id_elem, true);

    return status;
}

unsigned int
pcdom_document_clean(pcdom_document_t *document)
{
    if (pcdom_interface_node(document)->owner_document == document) {
        pcutils_mraw_clean(document->mraw);
        pcutils_mraw_clean(document->text);
        pcutils_hash_clean(document->tags);
        pcutils_hash_clean(document->ns);
        pcutils_hash_clean(document->attrs);
        pcutils_hash_clean(document->prefix);
        pcutils_hash_clean(document->id_elem);
    }

    document->node.first_child = NULL;
    document->node.last_child = NULL;
    document->element = NULL;
    document->doctype = NULL;

    return PURC_ERROR_OK;
}

pcdom_document_t *
pcdom_document_destroy(pcdom_document_t *document)
{
    if (document == NULL) {
        return NULL;
    }

    if (pcdom_interface_node(document)->owner_document != document) {
        pcdom_document_t *owner;

        owner = pcdom_interface_node(document)->owner_document;

        return pcutils_mraw_free(owner->mraw, document);
    }

    pcutils_mraw_destroy(document->text, true);
    pcutils_mraw_destroy(document->mraw, true);
    pcutils_hash_destroy(document->tags, true);
    pcutils_hash_destroy(document->ns, true);
    pcutils_hash_destroy(document->attrs, true);
    pcutils_hash_destroy(document->prefix, true);
    pcutils_hash_destroy(document->id_elem, true);

    return pcutils_free(document);
}

void
pcdom_document_attach_doctype(pcdom_document_t *document,
                                pcdom_document_type_t *doctype)
{
    document->doctype = doctype;
}

void
pcdom_document_attach_element(pcdom_document_t *document,
                                pcdom_element_t *element)
{
    document->element = element;
}

pcdom_element_t *
pcdom_document_create_element(pcdom_document_t *document,
                                const unsigned char *local_name, size_t lname_len,
                                void *reserved_for_opt, bool self_close)
{
    /* TODO: If localName does not match the Name production... */
    UNUSED_PARAM(reserved_for_opt);

    const unsigned char *ns_link;
    size_t ns_len;

    if (document->type == PCDOM_DOCUMENT_DTYPE_HTML) {
        ns_link = (const unsigned char *) "http://www.w3.org/1999/xhtml";

        /* FIXME: he will get len at the compilation stage?!? */
        ns_len = strlen((const char *) ns_link);
    }
    else {
        ns_link = NULL;
        ns_len = 0;
    }

    return pcdom_element_create(document, local_name, lname_len,
            ns_link, ns_len, NULL, 0, NULL, 0, true, self_close);
}

pcdom_element_t *
pcdom_document_destroy_element(pcdom_element_t *element)
{
    return pcdom_element_destroy(element);
}

pcdom_document_fragment_t *
pcdom_document_create_document_fragment(pcdom_document_t *document)
{
    return pcdom_document_fragment_interface_create(document);
}

pcdom_text_t *
pcdom_document_create_text_node(pcdom_document_t *document,
                                  const unsigned char *data, size_t len)
{
    pcdom_text_t *text;

    text = pcdom_document_create_interface(document,
                                             PCHTML_TAG__TEXT, PCHTML_NS_HTML);
    if (text == NULL) {
        return NULL;
    }

    pcutils_str_init(&text->char_data.data, document->text, len);
    if (text->char_data.data.data == NULL) {
        return pcdom_document_destroy_interface(text);
    }

    pcutils_str_append(&text->char_data.data, document->text, data, len);

    return text;
}

pcdom_cdata_section_t *
pcdom_document_create_cdata_section(pcdom_document_t *document,
                                      const unsigned char *data, size_t len)
{
    if (document->type != PCDOM_DOCUMENT_DTYPE_HTML) {
        return NULL;
    }

    const unsigned char *end = data + len;
    const unsigned char *ch = memchr(data, ']', sizeof(unsigned char) * len);

    while (ch != NULL) {
        if ((end - ch) < 3) {
            break;
        }

        if(memcmp(ch, "]]>", 3) == 0) {
            return NULL;
        }

        ch++;
        ch = memchr(ch, ']', sizeof(unsigned char) * (end - ch));
    }

    pcdom_cdata_section_t *cdata;

    cdata = pcdom_cdata_section_interface_create(document);
    if (cdata == NULL) {
        return NULL;
    }

    pcutils_str_init(&cdata->text.char_data.data, document->text, len);
    if (cdata->text.char_data.data.data == NULL) {
        return pcdom_cdata_section_interface_destroy(cdata);
    }

    pcutils_str_append(&cdata->text.char_data.data, document->text, data, len);

    return cdata;
}

pcdom_processing_instruction_t *
pcdom_document_create_processing_instruction(pcdom_document_t *document,
                                               const unsigned char *target, size_t target_len,
                                               const unsigned char *data, size_t data_len)
{
    /*
     * TODO: If target does not match the Name production,
     * then throw an "InvalidCharacterError" DOMException.
     */

    const unsigned char *end = data + data_len;
    const unsigned char *ch = memchr(data, '?', sizeof(unsigned char) * data_len);

    while (ch != NULL) {
        if ((end - ch) < 2) {
            break;
        }

        if(memcmp(ch, "?>", 2) == 0) {
            return NULL;
        }

        ch++;
        ch = memchr(ch, '?', sizeof(unsigned char) * (end - ch));
    }

    pcdom_processing_instruction_t *pi;

    pi = pcdom_processing_instruction_interface_create(document);
    if (pi == NULL) {
        return NULL;
    }

    pcutils_str_init(&pi->char_data.data, document->text, data_len);
    if (pi->char_data.data.data == NULL) {
        return pcdom_processing_instruction_interface_destroy(pi);
    }

    pcutils_str_init(&pi->target, document->text, target_len);
    if (pi->target.data == NULL) {
        pcutils_str_destroy(&pi->char_data.data, document->text, false);

        return pcdom_processing_instruction_interface_destroy(pi);
    }

    pcutils_str_append(&pi->char_data.data, document->text, data, data_len);
    pcutils_str_append(&pi->target, document->text, target, target_len);

    return pi;
}


pcdom_comment_t *
pcdom_document_create_comment(pcdom_document_t *document,
                                const unsigned char *data, size_t len)
{
    pcdom_comment_t *comment;

    comment = pcdom_document_create_interface(document, PCHTML_TAG__EM_COMMENT,
                                                PCHTML_NS_HTML);
    if (comment == NULL) {
        return NULL;
    }

    pcutils_str_init(&comment->char_data.data, document->text, len);
    if (comment->char_data.data.data == NULL) {
        return pcdom_document_destroy_interface(comment);
    }

    pcutils_str_append(&comment->char_data.data, document->text, data, len);

    return comment;
}

const unsigned char *
pcdom_document_type_name(pcdom_document_type_t *doc_type, size_t *len)
{
    const pcdom_attr_data_t *data;

    static const unsigned char pchtml_empty[] = "";

    data = pcdom_attr_data_by_id(doc_type->node.owner_document->attrs,
                                   doc_type->name);
    if (data == NULL || doc_type->name == PCDOM_ATTR__UNDEF) {
        if (len != NULL) {
            *len = 0;
        }

        return pchtml_empty;
    }

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pcutils_hash_entry_str(&data->entry);
}

