/*
 * @file ops.h
 * @author XueShuming
 * @date 2021/09/02
 * @brief The interfaces for vcm eval frame ops.
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

#ifndef _VCM_OPS_H
#define _VCM_OPS_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "private/debug.h"
#include "private/tree.h"
#include "private/list.h"
#include "private/vcm.h"
#include "purc-variant.h"

#include "eval.h"



#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


struct pvcm_eval_stack_frame_ops *
pcvcm_eval_get_ops_by_node(struct pcvcm_node *node);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined _VCM_OPS_H */

