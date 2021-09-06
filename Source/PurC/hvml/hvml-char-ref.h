/*
 * @file hvml-char-ref.h
 * @author XueShuming
 * @date 2021/09/03
 * @brief The interfaces for hvml character reference entity.
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

#ifndef PURC_HVML_CHAR_REF_H
#define PURC_HVML_CHAR_REF_H


#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "purc-utils.h"

struct pchvml_char_ref_search;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pchvml_char_ref_search* pchvml_char_ref_search_new(void);

void pchvml_char_ref_search_destroy (struct pchvml_char_ref_search* search);

bool pchvml_char_ref_advance (struct pchvml_char_ref_search* search,
        wchar_t uc);

const char* pchvml_char_ref_get_match (struct pchvml_char_ref_search* search);

/*
 * return arraylist of unicode character (wchar_t)
 */
struct pcutils_arrlist* pchvml_char_ref_get_buffered_ucs (
        struct pchvml_char_ref_search* search);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_CHAR_REF_H */

