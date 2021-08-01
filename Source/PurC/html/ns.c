/**
 * @file ns.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of name space.
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
 * This implementation of HTML parser is derived from Lexbor <http://lexbor.com/>.
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#include "private/errors.h"
#include "html/core/shs.h"

#define PCHTML_STR_RES_MAP_LOWERCASE
#include "html/core/str_res.h"

#include "html/ns.h"
#include "html/ns_res.h"


const pchtml_ns_data_t *
pchtml_ns_append(pchtml_hash_t *hash, const unsigned char *link, size_t length)
{
    pchtml_ns_data_t *data;
    const pchtml_shs_entry_t *entry;

    if (link == NULL || length == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pchtml_ns_res_shs_link_data,
                                              link, length);
    if (entry != NULL) {
        return entry->value;
    }

    data = pchtml_hash_insert(hash, pchtml_hash_insert_lower, link, length);
    if ((pchtml_ns_id_t) data <= PCHTML_NS__LAST_ENTRY) {
        return NULL;
    }

    data->ns_id = (pchtml_ns_id_t) data;

    return data;
}

const unsigned char *
pchtml_ns_by_id(pchtml_hash_t *hash, pchtml_ns_id_t ns_id, size_t *length)
{
    const pchtml_ns_data_t *data;

    data = pchtml_ns_data_by_id(hash, ns_id);
    if (data == NULL) {
        if (length != NULL) {
            *length = 0;
        }

        return NULL;
    }

    if (length != NULL) {
        *length = data->entry.length;
    }

    return pchtml_hash_entry_str(&data->entry);
}

const pchtml_ns_data_t *
pchtml_ns_data_by_id(pchtml_hash_t *hash, pchtml_ns_id_t ns_id)
{
    UNUSED_PARAM(hash);

    if (ns_id >= PCHTML_NS__LAST_ENTRY) {
        if (ns_id == PCHTML_NS__LAST_ENTRY) {
            return NULL;
        }

        return (const pchtml_ns_data_t *) ns_id;
    }

    return &pchtml_ns_res_data[ns_id];
}

const pchtml_ns_data_t *
pchtml_ns_data_by_link(pchtml_hash_t *hash, const unsigned char *link, size_t length)
{
    const pchtml_shs_entry_t *entry;

    if (link == NULL || length == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pchtml_ns_res_shs_link_data,
                                              link, length);
    if (entry != NULL) {
        return entry->value;
    }

    return pchtml_hash_search(hash, pchtml_hash_search_lower, link, length);
}

/* Prefix */
const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_append(pchtml_hash_t *hash,
                     const unsigned char *prefix, size_t length)
{
    pchtml_ns_prefix_data_t *data;
    const pchtml_shs_entry_t *entry;

    if (prefix == NULL || length == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pchtml_ns_res_shs_data,
                                              prefix, length);
    if (entry != NULL) {
        return entry->value;
    }

    data = pchtml_hash_insert(hash, pchtml_hash_insert_lower, prefix, length);
    if ((pchtml_ns_prefix_id_t) data <= PCHTML_NS__LAST_ENTRY) {
        return NULL;
    }

    data->prefix_id = (pchtml_ns_prefix_id_t) data;

    return data;
}

const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_data_by_id(pchtml_hash_t *hash, pchtml_ns_prefix_id_t prefix_id)
{
    UNUSED_PARAM(hash);

    if (prefix_id >= PCHTML_NS__LAST_ENTRY) {
        if (prefix_id == PCHTML_NS__LAST_ENTRY) {
            return NULL;
        }

        return (const pchtml_ns_prefix_data_t *) prefix_id;
    }

    return &pchtml_ns_prefix_res_data[prefix_id];
}

const pchtml_ns_prefix_data_t *
pchtml_ns_prefix_data_by_name(pchtml_hash_t *hash,
                           const unsigned char *prefix, size_t length)
{
    const pchtml_shs_entry_t *entry;

    if (prefix == NULL || length == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pchtml_ns_res_shs_data,
                                              prefix, length);
    if (entry != NULL) {
        return entry->value;
    }

    return pchtml_hash_search(hash, pchtml_hash_search_lower, prefix, length);
}
