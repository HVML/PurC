/**
 * @file ops.h
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for interpreter/ops
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

#ifndef PURC_INTERPRETER_OPS_H
#define PURC_INTERPRETER_OPS_H

#include "purc-macros.h"

PCA_EXTERN_C_BEGIN

void init_ops(void);

struct pcintr_element_ops* pcintr_get_undefined_ops(void);

struct pcintr_element_ops pcintr_get_ops_by_tag_id(enum pchvml_tag_id tag_id);
struct pcintr_element_ops pcintr_get_ops_by_element(pcvdom_element_t element);

struct pcintr_element_ops* pcintr_get_document_ops(void);
struct pcintr_element_ops* pcintr_get_hvml_ops(void);
struct pcintr_element_ops* pcintr_get_head_ops(void);
struct pcintr_element_ops* pcintr_get_body_ops(void);
struct pcintr_element_ops* pcintr_get_init_ops(void);
struct pcintr_element_ops* pcintr_get_archetype_ops(void);
struct pcintr_element_ops* pcintr_get_iterate_ops(void);
struct pcintr_element_ops* pcintr_get_update_ops(void);
struct pcintr_element_ops* pcintr_get_except_ops(void);
struct pcintr_element_ops* pcintr_get_observe_ops(void);
struct pcintr_element_ops* pcintr_get_test_ops(void);
struct pcintr_element_ops* pcintr_get_match_ops(void);
struct pcintr_element_ops* pcintr_get_choose_ops(void);
struct pcintr_element_ops* pcintr_get_catch_ops(void);
struct pcintr_element_ops* pcintr_get_forget_ops(void);
struct pcintr_element_ops* pcintr_get_fire_ops(void);
struct pcintr_element_ops* pcintr_get_back_ops(void);
struct pcintr_element_ops* pcintr_get_define_ops(void);
struct pcintr_element_ops* pcintr_get_include_ops(void);
struct pcintr_element_ops* pcintr_get_call_ops(void);
struct pcintr_element_ops* pcintr_get_return_ops(void);
struct pcintr_element_ops* pcintr_get_inherit_ops(void);
struct pcintr_element_ops* pcintr_get_exit_ops(void);
struct pcintr_element_ops* pcintr_get_clear_ops(void);
struct pcintr_element_ops* pcintr_get_erase_ops(void);
struct pcintr_element_ops* pcintr_get_sleep_ops(void);
struct pcintr_element_ops* pcintr_get_error_ops(void);
struct pcintr_element_ops* pcintr_get_differ_ops(void);
struct pcintr_element_ops* pcintr_get_archedata_ops(void);
struct pcintr_element_ops* pcintr_get_reduce_ops(void);
struct pcintr_element_ops* pcintr_get_sort_ops(void);
struct pcintr_element_ops* pcintr_get_bind_ops(void);
struct pcintr_element_ops* pcintr_get_load_ops(void);
struct pcintr_element_ops* pcintr_get_request_ops(void);

PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_OPS_H */


