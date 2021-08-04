/**
 * @file shs.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of shs.
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

#include "html/shs.h"
#include "html/str.h"

#define PCHTML_STR_RES_MAP_LOWERCASE
#define PCHTML_STR_RES_MAP_UPPERCASE
#include "html/str_res.h"


#define pchtml_shs_make_id_m(key, size, table_size)                            \
    (((((key[0] * key[size - 1]) * key[0]) + size) % table_size) + 0x01)

#define pchtml_shs_make_id_lower_m(key, size, table_size)                      \
    (((((pchtml_str_res_map_lowercase[key[0]]                                  \
     * pchtml_str_res_map_lowercase[key[size - 1]])                            \
     * pchtml_str_res_map_lowercase[key[0]])                                   \
     + size)                                                                   \
     % table_size) + 0x01)

#define pchtml_shs_make_id_upper_m(key, size, table_size)                      \
    (((((pchtml_str_res_map_uppercase[key[0]]                                  \
     * pchtml_str_res_map_uppercase[key[size - 1]])                            \
     * pchtml_str_res_map_uppercase[key[0]])                                   \
     + size)                                                                   \
     % table_size) + 0x01)


const pchtml_shs_entry_t *
pchtml_shs_entry_get_static(const pchtml_shs_entry_t *root,
                            const unsigned char *key, size_t key_len)
{
    const pchtml_shs_entry_t *entry;
    entry = root + pchtml_shs_make_id_m(key, key_len, root->key_len);

    while (entry->key != NULL)
    {
        if (entry->key_len == key_len) {
            if (pchtml_str_data_ncmp((const unsigned char *) entry->key,
                                     key, key_len))
            {
                return entry;
            }

            entry = &root[entry->next];
        }
        else if (entry->key_len > key_len) {
            return NULL;
        }
        else {
            entry = &root[entry->next];
        }
    }

    return NULL;
}

const pchtml_shs_entry_t *
pchtml_shs_entry_get_lower_static(const pchtml_shs_entry_t *root,
                                  const unsigned char *key, size_t key_len)
{
    const pchtml_shs_entry_t *entry;
    entry = root + pchtml_shs_make_id_lower_m(key, key_len, root->key_len);

    while (entry->key != NULL)
    {
        if (entry->key_len == key_len) {
            if (pchtml_str_data_nlocmp_right((const unsigned char *) entry->key,
                                             key, key_len))
            {
                return entry;
            }

            entry = &root[entry->next];
        }
        else if (entry->key_len > key_len) {
            return NULL;
        }
        else {
            entry = &root[entry->next];
        }
    }

    return NULL;
}

const pchtml_shs_entry_t *
pchtml_shs_entry_get_upper_static(const pchtml_shs_entry_t *root,
                                  const unsigned char *key, size_t key_len)
{
    const pchtml_shs_entry_t *entry;
    entry = root + pchtml_shs_make_id_upper_m(key, key_len, root->key_len);

    while (entry->key != NULL)
    {
        if (entry->key_len == key_len) {
            if (pchtml_str_data_nupcmp_right((const unsigned char *) entry->key,
                                             key, key_len))
            {
                return entry;
            }

            entry = &root[entry->next];
        }
        else if (entry->key_len > key_len) {
            return NULL;
        }
        else {
            entry = &root[entry->next];
        }
    }

    return NULL;
}
