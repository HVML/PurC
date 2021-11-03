/*
 * @file exe_filter.h
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for FILTER executor.
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


#ifndef PURC_EXECUTOR_FILTER_H
#define PURC_EXECUTOR_FILTER_H

#include "config.h"

#include "purc-macros.h"

#include "pcexe-helper.h"

struct filter_rule
{
    struct logical_expression       *lexp;
    enum for_clause_type             for_clause;
};

struct exe_filter_param {
    char *err_msg;
    int debug_flex;
    int debug_bison;

    struct filter_rule        rule;
    unsigned int              rule_valid:1;
};

PCA_EXTERN_C_BEGIN

int pcexec_exe_filter_register(void);

PCA_EXTERN_C_END

#endif // PURC_EXECUTOR_FILTER_H

