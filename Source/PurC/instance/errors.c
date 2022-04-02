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

#include "private/debug.h"
#include "private/errors.h"
#include "private/instance.h"

#include "private/interpreter.h" // FIXME:

#include <stdarg.h>

#ifndef NDEBUG                     /* { */
#if OS(LINUX)                      /* { */
#include <execinfo.h>
#include <link.h>
#include <regex.h>
#endif                             /* } */
#endif                             /* } */

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

static void
backtrace_release(struct pcdebug_backtrace *bt)
{
    if (!bt)
        return;

    PC_ASSERT(bt->refc == 0);
}

static void
backtrace_destroy(struct pcdebug_backtrace *bt)
{
    if (!bt)
        return;

    backtrace_release(bt);
    free(bt);
}

struct pcdebug_backtrace*
pcdebug_backtrace_ref(struct pcdebug_backtrace *bt)
{
    bt->refc += 1;
    return bt;
}

void
pcdebug_backtrace_unref(struct pcdebug_backtrace *bt)
{
    PC_ASSERT(bt->refc > 0);
    bt->refc -= 1;
    if (bt->refc > 0)
        return;

    backtrace_destroy(bt);
}

static void
backtrace_snapshot(struct pcinst *inst, const char *file, int line,
        const char *func)
{
    if (inst->bt) {
        PC_ASSERT(inst->bt->refc > 0);
        if (inst->bt->refc > 1) {
            pcdebug_backtrace_unref(inst->bt);
            inst->bt = NULL;
        }
    }

    if (inst->bt == NULL) {
        inst->bt = (struct pcdebug_backtrace*)malloc(sizeof(*inst->bt));
        if (inst->bt == NULL)
            return;
    }

    struct pcdebug_backtrace *bt = inst->bt;

    do {
        bt->file      = file;
        bt->line      = line;
        bt->func      = func;

#ifndef NDEBUG                     /* { */
#if OS(LINUX)                      /* { */
        bt->nr_stacks = backtrace(bt->c_stacks, PCA_TABLESIZE(bt->c_stacks));
#endif                             /* } */
#endif                             /* } */
        bt->refc = 1;
        return;
    } while (0);

    backtrace_destroy(bt);
}

static int
set_error_exinfo_with_debug(int errcode, purc_variant_t exinfo,
        const char *file, int line, const char *func)
{
#ifndef NDEBUG                     /* { */
    if (0 && errcode) {
        PC_DEBUGX("%s[%d]:%s(): %d",
                pcutils_basename((char*)file), line, func, errcode);
        if (exinfo != PURC_VARIANT_INVALID)
            PRINT_VARIANT(exinfo);
    }
#endif                             /* } */

    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        _noinst_errcode = errcode;
        return PURC_ERROR_NO_INSTANCE;
    }

    inst->errcode = errcode;
    PURC_VARIANT_SAFE_CLEAR(inst->err_exinfo);
    inst->err_exinfo = exinfo;

    const struct err_msg_info* info = get_error_info(errcode);
    if (info == NULL ||
            ((info->flags & PURC_EXCEPT_FLAGS_REQUIRED) && !exinfo)) {
#ifndef NDEBUG                     /* { */
        PC_DEBUGX("%s[%d]:%s(): %d",
                pcutils_basename((char*)file), line, func, errcode);
#endif                             /* } */
    }
    if (info) {
        inst->error_except = info->except_atom;
    }

    backtrace_snapshot(inst, file, line, func);

    // set the exception info into stack
    pcintr_stack_t stack = pcintr_get_stack();
    if (stack) {
        struct pcintr_exception *exception = &stack->exception;

        exception->error_except = info->except_atom;
        PURC_VARIANT_SAFE_CLEAR(exception->exinfo);
        exception->exinfo = exinfo;
        if (exinfo != PURC_VARIANT_INVALID) {
            purc_variant_ref(exinfo);
        }
        if (exception->bt) {
            pcdebug_backtrace_unref(exception->bt);
            exception->bt = NULL;
        }

        if (inst->bt)
            exception->bt = pcdebug_backtrace_ref(inst->bt);
        stack->except = errcode ? 1 : 0; // FIXME: when to set stack->error???
    }

    return PURC_ERROR_OK;
}

int
purc_set_error_exinfo_with_debug(int errcode, purc_variant_t exinfo,
        const char *file, int lineno, const char *func)
{
    // NOTE: this is intentionally!!!
    return set_error_exinfo_with_debug(errcode, exinfo, file, lineno, func);
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
    // FIXME: set-error-recursive
    v = purc_variant_make_string(buf, true);
    PC_ASSERT(v != PURC_VARIANT_INVALID);

    r = set_error_exinfo_with_debug(err_code, v,
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

#ifndef NDEBUG                     /* { */
#if OS(LINUX)                      /* { */
static void
dump_stack_by_cmd(int *level, const char *cmd)
{
    char *func = NULL;
    size_t func_len = 0;
    char *file_line = NULL;
    size_t file_line_len = 0;

    FILE *in = popen(cmd, "r");
    PC_ASSERT(in);
    while (!feof(in)) {
        ssize_t r = 0;
        r = getline(&func, &func_len, in);
        if (r == -1)
            break;

        PC_ASSERT(r > 0);
        func[r-1] = '\0';

        r = getline(&file_line, &file_line_len, in);
        PC_ASSERT(r > 0);
        file_line[r-1] = '\0';

        fprintf(stderr, "%02d: %s (%s)\n", *level, func, file_line);
        *level = *level + 1;
    }

    pclose(in);
    free(file_line);
    free(func);
}

static void
dump_stacks_ex(char **stacks, int nr_stacks, regex_t *regex)
{
    char cmd[4096];

    char *p = cmd;
    size_t len = sizeof(cmd);
    int n = snprintf(p, len, "addr2line -Cfsi");
    PC_ASSERT(n>0 && (size_t)n<len);
    p += n;
    len -= n;

    char  so[1024];
    char  addr1[256];
    char  addr2[64];

    char prev_so[sizeof(so)];
    prev_so[0] = '\0';

    int level = 0;
    int added = 0;

    for (int i=0; i<nr_stacks; ++i) {
        regmatch_t matches[20] = {};
        int r = regexec(regex, stacks[i], PCA_TABLESIZE(matches), matches, 0);
        PC_ASSERT(r != REG_NOMATCH);
        int n;
        n = snprintf(so, sizeof(so), "%.*s",
                matches[1].rm_eo - matches[1].rm_so,
                stacks[i] + matches[1].rm_so);
        if (n<0 || (size_t)n>=sizeof(so))
            break;
        n = snprintf(addr1, sizeof(addr1), "%.*s",
                matches[2].rm_eo - matches[2].rm_so,
                stacks[i] + matches[2].rm_so);
        if (n<0 || (size_t)n>=sizeof(addr1))
            break;
        n = snprintf(addr2, sizeof(addr2), "%.*s",
                matches[3].rm_eo - matches[3].rm_so,
                stacks[i] + matches[3].rm_so);
        if (n<0 || (size_t)n>=sizeof(addr2))
            break;

        void *paddr2 = (void*)strtoll(addr2, NULL, 0);
        Dl_info info = {};
        r = dladdr(paddr2, &info);
        if (r <= 0)
            break;

        if (strcmp(prev_so, so) == 0) {
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
                    so, (char*)paddr2 - (char*)info.dli_fbase);
            if (n<0 || (size_t)n>=len) {
                *p = '\0';
                n = 0;
                break;
            }
            added = 1;
        }
        p += n;
        len -= n;

        n = snprintf(prev_so, sizeof(prev_so), "%s", so);
        if (n<0 || (size_t)n>=sizeof(prev_so))
            break;
    }

    if (added)
        dump_stack_by_cmd(&level, cmd);
}
#endif                             /* } */
#endif                             /* } */

void pcinst_dump_stack(void)
{
    struct pcinst* inst = pcinst_current();
    PC_ASSERT(inst);

    pcdebug_backtrace_dump(inst->bt);
}

void
pcdebug_backtrace_dump(struct pcdebug_backtrace *bt)
{
    if (!bt)
        return;

#ifndef NDEBUG                     /* { */
#if OS(LINUX)                      /* { */
    if (bt->nr_stacks == 0)
        return;

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
    char **stacks;
    stacks = backtrace_symbols(bt->c_stacks, bt->nr_stacks);
    if (stacks)
        dump_stacks_ex(stacks, bt->nr_stacks, &regex);

    regfree(&regex);

    free(stacks);
#endif                             /* } */
#endif                             /* } */
}

