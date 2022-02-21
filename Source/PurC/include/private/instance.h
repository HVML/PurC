/*
 * @file instance.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/05
 * @brief The structures for PurC instance.
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

#ifndef PURC_PRIVATE_INSTANCE_H
#define PURC_PRIVATE_INSTANCE_H

#include "purc.h"

#include "config.h"

#include "private/variant.h"
#include "private/map.h"
#include "private/executor.h"
#include "private/interpreter.h"

struct pcinst {
    int errcode;
    purc_variant_t err_exinfo;
    const char *file;
    int lineno;
    const char *func;

    /* FIXME: enable the fields only NDEBUG is undefined */
#if OS(LINUX)                      /* { */
    void *c_stacks[64];
    int   nr_stacks;
    char  so[1024];
    char  addr1[256];
    char  addr2[64];
#endif                             /* } */

    char* app_name;
    char* runner_name;

    pcutils_map* local_data_map;

    struct pcvariant_heap variant_heap;

    struct pcrdr_conn *conn_to_rdr;

    /* FIXME: dynamically allocate the following heaps
       when HVML moduel is enabled. */
    struct pcexecutor_heap executor_heap;
    struct pcintr_heap    intr_heap;
};

/* gets the current instance */
struct pcinst* pcinst_current(void) WTF_INTERNAL;
pcvarmgr_list_t pcinst_get_variables(void) WTF_INTERNAL;

#endif /* not defined PURC_PRIVATE_INSTANCE_H */

