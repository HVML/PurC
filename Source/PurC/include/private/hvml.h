/**
 * @file hvml.h
 * @author Xu Xiaohong
 * @date 2021/08/23
 * @brief The internal interfaces for hvml parser.
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

#ifndef PURC_PRIVATE_HVML_H
#define PURC_PRIVATE_HVML_H

#include "config.h"
// #include "private/vdom.h"
// #include "private/instance.h"
#include "private/rbtree.h"
// #include "purc-rwstream.h"

#if 0
#error damn

#include "config.h"
#include "private/vdom.h"
#include "private/instance.h"
#include "purc-rwstream.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// initialize hvml module (once)
void pchvml_init_once(void) WTF_INTERNAL;

// initialize the hvml module for a PurC instance.
void pchvml_init_instance(struct pcinst* inst) WTF_INTERNAL;
// clean up the hvml module for a PurC instance.
void pchvml_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // 0

#endif  /* PURC_PRIVATE_HVML_H */

