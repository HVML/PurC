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
 * <https://github.com/lexbor/lexbor>, which is licensed under Apahce 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/edom.h"
#include "html_attr_const.h"

#include "html/tag.h"
#include "html/ns.h"

#include "html/str.h"
#include "html/utils.h"
#include "html/hash.h"


typedef struct pcedom_element_cb_ctx pcedom_element_cb_ctx_t;

typedef bool
(*pcedom_element_attr_cmp_f)(pcedom_element_cb_ctx_t *ctx,
                              pcedom_attr_t *attr);


struct pcedom_element_cb_ctx {
    pcedom_collection_t       *col;
    unsigned int               status;
    pcedom_element_attr_cmp_f cmp_func;

    pcedom_attr_id_t          name_id;
    pchtml_ns_prefix_id_t         prefix_id;

    const unsigned char           *value;
    size_t                     value_length;
};


static pchtml_action_t
pcedom_elements_by_tag_name_cb(pcedom_node_t *node, void *ctx);

static pchtml_action_t
pcedom_elements_by_tag_name_cb_all(pcedom_node_t *node, void *ctx);

static pchtml_action_t
pcedom_elements_by_class_name_cb(pcedom_node_t *node, void *ctx);

static pchtml_action_t
pcedom_elements_by_attr_cb(pcedom_node_t *node, void *ctx);

static bool
pcedom_elements_by_attr_cmp_full(pcedom_element_cb_ctx_t *ctx,
                                  pcedom_attr_t *attr);

static bool
pcedom_elements_by_attr_cmp_full_case(pcedom_element_cb_ctx_t *ctx,
                                       pcedom_attr_t *attr);

static bool
pcedom_elements_by_attr_cmp_begin(pcedom_element_cb_ctx_t *ctx,
                                   pcedom_attr_t *attr);

static bool
pcedom_elements_by_attr_cmp_begin_case(pcedom_element_cb_ctx_t *ctx,
                                        pcedom_attr_t *attr);

static bool
pcedom_elements_by_attr_cmp_end(pcedom_element_cb_ctx_t *ctx,
                                 pcedom_attr_t *attr);

static bool
pcedom_elements_by_attr_cmp_end_case(pcedom_element_cb_ctx_t *ctx,
                                      pcedom_attr_t *attr);

static bool
pcedom_elements_by_attr_cmp_contain(pcedom_element_cb_ctx_t *ctx,
                                     pcedom_attr_t *attr);

static bool
pcedom_elements_by_attr_cmp_contain_case(pcedom_element_cb_ctx_t *ctx,
                                          pcedom_attr_t *attr);

static const unsigned char *
pcedom_element_upper_update(pcedom_element_t *element, size_t *len);

const pchtml_tag_data_t *
pchtml_tag_append(pchtml_hash_t *hash, pchtml_tag_id_t tag_id,
               const unsigned char *name, size_t length);

const pchtml_tag_data_t *
pchtml_tag_append_lower(pchtml_hash_t *hash,
                     const unsigned char *name, size_t length);

const pchtml_ns_data_t *
pchtml_ns_append(pchtml_hash_t *hash, const unsigned char *link, size_t length);


pcedom_element_t *
pcedom_element_interface_create(pcedom_document_t *document)
{
    pcedom_element_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pcedom_element_t));
    if (element == NULL) {
        return NULL;
    }

    pcedom_node_t *node = pcedom_interface_node(element);

    node->owner_document = document;
    node->type = PCEDOM_NODE_TYPE_ELEMENT;

    return element;
}

pcedom_element_t *
pcedom_element_interface_destroy(pcedom_element_t *element)
{
    pcedom_attr_t *attr_next;
    pcedom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        attr_next = attr->next;

        pcedom_attr_interface_destroy(attr);

        attr = attr_next;
    }

    return pchtml_mraw_free(
        pcedom_interface_node(element)->owner_document->mraw,
        element);
}

unsigned int
pcedom_element_qualified_name_set(pcedom_element_t *element,
                                   const unsigned char *prefix, size_t prefix_len,
                                   const unsigned char *lname, size_t lname_len)
{
    unsigned char *key = (unsigned char *) lname;
    const pchtml_tag_data_t *tag_data;

    if (prefix != NULL && prefix_len != 0) {
        key = pchtml_malloc(prefix_len + lname_len + 2);
        if (key == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
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
        pcinst_set_error (PCEDOM_ERROR);
        return PCHTML_STATUS_ERROR;
    }

    element->qualified_name = (pchtml_tag_id_t) tag_data;

    return PCHTML_STATUS_OK;
}

pcedom_element_t *
pcedom_element_create(pcedom_document_t *document,
                       const unsigned char *local_name, size_t lname_len,
                       const unsigned char *ns_link, size_t ns_len,
                       const unsigned char *prefix, size_t prefix_len,
                       const unsigned char *is, size_t is_len,
                       bool sync_custom)
{
    UNUSED_PARAM(sync_custom);

    unsigned int status;
    const pchtml_ns_data_t *ns_data;
    const pchtml_tag_data_t *tag_data;
    const pchtml_ns_prefix_data_t *ns_prefix;
    pcedom_element_t *element;

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

    element = pcedom_document_create_interface(document, tag_data->tag_id,
                                                ns_data->ns_id);
    if (element == NULL) {
        return NULL;
    }

    if (prefix != NULL) {
        ns_prefix = pchtml_ns_prefix_append(document->prefix, prefix, prefix_len);
        if (ns_prefix == NULL) {
            return pcedom_document_destroy_interface(element);
        }

        element->node.prefix = ns_prefix->prefix_id;

        status = pcedom_element_qualified_name_set(element, prefix, prefix_len,
                                                    local_name, lname_len);
        if (status != PCHTML_STATUS_OK) {
            return pcedom_document_destroy_interface(element);
        }
    }

    if (is_len != 0) {
        status = pcedom_element_is_set(element, is, is_len);
        if (status != PCHTML_STATUS_OK) {
            return pcedom_document_destroy_interface(element);
        }
    }

    element->node.local_name = tag_data->tag_id;
    element->node.ns = ns_data->ns_id;

    if (ns_data->ns_id == PCHTML_NS_HTML && is_len != 0) {
        element->custom_state = PCEDOM_ELEMENT_CUSTOM_STATE_UNDEFINED;
    }
    else {
        element->custom_state = PCEDOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED;
    }

    return element;
}

pcedom_element_t *
pcedom_element_destroy(pcedom_element_t *element)
{
    return pcedom_document_destroy_interface(element);
}

bool
pcedom_element_has_attributes(pcedom_element_t *element)
{
    return element->first_attr != NULL;
}

pcedom_attr_t *
pcedom_element_set_attribute(pcedom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len,
                              const unsigned char *value, size_t value_len)
{
    unsigned int status;
    pcedom_attr_t *attr;

    attr = pcedom_element_attr_is_exist(element, qualified_name, qn_len);

    if (attr != NULL) {
        goto update;
    }

    attr = pcedom_attr_interface_create(element->node.owner_document);
    if (attr == NULL) {
        return NULL;
    }

    if (element->node.ns == PCHTML_NS_HTML
        && element->node.owner_document->type == PCEDOM_DOCUMENT_DTYPE_HTML)
    {
        status = pcedom_attr_set_name(attr, qualified_name, qn_len, true);
    }
    else {
        status = pcedom_attr_set_name(attr, qualified_name, qn_len, false);
    }

    if (status != PCHTML_STATUS_OK) {
        return pcedom_attr_interface_destroy(attr);
    }

update:

    status = pcedom_attr_set_value(attr, value, value_len);
    if (status != PCHTML_STATUS_OK) {
        return pcedom_attr_interface_destroy(attr);
    }

    pcedom_element_attr_append(element, attr);

    return attr;
}

const unsigned char *
pcedom_element_get_attribute(pcedom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len,
                              size_t *value_len)
{
    pcedom_attr_t *attr;

    attr = pcedom_element_attr_by_name(element, qualified_name, qn_len);
    if (attr == NULL) {
        if (value_len != NULL) {
            *value_len = 0;
        }

        return NULL;
    }

    return pcedom_attr_value(attr, value_len);
}

unsigned int
pcedom_element_remove_attribute(pcedom_element_t *element,
                                 const unsigned char *qualified_name, size_t qn_len)
{
    unsigned int status;
    pcedom_attr_t *attr;

    attr = pcedom_element_attr_by_name(element, qualified_name, qn_len);
    if (attr == NULL) {
        return PCHTML_STATUS_OK;
    }

    status = pcedom_element_attr_remove(element, attr);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    pcedom_attr_interface_destroy(attr);

    return PCHTML_STATUS_OK;
}

bool
pcedom_element_has_attribute(pcedom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len)
{
    return pcedom_element_attr_by_name(element, qualified_name, qn_len) != NULL;
}

unsigned int
pcedom_element_attr_append(pcedom_element_t *element, pcedom_attr_t *attr)
{
    if (attr->node.local_name == PCEDOM_ATTR_ID) {
        if (element->attr_id != NULL) {
            pcedom_element_attr_remove(element, element->attr_id);
            pcedom_attr_interface_destroy(element->attr_id);
        }

        element->attr_id = attr;
    }
    else if (attr->node.local_name == PCEDOM_ATTR_CLASS) {
        if (element->attr_class != NULL) {
            pcedom_element_attr_remove(element, element->attr_class);
            pcedom_attr_interface_destroy(element->attr_class);
        }

        element->attr_class = attr;
    }

    if (element->first_attr == NULL) {
        element->first_attr = attr;
        element->last_attr = attr;

        return PCHTML_STATUS_OK;
    }

    attr->prev = element->last_attr;
    element->last_attr->next = attr;

    element->last_attr = attr;

    return PCHTML_STATUS_OK;
}

unsigned int
pcedom_element_attr_remove(pcedom_element_t *element, pcedom_attr_t *attr)
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

    return PCHTML_STATUS_OK;
}

pcedom_attr_t *
pcedom_element_attr_by_name(pcedom_element_t *element,
                             const unsigned char *qualified_name, size_t length)
{
    const pcedom_attr_data_t *data;
    pchtml_hash_t *attrs = element->node.owner_document->attrs;
    pcedom_attr_t *attr = element->first_attr;

    if (element->node.ns == PCHTML_NS_HTML
        && element->node.owner_document->type == PCEDOM_DOCUMENT_DTYPE_HTML)
    {
        data = pcedom_attr_data_by_local_name(attrs, qualified_name, length);
    }
    else {
        data = pcedom_attr_data_by_qualified_name(attrs, qualified_name,
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

pcedom_attr_t *
pcedom_element_attr_by_local_name_data(pcedom_element_t *element,
                                        const pcedom_attr_data_t *data)
{
    pcedom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        if (attr->node.local_name == data->attr_id) {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

pcedom_attr_t *
pcedom_element_attr_by_id(pcedom_element_t *element,
                           pcedom_attr_id_t attr_id)
{
    pcedom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        if (attr->node.local_name == attr_id) {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

bool
pcedom_element_compare(pcedom_element_t *first, pcedom_element_t *second)
{
    pcedom_attr_t *f_attr = first->first_attr;
    pcedom_attr_t *s_attr = second->first_attr;

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
            if (pcedom_attr_compare(f_attr, s_attr)) {
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

pcedom_attr_t *
pcedom_element_attr_is_exist(pcedom_element_t *element,
                              const unsigned char *qualified_name, size_t length)
{
    const pcedom_attr_data_t *data;
    pcedom_attr_t *attr = element->first_attr;

    data = pcedom_attr_data_by_local_name(element->node.owner_document->attrs,
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
pcedom_element_is_set(pcedom_element_t *element,
                       const unsigned char *is, size_t is_len)
{
    if (element->is_value == NULL) {
        element->is_value = pchtml_mraw_calloc(element->node.owner_document->mraw,
                                               sizeof(pchtml_str_t));
        if (element->is_value == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    if (element->is_value->data == NULL) {
        pchtml_str_init(element->is_value,
                        element->node.owner_document->text, is_len);

        if (element->is_value->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    if (element->is_value->length != 0) {
        element->is_value->length = 0;
    }

    unsigned char *data = pchtml_str_append(element->is_value,
                                         element->node.owner_document->text,
                                         is, is_len);
    if (data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pcedom_element_prepare_by_attr(pcedom_document_t *document,
                                pcedom_element_cb_ctx_t *cb_ctx,
                                const unsigned char *qname, size_t qlen)
{
    size_t length;
    const unsigned char *prefix_end;
    const pcedom_attr_data_t *attr_data;
    const pchtml_ns_prefix_data_t *prefix_data;

    cb_ctx->prefix_id = PCHTML_NS__UNDEF;

    prefix_end = memchr(qname, ':', qlen);

    if (prefix_end != NULL) {
        length = prefix_end - qname;

        if (length == 0) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            return PCHTML_STATUS_ERROR_WRONG_ARGS;
        }

        prefix_data = pchtml_ns_prefix_data_by_name(document->prefix, qname, qlen);
        if (prefix_data == NULL) {
            return PCHTML_STATUS_STOP;
        }

        cb_ctx->prefix_id = prefix_data->prefix_id;

        length += 1;

        if (length >= qlen) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            return PCHTML_STATUS_ERROR_WRONG_ARGS;
        }

        qname += length;
        qlen -= length;
    }

    attr_data = pcedom_attr_data_by_local_name(document->attrs, qname, qlen);
    if (attr_data == NULL) {
        return PCHTML_STATUS_STOP;
    }

    cb_ctx->name_id = attr_data->attr_id;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pcedom_element_prepare_by(pcedom_document_t *document,
                           pcedom_element_cb_ctx_t *cb_ctx,
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
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            return PCHTML_STATUS_ERROR_WRONG_ARGS;
        }

        prefix_data = pchtml_ns_prefix_data_by_name(document->prefix, qname, qlen);
        if (prefix_data == NULL) {
            return PCHTML_STATUS_STOP;
        }

        cb_ctx->prefix_id = prefix_data->prefix_id;

        length += 1;

        if (length >= qlen) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            return PCHTML_STATUS_ERROR_WRONG_ARGS;
        }

        qname += length;
        qlen -= length;
    }

    tag_data = pchtml_tag_data_by_name(document->tags, qname, qlen);
    if (tag_data == NULL) {
        return PCHTML_STATUS_STOP;
    }

    cb_ctx->name_id = tag_data->tag_id;

    return PCHTML_STATUS_OK;
}

unsigned int
pcedom_elements_by_tag_name(pcedom_element_t *root,
                             pcedom_collection_t *collection,
                             const unsigned char *qualified_name, size_t len)
{
    unsigned int status;
    pcedom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;

    /* "*" (U+002A) */
    if (len == 1 && *qualified_name == 0x2A) {
        pcedom_node_simple_walk(pcedom_interface_node(root),
                                 pcedom_elements_by_tag_name_cb_all, &cb_ctx);

        return cb_ctx.status;
    }

    status = pcedom_element_prepare_by(root->node.owner_document,
                                        &cb_ctx, qualified_name, len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    pcedom_node_simple_walk(pcedom_interface_node(root),
                             pcedom_elements_by_tag_name_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pcedom_elements_by_tag_name_cb_all(pcedom_node_t *node, void *ctx)
{
    if (node->type != PCEDOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pcedom_element_cb_ctx_t *cb_ctx = ctx;

    cb_ctx->status = pcedom_collection_append(cb_ctx->col, node);
    if (cb_ctx->status != PCHTML_STATUS_OK) {
        return PCHTML_ACTION_STOP;
    }

    return PCHTML_ACTION_OK;
}

static pchtml_action_t
pcedom_elements_by_tag_name_cb(pcedom_node_t *node, void *ctx)
{
    if (node->type != PCEDOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pcedom_element_cb_ctx_t *cb_ctx = ctx;

    if (node->local_name == cb_ctx->name_id
        && node->prefix == cb_ctx->prefix_id)
    {
        cb_ctx->status = pcedom_collection_append(cb_ctx->col, node);
        if (cb_ctx->status != PCHTML_STATUS_OK) {
            return PCHTML_ACTION_STOP;
        }
    }

    return PCHTML_ACTION_OK;
}

unsigned int
pcedom_elements_by_class_name(pcedom_element_t *root,
                               pcedom_collection_t *collection,
                               const unsigned char *class_name, size_t len)
{
    if (class_name == NULL || len == 0) {
        return PCHTML_STATUS_OK;
    }

    pcedom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = class_name;
    cb_ctx.value_length = len;

    pcedom_node_simple_walk(pcedom_interface_node(root),
                             pcedom_elements_by_class_name_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pcedom_elements_by_class_name_cb(pcedom_node_t *node, void *ctx)
{
    if (node->type != PCEDOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pcedom_element_cb_ctx_t *cb_ctx = ctx;
    pcedom_element_t *el = pcedom_interface_element(node);

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

    pcedom_document_t *doc = el->node.owner_document;

    for (; data < end; data++) {
        if (pchtml_utils_whitespace(*data, ==, ||)) {

            if (pos != data && (data - pos) == (int)cb_ctx->value_length) {
                if (doc->compat_mode == PCEDOM_DOCUMENT_CMODE_QUIRKS) {
                    is_it = pchtml_str_data_ncasecmp(pos, cb_ctx->value,
                                                     cb_ctx->value_length);
                }
                else {
                    is_it = pchtml_str_data_ncmp(pos, cb_ctx->value,
                                                 cb_ctx->value_length);
                }

                if (is_it) {
                    cb_ctx->status = pcedom_collection_append(cb_ctx->col,
                                                               node);
                    if (cb_ctx->status != PCHTML_STATUS_OK) {
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
        if (doc->compat_mode == PCEDOM_DOCUMENT_CMODE_QUIRKS) {
            is_it = pchtml_str_data_ncasecmp(pos, cb_ctx->value,
                                             cb_ctx->value_length);
        }
        else {
            is_it = pchtml_str_data_ncmp(pos, cb_ctx->value,
                                         cb_ctx->value_length);
        }

        if (is_it) {
            cb_ctx->status = pcedom_collection_append(cb_ctx->col, node);
            if (cb_ctx->status != PCHTML_STATUS_OK) {
                return PCHTML_ACTION_STOP;
            }
        }
    }

    return PCHTML_ACTION_OK;
}

unsigned int
pcedom_elements_by_attr(pcedom_element_t *root,
                         pcedom_collection_t *collection,
                         const unsigned char *qualified_name, size_t qname_len,
                         const unsigned char *value, size_t value_len,
                         bool case_insensitive)
{
    unsigned int status;
    pcedom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pcedom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pcedom_elements_by_attr_cmp_full_case;
    }
    else {
        cb_ctx.cmp_func = pcedom_elements_by_attr_cmp_full;
    }

    pcedom_node_simple_walk(pcedom_interface_node(root),
                             pcedom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pcedom_elements_by_attr_begin(pcedom_element_t *root,
                               pcedom_collection_t *collection,
                               const unsigned char *qualified_name, size_t qname_len,
                               const unsigned char *value, size_t value_len,
                               bool case_insensitive)
{
    unsigned int status;
    pcedom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pcedom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pcedom_elements_by_attr_cmp_begin_case;
    }
    else {
        cb_ctx.cmp_func = pcedom_elements_by_attr_cmp_begin;
    }

    pcedom_node_simple_walk(pcedom_interface_node(root),
                             pcedom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pcedom_elements_by_attr_end(pcedom_element_t *root,
                             pcedom_collection_t *collection,
                             const unsigned char *qualified_name, size_t qname_len,
                             const unsigned char *value, size_t value_len,
                             bool case_insensitive)
{
    unsigned int status;
    pcedom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pcedom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pcedom_elements_by_attr_cmp_end_case;
    }
    else {
        cb_ctx.cmp_func = pcedom_elements_by_attr_cmp_end;
    }

    pcedom_node_simple_walk(pcedom_interface_node(root),
                             pcedom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pcedom_elements_by_attr_contain(pcedom_element_t *root,
                                 pcedom_collection_t *collection,
                                 const unsigned char *qualified_name, size_t qname_len,
                                 const unsigned char *value, size_t value_len,
                                 bool case_insensitive)
{
    unsigned int status;
    pcedom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pcedom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pcedom_elements_by_attr_cmp_contain_case;
    }
    else {
        cb_ctx.cmp_func = pcedom_elements_by_attr_cmp_contain;
    }

    pcedom_node_simple_walk(pcedom_interface_node(root),
                             pcedom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pcedom_elements_by_attr_cb(pcedom_node_t *node, void *ctx)
{
    if (node->type != PCEDOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pcedom_attr_t *attr;
    pcedom_element_cb_ctx_t *cb_ctx = ctx;
    pcedom_element_t *el = pcedom_interface_element(node);

    attr = pcedom_element_attr_by_id(el, cb_ctx->name_id);
    if (attr == NULL) {
        return PCHTML_ACTION_OK;
    }

    if ((cb_ctx->value_length == 0 && attr->value->length == 0)
        || cb_ctx->cmp_func(cb_ctx, attr))
    {
        cb_ctx->status = pcedom_collection_append(cb_ctx->col, node);

        if (cb_ctx->status != PCHTML_STATUS_OK) {
            return PCHTML_ACTION_STOP;
        }
    }

    return PCHTML_ACTION_OK;
}

static bool
pcedom_elements_by_attr_cmp_full(pcedom_element_cb_ctx_t *ctx,
                                  pcedom_attr_t *attr)
{
    if (ctx->value_length == attr->value->length
        && pchtml_str_data_ncmp(attr->value->data, ctx->value,
                                ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcedom_elements_by_attr_cmp_full_case(pcedom_element_cb_ctx_t *ctx,
                                       pcedom_attr_t *attr)
{
    if (ctx->value_length == attr->value->length
        && pchtml_str_data_ncasecmp(attr->value->data, ctx->value,
                                    ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcedom_elements_by_attr_cmp_begin(pcedom_element_cb_ctx_t *ctx,
                                   pcedom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length
        && pchtml_str_data_ncmp(attr->value->data, ctx->value,
                                ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcedom_elements_by_attr_cmp_begin_case(pcedom_element_cb_ctx_t *ctx,
                                        pcedom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length
        && pchtml_str_data_ncasecmp(attr->value->data,
                                    ctx->value, ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcedom_elements_by_attr_cmp_end(pcedom_element_cb_ctx_t *ctx,
                                 pcedom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length) {
        size_t dif = attr->value->length - ctx->value_length;

        if (pchtml_str_data_ncmp_end(&attr->value->data[dif],
                                     ctx->value, ctx->value_length))
        {
            return true;
        }
    }

    return false;
}

static bool
pcedom_elements_by_attr_cmp_end_case(pcedom_element_cb_ctx_t *ctx,
                                      pcedom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length) {
        size_t dif = attr->value->length - ctx->value_length;

        if (pchtml_str_data_ncasecmp_end(&attr->value->data[dif],
                                         ctx->value, ctx->value_length))
        {
            return true;
        }
    }

    return false;
}

static bool
pcedom_elements_by_attr_cmp_contain(pcedom_element_cb_ctx_t *ctx,
                                     pcedom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length
        && pchtml_str_data_ncmp_contain(attr->value->data, attr->value->length,
                                        ctx->value, ctx->value_length))
    {
        return true;
    }

    return false;
}

static bool
pcedom_elements_by_attr_cmp_contain_case(pcedom_element_cb_ctx_t *ctx,
                                          pcedom_attr_t *attr)
{
    if (ctx->value_length <= attr->value->length
        && pchtml_str_data_ncasecmp_contain(attr->value->data, attr->value->length,
                                            ctx->value, ctx->value_length))
    {
        return true;
    }

    return false;
}

const unsigned char *
pcedom_element_qualified_name(pcedom_element_t *element, size_t *len)
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

    return pchtml_hash_entry_str(&data->entry);
}

const unsigned char *
pcedom_element_qualified_name_upper(pcedom_element_t *element, size_t *len)
{
    pchtml_tag_data_t *data;

    if (element->upper_name == PCHTML_TAG__UNDEF) {
        return pcedom_element_upper_update(element, len);
    }

    data = (pchtml_tag_data_t *) element->upper_name;

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pchtml_hash_entry_str(&data->entry);
}

static const unsigned char *
pcedom_element_upper_update(pcedom_element_t *element, size_t *len)
{
    size_t length;
    pchtml_tag_data_t *data;
    const unsigned char *name;

    if (element->upper_name != PCHTML_TAG__UNDEF) {
        /* TODO: release current tag data if ref_count == 0. */
        /* data = (pchtml_tag_data_t *) element->upper_name; */
    }

    name = pcedom_element_qualified_name(element, &length);
    if (name == NULL) {
        return NULL;
    }

    data = pchtml_hash_insert(element->node.owner_document->tags,
                              pchtml_hash_insert_upper, name, length);
    if (data == NULL) {
        return NULL;
    }

    data->tag_id = element->node.local_name;

    if (len != NULL) {
        *len = length;
    }

    element->upper_name = (pchtml_tag_id_t) data;

    return pchtml_hash_entry_str(&data->entry);
}

const unsigned char *
pcedom_element_local_name(pcedom_element_t *element, size_t *len)
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

    return pchtml_hash_entry_str(&data->entry);
}

const unsigned char *
pcedom_element_prefix(pcedom_element_t *element, size_t *len)
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

    return pchtml_hash_entry_str(&data->entry);

empty:

    if (len != NULL) {
        *len = 0;
    }

    return NULL;
}

const unsigned char *
pcedom_element_tag_name(pcedom_element_t *element, size_t *len)
{
    pcedom_document_t *doc = pcedom_interface_node(element)->owner_document;

    if (element->node.ns != PCHTML_NS_HTML
        || doc->type != PCEDOM_DOCUMENT_DTYPE_HTML)
    {
        return pcedom_element_qualified_name(element, len);
    }

    return pcedom_element_qualified_name_upper(element, len);
}
