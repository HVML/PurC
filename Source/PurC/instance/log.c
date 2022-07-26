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

bool purc_enable_log(bool enable, bool use_syslog)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return false;

    if (enable) {
#if HAVE(VSYSLOG)
        if (use_syslog) {
            if (inst->fp_log && inst->fp_log != LOG_FILE_SYSLOG) {
                fclose(inst->fp_log);
            }
            inst->fp_log = LOG_FILE_SYSLOG;
        }
        else
#endif
        if (inst->fp_log == NULL) {
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
        }
    }
    else if (inst->fp_log && inst->fp_log != LOG_FILE_SYSLOG) {
        fclose(inst->fp_log);
        inst->fp_log = NULL;
    }

    return true;
}

void purc_log_with_tag(const char *tag, const char *msg, va_list ap)
{
    FILE *fp = NULL;
    struct pcinst* inst = pcinst_current();

    if (inst)
        fp = inst->fp_log;

#if HAVE(VSYSLOG)
    if (fp) {
        if (fp == LOG_FILE_SYSLOG) {
            const char *ident = purc_atom_to_string(inst->endpoint_atom);
            // TODO: we may need a lock to make sure the following two calls
            // can finish atomically.
            openlog(ident, LOG_PID, LOG_USER);
            vsyslog(LOG_INFO, msg, ap);
        }
        else
#endif
        {
            fprintf(fp, "%s >> ", tag);
            vfprintf(fp, msg, ap);
            fflush(fp);
        }
    }
    else {
        const char *ident = "[unknown]";
        if (inst && inst->endpoint_atom)
            ident = purc_atom_to_string(inst->endpoint_atom);

        fprintf(stderr, "%s %s >> ", ident, tag);
        vfprintf(stderr, msg, ap);
#ifndef NDEBUG
        fflush(stderr);
#endif
    }
}

