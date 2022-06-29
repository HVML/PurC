/**
 * @file executor.h
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The internal interfaces for executor.
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

#ifndef PURC_PRIVATE_EXECUTOR_H
#define PURC_PRIVATE_EXECUTOR_H

#include "config.h"

#include "purc-macros.h"
#include "purc-errors.h"
#include "purc-executor.h"

#include "private/map.h"

PCA_EXTERN_C_BEGIN

typedef struct pcexec_func_ops   pcexec_func_ops;
typedef struct pcexec_func_ops  *pcexec_func_ops_t;

struct pcexec_func_ops {
    // 选择器，可用于 `choose` 和 `test` 动作元素
    purc_variant_t (*chooser)(purc_variant_t on_value,
            purc_variant_t with_value);

    // 迭代器，仅用于 `iterate` 动作元素。
    purc_variant_t (*iterator)(purc_variant_t on_value,
            purc_variant_t with_value);

    // 规约器，仅用于 `reduce` 动作元素。
    purc_variant_t (*reducer)(purc_variant_t on_value,
            purc_variant_t with_value);

    // 排序器，仅用于 `sort` 动作元素。
    purc_variant_t (*sorter)(purc_variant_t on_value, purc_variant_t with_value,
            purc_variant_t against_value, bool desc, bool caseless);
};

typedef struct pcexec_class_ops   pcexec_class_ops;
typedef struct pcexec_class_ops  *pcexec_class_ops_t;

typedef struct pcexe_iterator_ops  pcexe_iterator_ops;
typedef struct pcexe_iterator_ops *pcexe_iterator_ops_t;

struct pcexec_class_ops {
    pcexe_iterator_ops_t (*instantiate)(void);
    void (*destroy)(pcexe_iterator_ops_t it);
};

// 外部迭代器操作集
struct purc_iterator_ops {
    purc_variant_t (*begin)(purc_variant_t on_value, purc_variant_t with_value);
    purc_variant_t (*next) (purc_variant_t it);
};


enum pcexec_type {
    PCEXEC_TYPE_INTERNAL,
    PCEXEC_TYPE_EXTERNAL_FUNC,
    PCEXEC_TYPE_EXTERNAL_CLASS,
};

typedef struct pcexec_ops   pcexec_ops;
typedef struct pcexec_ops  *pcexec_ops_t;

struct pcexec_ops {
    enum pcexec_type               type;
    union {
        purc_exec_ops              internal_ops;
        pcexec_func_ops            external_func_ops;
        pcexec_class_ops           external_class_ops;
    };
    purc_atom_t                    atom;
};

int pcexec_register_ops(pcexec_ops_t ops);

int pcexec_get_by_rule(const char *rule, pcexec_ops_t ops);


struct pcexecutor_heap {
    unsigned int       debug_flex:1;
    unsigned int       debug_bison:1;
};

// 用于迭代的迭代器
struct purc_exec_iter {
    size_t                     curr;
    unsigned int               valid:1;
};

// 执行器实例
struct purc_exec_inst {
    enum purc_exec_type         type;
    struct purc_exec_iter       it; // FIXME: one `it` for one `exec_inst`

    purc_variant_t              input;
    purc_variant_t              with;         // for FUNC/CLASS only
    bool                        asc_desc;

    purc_variant_t              selected_keys;

    char                       *err_msg;

    purc_variant_t              value;
};

struct pcinst;

void pcexecutor_set_debug(int debug_flex, int debug_bison);
void pcexecutor_get_debug(int *debug_flex, int *debug_bison);

void pcexecutor_inst_reset(struct purc_exec_inst *inst);


int pcexecutor_register(pcexec_ops_t ops);

int pcexecutor_get_by_rule(const char *rule, pcexec_ops_t ops);

purc_atom_t
pcexecutor_get_rule_name(const char *rule);


PCA_EXTERN_C_END

#endif // PURC_PRIVATE_EXECUTOR_H

