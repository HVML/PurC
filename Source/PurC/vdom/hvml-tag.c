/*
 * @file hvml-tag.c
 * @author Xu Xiaohong
 * @date 2021/08/31
 * @brief The implementation of public part for vdom.
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
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/vdom.h"

#include "hvml-tag.h"
#include "hvml-tag-static-list.inc"

#include <ctype.h>

/*
FNV_PRIME_64B = 0x100000001b3
FNV_INIT_64B  = 0xcbf29ce484222325

FNV_PRIME_32B = 0x01000193
FNV_INIT_32B  = 0x811c9dc5

def str2key_64b (_str):
    _bytes = _str.encode()
    hval = FNV_INIT_64B;

    for c in _bytes:
        hval ^= c
        hval *= FNV_PRIME_64B

    return hval & 0xFFFFFFFFFFFFFFFF

def str2key_32b (_str):
    _bytes = _str.encode()

    hval = FNV_INIT_32B;
    for c in _bytes:
        hval ^= c
        hval *= FNV_PRIME_32B

    return hval & 0xFFFFFFFF
*/

#if SIZEOF_PTR == 8

#define FNV_PRIME 0x100000001b3
#define FNV_INIT  0xcbf29ce484222325

static uint64_t
_hash_str(const char *name, size_t length)
{
    uint64_t v = FNV_INIT;
    for (size_t i=0; i<length; ++i) {
        v ^= (unsigned char)name[i];
        v *= FNV_PRIME;
    }

    return v;
}

#else /* SIZEOF_PTR == 4 */

#define FNV_PRIME 0x01000193
#define FNV_INIT  0x811c9dc5

static uint32_t
_hash_str(const char *name, size_t length)
{
    uint32_t v = FNV_INIT;
    for (size_t i=0; i<length; ++i) {
        v ^= (unsigned char)name[i];
        v *= FNV_PRIME;
    }

    return v;
}

#endif /* SIZEOF_PTR == 8 */

const struct pchvml_tag_entry*
pchvml_tag_static_search(const char* name, size_t length)
{
    uint64_t v = _hash_str(name, length);
    uint64_t idx = v % PCHVML_BASE_STATIC_SIZE;

    const struct pchvml_tag_static_list *rec;
    rec = &pchvml_tag_static_list_index[idx];

    while (rec) {
        const struct pchvml_tag_entry *entry;
        entry = rec->ctx;
        if (!entry) {
            return NULL;
        }
        if (!entry->name) {
            return NULL;
        }
        if (strcasecmp(name, entry->name)==0) {
            return entry;
        }
        if (rec->next==0) {
            return NULL;
        }
        idx = rec->next;
        rec = &pchvml_tag_static_list_index[idx];
    }
    return NULL;
}

