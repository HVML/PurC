/*
** Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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


#include "purc/purc.h"

#include "../helpers.h"
#include "../tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <gtest/gtest.h>
#include <wtf/Compiler.h>

#define NR_THREADS          10

#if 0
static volatile purc_atom_t other_inst[NR_THREADS];
static volatile purc_atom_t main_inst;
static volatile pthread_t other_threads[NR_THREADS];
#endif

struct thread_arg {
    int         nr;
    const char *rdr_uri;
    sem_t      *wait;
};

static void* general_thread_entry(void* arg)
{
    struct thread_arg *my_arg = (struct thread_arg *)arg;
    char runner_name[32];

    snprintf(runner_name, sizeof(runner_name), "client%d", my_arg->nr);

    char *rdr_uri = strdup(my_arg->rdr_uri);
    struct purc_instance_extra_info inst_info = {
        PURC_RDRCOMM_SOCKET,
        rdr_uri,
        "main",
        "The main workspace",
        NULL,               // workspace_layout
        0,                  // allow_switching_rdr (since 0.9.18)
        0,                  // allow_scaling_by_denisty
        0,                  // keep_alive (since 0.9.22)
    };

    sem_post(my_arg->wait);

    // initial purc instance
    int ret = purc_init_ex(PURC_MODULE_HVML, APP_NAME,
            runner_name, &inst_info);
    free(rdr_uri);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);
    purc_log_info("Return value of purc_init_ex(): %d\n", ret);

    if (ret == 0) {
        char path[PATH_MAX];
        const char *env = "SOURCE_FILES";
        const char *rel = "hvml/client.hvml";
        test_getpath_from_env_or_rel(path, sizeof(path), env, rel);
        purc_log_info("Loading HVML program from: %s\n", path);

        size_t n;
        char *buf = purc_load_file_contents(path, &n);
        if (buf) {
            purc_vdom_t vdom;
            vdom = purc_load_hvml_from_string(buf);
            free(buf);

            if (vdom) {
                purc_coroutine_t cor = purc_schedule_vdom(vdom, 0, NULL,
                    PCRDR_PAGE_TYPE_PLAINWIN, "main", NULL,
                    "hello", NULL, NULL, NULL);
                if (cor) {
                    purc_run((purc_cond_handler)client_cond_handler);
                }
            }
            else {
                purc_log_info("Failed to parse HVML program from: %s\n", path);
            }
        }
        else {
            purc_log_info("Failed to load HVML program from: %s\n", path);
        }
    }

    if (ret == 0)
        purc_cleanup();
    return NULL;
}

int create_thread(int nr, const char *rdr_uri)
{
    int ret;
    struct thread_arg arg;
    pthread_t th;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    arg.nr = nr;
    arg.rdr_uri = rdr_uri;
ALLOW_DEPRECATED_DECLARATIONS_BEGIN
    sem_unlink("sync");
    arg.wait = sem_open("sync", O_CREAT | O_EXCL, 0644, 0);
    if (arg.wait == SEM_FAILED) {
        purc_log_error("failed to create semaphore: %s\n", strerror(errno));
        return -1;
    }
    ret = pthread_create(&th, NULL, general_thread_entry, &arg);
    if (ret) {
        sem_close(arg.wait);
        purc_log_error("failed to create thread: %d\n", nr);
        return -1;
    }
    pthread_attr_destroy(&attr);

    sem_wait(arg.wait);
    sem_close(arg.wait);
ALLOW_DEPRECATED_DECLARATIONS_END

    return ret;
}

int create_client_threads(int n, const char *rdr_uri)
{
    for (int i = 0; i < n; i++) {
        create_thread(i, rdr_uri);
    }

    return 0;
}

