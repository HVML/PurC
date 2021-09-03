/*
 * @file hvml-entity.h
 * @author XueShuming
 * @date 2021/09/03
 * @brief The interfaces for hvml entity.
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

#ifndef PURC_HVML_ENTITY_H
#define PURC_HVML_ENTITY_H

#include <stddef.h>
#include <stdint.h>
#include <private/arraylist.h>

#include "config.h"
#include "purc-utils.h"

struct pchvml_entity;
struct pchvml_entity_search;

typedef struct pchvml_entity* (first_entry_starting_with_fn)(char c);
typedef struct pchvml_entity* (last_entry_starting_with_fn)(char c);


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

const char* pchvml_entity_get_entity(struct pchvml_entity* entity);
size_t pchvml_entity_get_entity_length(struct pchvml_entity* entity);
wchar_t pchvml_entity_get_first_value(struct pchvml_entity* entity);
wchar_t pchvml_entity_get_last_value(struct pchvml_entity* entity);


struct pchvml_entity_search* pchvml_entity_search_new_ex(
        struct pchvml_entity* first, struct pchvml_entity* last,
        first_entry_starting_with_fn* first_starting_with,
        last_entry_starting_with_fn* last_starting_with);

PCA_INLINE
struct pchvml_entity_search* pchvml_entity_search_new(
        struct pchvml_entity* first, struct pchvml_entity* last)
{
    return pchvml_entity_search_new_ex(first, last, NULL, NULL);
}

void pchvml_entity_search_destroy(struct pchvml_entity_search* search);

struct pchvml_entity* pchvml_entity_search_most_recent_match(
        struct pchvml_entity_search* search);

size_t pchvml_entity_search_current_length(struct pchvml_entity_search* search);

bool pchvml_entity_advance(struct pchvml_entity_search* search, wchar_t uc);



#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_ENTITY_H */

