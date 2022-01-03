/*
 * @file errors.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/02
 * @brief The get/set error code of PurC.
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

#define _GNU_SOURCE

#include "purc-errors.h"

#include "private/errors.h"
#include "private/instance.h"

#include "private/interpreter.h" // FIXME:

#include <dlfcn.h>
#include <elf.h>
#include <execinfo.h>
#include <link.h>
#include <regex.h>
#include <stdarg.h>

static const struct err_msg_info* get_error_info(int errcode);

static int _noinst_errcode;

int purc_get_last_error(void)
{
    const struct pcinst* inst = pcinst_current();
    if (inst) {
        return inst->errcode;
    }

    return _noinst_errcode;
}

purc_variant_t purc_get_last_error_ex(void)
{
    const struct pcinst* inst = pcinst_current();
    if (inst) {
        return inst->err_exinfo;
    }

    return PURC_VARIANT_INVALID;
}

int purc_set_error_exinfo_with_debug(int errcode, purc_variant_t exinfo,
        const char *file, int lineno, const char *func)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        _noinst_errcode = errcode;
        return PURC_ERROR_NO_INSTANCE;
    }

    inst->errcode = errcode;
    PURC_VARIANT_SAFE_CLEAR(inst->err_exinfo);
    inst->err_exinfo = exinfo;
    inst->file       = file;
    inst->lineno     = lineno;
    inst->func       = func;

    inst->nr_stacks = backtrace(inst->c_stacks, PCA_TABLESIZE(inst->c_stacks));

    // set the exception info into stack
    pcintr_stack_t stack = purc_get_stack();
    if (stack) {
        const struct err_msg_info* info = get_error_info(errcode);
        if (info == NULL ||
                ((info->flags & PURC_EXCEPT_FLAGS_REQUIRED) && !exinfo)) {
            return PURC_ERROR_INVALID_VALUE;
        }
        stack->error_except = info->except_atom;
        PURC_VARIANT_SAFE_CLEAR(stack->err_except_info);
        stack->err_except_info = exinfo;
        if (exinfo != PURC_VARIANT_INVALID) {
            purc_variant_ref(exinfo);
        }
        stack->file = file;
        stack->lineno = lineno;
        stack->func = func;
        stack->except = errcode ? 1 : 0; // FIXME: when to set stack->error???
    }

    return PURC_ERROR_OK;
}

int
purc_set_error_with_info_debug(int err_code,
        const char *file, int lineno, const char *func,
        const char *fmt, ...)
{
    char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, sizeof(buf), fmt, ap);
    // TODO: remove below 2 lines
    PC_ASSERT(r >= 0);
    PC_ASSERT((size_t)r < sizeof(buf));
    (void)r;
    va_end(ap);

    purc_variant_t v;
    v = purc_variant_make_string(buf, true);
    PC_ASSERT(v != PURC_VARIANT_INVALID);

    r = purc_set_error_exinfo_with_debug(err_code, v,
            file, lineno, func);

    return r;
}

static LIST_HEAD(_err_msg_seg_list);

/* Error Messages */
#define UNKNOWN_ERR_CODE    "Unknown Error Code"

const struct err_msg_info* get_error_info(int errcode)
{
    struct list_head *p;

    list_for_each(p, &_err_msg_seg_list) {
        struct err_msg_seg *seg = container_of (p, struct err_msg_seg, list);
        if (errcode >= seg->first_errcode && errcode <= seg->last_errcode) {
            return &seg->info[errcode - seg->first_errcode];
        }
    }

    return NULL;
}


const char* purc_get_error_message(int errcode)
{
    const struct err_msg_info* info = get_error_info(errcode);
    return info ? info->msg : UNKNOWN_ERR_CODE;
}

purc_atom_t purc_get_error_exception(int errcode)
{
    const struct err_msg_info* info = get_error_info(errcode);
    return info ? info->except_atom : 0;
}

void pcinst_register_error_message_segment(struct err_msg_seg* seg)
{
    list_add(&seg->list, &_err_msg_seg_list);
    if (seg->info == NULL) {
        return;
    }

    int count = seg->last_errcode - seg->first_errcode + 1;
    for (int i = 0; i < count; i++) {
        seg->info[i].except_atom = purc_get_except_atom_by_id(
                seg->info[i].except_id);
    }
}

static void
dump_stack_by_cmd(int *level, const char *cmd)
{
    char *func = NULL;
    size_t func_len = 0;
    char *file_lineno = NULL;
    size_t file_lineno_len = 0;

    FILE *in = popen(cmd, "r");
    PC_ASSERT(in);
    while (!feof(in)) {
        ssize_t r = 0;
        r = getline(&func, &func_len, in);
        if (r == -1)
            break;

        PC_ASSERT(r > 0);
        func[r-1] = '\0';

        r = getline(&file_lineno, &file_lineno_len, in);
        PC_ASSERT(r > 0);
        file_lineno[r-1] = '\0';

        fprintf(stderr, "%02d: %s (%s)\n", *level, func, file_lineno);
        *level = *level + 1;
    }

    pclose(in);
    free(file_lineno);
    free(func);
}

static void
dump_stacks_ex(char **stacks, regex_t *regex)
{
    char cmd[4096];

    struct pcinst* inst = pcinst_current();
    PC_ASSERT(inst);

    PC_ASSERT(inst->nr_stacks > 0);

    char *p = cmd;
    size_t len = sizeof(cmd);
    int n = snprintf(p, len, "addr2line -Cfsi");
    PC_ASSERT(n>0 && (size_t)n<len);
    p += n;
    len -= n;

    char prev_so[sizeof(inst->so)];
    prev_so[0] = '\0';

    int level = 0;
    int added = 0;

    for (int i=0; i<inst->nr_stacks; ++i) {
        regmatch_t matches[20] = {};
        int r = regexec(regex, stacks[i], PCA_TABLESIZE(matches), matches, 0);
        PC_ASSERT(r != REG_NOMATCH);
        int n;
        n = snprintf(inst->so, sizeof(inst->so), "%.*s",
                matches[1].rm_eo - matches[1].rm_so,
                stacks[i] + matches[1].rm_so);
        if (n<0 || (size_t)n>=sizeof(inst->so))
            break;
        n = snprintf(inst->addr1, sizeof(inst->addr1), "%.*s",
                matches[2].rm_eo - matches[2].rm_so,
                stacks[i] + matches[2].rm_so);
        if (n<0 || (size_t)n>=sizeof(inst->addr1))
            break;
        n = snprintf(inst->addr2, sizeof(inst->addr2), "%.*s",
                matches[3].rm_eo - matches[3].rm_so,
                stacks[i] + matches[3].rm_so);
        if (n<0 || (size_t)n>=sizeof(inst->addr2))
            break;

        void *paddr2 = (void*)strtoll(inst->addr2, NULL, 0);
        Dl_info info = {};
        r = dladdr(paddr2, &info);
        if (r <= 0)
            break;

        if (strcmp(prev_so, inst->so) == 0) {
            n = snprintf(p, len,
                    " 0x%zx",
                    (char*)paddr2 - (char*)info.dli_fbase);
            if (n<0 || (size_t)n>=len) {
                *p = '\0';
                n = 0;
                break;
            }
            added = 1;
        }
        else {
            if (added)
                dump_stack_by_cmd(&level, cmd);
            cmd[0] = '\0';
            added = 0;
            p = cmd;
            len = sizeof(cmd);
            n = snprintf(p, len, "addr2line -Cfsi");
            if (n<0 || (size_t)n>=len) {
                *p = '\0';
                n = 0;
                break;
            }
            p += n;
            len -= n;

            n = snprintf(p, len,
                    " -e '%s' '0x%zx'",
                    inst->so, (char*)paddr2 - (char*)info.dli_fbase);
            if (n<0 || (size_t)n>=len) {
                *p = '\0';
                n = 0;
                break;
            }
            added = 1;
        }
        p += n;
        len -= n;

        n = snprintf(prev_so, sizeof(prev_so), "%s", inst->so);
        if (n<0 || (size_t)n>=sizeof(prev_so))
            break;
    }

    if (added)
        dump_stack_by_cmd(&level, cmd);
}

void pcinst_dump_stack(void)
{
    struct pcinst* inst = pcinst_current();
    PC_ASSERT(inst);

    PC_ASSERT(inst->nr_stacks > 0);

    regex_t regex;
    const char *pattern = "([^(]+)\\(([^(]+)\\) \\[([^]]+)\\]";

    int r = regcomp(&regex, pattern, REG_EXTENDED);
    if (r) {
        char buf[1024];
        regerror(r, &regex, buf, sizeof(buf));
        fprintf(stderr, "regcomp failed: [%s]\n", buf);
        regfree(&regex);
        return;
    }

    PC_ASSERT(r == 0);
    char **stacks = backtrace_symbols(inst->c_stacks, inst->nr_stacks);
    if (stacks)
        dump_stacks_ex(stacks, &regex);

    regfree(&regex);

    free(stacks);
}

