/*
 * @file exe_class.h
 * @author Xu Xiaohong
 * @date 2022/06/16
 * @brief The implementation of public part for CLASS executor.
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


#ifndef PURC_EXECUTOR_CLASS_H
#define PURC_EXECUTOR_CLASS_H

#include "config.h"

#include "purc-macros.h"

#include "private/debug.h"

#include "pcexe-helper.h"

struct class_rule
{
    char                               *name;
    char                               *module;
};

struct exe_class_param {
    char *err_msg;
    int debug_flex;
    int debug_bison;

    struct class_rule   rule;
    unsigned int       rule_valid:1;
};

PCA_EXTERN_C_BEGIN

int pcexec_exe_class_register(void);

int exe_class_parse(const char *input, size_t len,
        struct exe_class_param *param);

static inline void
class_rule_release(struct class_rule *rule)
{
    if (rule->name) {
        free(rule->name);
        rule->name = NULL;
    }
    if (rule->module) {
        free(rule->module);
        rule->module = NULL;
    }
}

static inline void
exe_class_param_reset(struct exe_class_param *param)
{
    if (!param)
        return;

    if (param->err_msg) {
        free(param->err_msg);
        param->err_msg = NULL;
    }

    class_rule_release(&param->rule);
}

PCA_EXTERN_C_END

#endif // PURC_EXECUTOR_CLASS_H

