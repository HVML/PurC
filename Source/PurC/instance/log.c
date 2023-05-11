/*
 * log.c - The implementation of log facility.
 * Date: 2022/03/08
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2022
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

#include "config.h"

#include "purc-helpers.h"
#include "purc-errors.h"

#if HAVE(SYSLOG_H)
#include <syslog.h>
#endif /* HAVE_SYSLOG_H */

#include "private/instance.h"
#include "private/ports.h"

#include <limits.h>

bool purc_enable_log_ex(unsigned level_mask, purc_log_facility_k facility)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return false;

    if (inst->fp_log &&
            (inst->fp_log != LOG_FILE_SYSLOG &&
             inst->fp_log != stdout && inst->fp_log != stderr)) {
        fclose(inst->fp_log);
        inst->fp_log = NULL;
    }

    inst->log_level_mask = level_mask;
    if (level_mask) {
        switch (facility) {
        case PURC_LOG_FACILITY_FILE: {
            char logfile_path[PATH_MAX + 1];
            int n = snprintf(logfile_path, sizeof(logfile_path),
                    PURC_LOG_FILE_PATH_FORMAT,
                    inst->app_name, inst->runner_name);

            if (n < 0) {
                purc_set_error(PURC_ERROR_OUTPUT);
                return false;
            }
            else if ((size_t)n >= sizeof(logfile_path)) {
                purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
                return false;
            }

            inst->fp_log = fopen(logfile_path, "a");
            if (inst->fp_log == NULL) {
                purc_set_error(PURC_ERROR_BAD_STDC_CALL);
                return false;
            }
            break;
        }

        case PURC_LOG_FACILITY_STDOUT:
            inst->fp_log = stdout;
            break;

        case PURC_LOG_FACILITY_STDERR:
            inst->fp_log = stderr;
            break;

        case PURC_LOG_FACILITY_SYSLOG:
            inst->fp_log = LOG_FILE_SYSLOG;
            break;
        }
    }
    else if (inst->fp_log &&
            (inst->fp_log != LOG_FILE_SYSLOG &&
             inst->fp_log != stdout && inst->fp_log != stderr)) {
        fclose(inst->fp_log);
        inst->fp_log = NULL;
    }

    return true;
}

static struct {
    const char *tag;
    int         sys_level;
} level_info[] = {
    { PURC_LOG_LEVEL_EMERG,     LOG_EMERG },
    { PURC_LOG_LEVEL_ALERT,     LOG_ALERT },
    { PURC_LOG_LEVEL_CRIT,      LOG_CRIT },
    { PURC_LOG_LEVEL_ERR,       LOG_ERR },
    { PURC_LOG_LEVEL_WARNING,   LOG_WARNING },
    { PURC_LOG_LEVEL_NOTICE,    LOG_NOTICE },
    { PURC_LOG_LEVEL_INFO,      LOG_INFO },
    { PURC_LOG_LEVEL_DEBUG,     LOG_DEBUG },
};

/* Make sure the number of log level tags matches the number of log levels */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(levels,
        PCA_TABLESIZE(level_info) == PURC_LOG_LEVEL_nr);

#undef _COMPILE_TIME_ASSERT

void purc_log_with_tag(purc_log_level_k level, const char *tag,
        const char *msg, va_list ap)
{
    FILE *fp;
    struct pcinst* inst = pcinst_current();

    if (inst) {
        if (inst->fp_log == NULL ||
                (inst->log_level_mask & (0x01U << level)) == 0) {
            return;
        }
        fp = inst->fp_log;
    }
    else {
        fp = stdout;
    }

    if (fp == LOG_FILE_SYSLOG) {
#if HAVE(VSYSLOG)
        const char *ident = purc_atom_to_string(inst->endpoint_atom);
        // TODO: we may need a lock to make sure the following two calls
        // can finish atomically.
        openlog(ident, LOG_PID, LOG_USER);
        vsyslog(LOG_INFO | level_info[level].sys_level, msg, ap);
        return;
#else
        fp = stdout;
#endif
    }

    const char *ident = "[unknown]";
    if (inst && inst->endpoint_atom)
        ident = purc_atom_to_string(inst->endpoint_atom);

    fprintf(fp, "%s %s >> ", ident, tag);
    vfprintf(fp, msg, ap);
    if (fp != stderr)
        fflush(fp);
}

void purc_log_with_level(purc_log_level_k level, const char *msg, va_list ap)
{
    purc_log_with_tag(level, level_info[level].tag, msg, ap);
}

