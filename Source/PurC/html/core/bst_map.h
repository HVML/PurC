/**
 * @file bst_map.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for bst_map.
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

#ifndef PCHTML_BST_MAP_H
#define PCHTML_BST_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/bst.h"
#include "html/core/str.h"
#include "html/core/mraw.h"
#include "html/core/dobject.h"


typedef struct {
    pchtml_str_t str;
    void         *value;
}
pchtml_bst_map_entry_t;

typedef struct {
    pchtml_bst_t     *bst;
    pchtml_mraw_t    *mraw;
    pchtml_dobject_t *entries;

}
pchtml_bst_map_t;


pchtml_bst_map_t *
pchtml_bst_map_create(void) WTF_INTERNAL;

unsigned int
pchtml_bst_map_init(pchtml_bst_map_t *bst_map, size_t size) WTF_INTERNAL;

void
pchtml_bst_map_clean(pchtml_bst_map_t *bst_map) WTF_INTERNAL;

pchtml_bst_map_t *
pchtml_bst_map_destroy(pchtml_bst_map_t *bst_map, bool self_destroy) WTF_INTERNAL;


pchtml_bst_map_entry_t *
pchtml_bst_map_search(pchtml_bst_map_t *bst_map, pchtml_bst_entry_t *scope,
                      const unsigned char *key, size_t key_len) WTF_INTERNAL;

pchtml_bst_map_entry_t *
pchtml_bst_map_insert(pchtml_bst_map_t *bst_map, pchtml_bst_entry_t **scope,
                      const unsigned char *key, size_t key_len, void *value) WTF_INTERNAL;

pchtml_bst_map_entry_t *
pchtml_bst_map_insert_not_exists(pchtml_bst_map_t *bst_map,
                                 pchtml_bst_entry_t **scope,
                                 const unsigned char *key, size_t key_len) WTF_INTERNAL;

void *
pchtml_bst_map_remove(pchtml_bst_map_t *bst_map, pchtml_bst_entry_t **scope,
                      const unsigned char *key, size_t key_len) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_mraw_t *
pchtml_bst_map_mraw(pchtml_bst_map_t *bst_map)
{
    return bst_map->mraw;
}

/*
 * No inline functions for ABI.
 */
pchtml_mraw_t *
pchtml_bst_map_mraw_noi(pchtml_bst_map_t *bst_map);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_BST_MAP_H */

