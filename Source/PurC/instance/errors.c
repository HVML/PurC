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
#include <errno.h>

#ifndef NDEBUG                     /* { */
#if OS(LINUX)                      /* { */
#include <execinfo.h>
#include <link.h>
#include <regex.h>
#endif                             /* } */
#endif                             /* } */

static const char *except_messages[] = {
    /* PURC_EXCEPT_OK */
    "OK",
    /* PURC_EXCEPT_ANY */
    "ANY",
    /* PURC_EXCEPT_AGAIN */
    "Try again",
    /* PURC_EXCEPT_BAD_ENCODING */
    "Bad encoding",
    /* PURC_EXCEPT_BAD_HVML_TAG */
    "Bad HVML tag",
    /* PURC_EXCEPT_BAD_HVML_ATTR_NAME */
    "Bad HVML attr name",
    /* PURC_EXCEPT_BAD_HVML_ATTR_VALUE */
    "Bad HVML attr value",
    /* PURC_EXCEPT_BAD_HVML_CONTENT */
    "Bad HVML content",
    /* PURC_EXCEPT_BAD_TARGET_HTML */
    "Bad target html",
    /* PURC_EXCEPT_BAD_TARGET_XGML */
    "Bad target xgml",
    /* PURC_EXCEPT_BAD_TARGET_XML */
    "Bad target xml",
    /* PURC_EXCEPT_BAD_EXPRESSION */
    "Bad expression",
    /* PURC_EXCEPT_BAD_EXECUTOR */
    "Bad executor",
    /* PURC_EXCEPT_BAD_NAME */
    "Bad name",
    /* PURC_EXCEPT_NO_DATA */
    "No data",
    /* PURC_EXCEPT_NOT_ITERABLE */
    "Not iterable",
    /* PURC_EXCEPT_BAD_INDEX */
    "Bad index",
    /* PURC_EXCEPT_NO_SUCH_KEY */
    "No such key",
    /* PURC_EXCEPT_DUPLICATE_KEY */
    "Duplicate key",
    /* PURC_EXCEPT_ARGUMENT_MISSED */
    "Argument missed",
    /* PURC_EXCEPT_WRONG_DATA_TYPE */
    "Wrong data type",
    /* PURC_EXCEPT_INVALID_VALUE */
    "Invalid value",
    /* PURC_EXCEPT_MAX_ITERATION_COUNT */
    "Max iteration count",
    /* PURC_EXCEPT_MAX_RECURSION_DEPTH */
    "Max recursion depth",
    /* PURC_EXCEPT_UNAUTHORIZED */
    "Unauthorized",
    /* PURC_EXCEPT_TIMEOUT */
    "Timeout",
    /* PURC_EXCEPT_E_DOM_FAILURE */
    "eDom failure",
    /* PURC_EXCEPT_LOST_RENDERER */
    "Lost renderer",
    /* PURC_EXCEPT_MEMORY_FAILURE */
    "Memory failure",
    /* PURC_EXCEPT_INTERNAL_FAILURE */
    "Internal failure",
    /* PURC_EXCEPT_EXTERNAL_FAILURE */
    "External (dynamic variant object) failure",
    /* PURC_EXCEPT_ZERO_DIVISION */
    "Zero division",
    /* PURC_EXCEPT_OVERFLOW */
    "Overflow",
    /* PURC_EXCEPT_UNDERFLOW */
    "Underflow",
    /* PURC_EXCEPT_INVALID_FLOAT */
    "Invalid float",
    /* PURC_EXCEPT_ACCESS_DENIED */
    "Access denied",
    /* PURC_EXCEPT_IO_FAILURE */
    "IO failure",
    /* PURC_EXCEPT_TOO_SMALL */
    "Too small",
    /* PURC_EXCEPT_TOO_MANY */
    "Too many",
    /* PURC_EXCEPT_TOO_LONG */
    "Too long",
    /* PURC_EXCEPT_TOO_LARGE */
    "Too large",
    /* PURC_EXCEPT_NOT_DESIRED_ENTITY */
    "Not desired entity",
    /* PURC_EXCEPT_INVALID_OPERAND */
    "Invalid operand",
    /* PURC_EXCEPT_ENTITY_NOT_FOUND */
    "Entity not found",
    /* PURC_EXCEPT_ENTITY_EXISTS */
    "Entity exists",
    /* PURC_EXCEPT_ENTITY_GONE */
    "Entity gone",
    /* PURC_EXCEPT_NO_STORAGE_SPACE */
    "No storage space",
    /* PURC_EXCEPT_BROKEN_PIPE */
    "Broken pipe",
    /* PURC_EXCEPT_CONNECTION_ABORTED */
    "Connection aborted",
    /* PURC_EXCEPT_CONNECTION_REFUSED */
    "Connection refused",
    /* PURC_EXCEPT_CONNECTION_RESET */
    "Connection reset",
    /* PURC_EXCEPT_NAME_RESOLUTION_FAILED */
    "Name resolution failed",
    /* PURC_EXCEPT_REQUEST_FAILED */
    "Request failed",
    /* PURC_EXCEPT_SYS_FAULT */
    "System fault",
    /* PURC_EXCEPT_OS_FAILURE */
    "OS failure",
    /* PURC_EXCEPT_NOT_READY */
    "Not ready",
    /* PURC_EXCEPT_NOT_IMPLEMENTED */
    "Not implemented",
    /* PURC_EXCEPT_UNSUPPORTED */
    "Unsupported",
    /* PURC_EXCEPT_INCOMPLETED */
    "Incompleted",
    /* PURC_EXCEPT_DUPLICATE_NAME */
    "Duplicate name",
    /* PURC_EXCEPT_CHILD_TERMINATED */
    "ChildTerminated",
    /* PURC_EXCEPT_CONFLICT */
    "Conflict",
    /* PURC_EXCEPT_GONE */
    "Gone",
    /* PURC_EXCEPT_MISMATCHED_VERSION */
    "MismatchedVersion",
    /* PURC_EXCEPT_NOT_ACCEPTABLE */
    "NotAcceptable",
    /* PURC_EXCEPT_NOT_ALLOWED */
    "NotAllowed",
    /* PURC_EXCEPT_NOT_FOUND */
    "NotFound",
    /* PURC_EXCEPT_TOO_EARLY */
    "TooEarly",
    /* PURC_EXCEPT_UNAVAILABLE_LEGALLY */
    "UnavailableLegally",
    /* PURC_EXCEPT_UNMET_PRECONDITION */
    "UnmetPrecondition",
    /* PURC_EXCEPT_PROTOCOL_VIOLATION */
    "ProtocolViolation",
    /* PURC_EXCEPT_TLS_FAILURE */
    "TLSFailure",
};


#define _COMPILE_TIME_ASSERT(name, x)               \
           typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(except_msgs,
                PCA_TABLESIZE(except_messages) == PURC_EXCEPT_NR);

#undef _COMPILE_TIME_ASSERT

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
// #define PRINT_ERRCODE
#ifdef PRINT_ERRCODE               /* { */
        pcdebug_backtrace_dump(bt);
#endif                             /* } */
#undef PRINT_ERRCODE
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
// #define PRINT_ERRCODE
#ifdef PRINT_ERRCODE               /* { */
    if (errcode) {
        PC_DEBUGX("errcode: %d[0x%x]", errcode, errcode);
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

    inst->err_element = NULL;

    pcintr_stack_t stack = pcintr_get_stack();
    if (stack) {
        struct pcintr_stack_frame *frame;
        frame = pcintr_stack_get_bottom_frame(stack);
        if (frame) {
            inst->err_element = frame->pos;
        }
    }

    const struct err_msg_info* info = get_error_info(errcode);
    if (info == NULL ||
            ((info->flags & PURC_EXCEPT_FLAGS_REQUIRED) && !exinfo)) {
#ifdef PRINT_ERRCODE               /* { */
        PC_DEBUGX("errcode: %d[0x%x]", errcode, errcode);
#endif                             /* } */
    }
    if (info) {
        inst->error_except = info->except_atom;
    }

    backtrace_snapshot(inst, file, line, func);

    return PURC_ERROR_OK;
#undef PRINT_ERRCODE
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

const char* purc_get_except_message(int except)
{
    if (except >= PURC_EXCEPT_FIRST && except < PURC_EXCEPT_NR) {
        return except_messages[except];
    }
    return NULL;
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

int
purc_error_from_errno (int err_no)
{
    UNUSED_PARAM(err_no);
    switch (err_no)
    {
    case 0:
        return PURC_ERROR_OK;
        break;

#ifdef EEXIST
    case EEXIST:
        return PURC_ERROR_EXISTS;
        break;
#endif

#ifdef EISDIR
    case EISDIR:
        return PCRWSTREAM_ERROR_IS_DIR;
        break;
#endif

#ifdef EACCES
    case EACCES:
        return PURC_ERROR_ACCESS_DENIED;
        break;
#endif


#ifdef ENAMETOOLONG
    case ENAMETOOLONG:
        return PURC_ERROR_TOO_LONG;
        break;
#endif

#ifdef ENOENT
    case ENOENT:
        return PURC_ERROR_NOT_EXISTS;
        break;
#endif

#ifdef ENOTDIR
    case ENOTDIR:
        return PURC_ERROR_NOT_DESIRED_ENTITY;
        break;
#endif

#ifdef EROFS
    case EROFS:
        return PURC_ERROR_ACCESS_DENIED;
        break;
#endif

#ifdef ELOOP
    case ELOOP:
        return PURC_ERROR_TOO_MANY;
        break;
#endif

#ifdef ENOSPC
    case ENOSPC:
        return PCRWSTREAM_ERROR_NO_SPACE;
        break;
#endif

#ifdef ENOMEM
    case ENOMEM:
        return PCRWSTREAM_ERROR_NO_SPACE;
        break;
#endif

#ifdef EINVAL
    case EINVAL:
        return PURC_ERROR_INVALID_VALUE;
        break;
#endif

#ifdef EPERM
    case EPERM:
        return PURC_ERROR_ACCESS_DENIED;
        break;
#endif

#ifdef ENOTSUP
    case ENOTSUP:
        return PURC_ERROR_NOT_SUPPORTED;
        break;
#endif

    /* EOPNOTSUPP == ENOTSUP on Linux, but POSIX considers them distinct */
#if defined (EOPNOTSUPP) && (!defined (ENOTSUP) || (EOPNOTSUPP != ENOTSUP))
    case EOPNOTSUPP:
        return PURC_ERROR_NOT_SUPPORTED;
        break;
#endif

#ifdef EPROTONOSUPPORT
    case EPROTONOSUPPORT:
        return PURC_ERROR_NOT_SUPPORTED;
        break;
#endif

#ifdef ESOCKTNOSUPPORT
    case ESOCKTNOSUPPORT:
        return PURC_ERROR_NOT_SUPPORTED;
        break;
#endif

#ifdef EPFNOSUPPORT
    case EPFNOSUPPORT:
        return PURC_ERROR_NOT_SUPPORTED;
        break;
#endif

#ifdef EAFNOSUPPORT
    case EAFNOSUPPORT:
        return PURC_ERROR_NOT_SUPPORTED;
        break;
#endif

#ifdef ETIMEDOUT
    case ETIMEDOUT:
        return PURC_ERROR_TIMEOUT;
        break;
#endif

#ifdef EBUSY
    case EBUSY:
        return PURC_ERROR_NOT_READY;
        break;
#endif

#ifdef EADDRINUSE
    case EADDRINUSE:
        return PURC_ERROR_CONFLICT;
        break;
#endif

#ifdef EWOULDBLOCK
    case EWOULDBLOCK:
        return PCRWSTREAM_ERROR_IO;
        break;
#endif

/* EWOULDBLOCK == EAGAIN on most systems, but POSIX considers them distinct */
#if defined (EAGAIN) && (!defined (EWOULDBLOCK) || (EWOULDBLOCK != EAGAIN))
    case EAGAIN:
        return PCRWSTREAM_ERROR_IO;
        break;
#endif

#ifdef EMFILE
    case EMFILE:
        return PURC_ERROR_TOO_MANY;
        break;
#endif

#ifdef EHOSTUNREACH
    case EHOSTUNREACH:
        return PURC_ERROR_CONNECTION_REFUSED;
        break;
#endif

#ifdef ENETUNREACH
    case ENETUNREACH:
        return PURC_ERROR_CONNECTION_REFUSED;
        break;
#endif

#ifdef ECONNREFUSED
    case ECONNREFUSED:
        return PURC_ERROR_CONNECTION_REFUSED;
        break;
#endif

#ifdef EPIPE
    case EPIPE:
        return PURC_ERROR_BROKEN_PIPE;
        break;
#endif

#ifdef ECONNRESET
    case ECONNRESET:
        return PURC_ERROR_CONNECTION_RESET;
        break;
#endif

#ifdef ENOTCONN
    case ENOTCONN:
        return PURC_ERROR_NOT_READY;
        break;
#endif

#ifdef EMSGSIZE
    case EMSGSIZE:
        return PURC_ERROR_TOO_LARGE_ENTITY;
        break;
#endif

#ifdef ESPIPE
    case ESPIPE:
        return PURC_ERROR_NOT_SUPPORTED;
        break;
#endif

#ifdef EOVERFLOW
    case EOVERFLOW:
        return PURC_ERROR_TOO_LARGE_ENTITY;
        break;
#endif

#ifdef EFBIG
    case EFBIG:
        return PURC_ERROR_TOO_LARGE_ENTITY;
        break;
#endif

#ifdef EDQUOT
    case EDQUOT:
        return PCRWSTREAM_ERROR_NO_SPACE;
        break;
#endif

#ifdef EBADF
    case EBADF: /* Since 0.9.22 */
        return PURC_ERROR_INVALID_VALUE;
        break;
#endif
    }

    return PURC_ERROR_BAD_SYSTEM_CALL;
}

void
pcinst_dump_err_except_info(purc_variant_t err_except_info)
{
    // FIXME: do NOT forget to check if VARIANT module has been initialized!!!

    if (purc_variant_is_type(err_except_info, PURC_VARIANT_TYPE_STRING)) {
        fprintf(stderr, "err_except_info: %s\n",
                purc_variant_get_string_const(err_except_info));
    }
    else {
        char buf[1024];
        buf[0] = '\0';
        int r = pcvariant_serialize(buf, sizeof(buf), err_except_info);
        PC_ASSERT(r >= 0);
        if ((size_t)r>=sizeof(buf)) {
            buf[sizeof(buf)-1] = '\0';
            buf[sizeof(buf)-2] = '.';
            buf[sizeof(buf)-3] = '.';
            buf[sizeof(buf)-4] = '.';
        }
        fprintf(stderr, "err_except_info: %s\n", buf);
    }
}

void
pcinst_dump_err_info(void)
{
    const struct pcinst* inst = pcinst_current();
    if (!inst) {
        fprintf(stderr, "warning: NO instance at all\n");
        return;
    }

    purc_atom_t     error_except    = inst->error_except;
    purc_variant_t  err_except_info = inst->err_exinfo;
    struct pcdebug_backtrace *bt    = inst->bt;

    if (bt) {
        fprintf(stderr, "error_except: generated @%s[%d]:%s()\n",
                pcutils_basename((char*)bt->file), bt->line, bt->func);
    }
    if (error_except) {
        fprintf(stderr, "error_except: %s\n",
                purc_atom_to_string(error_except));
    }
    if (err_except_info) {
        pcinst_dump_err_except_info(err_except_info);
    }
}

bool
pcinst_is_ignorable_error(int err)
{
    switch (err) {
    case PURC_ERROR_OUT_OF_MEMORY:
        return false;

    default:
        return true;
    }
}
