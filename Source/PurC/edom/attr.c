/**
 * @file attr.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of html element attribution.
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
#include "private/edom.h"
#include "html_attr_res.h"

pcedom_attr_data_t *
pcedom_attr_local_name_append(pchtml_hash_t *hash,
                const unsigned char *name, size_t length) WTF_INTERNAL;

pcedom_attr_data_t *
pcedom_attr_qualified_name_append(pchtml_hash_t *hash, const unsigned char *name,
                size_t length) WTF_INTERNAL;

const pchtml_ns_data_t *
pchtml_ns_append(pchtml_hash_t *hash, const unsigned char *link, size_t length);


pcedom_attr_t *
pcedom_attr_interface_create(pcedom_document_t *document)
{
    pcedom_attr_t *attr;

    attr = pchtml_mraw_calloc(document->mraw, sizeof(pcedom_attr_t));
    if (attr == NULL) {
        return NULL;
    }

    pcedom_node_t *node = pcedom_interface_node(attr);

    node->owner_document = document;
    node->type = PCEDOM_NODE_TYPE_ATTRIBUTE;

    return attr;
}

pcedom_attr_t *
pcedom_attr_interface_destroy(pcedom_attr_t *attr)
{
    pcedom_document_t *doc = pcedom_interface_node(attr)->owner_document;

    if (attr->value != NULL) {
        if (attr->value->data != NULL) {
            pchtml_mraw_free(doc->text, attr->value->data);
        }

        pchtml_mraw_free(doc->mraw, attr->value);
    }

    return pchtml_mraw_free(doc->mraw, attr);
}

unsigned int
pcedom_attr_set_name(pcedom_attr_t *attr, const unsigned char *name,
                      size_t length, bool to_lowercase)
{
    pcedom_attr_data_t *data;
    pcedom_document_t *doc = pcedom_interface_node(attr)->owner_document;

    data = pcedom_attr_local_name_append(doc->attrs, name, length);
    if (data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    attr->node.local_name = (pcedom_attr_id_t) data;

    if (to_lowercase == false) {
        data = pcedom_attr_qualified_name_append(doc->attrs, name, length);
        if (data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        attr->qualified_name = (pcedom_attr_id_t) data;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pcedom_attr_set_name_ns(pcedom_attr_t *attr, const unsigned char *link,
                         size_t link_length, const unsigned char *name,
                         size_t name_length, bool to_lowercase)
{
    size_t length;
    unsigned char *p;
    const pchtml_ns_data_t *ns_data;
    pcedom_attr_data_t *data;
    pcedom_document_t *doc = pcedom_interface_node(attr)->owner_document;

    ns_data = pchtml_ns_append(doc->ns, link, link_length);
    if (attr->node.ns == PCHTML_NS__UNDEF) {
        pcinst_set_error (PCEDOM_ERROR);
        return PCHTML_STATUS_ERROR;
    }

    attr->node.ns = ns_data->ns_id;

    /* TODO: append check https://www.w3.org/TR/xml/#NT-Name */

    p = (unsigned char *) memchr(name, ':', name_length);
    if (p == NULL) {
        return pcedom_attr_set_name(attr, name, name_length, to_lowercase);
    }

    length = p - name;

    /* local name */
    data = pcedom_attr_local_name_append(doc->attrs, &name[(length + 1)],
                                          (name_length - (length + 1)));
    if (data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    attr->node.local_name = (pcedom_attr_id_t) data;

    /* qualified name */
    data = pcedom_attr_qualified_name_append(doc->attrs, name, name_length);
    if (data == NULL) {
        pcinst_set_error (PCEDOM_ERROR);
        return PCHTML_STATUS_ERROR;
    }

    attr->qualified_name = (pcedom_attr_id_t) data;

    /* prefix */
    attr->node.prefix = (pchtml_ns_prefix_id_t) pchtml_ns_prefix_append(doc->ns, name,
                                                                  length);
    if (attr->node.prefix == 0) {
        pcinst_set_error (PCEDOM_ERROR);
        return PCHTML_STATUS_ERROR;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pcedom_attr_set_value(pcedom_attr_t *attr,
                       const unsigned char *value, size_t value_len)
{
    pcedom_document_t *doc = pcedom_interface_node(attr)->owner_document;

    if (attr->value == NULL) {
        attr->value = pchtml_mraw_calloc(doc->mraw, sizeof(pchtml_str_t));
        if (attr->value == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    if (attr->value->data == NULL) {
        pchtml_str_init(attr->value, doc->text, value_len);
        if (attr->value->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }
    else {
        attr->value->length = 0;

        if (pchtml_str_size(attr->value) <= value_len) {
            const unsigned char *tmp;

            tmp = pchtml_str_realloc(attr->value, doc->text, (value_len + 1));
            if (tmp == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }
        }
    }

    memcpy(attr->value->data, value, sizeof(unsigned char) * value_len);

    attr->value->data[value_len] = 0x00;
    attr->value->length = value_len;

    return PCHTML_STATUS_OK;
}

unsigned int
pcedom_attr_set_value_wo_copy(pcedom_attr_t *attr,
                               unsigned char *value, size_t value_len)
{
    if (attr->value == NULL) {
        pcedom_document_t *doc = pcedom_interface_node(attr)->owner_document;

        attr->value = pchtml_mraw_alloc(doc->mraw, sizeof(pchtml_str_t));
        if (attr->value == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    attr->value->data = value;
    attr->value->length = value_len;

    return PCHTML_STATUS_OK;
}

unsigned int
pcedom_attr_set_existing_value(pcedom_attr_t *attr,
                                const unsigned char *value, size_t value_len)
{
    return pcedom_attr_set_value(attr, value, value_len);
}

unsigned int
pcedom_attr_clone_name_value(pcedom_attr_t *attr_from,
                              pcedom_attr_t *attr_to)
{
    attr_to->node.local_name = attr_from->node.local_name;
    attr_to->qualified_name = attr_from->qualified_name;

    return PCHTML_STATUS_OK;
}

bool
pcedom_attr_compare(pcedom_attr_t *first, pcedom_attr_t *second)
{
    if (first->node.local_name == second->node.local_name
        && first->node.ns == second->node.ns
        && first->qualified_name == second->qualified_name)
    {
        if (first->value == NULL) {
            if (second->value == NULL) {
                return true;
            }

            return false;
        }

        if (second->value != NULL
            && first->value->length == second->value->length
            && pchtml_str_data_ncmp(first->value->data, second->value->data,
                                    first->value->length))
        {
            return true;
        }
    }

    return false;
}

pcedom_attr_data_t *
pcedom_attr_local_name_append(pchtml_hash_t *hash,
                               const unsigned char *name, size_t length)
{
    pcedom_attr_data_t *data;
    const pchtml_shs_entry_t *entry;

    if (name == NULL || length == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pcedom_attr_res_shs_data,
                                              name, length);
    if (entry != NULL) {
        return entry->value;
    }

    data = pchtml_hash_insert(hash, pchtml_hash_insert_lower, name, length);
    if ((pcedom_attr_id_t) data <= PCEDOM_ATTR__LAST_ENTRY) {
        return NULL;
    }

    data->attr_id = (uintptr_t) data;

    return data;
}

pcedom_attr_data_t *
pcedom_attr_qualified_name_append(pchtml_hash_t *hash, const unsigned char *name,
                                   size_t length)
{
    pcedom_attr_data_t *data;

    if (name == NULL || length == 0) {
        return NULL;
    }

    data = pchtml_hash_insert(hash, pchtml_hash_insert_raw, name, length);
    if ((pcedom_attr_id_t) data <= PCEDOM_ATTR__LAST_ENTRY) {
        return NULL;
    }

    data->attr_id = (uintptr_t) data;

    return data;
}

const pcedom_attr_data_t *
pcedom_attr_data_by_id(pchtml_hash_t *hash, pcedom_attr_id_t attr_id)
{
    UNUSED_PARAM(hash);

    if (attr_id >= PCEDOM_ATTR__LAST_ENTRY) {
        if (attr_id == PCEDOM_ATTR__LAST_ENTRY) {
            return NULL;
        }

        return (const pcedom_attr_data_t *) attr_id;
    }

    return &pcedom_attr_res_data_default[attr_id];
}

const pcedom_attr_data_t *
pcedom_attr_data_by_local_name(pchtml_hash_t *hash,
                                const unsigned char *name, size_t length)
{
    const pchtml_shs_entry_t *entry;

    if (name == NULL || length == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pcedom_attr_res_shs_data,
                                              name, length);
    if (entry != NULL) {
        return entry->value;
    }

    return pchtml_hash_search(hash, pchtml_hash_search_lower, name, length);
}

const pcedom_attr_data_t *
pcedom_attr_data_by_qualified_name(pchtml_hash_t *hash,
                                    const unsigned char *name, size_t length)
{
    const pchtml_shs_entry_t *entry;

    if (name == NULL || length == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_static(pcedom_attr_res_shs_data,
                                        name, length);
    if (entry != NULL) {
        return entry->value;
    }

    return pchtml_hash_search(hash, pchtml_hash_search_raw, name, length);
}

const unsigned char *
pcedom_attr_qualified_name(pcedom_attr_t *attr, size_t *len)
{
    const pcedom_attr_data_t *data;

    if (attr->qualified_name != 0) {
        data = pcedom_attr_data_by_id(attr->node.owner_document->attrs,
                                       attr->qualified_name);
    }
    else {
        data = pcedom_attr_data_by_id(attr->node.owner_document->attrs,
                                       attr->node.local_name);
    }

    if (len != NULL) {
        *len = data->entry.length;
    }

    return pchtml_hash_entry_str(&data->entry);
}
