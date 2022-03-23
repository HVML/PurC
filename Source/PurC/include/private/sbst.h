/**
 * @file sbst.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for sbst.
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

#ifndef PCUTILS_SBST_H
#define PCUTILS_SBST_H

#include "config.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char key;

    void       *value;
    size_t     value_len;

    size_t     left;
    size_t     right;
    size_t     next;
} pcutils_sbst_entry_static_t;


/*
 * Inline functions
 */
static inline const pcutils_sbst_entry_static_t *
pcutils_sbst_entry_static_find(const pcutils_sbst_entry_static_t *strt,
                              const pcutils_sbst_entry_static_t *root,
                              const unsigned char key)
{
    while (root != strt) {
        if (root->key == key) {
            return root;
        }
        else if (key > root->key) {
            root = &strt[root->right];
        }
        else {
            root = &strt[root->left];
        }
    }

    return NULL;
}

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCUTILS_SBST_H */
