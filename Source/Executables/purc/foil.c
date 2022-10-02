/*
 * @file foil.c
 * @author Vincent Wei
 * @date 2022/10/02
 * @brief The built-in text-mode renderer.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#include "foil.h"

static void event_loop(void)
{
    size_t n;
    int ret;

    do {
        ret = purc_inst_holding_messages_count(&n);

        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed: %d\n", ret);
        }
        else if (n > 0) {
            purc_log_info("purc_inst_holding_messages_count returns: %d\n", (int)n);

            pcrdr_msg *msg = purc_inst_take_away_message(0);
            if (msg->type == PCRDR_MSG_TYPE_EVENT) {
                const char *event_name;
                event_name = purc_variant_get_string_const(msg->eventName);

                if (strcmp(event_name, "init") == 0 &&
                        msg->target == PCRDR_MSG_TARGET_INSTANCE &&
                        msg->targetValue == 0) {
                    purc_log_info("got the quit from %s\n",
                            purc_variant_get_string_const(msg->sourceURI));
                    pcrdr_release_message(msg);
                    break;
                }
                else if (strcmp(event_name, "quit") == 0 &&
                        msg->target == PCRDR_MSG_TARGET_INSTANCE &&
                        msg->targetValue == 0) {
                    purc_log_info("got the quit from %s\n",
                            purc_variant_get_string_const(msg->sourceURI));
                    pcrdr_release_message(msg);
                    break;
                }
                else {
                    purc_log_info("got an event message not interested in:\n");
                    purc_log_info("    type:        %d\n", msg->type);
                    purc_log_info("    target:      %d\n", msg->target);
                    purc_log_info("    targetValue: %d\n", (int)msg->targetValue);
                    purc_log_info("    eventName:   %s\n", event_name);
                    purc_log_info("    sourceURI: %s\n",
                            purc_variant_get_string_const(msg->sourceURI));
                }
            }
            else if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
                purc_log_info("got a request message for %s from %s\n",
                        purc_variant_get_string_const(msg->requestId),
                        purc_variant_get_string_const(msg->sourceURI));
            }
            else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
                purc_log_info("got a response message for %s from %s\n",
                        purc_variant_get_string_const(msg->requestId),
                        purc_variant_get_string_const(msg->sourceURI));
            }

            pcrdr_release_message(msg);
        }
        else {
            pcutils_usleep(10000);  // 10m
        }

    } while(true);
}

struct thread_arg {
    sem_t          *wait;
    const char     *app_name;
    const char     *run_name;
    purc_atom_t     rid;
};

static void* foil_thread_entry(void* arg)
{
    struct thread_arg *my_arg = (struct thread_arg *)arg;
    sem_t *sw = my_arg->wait;

    my_arg->rid = 0;
    // initial purc instance
    int ret = purc_init_ex(PURC_MODULE_EJSON | PURC_MODULE_HTML,
            my_arg->app_name, my_arg->run_name, NULL);
    if (ret == PURC_ERROR_OK) {
        my_arg->rid = purc_inst_create_move_buffer(
                PCINST_MOVE_BUFFER_FLAG_NONE, 16);
    }

    purc_enable_log(true, true);

    sem_post(sw);

    if (my_arg->rid) {
        event_loop();
        purc_inst_destroy_move_buffer();
    }

    if (ret == PURC_ERROR_OK) {
        purc_cleanup();
    }

    return NULL;
}

#define SEM_NAME_SYNC_START     "sync-foil-start"

purc_atom_t foil_init(const char *rdr_uri)
{
    int ret;
    struct thread_arg arg;
    pthread_t th;

    char app_name[PURC_LEN_APP_NAME + 1];
    if (purc_extract_app_name(rdr_uri, app_name) == 0) {
        purc_log_error("bad renderer URI: %s\n", rdr_uri);
        return 0;
    }

    char run_name[PURC_LEN_RUNNER_NAME + 1];
    if (purc_extract_runner_name(rdr_uri, app_name) == 0) {
        purc_log_error("bad renderer URI: %s\n", rdr_uri);
        return 0;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

ALLOW_DEPRECATED_DECLARATIONS_BEGIN
    sem_unlink(SEM_NAME_SYNC_START);
    arg.wait = sem_open(SEM_NAME_SYNC_START, O_CREAT | O_EXCL, 0644, 0);
    if (arg.wait == SEM_FAILED) {
        purc_log_error("failed to create semaphore: %s\n", strerror(errno));
        goto failed;
    }

    arg.app_name = app_name;
    arg.run_name = run_name;
    ret = pthread_create(&th, &attr, foil_thread_entry, &arg);
    if (ret) {
        purc_log_error("failed to create thread for built-in renderer: %s\n",
                strerror(errno));
        sem_close(arg.wait);
        goto failed;
    }
    pthread_attr_destroy(&attr);

    sem_wait(arg.wait);
    sem_close(arg.wait);
ALLOW_DEPRECATED_DECLARATIONS_END

    return arg.rid;

failed:
    pthread_attr_destroy(&attr);
    return 0;
}

