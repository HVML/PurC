/**
 * @file element.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of html element.
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
#include "private/mem.h"
#include "private/utils.h"

typedef struct pcdom_element_cb_ctx pcdom_element_cb_ctx_t;

typedef bool
(*pcdom_element_attr_cmp_f)(pcdom_element_cb_ctx_t *ctx,
                              pcdom_attr_t *attr);


struct pcdom_element_cb_ctx {
    pcdom_collection_t       *col;
    unsigned int               status;
    pcdom_element_attr_cmp_f cmp_func;

    pcdom_attr_id_t          name_id;
    pchtml_ns_prefix_id_t         prefix_id;

    const unsigned char           *value;
    size_t                     value_length;
};


static pchtml_action_t
pcdom_elements_by_tag_name_cb(pcdom_node_t *node, void *ctx);

static pchtml_action_t
pcdom_elements_by_tag_name_cb_all(pcdom_node_t *node, void *ctx);

static pchtml_action_t
pcdom_elements_by_class_name_cb(pcdom_node_t *node, void *ctx);

static pchtml_action_t
pcdom_elements_by_attr_cb(pcdom_node_t *node, void *ctx);

static bool
pcdom_elements_by_attr_cmp_full(pcdom_element_cb_ctx_t *ctx,
                                  pcdom_attr_t *attr);

static bool
pcdom_elements_by_attr_cmp_full_case(pcdom_element_cb_ctx_t *ctx,
                                       pcdom_attr_t *attr);

static bool
pcdom_elements_by_attr_cmp_begin(pcdom_element_cb_ctx_t *ctx,
                                   pcdom_attr_t *attr);

static bool
pcdom_elements_by_attr_cmp_begin_case(pcdom_element_cb_ctx_t *ctx,
                                        pcdom_attr_t *attr);

static bool
pcdom_elements_by_attr_cmp_end(pcdom_element_cb_ctx_t *ctx,
                                 pcdom_attr_t *attr);

static bool
pcdom_elements_by_attr_cmp_end_case(pcdom_element_cb_ctx_t *ctx,
                                      pcdom_attr_t *attr);

static bool
pcdom_elements_by_attr_cmp_contain(pcdom_element_cb_ctx_t *ctx,
                                     pcdom_attr_t *attr);

static bool
pcdom_elements_by_attr_cmp_contain_case(pcdom_element_cb_ctx_t *ctx,
                                          pcdom_attr_t *attr);

static const unsigned char *
pcdom_element_upper_update(pcdom_element_t *element, size_t *len);

const pchtml_tag_data_t *
pchtml_tag_append(pcutils_hash_t *hash, pchtml_tag_id_t tag_id,
               const unsigned char *name, size_t length);

const pchtml_tag_data_t *
pchtml_tag_append_lower(pcutils_hash_t *hash,
                     const unsigned char *name, size_t length);

const pchtml_ns_data_t *
pchtml_ns_append(pcutils_hash_t *hash, const unsigned char *link, size_t length);


pcdom_element_t *
pcdom_element_interface_create(pcdom_document_t *document)
{
    pcdom_element_t *element;

    element = pcutils_mraw_calloc(document->mraw,
                                 sizeof(pcdom_element_t));
    if (element == NULL) {
        return NULL;
    }

    pcdom_node_t *node = pcdom_interface_node(element);

    node->owner_document = pcdom_document_owner(document);
    node->type = PCDOM_NODE_TYPE_ELEMENT;

    return element;
}

pcdom_element_t *
pcdom_element_interface_destroy(pcdom_element_t *element)
{
    pcdom_attr_t *attr_next;
    pcdom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        attr_next = attr->next;

        pcdom_attr_interface_destroy(attr);

        attr = attr_next;
    }

    return pcutils_mraw_free(
        pcdom_interface_node(element)->owner_document->mraw,
        element);
}

unsigned int
pcdom_element_qualified_name_set(pcdom_element_t *element,
                                   const unsigned char *prefix, size_t prefix_len,
                                   const unsigned char *lname, size_t lname_len)
{
    unsigned char *key = (unsigned char *) lname;
    const pchtml_tag_data_t *tag_data;

    if (prefix != NULL && prefix_len != 0) {
        key = pcutils_malloc(prefix_len + lname_len + 2);
        if (key == NULL) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }

        memcpy(key, prefix, prefix_len);
        memcpy(&key[prefix_len + 1], lname, lname_len);

        lname_len = prefix_len + lname_len + 1;

        key[prefix_len] = ':';
        key[lname_len] = '\0';
    }

    tag_data = pchtml_tag_append(element->node.owner_document->tags,
                              element->node.local_name, key, lname_len);
    if (tag_data == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    element->qualified_name = (pchtml_tag_id_t) tag_data;

    return PURC_ERROR_OK;
}

pcdom_element_t *
pcdom_element_create(pcdom_document_t *document,
                       const unsigned char *local_name, size_t lname_len,
                       const unsigned char *ns_link, size_t ns_len,
                       const unsigned char *prefix, size_t prefix_len,
                       const unsigned char *is, size_t is_len,
                       bool sync_custom, bool self_close)
{
    UNUSED_PARAM(sync_custom);

    unsigned int status;
    const pchtml_ns_data_t *ns_data;
    const pchtml_tag_data_t *tag_data;
    const pchtml_ns_prefix_data_t *ns_prefix;
    pcdom_element_t *element;

    /* TODO: Must implement custom elements */

    /* 7. Otherwise */

    ns_data = NULL;
    tag_data = NULL;
    ns_prefix = NULL;

    tag_data = pchtml_tag_append_lower(document->tags, local_name, lname_len);
    if (tag_data == NULL) {
        return NULL;
    }

    if (ns_link != NULL) {
        ns_data = pchtml_ns_append(document->ns, ns_link, ns_len);
    }
    else {
        ns_data = pchtml_ns_data_by_id(document->ns, PCHTML_NS__UNDEF);
    }

    if (ns_data == NULL) {
        return NULL;
    }

    element = pcdom_document_create_interface(document, tag_data->tag_id,
                                                ns_data->ns_id);
    if (element == NULL) {
        return NULL;
    }

    if (prefix != NULL) {
        ns_prefix = pchtml_ns_prefix_append(document->prefix, prefix, prefix_len);
        if (ns_prefix == NULL) {
            return pcdom_document_destroy_interface(element);
        }

        element->node.prefix = ns_prefix->prefix_id;

        status = pcdom_element_qualified_name_set(element, prefix, prefix_len,
                                                    local_name, lname_len);
        if (status != PURC_ERROR_OK) {
            return pcdom_document_destroy_interface(element);
        }
    }

    if (is_len != 0) {
        status = pcdom_element_is_set(element, is, is_len);
        if (status != PURC_ERROR_OK) {
            return pcdom_document_destroy_interface(element);
        }
    }

    element->node.local_name = tag_data->tag_id;
    element->node.ns = ns_data->ns_id;

    if (ns_data->ns_id == PCHTML_NS_HTML && is_len != 0) {
        element->custom_state = PCDOM_ELEMENT_CUSTOM_STATE_UNDEFINED;
    }
    else {
        element->custom_state = PCDOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED;
    }
    element->self_close = self_close;

    return element;
}

pcdom_element_t *
pcdom_element_destroy(pcdom_element_t *element)
{
    return pcdom_document_destroy_interface(element);
}

bool
pcdom_element_has_attributes(pcdom_element_t *element)
{
    return element->first_attr != NULL;
}

pcdom_attr_t *
pcdom_element_set_attribute(pcdom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len,
                              const unsigned char *value, size_t value_len)
{
    unsigned int status;
    pcdom_attr_t *attr;
    int exists = 0;

    attr = pcdom_element_attr_is_exist(element, qualified_name, qn_len);

    if (attr != NULL) {
        exists = 1;
        goto update;
    }

    attr = pcdom_attr_interface_create(element->node.owner_document);
    if (attr == NULL) {
        return NULL;
    }

    if (element->node.ns == PCHTML_NS_HTML
        && element->node.owner_document->type == PCDOM_DOCUMENT_DTYPE_HTML)
    {
        status = pcdom_attr_set_name(attr, qualified_name, qn_len, true);
    }
    else {
        status = pcdom_attr_set_name(attr, qualified_name, qn_len, false);
    }

    if (status != PURC_ERROR_OK) {
        return pcdom_attr_interface_destroy(attr);
    }

update:

    if (exists &&
            (attr->node.local_name == PCDOM_ATTR_ID)) {
        pcutils_hash_remove(element->node.owner_document->id_elem,
                pcutils_hash_search_raw, attr->value->data, attr->value->length);
    }

    status = pcdom_attr_set_value(attr, value, value_len);
    if (status != PURC_ERROR_OK) {
        return pcdom_attr_interface_destroy(attr);
    }

    if (!exists)
        pcdom_element_attr_append(element, attr);

    if (attr->node.local_name == PCDOM_ATTR_ID) {
        pchtml_id_elem_data_t *data = pcutils_hash_insert(
                element->node.owner_document->id_elem,
                pcutils_hash_insert_raw, attr->value->data, attr->value->length);
        if (data) {
            data->elem = element;
        }
    }
    return attr;
}

const unsigned char *
pcdom_element_get_attribute(pcdom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len,
                              size_t *value_len)
{
    pcdom_attr_t *attr;

    attr = pcdom_element_attr_by_name(element, qualified_name, qn_len);
    if (attr == NULL) {
        if (value_len != NULL) {
            *value_len = 0;
        }

        return NULL;
    }

    return pcdom_attr_value(attr, value_len);
}

unsigned int
pcdom_element_remove_attribute(pcdom_element_t *element,
                                 const unsigned char *qualified_name, size_t qn_len)
{
    unsigned int status;
    pcdom_attr_t *attr;

    attr = pcdom_element_attr_by_name(element, qualified_name, qn_len);
    if (attr == NULL) {
        return PURC_ERROR_OK;
    }

    status = pcdom_element_attr_remove(element, attr);
    if (status != PURC_ERROR_OK) {
        return status;
    }

    pcdom_attr_interface_destroy(attr);

    return PURC_ERROR_OK;
}

bool
pcdom_element_has_attribute(pcdom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len)
{
    return pcdom_element_attr_by_name(element, qualified_name, qn_len) != NULL;
}

unsigned int
pcdom_element_attr_append(pcdom_element_t *element, pcdom_attr_t *attr)
{
    if (attr->node.local_name == PCDOM_ATTR_ID) {
        if (element->attr_id != NULL) {
            pcdom_element_attr_remove(element, element->attr_id);
            pcdom_attr_interface_destroy(element->attr_id);
        }

        element->attr_id = attr;
    }
    else if (attr->node.local_name == PCDOM_ATTR_CLASS) {
        if (element->attr_class != NULL) {
            pcdom_element_attr_remove(element, element->attr_class);
            pcdom_attr_interface_destroy(element->attr_class);
        }

        element->attr_class = attr;
    }

    if (element->first_attr == NULL) {
        element->first_attr = attr;
        element->last_attr = attr;

        return PURC_ERROR_OK;
    }

    attr->prev = element->last_attr;
    element->last_attr->next = attr;

    element->last_attr = attr;

    return PURC_ERROR_OK;
}

unsigned int
pcdom_element_attr_remove(pcdom_element_t *element, pcdom_attr_t *attr)
{
    if (element->attr_id == attr) {
        element->attr_id = NULL;
    }
    else if (element->attr_class == attr) {
        element->attr_class = NULL;
    }

    if (attr->prev != NULL) {
        attr->prev->next = attr->next;
    }
    else {
        element->first_attr = attr->next;
    }

    if (attr->next != NULL) {
        attr->next->prev = attr->prev;
    }
    else {
        element->last_attr = attr->prev;
    }

    attr->next = NULL;
    attr->prev = NULL;

    return PURC_ERROR_OK;
}

pcdom_attr_t *
pcdom_element_attr_by_name(pcdom_element_t *element,
                             const unsigned char *qualified_name, size_t length)
{
    const pcdom_attr_data_t *data;
    pcutils_hash_t *attrs = element->node.owner_document->attrs;
    pcdom_attr_t *attr = element->first_attr;

    if (element->node.ns == PCHTML_NS_HTML
        && element->node.owner_document->type == PCDOM_DOCUMENT_DTYPE_HTML)
    {
        data = pcdom_attr_data_by_local_name(attrs, qualified_name, length);
    }
    else {
        data = pcdom_attr_data_by_qualified_name(attrs, qualified_name,
                                                   length);
    }

    if (data == NULL) {
        return NULL;
    }

    while (attr != NULL) {
        if (attr->node.local_name == data->attr_id
            || attr->qualified_name == data->attr_id)
        {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

pcdom_attr_t *
pcdom_element_attr_by_local_name_data(pcdom_element_t *element,
                                        const pcdom_attr_data_t *data)
{
    pcdom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        if (attr->node.local_name == data->attr_id) {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

pcdom_attr_t *
pcdom_element_attr_by_id(pcdom_element_t *element,
                           pcdom_attr_id_t attr_id)
{
    pcdom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        if (attr->node.local_name == attr_id) {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

bool
pcdom_element_compare(pcdom_element_t *first, pcdom_element_t *second)
{
    pcdom_attr_t *f_attr = first->first_attr;
    pcdom_attr_t *s_attr = second->first_attr;

    if (first->node.local_name != second->node.local_name
        || first->node.ns != second->node.ns
        || first->qualified_name != second->qualified_name)
    {
        return false;
    }

    /* Compare attr counts */
    while (f_attr != NULL && s_attr != NULL) {
        f_attr = f_attr->next;
        s_attr = s_attr->next;
    }

    if (f_attr != NULL || s_attr != NULL) {
        return false;
    }

    /* Compare attr */
    f_attr = first->first_attr;

    while (f_attr != NULL) {
        s_attr = second->first_attr;

        while (s_attr != NULL) {
            if (pcdom_attr_compare(f_attr, s_attr)) {
                break;
            }

            s_attr = s_attr->next;
        }

        if (s_attr == NULL) {
            return false;
        }

        f_attr = f_attr->next;
    }

    return true;
}

pcdom_attr_t *
pcdom_element_attr_is_exist(pcdom_element_t *element,
                              const unsigned char *qualified_name, size_t length)
{
    const pcdom_attr_data_t *data;
    pcdom_attr_t *attr = element->first_attr;

    data = pcdom_attr_data_by_local_name(element->node.owner_document->attrs,
                                           qualified_name, length);
    if (data == NULL) {
        return NULL;
    }

    while (attr != NULL) {
        if (attr->node.local_name == data->attr_id
            || attr->qualified_name == data->attr_id)
        {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

unsigned int
pcdom_element_is_set(pcdom_element_t *element,
                       const unsigned char *is, size_t is_len)
{
    if (element->is_value == NULL) {
        element->is_value = pcutils_mraw_calloc(element->node.owner_document->mraw,
                                               sizeof(pcutils_str_t));
        if (element->is_value == NULL) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }
    }

    if (element->is_value->data == NULL) {
        pcutils_str_init(element->is_value,
                        element->node.owner_document->text, is_len);

        if (element->is_value->data == NULL) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }
    }

    if (element->is_value->length != 0) {
        element->is_value->length = 0;
    }

    unsigned char *data = pcutils_str_append(element->is_value,
                                         element->node.owner_document->text,
                                         is, is_len);
    if (data == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    return PURC_ERROR_OK;
}

static inline unsigned int
pcdom_element_prepare_by_attr(pcdom_document_t *document,
                                pcdom_element_cb_ctx_t *cb_ctx,
                                const unsigned char *qname, size_t qlen)
{
    size_t length;
    const unsigned char *prefix_end;
    const pcdom_attr_data_t *attr_data;
    const pchtml_ns_prefix_data_t *prefix_data;

    cb_ctx->prefix_id = PCHTML_NS__UNDEF;

    prefix_end = memchr(qname, ':', qlen);

    if (prefix_end != NULL) {
        length = prefix_end - qname;

        if (length == 0) {
            return PURC_ERROR_INVALID_VALUE;
        }

        prefix_data = pchtml_ns_prefix_data_by_name(document->prefix, qname, qlen);
        if (prefix_data == NULL) {
            return PCHTML_STATUS_STOP;
        }

        cb_ctx->prefix_id = prefix_data->prefix_id;

        length += 1;

        if (length >= qlen) {
            return PURC_ERROR_INVALID_VALUE;
        }

        qname += length;
        qlen -= length;
    }

    attr_data = pcdom_attr_data_by_local_name(document->attrs, qname, qlen);
    if (attr_data == NULL) {
        return PCHTML_STATUS_STOP;
    }

    cb_ctx->name_id = attr_data->attr_id;

    return PURC_ERROR_OK;
}

static inline unsigned int
pcdom_element_prepare_by(pcdom_document_t *document,
                           pcdom_element_cb_ctx_t *cb_ctx,
                           const unsigned char *qname, size_t qlen)
{
    size_t length;
    const unsigned char *prefix_end;
    const pchtml_tag_data_t *tag_data;
    const pchtml_ns_prefix_data_t *prefix_data;

    cb_ctx->prefix_id = PCHTML_NS__UNDEF;

    prefix_end = memchr(qname, ':', qlen);

    if (prefix_end != NULL) {
        length = prefix_end - qname;

        if (length == 0) {
            return PURC_ERROR_INVALID_VALUE;
        }

        prefix_data = pchtml_ns_prefix_data_by_name(document->prefix, qname, qlen);
        if (prefix_data == NULL) {
            return PCHTML_STATUS_STOP;
        }

        cb_ctx->prefix_id = prefix_data->prefix_id;

        length += 1;

        if (length >= qlen) {
            return PURC_ERROR_INVALID_VALUE;
        }

        qname += length;
        qlen -= length;
    }

    tag_data = pchtml_tag_data_by_name(document->tags, qname, qlen);
    if (tag_data == NULL) {
        return PCHTML_STATUS_STOP;
    }

    cb_ctx->name_id = tag_data->tag_id;

    return PURC_ERROR_OK;
}

unsigned int
pcdom_elements_by_tag_name(pcdom_element_t *root,
                             pcdom_collection_t *collection,
                             const unsigned char *qualified_name, size_t len)
{
    unsigned int status;
    pcdom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;

    /* "*" (U+002A) */
    if (len == 1 && *qualified_name == 0x2A) {
        pcdom_node_simple_walk(pcdom_interface_node(root),
                                 pcdom_elements_by_tag_name_cb_all, &cb_ctx);

        return cb_ctx.status;
    }

    status = pcdom_element_prepare_by(root->node.owner_document,
                                        &cb_ctx, qualified_name, len);
    if (status != PURC_ERROR_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PURC_ERROR_OK;
        }

        return status;
    }

    pcdom_node_simple_walk(pcdom_interface_node(root),
                             pcdom_elements_by_tag_name_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pcdom_elements_by_tag_name_cb_all(pcdom_node_t *node, void *ctx)
{
    if (node->type != PCDOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pcdom_element_cb_ctx_t *cb_ctx = ctx;

    cb_ctx->status = pcdom_collection_append(cb_ctx->col, node);
    if (cb_ctx->status != PURC_ERROR_OK) {
        return PCHTML_ACTION_STOP;
    }

    return PCHTML_ACTION_OK;
}

static pchtml_action_t
pcdom_elements_by_tag_name_cb(pcdom_node_t *node, void *ctx)
{
    if (node->type != PCDOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pcdom_element_cb_ctx_t *cb_ctx = ctx;

    if (node->local_name == cb_ctx->name_id
        && node->prefix == cb_ctx->prefix_id)
    {
        cb_ctx->status = pcdom_collection_append(cb_ctx->col, node);
        if (cb_ctx->status != PURC_ERROR_OK) {
            return PCHTML_ACTION_STOP;
        }
    }

    return PCHTML_ACTION_OK;
}

unsigned int
pcdom_elements_by_class_name(pcdom_element_t *root,
                               pcdom_collection_t *collection,
                               const unsigned char *class_name, size_t len)
{
    if (class_name == NULL || len == 0) {
        return PURC_ERROR_OK;
    }

    pcdom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = class_name;
    cb_ctx.value_length = len;

    pcdom_node_simple_walk(pcdom_interface_node(root),
                             pcdom_elements_by_class_name_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pcdom_elements_by_class_name_cb(pcdom_node_t *node, void *ctx)
{
    if (node->type != PCDOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pcdom_element_cb_ctx_t *cb_ctx = ctx;
    pcdom_element_t *el = pcdom_interface_element(node);

    if (el->attr_class == NULL
        || el->attr_class->value->length < cb_ctx->value_length)
    {
        return PCHTML_ACTION_OK;
    }

    const unsigned char *data = el->attr_class->value->data;
    size_t length = el->attr_class->value->length;

    bool is_it = false;
    const unsigned char *pos = data;
    const unsigned char *end = data + length;

    pcdom_document_t *doc = el->node.owner_document;

    for (; data < end; data++) {
        if (pcutils_html_whitespace(*data, ==, ||)) {

            if (pos != data && (data - pos) == (int)cb_ctx->value_length) {
                if (doc->compat_mode == PCDOM_DOCUMENT_CMODE_QUIRKS) {
                    is_it = pcutils_str_data_ncasecmp(pos, cb_ctx->value,
                                                     cb_ctx->value_length);
                }
                else {
                    is_it = pcutils_str_data_ncmp(pos, cb_ctx->value,
                                                 cb_ctx->value_length);
                }

                if (is_it) {
                    cb_ctx->status = pcdom_collection_append(cb_ctx->col,
                                                               node);
                    if (cb_ctx->status != PURC_ERROR_OK) {
                        return PCHTML_ACTION_STOP;
                    }

                    return PCHTML_ACTION_OK;
                }
            }

            if ((end - data) < (int)cb_ctx->value_length) {
                return PCHTML_ACTION_OK;
            }

            pos = data + 1;
        }
    }

    if ((end - pos) == (int)cb_ctx->value_length) {
        if (doc->compat_mode == PCDOM_DOCUMENT_CMODE_QUIRKS) {
            is_it = pcutils_str_data_ncasecmp(pos, cb_ctx->value,
                                             cb_ctx->value_length);
        }
        else {
            is_it = pcutils_str_data_ncmp(pos, cb_ctx->value,
                                         cb_ctx->value_length);
        }

        if (is_it) {
            cb_ctx->status = pcdom_collection_append(cb_ctx->col, node);
            if (cb_ctx->status != PURC_ERROR_OK) {
                return PCHTML_ACTION_STOP;
            }
        }
    }

    return PCHTML_ACTION_OK;
}

unsigned int
pcdom_elements_by_attr(pcdom_element_t *root,
                         pcdom_collection_t *collection,
                         const unsigned char *qualified_name, size_t qname_len,
                         const unsigned char *value, size_t value_len,
                         bool case_insensitive)
{
    unsigned int status;
    pcdom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pcdom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PURC_ERROR_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PURC_ERROR_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pcdom_elements_by_attr_cmp_full_case;
    }
    else {
        cb_ctx.cmp_func = pcdom_elements_by_attr_cmp_full;
    }

    pcdom_node_simple_walk(pcdom_interface_node(root),
                             pcdom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pcdom_elements_by_attr_begin(pcdom_element_t *root,
                               pcdom_collection_t *collection,
                               const unsigned char *qualified_name, size_t qname_len,
                               const unsigned char *value, size_t value_len,
                               bool case_insensitive)
{
    unsigned int status;
    pcdom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pcdom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PURC_ERROR_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PURC_ERROR_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pcdom_elements_by_attr_cmp_begin_case;
    }
    else {
        cb_ctx.cmp_func = pcdom_elements_by_attr_cmp_begin;
    }

    pcdom_node_simple_walk(pcdom_interface_node(root),
                             pcdom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pcdom_elements_by_attr_end(pcdom_element_t *root,
                             pcdom_collection_t *collection,
                             const unsigned char *qualified_name, size_t qname_len,
                             const unsigned char *value, size_t value_len,
                             bool case_insensitive)
{
    unsigned int status;
    pcdom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pcdom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PURC_ERROR_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PURC_ERROR_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pcdom_elements_by_attr_cmp_end_case;
    }
    else {
        cb_ctx.cmp_func = pcdom_elements_by_attr_cmp_end;
    }

    pcdom_node_simple_walk(pcdom_interface_node(root),
                             pcdom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pcdom_elements_by_attr_contain(pcdom_element_t *root,
                                 pcdom_collection_t *collection,
                                 const unsigned char *qualified_name, size_t qname_len,
                                 const unsigned char *value, size_t value_len,
                                 bool case_insensitive)
{
    unsigned int status;
    pcdom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pcdom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PURC_ERROR_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PURC_ERROR_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pcdom_elements_by_attr_cmp_contain_case;
    }
    else {
        cb_ctx.cmp_func = pcdom_elements_by_attr_cmp_contain;
    }

    pcdom_node_simple_walk(pcdom_interface_node(root),
                             pcdom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pcdom_elements_by_attr_cb(pcdom_node_t *node, void *ctx)
{
    if (node->type != PCDOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pcdom_attr_t *attr;
    pcdom_element_cb_ctx_t *cb_ctx = ctx;
    pcdom_element_t *el = pcdom_interface_element(node);

    attr = pcdom_element_attr_by_id(el, cb_ctx->name_id);
    if (attr == NULL) {
        return PCHTML_ACTION_OK;
    }

    if ((cb_ctx->value_length == 0 && attr->value->length == 0)
        || cb_ctx->cmp_func(cb_ctx, attr))
    {
        cb_ctx->status = pcdom_collection_append(cb_ctx->col, node);

        if (cb_ctx->status != PURC_ERROR_OK) {
            return PCHTML_ACTION_STOP;
        }
    }

    return PCHTML_ACTION_OK;
}

static bool
pcdom_elements_by_attr_cmp_full(pcdom_element_cb_ctx_t *ctx,
                                  pcdom_attr_t *attr)
{
    if (ctx->value_length == attr->value->length
        && pcutils_str_data_ncmp(attr->value->data, ctx->value,
                                ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcdom_elements_by_attr_cmp_full_case(pcdom_element_cb_ctx_t *ctx,
                                       pcdom_attr_t *attr)
{
    if (ctx->value_length == attr->value->length
        && pcutils_str_data_ncasecmp(attr->value->data, ctx->value,
                                    ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcdom_elements_by_attr_cmp_begin(pcdom_element_cb_ctx_t *ctx,
                                   pcdom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length
        && pcutils_str_data_ncmp(attr->value->data, ctx->value,
                                ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcdom_elements_by_attr_cmp_begin_case(pcdom_element_cb_ctx_t *ctx,
                                        pcdom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length
        && pcutils_str_data_ncasecmp(attr->value->data,
                                    ctx->value, ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcdom_elements_by_attr_cmp_end(pcdom_element_cb_ctx_t *ctx,
                                 pcdom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length) {
        size_t dif = attr->value->length - ctx->value_length;

        if (pcutils_str_data_ncmp_end(&attr->value->data[dif],
                                     ctx->value, ctx->value_length))
        {
            return true;
        }
    }

    return false;
}

static bool
pcdom_elements_by_attr_cmp_end_case(pcdom_element_cb_ctx_t *ctx,
                                      pcdom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length) {
        size_t dif = attr->value->length - ctx->value_length;

        if (pcutils_str_data_ncasecmp_end(&attr->value->data[dif],
                                         ctx->value, ctx->value_length))
        {
            return true;
        }
    }

    return false;
}

static bool
pcdom_elements_by_attr_cmp_contain(pcdom_element_cb_ctx_t *ctx,
                                     pcdom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length
        && pcutils_str_data_ncmp_contain(attr->value->data, attr->value->length,
                                        ctx->value, ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcdom_elements_by_attr_cmp_contain_case(pcdom_element_cb_ctx_t *ctx,
                                          pcdom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length
        && pcutils_str_data_ncasecmp_contain(attr->value->data, attr->value->length,
                                            ctx->value, ctx->value_length))
    {
        return true;
    }

    return false;
}

const unsigned char *
pcdom_element_qualified_name(pcdom_element_t *element, size_t *len)
{
    const pchtml_tag_data_t *data;

    if (element->qualified_name != 0) {
        data = pchtml_tag_data_by_id(element->node.owner_document->tags,
                                  element->qualified_name);
    }
    else {
        data = pchtml_tag_data_by_id(element->node.owner_document->tags,
                                  element->node.local_name);
    }

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pcutils_hash_entry_str(&data->entry);
}

const unsigned char *
pcdom_element_qualified_name_upper(pcdom_element_t *element, size_t *len)
{
    pchtml_tag_data_t *data;

    if (element->upper_name == PCHTML_TAG__UNDEF) {
        return pcdom_element_upper_update(element, len);
    }

    data = (pchtml_tag_data_t *) element->upper_name;

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pcutils_hash_entry_str(&data->entry);
}

static const unsigned char *
pcdom_element_upper_update(pcdom_element_t *element, size_t *len)
{
    size_t length;
    pchtml_tag_data_t *data;
    const unsigned char *name;

    if (element->upper_name != PCHTML_TAG__UNDEF) {
        /* TODO: release current tag data if ref_count == 0. */
        /* data = (pchtml_tag_data_t *) element->upper_name; */
    }

    name = pcdom_element_qualified_name(element, &length);
    if (name == NULL) {
        return NULL;
    }

    data = pcutils_hash_insert(element->node.owner_document->tags,
                              pcutils_hash_insert_upper, name, length);
    if (data == NULL) {
        return NULL;
    }

    data->tag_id = element->node.local_name;

    if (len != NULL) {
        *len = length;
    }

    element->upper_name = (pchtml_tag_id_t) data;

    return pcutils_hash_entry_str(&data->entry);
}

const unsigned char *
pcdom_element_local_name(pcdom_element_t *element, size_t *len)
{
    const pchtml_tag_data_t *data;

    data = pchtml_tag_data_by_id(element->node.owner_document->tags,
                              element->node.local_name);
    if (data == NULL) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pcutils_hash_entry_str(&data->entry);
}

const unsigned char *
pcdom_element_prefix(pcdom_element_t *element, size_t *len)
{
    const pchtml_ns_prefix_data_t *data;

    if (element->node.prefix == PCHTML_NS__UNDEF) {
        goto empty;
    }

    data = pchtml_ns_prefix_data_by_id(element->node.owner_document->tags,
                                    element->node.prefix);
    if (data == NULL) {
        goto empty;
    }

    return pcutils_hash_entry_str(&data->entry);

empty:

    if (len != NULL) {
        *len = 0;
    }

    return NULL;
}

const unsigned char *
pcdom_element_tag_name(pcdom_element_t *element, size_t *len)
{
    pcdom_document_t *doc = pcdom_interface_node(element)->owner_document;

    if (element->node.ns != PCHTML_NS_HTML
        || doc->type != PCDOM_DOCUMENT_DTYPE_HTML)
    {
        return pcdom_element_qualified_name(element, len);
    }

    return pcdom_element_qualified_name_upper(element, len);
}
