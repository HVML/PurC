/*
 * @file hvml-character-reference.h
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

#ifndef PURC_HVML_CHARACTER_REFERENCE_H
#define PURC_HVML_CHARACTER_REFERENCE_H


#include "hvml-entity.h"

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

const struct pchvml_entity* pchvml_character_reference_first(void);
const struct pchvml_entity* pchvml_character_reference_last(void);

const struct pchvml_entity* pchvml_character_reference_first_starting_with(
        char c);
const struct pchvml_entity* pchvml_character_reference_last_starting_with(
        char c);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_CHARACTER_REFERENCE_H */

