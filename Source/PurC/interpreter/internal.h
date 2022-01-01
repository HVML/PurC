/**
 * @file internal.h
 * @author Xu Xiaohong
 * @date 2021/12/18
 * @brief The internal interfaces for interpreter/internal
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

#ifndef PURC_INTERPRETER_INTERNAL_H
#define PURC_INTERPRETER_INTERNAL_H

#include "purc-macros.h"

#include "private/interpreter.h"

#include <libgen.h>

PCA_EXTERN_C_BEGIN

int
pcintr_element_eval_attrs(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element);

int
pcintr_element_eval_vcm_content(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element);


void pcintr_coroutine_ready(void);

void
pcintr_set_base_uri(const char* base_uri);

purc_variant_t
pcintr_load_from_uri(const char* uri);

purc_variant_t
pcintr_make_element_variant(struct pcvdom_element *element);

int
pcintr_set_symbol_var_at_sign(void);

PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_INTERNAL_H */



