/*
 * @file hvml-sbst.h
 * @author XueShuming
 * @date 2021/09/06
 * @brief The interfaces for hvml sbst.
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

struct pchvml_sbst;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pchvml_sbst* pchvml_sbst_new_char_ref(void);
struct pchvml_sbst* pchvml_sbst_new_markup_declaration_open_state(void);
struct pchvml_sbst* pchvml_sbst_new_after_doctype_name_state(void);

void pchvml_sbst_destroy(struct pchvml_sbst* sbst);

bool pchvml_sbst_advance_ex(struct pchvml_sbst* sbst,
        uint32_t uc, bool case_insensitive);

/*
 * case sensitive
 */
PCA_INLINE
bool pchvml_sbst_advance(struct pchvml_sbst* sbst, uint32_t uc)
{
    return pchvml_sbst_advance_ex(sbst, uc, false);
}

const char* pchvml_sbst_get_match(struct pchvml_sbst* sbst);

/*
 * return arraylist of unicode character (uint32_t)
 */
struct pcutils_arrlist* pchvml_sbst_get_buffered_ucs(
        struct pchvml_sbst* sbst);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_CHAR_REF_H */

