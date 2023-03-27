/*
 * @file hvml-attr.c
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

#include "hvml-attr.h"
#include "hvml-attr-static-list.inc"

const struct pchvml_attr_entry*
pchvml_attr_static_search(const char* name, size_t length)
{
    unsigned int v = (unsigned char)purc_tolower(name[0])
        * (unsigned char)purc_tolower(name[length-1]);
    unsigned idx = v % PCHVML_ATTR_STATIC_SIZE;

    const struct pchvml_attr_entry *entry = &pchvml_attr_static_list_index[idx];
    while (entry) {
        if (!entry->name) {
            return NULL;
        }
        if (strncasecmp(name, entry->name, length)==0) {
            return entry;
        }
        if (entry->next==0) {
            return NULL;
        }
        idx = entry->next;
        entry = &pchvml_attr_static_list_index[idx];
    }
    return NULL;
}



