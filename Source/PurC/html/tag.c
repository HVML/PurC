/**
 * @file tag.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html tag.
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


#include "private/errors.h"

#include "html/tag.h"
#include "tag_res.h"


const pchtml_tag_data_t *
pchtml_tag_append(pchtml_hash_t *hash, pchtml_tag_id_t tag_id,
               const unsigned char *name, size_t length)
{
    pchtml_tag_data_t *data;
    const pchtml_shs_entry_t *entry;

    entry = pchtml_shs_entry_get_static(pchtml_tag_res_shs_data_default,
                                        name, length);
    if (entry != NULL) {
        return entry->value;
    }

    data = pchtml_hash_insert(hash, pchtml_hash_insert_raw, name, length);
    if (data == NULL) {
        return NULL;
    }

    if (tag_id == PCHTML_TAG__UNDEF) {
        data->tag_id = (pchtml_tag_id_t) data;
    }
    else {
        data->tag_id = tag_id;
    }

    return data;
}

const pchtml_tag_data_t *
pchtml_tag_append_lower(pchtml_hash_t *hash, const unsigned char *name, size_t length)
{
    pchtml_tag_data_t *data;
    const pchtml_shs_entry_t *entry;

    entry = pchtml_shs_entry_get_lower_static(pchtml_tag_res_shs_data_default,
                                              name, length);
    if (entry != NULL) {
        return entry->value;
    }

    data = pchtml_hash_insert(hash, pchtml_hash_insert_lower, name, length);
    if (data == NULL) {
        return NULL;
    }

    data->tag_id = (pchtml_tag_id_t) data;

    return data;
}

const pchtml_tag_data_t *
pchtml_tag_data_by_id(pchtml_hash_t *hash, pchtml_tag_id_t tag_id)
{
    UNUSED_PARAM(hash);

    if (tag_id >= PCHTML_TAG__LAST_ENTRY) {
        if (tag_id == PCHTML_TAG__LAST_ENTRY) {
            return NULL;
        }

        return (const pchtml_tag_data_t *) tag_id;
    }

    return &pchtml_tag_res_data_default[tag_id];
}

const pchtml_tag_data_t *
pchtml_tag_data_by_name(pchtml_hash_t *hash, const unsigned char *name, size_t len)
{
    const pchtml_shs_entry_t *entry;

    if (name == NULL || len == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pchtml_tag_res_shs_data_default,
                                              name, len);
    if (entry != NULL) {
        return (const pchtml_tag_data_t *) entry->value;
    }

    return (const pchtml_tag_data_t *) pchtml_hash_search(hash,
                                           pchtml_hash_search_lower, name, len);
}

const pchtml_tag_data_t *
pchtml_tag_data_by_name_upper(pchtml_hash_t *hash,
                           const unsigned char *name, size_t len)
{
    uintptr_t dif;
    const pchtml_shs_entry_t *entry;

    if (name == NULL || len == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_upper_static(pchtml_tag_res_shs_data_default,
                                              name, len);
    if (entry != NULL) {
        dif = (const pchtml_tag_data_t *) entry->value - pchtml_tag_res_data_default;

        return (const pchtml_tag_data_t *) (pchtml_tag_res_data_upper_default + dif);
    }

    return (const pchtml_tag_data_t *) pchtml_hash_search(hash,
                                           pchtml_hash_search_upper, name, len);
}

