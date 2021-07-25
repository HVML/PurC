/**
 * @file perf.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html performance.
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

#ifndef PCHTML_PERF_H
#define PCHTML_PERF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/core/base.h"


#ifdef PCHTML_WITH_PERF


void *
pchtml_perf_create(void) WTF_INTERNAL;

void
pchtml_perf_clean(void *perf) WTF_INTERNAL;

void
pchtml_perf_destroy(void *perf) WTF_INTERNAL;

unsigned int
pchtml_perf_begin(void *perf) WTF_INTERNAL;

unsigned int
pchtml_perf_end(void *perf) WTF_INTERNAL;

double
pchtml_perf_in_sec(void *perf) WTF_INTERNAL;


#endif /* PCHTML_WITH_PERF */

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_PERF_H */
