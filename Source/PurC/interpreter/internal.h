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

#define TO_DEBUG

#ifdef TO_DEBUG
#ifndef D
#define D(fmt, ...)                                           \
    fprintf(stderr, "%s[%d]:%s(): " fmt "\n",                 \
        basename((char*)__FILE__), __LINE__, __func__,        \
        ##__VA_ARGS__);
#endif // D
#else // ! TO_DEBUG
#define D(fmt, ...) ({                                        \
    UNUSED_PARAM(vtt_to_string);                              \
    UNUSED_PARAM(vgim_to_string);                             \
    })
#endif // TO_DEBUG

PCA_EXTERN_C_BEGIN

int
pcintr_element_eval_attrs(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element);

int
pcintr_element_eval_vcm_content(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element);


void pcintr_coroutine_ready(void);

PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_INTERNAL_H */



