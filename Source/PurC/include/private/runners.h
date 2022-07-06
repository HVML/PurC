/*
 * @file runners.h
 * @author Vincent Wei
 * @date 2022/07/05
 * @brief The private interface for runner management.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PRIVATE_RUNNERS_H
#define PURC_PRIVATE_RUNNERS_H

#include <stdbool.h>

#define PCRUN_LOCAL_DATA    "inst-info"

/* operations */
enum {
    PCRUN_K_OPERATION_FIRST = 0,
    PCRUN_K_OPERATION_createCoroutine = PCRUN_K_OPERATION_FIRST,
#define PCRUN_OPERATION_createCoroutine     "createCoroutine"
    PCRUN_K_OPERATION_killCoroutine,
#define PCRUN_OPERATION_killCoroutine       "killCoroutine"
    PCRUN_K_OPERATION_pauseCoroutine,
#define PCRUN_OPERATION_pauseCoroutine      "pauseCoroutine"
    PCRUN_K_OPERATION_resumeCoroutine,
#define PCRUN_OPERATION_resumeCoroutine     "resumeCoroutine"
    PCRUN_K_OPERATION_shutdownInstance,
#define PCRUN_OPERATION_shutdownInstance    "shutdownInstance"

    /* XXX: change this when you append a new operation */
    PCRUN_K_OPERATION_LAST = PCRUN_K_OPERATION_shutdownInstance,
};

#define PCRUN_NR_OPERATIONS \
    (PCRUN_K_OPERATION_LAST - PCRUN_K_OPERATION_FIRST + 1)

struct pcrun_inst_info {
    bool request_to_shutdown;
};

PCA_EXTERN_C_BEGIN

PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_RUNNERS_H */

