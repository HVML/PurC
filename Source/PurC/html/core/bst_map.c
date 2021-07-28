/**
 * @file bst_map.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of bst_map.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/core/bst_map.h"
#include "html/core/utils.h"


pchtml_bst_map_t *
pchtml_bst_map_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_bst_map_t));
}

unsigned int
pchtml_bst_map_init(pchtml_bst_map_t *bst_map, size_t size)
{
    unsigned int status;

    if (bst_map == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (size == 0) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    /* bst */
    bst_map->bst = pchtml_bst_create();
    status = pchtml_bst_init(bst_map->bst, size);
    if (status) {
        return status;
    }

    /* dobject */
    bst_map->entries = pchtml_dobject_create();
    status = pchtml_dobject_init(bst_map->entries, size,
                                 sizeof(pchtml_bst_map_entry_t));
    if (status) {
        return status;
    }

    /* mraw */
    bst_map->mraw = pchtml_mraw_create();
    status = pchtml_mraw_init(bst_map->mraw, (size * 6));
    if (status) {
        return status;
    }

    return PCHTML_STATUS_OK;
}

void
pchtml_bst_map_clean(pchtml_bst_map_t *bst_map)
{
    pchtml_bst_clean(bst_map->bst);
    pchtml_mraw_clean(bst_map->mraw);
    pchtml_dobject_clean(bst_map->entries);
}

pchtml_bst_map_t *
pchtml_bst_map_destroy(pchtml_bst_map_t *bst_map, bool self_destroy)
{
    if (bst_map == NULL) {
        return NULL;
    }

    bst_map->bst = pchtml_bst_destroy(bst_map->bst, true);
    bst_map->mraw = pchtml_mraw_destroy(bst_map->mraw, true);
    bst_map->entries = pchtml_dobject_destroy(bst_map->entries, true);

    if (self_destroy) {
        return pchtml_free(bst_map);
    }

    return bst_map;
}

pchtml_bst_map_entry_t *
pchtml_bst_map_search(pchtml_bst_map_t *bst_map, pchtml_bst_entry_t *scope,
                      const unsigned char *key, size_t key_len)
{
    pchtml_bst_map_entry_t *entry;
    pchtml_bst_entry_t *bst_entry;

    size_t hash_id = pchtml_utils_hash_hash(key, key_len);

    bst_entry = pchtml_bst_search(bst_map->bst, scope, hash_id);
    if (bst_entry == NULL) {
        return NULL;
    }

    do {
        entry = bst_entry->value;

        if (entry->str.length == key_len &&
            pchtml_str_data_cmp(entry->str.data, key))
        {
            return entry;
        }

        bst_entry = bst_entry->next;
    }
    while (bst_entry != NULL);

    return NULL;
}

pchtml_bst_map_entry_t *
pchtml_bst_map_insert(pchtml_bst_map_t *bst_map,
                      pchtml_bst_entry_t **scope,
                      const unsigned char *key, size_t key_len, void *value)
{
    pchtml_bst_map_entry_t *entry;

    entry = pchtml_bst_map_insert_not_exists(bst_map, scope, key, key_len);
    if (entry == NULL) {
        return NULL;
    }

    entry->value = value;

    return entry;
}

pchtml_bst_map_entry_t *
pchtml_bst_map_insert_not_exists(pchtml_bst_map_t *bst_map,
                                 pchtml_bst_entry_t **scope,
                                 const unsigned char *key, size_t key_len)
{
    pchtml_bst_map_entry_t *entry;
    pchtml_bst_entry_t *bst_entry;

    size_t hash_id = pchtml_utils_hash_hash(key, key_len);

    bst_entry = pchtml_bst_insert_not_exists(bst_map->bst, scope, hash_id);
    if (bst_entry == NULL) {
        return NULL;
    }

    if (bst_entry->value == NULL) {
        goto new_entry;
    }

    do {
        entry = bst_entry->value;

        if (entry->str.length == key_len &&
            pchtml_str_data_cmp(entry->str.data, key))
        {
            return entry;
        }

        if (bst_entry->next == NULL) {
            bst_entry->next = pchtml_bst_entry_make(bst_map->bst, hash_id);
            bst_entry = bst_entry->next;

            if (bst_entry == NULL) {
                return NULL;
            }

            goto new_entry;
        }

        bst_entry = bst_entry->next;
    }
    while (1);

    return NULL;

new_entry:

    entry = pchtml_dobject_calloc(bst_map->entries);
    if (entry == NULL) {
        return NULL;
    }

    pchtml_str_init(&entry->str, bst_map->mraw, key_len);
    if (entry->str.data == NULL) {
        pchtml_dobject_free(bst_map->entries, entry);

        return NULL;
    }

    pchtml_str_append(&entry->str, bst_map->mraw, key, key_len);

    bst_entry->value = entry;

    return entry;
}

void *
pchtml_bst_map_remove(pchtml_bst_map_t *bst_map, pchtml_bst_entry_t **scope,
                      const unsigned char *key, size_t key_len)
{
    pchtml_bst_map_entry_t *entry;
    pchtml_bst_entry_t *bst_entry;

    size_t hash_id = pchtml_utils_hash_hash(key, key_len);

    bst_entry = pchtml_bst_search(bst_map->bst, *scope, hash_id);
    if (bst_entry == NULL) {
        return NULL;
    }

    do {
        entry = bst_entry->value;

        if (entry->str.length == key_len &&
            pchtml_str_data_cmp(entry->str.data, key))
        {
            void *value = entry->value;

            pchtml_bst_remove_by_pointer(bst_map->bst, bst_entry, scope);

            pchtml_str_destroy(&entry->str, bst_map->mraw, false);
            pchtml_dobject_free(bst_map->entries, entry);

            return value;
        }

        bst_entry = bst_entry->next;
    }
    while (bst_entry != NULL);

    return NULL;
}

/*
 * No inline functions.
 */
pchtml_mraw_t *
pchtml_bst_map_mraw_noi(pchtml_bst_map_t *bst_map)
{
    return pchtml_bst_map_mraw(bst_map);
}
