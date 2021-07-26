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
 */


#include "private/errors.h"

#include "html/dom/interfaces/element.h"
#include "html/dom/interfaces/attr.h"
#include "html/tag/tag.h"
#include "html/ns/ns.h"

#include "html/core/str.h"
#include "html/core/utils.h"
#include "html/core/hash.h"


typedef struct pchtml_dom_element_cb_ctx pchtml_dom_element_cb_ctx_t;

typedef bool
(*pchtml_dom_element_attr_cmp_f)(pchtml_dom_element_cb_ctx_t *ctx,
                              pchtml_dom_attr_t *attr);


struct pchtml_dom_element_cb_ctx {
    pchtml_dom_collection_t       *col;
    unsigned int               status;
    pchtml_dom_element_attr_cmp_f cmp_func;

    pchtml_dom_attr_id_t          name_id;
    pchtml_ns_prefix_id_t         prefix_id;

    const unsigned char           *value;
    size_t                     value_length;
};


static pchtml_action_t
pchtml_dom_elements_by_tag_name_cb(pchtml_dom_node_t *node, void *ctx);

static pchtml_action_t
pchtml_dom_elements_by_tag_name_cb_all(pchtml_dom_node_t *node, void *ctx);

static pchtml_action_t
pchtml_dom_elements_by_class_name_cb(pchtml_dom_node_t *node, void *ctx);

static pchtml_action_t
pchtml_dom_elements_by_attr_cb(pchtml_dom_node_t *node, void *ctx);

static bool
pchtml_dom_elements_by_attr_cmp_full(pchtml_dom_element_cb_ctx_t *ctx,
                                  pchtml_dom_attr_t *attr);

static bool
pchtml_dom_elements_by_attr_cmp_full_case(pchtml_dom_element_cb_ctx_t *ctx,
                                       pchtml_dom_attr_t *attr);

static bool
pchtml_dom_elements_by_attr_cmp_begin(pchtml_dom_element_cb_ctx_t *ctx,
                                   pchtml_dom_attr_t *attr);

static bool
pchtml_dom_elements_by_attr_cmp_begin_case(pchtml_dom_element_cb_ctx_t *ctx,
                                        pchtml_dom_attr_t *attr);

static bool
pchtml_dom_elements_by_attr_cmp_end(pchtml_dom_element_cb_ctx_t *ctx,
                                 pchtml_dom_attr_t *attr);

static bool
pchtml_dom_elements_by_attr_cmp_end_case(pchtml_dom_element_cb_ctx_t *ctx,
                                      pchtml_dom_attr_t *attr);

static bool
pchtml_dom_elements_by_attr_cmp_contain(pchtml_dom_element_cb_ctx_t *ctx,
                                     pchtml_dom_attr_t *attr);

static bool
pchtml_dom_elements_by_attr_cmp_contain_case(pchtml_dom_element_cb_ctx_t *ctx,
                                          pchtml_dom_attr_t *attr);

static const unsigned char *
pchtml_dom_element_upper_update(pchtml_dom_element_t *element, size_t *len);

const pchtml_tag_data_t *
pchtml_tag_append(pchtml_hash_t *hash, pchtml_tag_id_t tag_id,
               const unsigned char *name, size_t length);

const pchtml_tag_data_t *
pchtml_tag_append_lower(pchtml_hash_t *hash,
                     const unsigned char *name, size_t length);

const pchtml_ns_data_t *
pchtml_ns_append(pchtml_hash_t *hash, const unsigned char *link, size_t length);


pchtml_dom_element_t *
pchtml_dom_element_interface_create(pchtml_dom_document_t *document)
{
    pchtml_dom_element_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pchtml_dom_element_t));
    if (element == NULL) {
        return NULL;
    }

    pchtml_dom_node_t *node = pchtml_dom_interface_node(element);

    node->owner_document = document;
    node->type = PCHTML_DOM_NODE_TYPE_ELEMENT;

    return element;
}

pchtml_dom_element_t *
pchtml_dom_element_interface_destroy(pchtml_dom_element_t *element)
{
    pchtml_dom_attr_t *attr_next;
    pchtml_dom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        attr_next = attr->next;

        pchtml_dom_attr_interface_destroy(attr);

        attr = attr_next;
    }

    return pchtml_mraw_free(
        pchtml_dom_interface_node(element)->owner_document->mraw,
        element);
}

unsigned int
pchtml_dom_element_qualified_name_set(pchtml_dom_element_t *element,
                                   const unsigned char *prefix, size_t prefix_len,
                                   const unsigned char *lname, size_t lname_len)
{
    unsigned char *key = (unsigned char *) lname;
    const pchtml_tag_data_t *tag_data;

    if (prefix != NULL && prefix_len != 0) {
        key = pchtml_malloc(prefix_len + lname_len + 2);
        if (key == NULL) {
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
        return PCHTML_STATUS_ERROR;
    }

    element->qualified_name = (pchtml_tag_id_t) tag_data;

    return PCHTML_STATUS_OK;
}

pchtml_dom_element_t *
pchtml_dom_element_create(pchtml_dom_document_t *document,
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
    pchtml_dom_element_t *element;

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

    element = pchtml_dom_document_create_interface(document, tag_data->tag_id,
                                                ns_data->ns_id);
    if (element == NULL) {
        return NULL;
    }

    if (prefix != NULL) {
        ns_prefix = pchtml_ns_prefix_append(document->prefix, prefix, prefix_len);
        if (ns_prefix == NULL) {
            return pchtml_dom_document_destroy_interface(element);
        }

        element->node.prefix = ns_prefix->prefix_id;

        status = pchtml_dom_element_qualified_name_set(element, prefix, prefix_len,
                                                    local_name, lname_len);
        if (status != PCHTML_STATUS_OK) {
            return pchtml_dom_document_destroy_interface(element);
        }
    }

    if (is_len != 0) {
        status = pchtml_dom_element_is_set(element, is, is_len);
        if (status != PCHTML_STATUS_OK) {
            return pchtml_dom_document_destroy_interface(element);
        }
    }

    element->node.local_name = tag_data->tag_id;
    element->node.ns = ns_data->ns_id;

    if (ns_data->ns_id == PCHTML_NS_HTML && is_len != 0) {
        element->custom_state = PCHTML_DOM_ELEMENT_CUSTOM_STATE_UNDEFINED;
    }
    else {
        element->custom_state = PCHTML_DOM_ELEMENT_CUSTOM_STATE_UNCUSTOMIZED;
    }

    return element;
}

pchtml_dom_element_t *
pchtml_dom_element_destroy(pchtml_dom_element_t *element)
{
    return pchtml_dom_document_destroy_interface(element);
}

bool
pchtml_dom_element_has_attributes(pchtml_dom_element_t *element)
{
    return element->first_attr != NULL;
}

pchtml_dom_attr_t *
pchtml_dom_element_set_attribute(pchtml_dom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len,
                              const unsigned char *value, size_t value_len)
{
    unsigned int status;
    pchtml_dom_attr_t *attr;

    attr = pchtml_dom_element_attr_is_exist(element, qualified_name, qn_len);

    if (attr != NULL) {
        goto update;
    }

    attr = pchtml_dom_attr_interface_create(element->node.owner_document);
    if (attr == NULL) {
        return NULL;
    }

    if (element->node.ns == PCHTML_NS_HTML
        && element->node.owner_document->type == PCHTML_DOM_DOCUMENT_DTYPE_HTML)
    {
        status = pchtml_dom_attr_set_name(attr, qualified_name, qn_len, true);
    }
    else {
        status = pchtml_dom_attr_set_name(attr, qualified_name, qn_len, false);
    }

    if (status != PCHTML_STATUS_OK) {
        return pchtml_dom_attr_interface_destroy(attr);
    }

update:

    status = pchtml_dom_attr_set_value(attr, value, value_len);
    if (status != PCHTML_STATUS_OK) {
        return pchtml_dom_attr_interface_destroy(attr);
    }

    pchtml_dom_element_attr_append(element, attr);

    return attr;
}

const unsigned char *
pchtml_dom_element_get_attribute(pchtml_dom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len,
                              size_t *value_len)
{
    pchtml_dom_attr_t *attr;

    attr = pchtml_dom_element_attr_by_name(element, qualified_name, qn_len);
    if (attr == NULL) {
        if (value_len != NULL) {
            *value_len = 0;
        }

        return NULL;
    }

    return pchtml_dom_attr_value(attr, value_len);
}

unsigned int
pchtml_dom_element_remove_attribute(pchtml_dom_element_t *element,
                                 const unsigned char *qualified_name, size_t qn_len)
{
    unsigned int status;
    pchtml_dom_attr_t *attr;

    attr = pchtml_dom_element_attr_by_name(element, qualified_name, qn_len);
    if (attr == NULL) {
        return PCHTML_STATUS_OK;
    }

    status = pchtml_dom_element_attr_remove(element, attr);
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    pchtml_dom_attr_interface_destroy(attr);

    return PCHTML_STATUS_OK;
}

bool
pchtml_dom_element_has_attribute(pchtml_dom_element_t *element,
                              const unsigned char *qualified_name, size_t qn_len)
{
    return pchtml_dom_element_attr_by_name(element, qualified_name, qn_len) != NULL;
}

unsigned int
pchtml_dom_element_attr_append(pchtml_dom_element_t *element, pchtml_dom_attr_t *attr)
{
    if (attr->node.local_name == PCHTML_DOM_ATTR_ID) {
        if (element->attr_id != NULL) {
            pchtml_dom_element_attr_remove(element, element->attr_id);
            pchtml_dom_attr_interface_destroy(element->attr_id);
        }

        element->attr_id = attr;
    }
    else if (attr->node.local_name == PCHTML_DOM_ATTR_CLASS) {
        if (element->attr_class != NULL) {
            pchtml_dom_element_attr_remove(element, element->attr_class);
            pchtml_dom_attr_interface_destroy(element->attr_class);
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
pchtml_dom_element_attr_remove(pchtml_dom_element_t *element, pchtml_dom_attr_t *attr)
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

pchtml_dom_attr_t *
pchtml_dom_element_attr_by_name(pchtml_dom_element_t *element,
                             const unsigned char *qualified_name, size_t length)
{
    const pchtml_dom_attr_data_t *data;
    pchtml_hash_t *attrs = element->node.owner_document->attrs;
    pchtml_dom_attr_t *attr = element->first_attr;

    if (element->node.ns == PCHTML_NS_HTML
        && element->node.owner_document->type == PCHTML_DOM_DOCUMENT_DTYPE_HTML)
    {
        data = pchtml_dom_attr_data_by_local_name(attrs, qualified_name, length);
    }
    else {
        data = pchtml_dom_attr_data_by_qualified_name(attrs, qualified_name,
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

pchtml_dom_attr_t *
pchtml_dom_element_attr_by_local_name_data(pchtml_dom_element_t *element,
                                        const pchtml_dom_attr_data_t *data)
{
    pchtml_dom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        if (attr->node.local_name == data->attr_id) {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

pchtml_dom_attr_t *
pchtml_dom_element_attr_by_id(pchtml_dom_element_t *element,
                           pchtml_dom_attr_id_t attr_id)
{
    pchtml_dom_attr_t *attr = element->first_attr;

    while (attr != NULL) {
        if (attr->node.local_name == attr_id) {
            return attr;
        }

        attr = attr->next;
    }

    return NULL;
}

bool
pchtml_dom_element_compare(pchtml_dom_element_t *first, pchtml_dom_element_t *second)
{
    pchtml_dom_attr_t *f_attr = first->first_attr;
    pchtml_dom_attr_t *s_attr = second->first_attr;

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
            if (pchtml_dom_attr_compare(f_attr, s_attr)) {
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

pchtml_dom_attr_t *
pchtml_dom_element_attr_is_exist(pchtml_dom_element_t *element,
                              const unsigned char *qualified_name, size_t length)
{
    const pchtml_dom_attr_data_t *data;
    pchtml_dom_attr_t *attr = element->first_attr;

    data = pchtml_dom_attr_data_by_local_name(element->node.owner_document->attrs,
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
pchtml_dom_element_is_set(pchtml_dom_element_t *element,
                       const unsigned char *is, size_t is_len)
{
    if (element->is_value == NULL) {
        element->is_value = pchtml_mraw_calloc(element->node.owner_document->mraw,
                                               sizeof(pchtml_str_t));
        if (element->is_value == NULL) {
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    if (element->is_value->data == NULL) {
        pchtml_str_init(element->is_value,
                        element->node.owner_document->text, is_len);

        if (element->is_value->data == NULL) {
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
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_dom_element_prepare_by_attr(pchtml_dom_document_t *document,
                                pchtml_dom_element_cb_ctx_t *cb_ctx,
                                const unsigned char *qname, size_t qlen)
{
    size_t length;
    const unsigned char *prefix_end;
    const pchtml_dom_attr_data_t *attr_data;
    const pchtml_ns_prefix_data_t *prefix_data;

    cb_ctx->prefix_id = PCHTML_NS__UNDEF;

    prefix_end = memchr(qname, ':', qlen);

    if (prefix_end != NULL) {
        length = prefix_end - qname;

        if (length == 0) {
            return PCHTML_STATUS_ERROR_WRONG_ARGS;
        }

        prefix_data = pchtml_ns_prefix_data_by_name(document->prefix, qname, qlen);
        if (prefix_data == NULL) {
            return PCHTML_STATUS_STOP;
        }

        cb_ctx->prefix_id = prefix_data->prefix_id;

        length += 1;

        if (length >= qlen) {
            return PCHTML_STATUS_ERROR_WRONG_ARGS;
        }

        qname += length;
        qlen -= length;
    }

    attr_data = pchtml_dom_attr_data_by_local_name(document->attrs, qname, qlen);
    if (attr_data == NULL) {
        return PCHTML_STATUS_STOP;
    }

    cb_ctx->name_id = attr_data->attr_id;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_dom_element_prepare_by(pchtml_dom_document_t *document,
                           pchtml_dom_element_cb_ctx_t *cb_ctx,
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
            return PCHTML_STATUS_ERROR_WRONG_ARGS;
        }

        prefix_data = pchtml_ns_prefix_data_by_name(document->prefix, qname, qlen);
        if (prefix_data == NULL) {
            return PCHTML_STATUS_STOP;
        }

        cb_ctx->prefix_id = prefix_data->prefix_id;

        length += 1;

        if (length >= qlen) {
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
pchtml_dom_elements_by_tag_name(pchtml_dom_element_t *root,
                             pchtml_dom_collection_t *collection,
                             const unsigned char *qualified_name, size_t len)
{
    unsigned int status;
    pchtml_dom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;

    /* "*" (U+002A) */
    if (len == 1 && *qualified_name == 0x2A) {
        pchtml_dom_node_simple_walk(pchtml_dom_interface_node(root),
                                 pchtml_dom_elements_by_tag_name_cb_all, &cb_ctx);

        return cb_ctx.status;
    }

    status = pchtml_dom_element_prepare_by(root->node.owner_document,
                                        &cb_ctx, qualified_name, len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    pchtml_dom_node_simple_walk(pchtml_dom_interface_node(root),
                             pchtml_dom_elements_by_tag_name_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pchtml_dom_elements_by_tag_name_cb_all(pchtml_dom_node_t *node, void *ctx)
{
    if (node->type != PCHTML_DOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pchtml_dom_element_cb_ctx_t *cb_ctx = ctx;

    cb_ctx->status = pchtml_dom_collection_append(cb_ctx->col, node);
    if (cb_ctx->status != PCHTML_STATUS_OK) {
        return PCHTML_ACTION_STOP;
    }

    return PCHTML_ACTION_OK;
}

static pchtml_action_t
pchtml_dom_elements_by_tag_name_cb(pchtml_dom_node_t *node, void *ctx)
{
    if (node->type != PCHTML_DOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pchtml_dom_element_cb_ctx_t *cb_ctx = ctx;

    if (node->local_name == cb_ctx->name_id
        && node->prefix == cb_ctx->prefix_id)
    {
        cb_ctx->status = pchtml_dom_collection_append(cb_ctx->col, node);
        if (cb_ctx->status != PCHTML_STATUS_OK) {
            return PCHTML_ACTION_STOP;
        }
    }

    return PCHTML_ACTION_OK;
}

unsigned int
pchtml_dom_elements_by_class_name(pchtml_dom_element_t *root,
                               pchtml_dom_collection_t *collection,
                               const unsigned char *class_name, size_t len)
{
    if (class_name == NULL || len == 0) {
        return PCHTML_STATUS_OK;
    }

    pchtml_dom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = class_name;
    cb_ctx.value_length = len;

    pchtml_dom_node_simple_walk(pchtml_dom_interface_node(root),
                             pchtml_dom_elements_by_class_name_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pchtml_dom_elements_by_class_name_cb(pchtml_dom_node_t *node, void *ctx)
{
    if (node->type != PCHTML_DOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pchtml_dom_element_cb_ctx_t *cb_ctx = ctx;
    pchtml_dom_element_t *el = pchtml_dom_interface_element(node);

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

    pchtml_dom_document_t *doc = el->node.owner_document;

    for (; data < end; data++) {
        if (pchtml_utils_whitespace(*data, ==, ||)) {

            if (pos != data && (data - pos) == (int)cb_ctx->value_length) {
                if (doc->compat_mode == PCHTML_DOM_DOCUMENT_CMODE_QUIRKS) {
                    is_it = pchtml_str_data_ncasecmp(pos, cb_ctx->value,
                                                     cb_ctx->value_length);
                }
                else {
                    is_it = pchtml_str_data_ncmp(pos, cb_ctx->value,
                                                 cb_ctx->value_length);
                }

                if (is_it) {
                    cb_ctx->status = pchtml_dom_collection_append(cb_ctx->col,
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
        if (doc->compat_mode == PCHTML_DOM_DOCUMENT_CMODE_QUIRKS) {
            is_it = pchtml_str_data_ncasecmp(pos, cb_ctx->value,
                                             cb_ctx->value_length);
        }
        else {
            is_it = pchtml_str_data_ncmp(pos, cb_ctx->value,
                                         cb_ctx->value_length);
        }

        if (is_it) {
            cb_ctx->status = pchtml_dom_collection_append(cb_ctx->col, node);
            if (cb_ctx->status != PCHTML_STATUS_OK) {
                return PCHTML_ACTION_STOP;
            }
        }
    }

    return PCHTML_ACTION_OK;
}

unsigned int
pchtml_dom_elements_by_attr(pchtml_dom_element_t *root,
                         pchtml_dom_collection_t *collection,
                         const unsigned char *qualified_name, size_t qname_len,
                         const unsigned char *value, size_t value_len,
                         bool case_insensitive)
{
    unsigned int status;
    pchtml_dom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pchtml_dom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pchtml_dom_elements_by_attr_cmp_full_case;
    }
    else {
        cb_ctx.cmp_func = pchtml_dom_elements_by_attr_cmp_full;
    }

    pchtml_dom_node_simple_walk(pchtml_dom_interface_node(root),
                             pchtml_dom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pchtml_dom_elements_by_attr_begin(pchtml_dom_element_t *root,
                               pchtml_dom_collection_t *collection,
                               const unsigned char *qualified_name, size_t qname_len,
                               const unsigned char *value, size_t value_len,
                               bool case_insensitive)
{
    unsigned int status;
    pchtml_dom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pchtml_dom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pchtml_dom_elements_by_attr_cmp_begin_case;
    }
    else {
        cb_ctx.cmp_func = pchtml_dom_elements_by_attr_cmp_begin;
    }

    pchtml_dom_node_simple_walk(pchtml_dom_interface_node(root),
                             pchtml_dom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pchtml_dom_elements_by_attr_end(pchtml_dom_element_t *root,
                             pchtml_dom_collection_t *collection,
                             const unsigned char *qualified_name, size_t qname_len,
                             const unsigned char *value, size_t value_len,
                             bool case_insensitive)
{
    unsigned int status;
    pchtml_dom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pchtml_dom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pchtml_dom_elements_by_attr_cmp_end_case;
    }
    else {
        cb_ctx.cmp_func = pchtml_dom_elements_by_attr_cmp_end;
    }

    pchtml_dom_node_simple_walk(pchtml_dom_interface_node(root),
                             pchtml_dom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

unsigned int
pchtml_dom_elements_by_attr_contain(pchtml_dom_element_t *root,
                                 pchtml_dom_collection_t *collection,
                                 const unsigned char *qualified_name, size_t qname_len,
                                 const unsigned char *value, size_t value_len,
                                 bool case_insensitive)
{
    unsigned int status;
    pchtml_dom_element_cb_ctx_t cb_ctx = {0};

    cb_ctx.col = collection;
    cb_ctx.value = value;
    cb_ctx.value_length = value_len;

    status = pchtml_dom_element_prepare_by_attr(root->node.owner_document,
                                             &cb_ctx, qualified_name, qname_len);
    if (status != PCHTML_STATUS_OK) {
        if (status == PCHTML_STATUS_STOP) {
            return PCHTML_STATUS_OK;
        }

        return status;
    }

    if (case_insensitive) {
        cb_ctx.cmp_func = pchtml_dom_elements_by_attr_cmp_contain_case;
    }
    else {
        cb_ctx.cmp_func = pchtml_dom_elements_by_attr_cmp_contain;
    }

    pchtml_dom_node_simple_walk(pchtml_dom_interface_node(root),
                             pchtml_dom_elements_by_attr_cb, &cb_ctx);

    return cb_ctx.status;
}

static pchtml_action_t
pchtml_dom_elements_by_attr_cb(pchtml_dom_node_t *node, void *ctx)
{
    if (node->type != PCHTML_DOM_NODE_TYPE_ELEMENT) {
        return PCHTML_ACTION_OK;
    }

    pchtml_dom_attr_t *attr;
    pchtml_dom_element_cb_ctx_t *cb_ctx = ctx;
    pchtml_dom_element_t *el = pchtml_dom_interface_element(node);

    attr = pchtml_dom_element_attr_by_id(el, cb_ctx->name_id);
    if (attr == NULL) {
        return PCHTML_ACTION_OK;
    }

    if ((cb_ctx->value_length == 0 && attr->value->length == 0)
        || cb_ctx->cmp_func(cb_ctx, attr))
    {
        cb_ctx->status = pchtml_dom_collection_append(cb_ctx->col, node);

        if (cb_ctx->status != PCHTML_STATUS_OK) {
            return PCHTML_ACTION_STOP;
        }
    }

    return PCHTML_ACTION_OK;
}

static bool
pchtml_dom_elements_by_attr_cmp_full(pchtml_dom_element_cb_ctx_t *ctx,
                                  pchtml_dom_attr_t *attr)
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
pchtml_dom_elements_by_attr_cmp_full_case(pchtml_dom_element_cb_ctx_t *ctx,
                                       pchtml_dom_attr_t *attr)
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
pchtml_dom_elements_by_attr_cmp_begin(pchtml_dom_element_cb_ctx_t *ctx,
                                   pchtml_dom_attr_t *attr)
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
pchtml_dom_elements_by_attr_cmp_begin_case(pchtml_dom_element_cb_ctx_t *ctx,
                                        pchtml_dom_attr_t *attr)
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
pchtml_dom_elements_by_attr_cmp_end(pchtml_dom_element_cb_ctx_t *ctx,
                                 pchtml_dom_attr_t *attr)
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
pchtml_dom_elements_by_attr_cmp_end_case(pchtml_dom_element_cb_ctx_t *ctx,
                                      pchtml_dom_attr_t *attr)
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
pchtml_dom_elements_by_attr_cmp_contain(pchtml_dom_element_cb_ctx_t *ctx,
                                     pchtml_dom_attr_t *attr)
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
pchtml_dom_elements_by_attr_cmp_contain_case(pchtml_dom_element_cb_ctx_t *ctx,
                                          pchtml_dom_attr_t *attr)
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
pchtml_dom_element_qualified_name(pchtml_dom_element_t *element, size_t *len)
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
pchtml_dom_element_qualified_name_upper(pchtml_dom_element_t *element, size_t *len)
{
    pchtml_tag_data_t *data;

    if (element->upper_name == PCHTML_TAG__UNDEF) {
        return pchtml_dom_element_upper_update(element, len);
    }

    data = (pchtml_tag_data_t *) element->upper_name;

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pchtml_hash_entry_str(&data->entry);
}

static const unsigned char *
pchtml_dom_element_upper_update(pchtml_dom_element_t *element, size_t *len)
{
    size_t length;
    pchtml_tag_data_t *data;
    const unsigned char *name;

    if (element->upper_name != PCHTML_TAG__UNDEF) {
        /* TODO: release current tag data if ref_count == 0. */
        /* data = (pchtml_tag_data_t *) element->upper_name; */
    }

    name = pchtml_dom_element_qualified_name(element, &length);
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
pchtml_dom_element_local_name(pchtml_dom_element_t *element, size_t *len)
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
pchtml_dom_element_prefix(pchtml_dom_element_t *element, size_t *len)
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
pchtml_dom_element_tag_name(pchtml_dom_element_t *element, size_t *len)
{
    pchtml_dom_document_t *doc = pchtml_dom_interface_node(element)->owner_document;

    if (element->node.ns != PCHTML_NS_HTML
        || doc->type != PCHTML_DOM_DOCUMENT_DTYPE_HTML)
    {
        return pchtml_dom_element_qualified_name(element, len);
    }

    return pchtml_dom_element_qualified_name_upper(element, len);
}



/*
 * No inline functions for ABI.
 */
const unsigned char *
pchtml_dom_element_id_noi(pchtml_dom_element_t *element, size_t *len)
{
    return pchtml_dom_element_id(element, len);
}

const unsigned char *
pchtml_dom_element_class_noi(pchtml_dom_element_t *element, size_t *len)
{
    return pchtml_dom_element_class(element, len);
}

bool
pchtml_dom_element_is_custom_noi(pchtml_dom_element_t *element)
{
    return pchtml_dom_element_is_custom(element);
}

bool
pchtml_dom_element_custom_is_defined_noi(pchtml_dom_element_t *element)
{
    return pchtml_dom_element_custom_is_defined(element);
}

pchtml_dom_attr_t *
pchtml_dom_element_first_attribute_noi(pchtml_dom_element_t *element)
{
    return pchtml_dom_element_first_attribute(element);
}

pchtml_dom_attr_t *
pchtml_dom_element_next_attribute_noi(pchtml_dom_attr_t *attr)
{
    return pchtml_dom_element_next_attribute(attr);
}

pchtml_dom_attr_t *
pchtml_dom_element_prev_attribute_noi(pchtml_dom_attr_t *attr)
{
    return pchtml_dom_element_prev_attribute(attr);
}

pchtml_dom_attr_t *
pchtml_dom_element_last_attribute_noi(pchtml_dom_element_t *element)
{
    return pchtml_dom_element_last_attribute(element);
}

pchtml_dom_attr_t *
pchtml_dom_element_id_attribute_noi(pchtml_dom_element_t *element)
{
    return pchtml_dom_element_id_attribute(element);
}

pchtml_dom_attr_t *
pchtml_dom_element_class_attribute_noi(pchtml_dom_element_t *element)
{
    return pchtml_dom_element_class_attribute(element);
}
