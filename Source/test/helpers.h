/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "purc/purc.h"

#include "config.h"

#include <libgen.h>
#include <vector>
#include <iostream>

#include <gtest/gtest.h>

#ifndef _D            /* { */
#define _D(fmt, ...)                                                        \
    purc_log_debug("%s[%d]:%s(): " fmt "\n",                                \
            pcutils_basename(__FILE__), __LINE__, __func__, ##__VA_ARGS__)
#endif                /* } */

#define PRINTF(...)                                                       \
    do {                                                                  \
        fprintf(stdout, "\e[0;32m[          ] \e[0m");                    \
        fprintf(stdout, __VA_ARGS__);                                     \
    } while(false)

#if OS(LINUX) || OS(UNIX)

// get path from env or __FILE__/../<rel> otherwise
#define test_getpath_from_env_or_rel(_path, _len, _env, _rel)           \
do {                                                                    \
    const char *p = getenv(_env);                                       \
    if (p) {                                                            \
        snprintf(_path, _len, "%s", p);                                 \
    } else {                                                            \
        char tmp[4096+1];                                           \
        snprintf(tmp, sizeof(tmp), __FILE__);                           \
        const char *folder = dirname(tmp);                              \
        snprintf(_path, _len, "%s/%s", folder, _rel);                   \
    }                                                                   \
} while (0)

#define test_getbool_from_env_or_default(_env, _def)                    \
({                                                                      \
    bool _v = _def;                                                     \
    const char *p = getenv(_env);                                       \
    if (p && (strcmp(p, "1")==0                                         \
          || strcasecmp(p, "TRUE")==0                                   \
          || strcasecmp(p, "ON")==0))                                   \
    {                                                                   \
        _v = true;                                                      \
    }                                                                   \
    _v;                                                                 \
})

#else

#error "Please define test_getpath_from_env_or_rel for this operating system"

#endif // OS(LINUX) || OS(UNIX)

// Workaround: gtest, INSTANTIATE_TEST_SUITE_P, valgrind
class MemCollector
{
public:
    ~MemCollector(void) {
        cleanup();
    }
public:
    static char* strdup(const char *s) {
        char *p = ::strdup(s);
        get_singleton()->allocates.push_back(p);
        return p;
    }
private:
    static MemCollector* get_singleton(void) {
        static MemCollector         single;
        return &single;
    }
    void cleanup() {
        for (char *v : allocates) {
            free(v);
        }
    }
private:
    std::vector<char *>         allocates;
};

#define APP_NAME            "cn.fmsoft.hvml.test"
#define RUNNER_NAME         "test_init"

class PurCInstance
{
public:
    PurCInstance(unsigned int modules, const char *app = NULL,
            const char *runner = NULL,
            struct purc_instance_extra_info* info = NULL) {
        init_ok = -1;
        if (app == NULL)
            app = APP_NAME;
        if (runner == NULL)
            runner = RUNNER_NAME;
        if (info == NULL) {
            this->info = {};
            info = &this->info;
        }

        if (purc_init_ex (modules, app, runner, info))
            return;

        init_ok = 0;
    }

    PurCInstance(const char *app = NULL, const char *runner = NULL,
            bool enable_remote_fetcher = false) {
        init_ok = -1;
        info = {};
        if (app == NULL)
            app = APP_NAME;
        if (runner == NULL)
            runner = RUNNER_NAME;

        unsigned int modules = enable_remote_fetcher ?
            PURC_MODULE_HVML | PURC_HAVE_FETCHER_R :
            PURC_MODULE_HVML ^ PURC_HAVE_FETCHER;
        if (purc_init_ex (modules, app, runner, &info))
            return;

        init_ok = 0;
    }

    PurCInstance(bool enable_remote_fetcher) {
        init_ok = -1;
        info = {};
        const char *app = APP_NAME;
        const char *runner = RUNNER_NAME;

        unsigned int modules = enable_remote_fetcher ?
            PURC_MODULE_HVML | PURC_HAVE_FETCHER_R :
            PURC_MODULE_HVML;
        if (purc_init_ex (modules, app, runner, &info))
            return;

        init_ok = 0;
    }

    ~PurCInstance(void) {
        if (init_ok == 0) {
            purc_cleanup();
        }
    }

public:
    operator bool(void) const {
        return init_ok == 0;
    }
    struct purc_instance_extra_info* get_info(void) {
        if (init_ok)
            return &info;
        return NULL;
    }

private:
    int init_ok;
    struct purc_instance_extra_info    info;
};

