/**
 * @file pcrdr.h
 * @date 2022/02/21
 * @brief The internal interfaces for PCRDR protocol handling.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (<https://github.com/VincentWei>), 2021, 2022
 *
 * This file is a part of PurC (short for Purring Cat), an PCRDR interpreter.
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

#ifndef PURC_PRIVATE_PCRDR_H
#define PURC_PRIVATE_PCRDR_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void pcrdr_init_once(void) WTF_INTERNAL;

int pcrdr_init_instance(struct pcinst* inst,
        const purc_instance_extra_info *extra_info) WTF_INTERNAL;

void pcrdr_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_PCRDR_H */

