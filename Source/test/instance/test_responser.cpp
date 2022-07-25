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

#include "purc.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <gtest/gtest.h>
#include <wtf/Compiler.h>

#define NR_MAX_REQUESTS         100
#define NR_REQUESTERS           10
#define APP_NAME                "cn.fmsoft.purc.test"

static volatile purc_atom_t inst_requesters[NR_REQUESTERS];
static volatile purc_atom_t inst_responser;
static volatile pthread_t thread_requesters[NR_REQUESTERS];

struct thread_arg {
    sem_t  *wait;
    int     nr;
};

static void* general_thread_entry(void* arg)
{
    struct thread_arg *my_arg = (struct thread_arg *)arg;
    sem_t *sw = my_arg->wait;
    int    nr = my_arg->nr;

    char runner_name[32];

    sprintf(runner_name, "requester%d", nr);

    // initial purc instance
    int ret = purc_init_ex(PURC_MODULE_VARIANT, APP_NAME,
            runner_name, NULL);
    assert(ret == PURC_ERROR_OK);

    if (ret == PURC_ERROR_OK) {
        purc_enable_log(true, false);
        inst_requesters[nr] =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
        purc_log_info("purc_inst_create_move_buffer returns: %x\n",
                inst_requesters[nr]);
    }
    sem_post(sw);

    size_t n;
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

                if (strcmp(event_name, "quit") == 0 &&
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
            else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
                purc_log_info("got a response message for request: %s from %s\n",
                        purc_variant_get_string_const(msg->requestId),
                        purc_variant_get_string_const(msg->sourceURI));

                if (strcmp(runner_name,
                        purc_variant_get_string_const(msg->requestId))) {
                    purc_log_error("requestId in response not matched\n");
                }

                if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON) {
                    purc_log_error("dataType in response not matched\n");
                }

                int64_t i64;
                purc_variant_cast_to_longint(msg->data, &i64, false);
                if ((int)i64 == nr) {
                    purc_log_error("data in response not matched\n");
                }
            }

            pcrdr_release_message(msg);
        }
        else {
            usleep(10000);  // 10m

            pcrdr_msg *request = pcrdr_make_request_message(
                PCRDR_MSG_TARGET_INSTANCE, inst_responser,
                "ping", runner_name, runner_name,
                PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
                NULL,
                PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

            request->dataType = PCRDR_MSG_DATA_TYPE_JSON;
            request->data = purc_variant_make_longint((int64_t)nr);
            purc_inst_move_message(inst_responser, request);
            pcrdr_release_message(request);
        }

    } while (true);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %d messages discarded\n", (int)n);

    purc_cleanup();
    return NULL;
}

static int create_requester(int nr)
{
    int ret;
    struct thread_arg arg;
    pthread_t th;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    arg.nr = nr;
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

    thread_requesters[nr] = th;

    return ret;
}

TEST(instance, responser)
{
    int ret;

    // initial purc
    ret = purc_init_ex(PURC_MODULE_VARIANT, APP_NAME, "responser",
            NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_enable_log(true, false);

    inst_responser =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    ASSERT_NE(inst_responser, 0);

    for (int i = 1; i < NR_REQUESTERS; i++) {
        create_requester(i);
        ASSERT_NE(inst_requesters[i], 0);
    }

    size_t n;
    int nr_got = 0;
    do {
        ret = purc_inst_holding_messages_count(&n);
        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed\n");
            break;
        }
        else if (n > 0) {
            pcrdr_msg *request = purc_inst_take_away_message(0);

            ASSERT_EQ(request->type, PCRDR_MSG_TYPE_REQUEST);
            ASSERT_EQ(request->target, PCRDR_MSG_TARGET_INSTANCE);
            ASSERT_EQ(request->targetValue, inst_responser);
            ASSERT_EQ(request->elementType, PCRDR_MSG_ELEMENT_TYPE_VOID);
            ASSERT_EQ(request->dataType, PCRDR_MSG_DATA_TYPE_JSON);
            ASSERT_STREQ(purc_variant_get_string_const(request->operation), "ping");

            /* sourceURI contains only the runner name of the requester */
            const char *source_uri;
            source_uri = purc_variant_get_string_const(request->sourceURI);
            ASSERT_STRNE(source_uri, PCRDR_SOURCEURI_ANONYMOUS);

            char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];
            purc_atom_t request_atom;

            purc_assemble_endpoint_name(PCRDR_LOCALHOST,
                        APP_NAME, source_uri, endpoint_name);

            request_atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
                    endpoint_name);
            ASSERT_NE(request_atom, 0);

            pcrdr_msg *response = pcrdr_make_void_message();

            response->type = PCRDR_MSG_TYPE_RESPONSE;
            response->requestId = purc_variant_ref(request->requestId);
            response->sourceURI = purc_variant_make_string_static("responser", false);
            response->retCode = 200;
            response->resultValue = 0;
            response->dataType = PCRDR_MSG_DATA_TYPE_JSON;
            response->data = purc_variant_ref(request->data);

            pcrdr_release_message(request);

            purc_inst_move_message(request_atom, response);
            pcrdr_release_message(response);

            nr_got++;
            if (nr_got == NR_MAX_REQUESTS) {
                pcrdr_msg *event;
                event = pcrdr_make_event_message(
                        PCRDR_MSG_TARGET_INSTANCE,
                        0,
                        "quit", "responser",
                        PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
                        PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

                // broadcast the event
                if (purc_inst_move_message(PURC_EVENT_TARGET_BROADCAST, event) == 0) {
                    purc_log_error("purc_inst_move_message: no recipient\n");
                }
                pcrdr_release_message(event);
                break;
            }
        }
        else {
            usleep(10000);  // 10m
        }

    } while (true);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %d messages discarded\n", (int)n);

    for (int i = 1; i < NR_REQUESTERS; i++) {
        pthread_join(thread_requesters[i], NULL);
    }

    purc_cleanup();
}

