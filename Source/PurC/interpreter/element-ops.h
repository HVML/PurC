/**
 * @file element-ops.h
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for interpreter/element-ops
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

#ifndef PURC_INTERPRETER_ELEMENT_H
#define PURC_INTERPRETER_ELEMENT_H

#include "purc-macros.h"

PCA_EXTERN_C_BEGIN

struct pcintr_element_ops* pcintr_archetype_get_ops(void);
struct pcintr_element_ops* pcintr_choose_get_ops(void);
struct pcintr_element_ops* pcintr_hvml_get_ops(void);
struct pcintr_element_ops* pcintr_init_get_ops(void);
struct pcintr_element_ops* pcintr_iterate_get_ops(void);

PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_ELEMENT_H */


