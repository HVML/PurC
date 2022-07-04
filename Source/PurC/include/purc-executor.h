/**
 * @file purc-executor.h
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The internal interfaces for executor.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PURC_EXECUTOR_H
#define PURC_PURC_EXECUTOR_H

#include "purc-macros.h"
#include "purc-errors.h"
#include "purc-variant.h"

#define PURC_ENVV_EXECUTOR_PATH "PURC_EXECUTOR_PATH"

PCA_EXTERN_C_BEGIN

struct purc_exec_inst;
typedef struct purc_exec_inst purc_exec_inst;
typedef struct purc_exec_inst* purc_exec_inst_t;

struct purc_exec_iter;
typedef struct purc_exec_iter purc_exec_iter;
typedef struct purc_exec_iter* purc_exec_iter_t;

enum purc_exec_type {
    PURC_EXEC_TYPE_CHOOSE = 0,
    PURC_EXEC_TYPE_ITERATE,
    PURC_EXEC_TYPE_REDUCE,
};

/** The operation set for the built-in executors */
typedef struct purc_exec_ops {
    /** the operation to create a instance of the built-in executor */
    purc_exec_inst_t (*create) (enum purc_exec_type type,
            purc_variant_t input, bool asc_desc);

    /** the operation for `choose` tag. */
    purc_variant_t (*choose) (purc_exec_inst_t inst, const char* rule);

    /** the operation to get the iterator */
    purc_exec_iter_t (*it_begin) (purc_exec_inst_t inst, const char* rule);

    /** the operation to get the value of an interator */
    purc_variant_t (*it_value) (purc_exec_inst_t inst, purc_exec_iter_t it);

    /** the operation to forward the iterator to the next item */
    purc_exec_iter_t (*it_next) (purc_exec_inst_t inst, purc_exec_iter_t it,
             const char* rule);

    /** the operation for `reduce` tag. */
    purc_variant_t (*reduce) (purc_exec_inst_t inst, const char* rule);

    /** the operation to destroy the instance of the built-in executor. */
    bool (*destroy) (purc_exec_inst_t inst);
} purc_exec_ops;
typedef struct purc_exec_ops* purc_exec_ops_t;

struct purc_iterator_ops {
    purc_variant_t (*begin)(purc_variant_t on_value, purc_variant_t with_value);
    // @return:
    // PURC_INVALID_VALUE and no purc-error: no further iteration is available
    // PURC_INVALID_VALUE otherwise: exception: internal failure
    // otherwise: current value, and it moves forward
    purc_variant_t (*next)(purc_variant_t it);
};
typedef struct purc_iterator_ops  purc_iterator_ops;
typedef struct purc_iterator_ops* purc_iterator_ops_t;

// typedef purc_iterator_ops_t (*iterator_instantiate)(void);

/** Register a built-in executor */
bool purc_register_executor(const char* name, purc_exec_ops_t ops);

/** Retrieve the operation set of a built-in executor */
bool purc_get_executor(const char* name, purc_exec_ops_t ops);

PCA_EXTERN_C_END

#endif // PURC_PURC_EXECUTOR_H

