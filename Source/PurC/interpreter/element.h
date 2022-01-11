/**
 * @file element.h
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for interpreter/element
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
#include "purc-variant.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/dom.h"

PCA_EXTERN_C_BEGIN

purc_variant_t
pcintr_make_elements(size_t nr_elems,
        struct pcdom_element **elems) WTF_INTERNAL;

purc_variant_t
pcintr_query_elements(struct pcdom_element *root,
    const char *css) WTF_INTERNAL;

PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_ELEMENT_H */

