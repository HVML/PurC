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


#include "purc/purc.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <gtest/gtest.h>
#include <wtf/Compiler.h>

#define NR_THREADS          10

static volatile purc_atom_t other_inst[NR_THREADS];
static volatile purc_atom_t main_inst;
static volatile pthread_t other_threads[NR_THREADS];

struct thread_arg {
    sem_t  *wait;
    int     nr;
};

static const char *jsons[] = {
    "true",
    "100",
    "[100, 200, 300]",
    "[100, 200, 300, [100, 200, 300]]",
    "{ }",
    "[ ]",
    "{ 'r': 0, 'g': 0, 'b': 0 }",
    "[ {x: 0 } ]",
    "{ name: 'PurC', os: ['Linux', 'macOS', 'HybridOS', 'Windows'], emptyObject: {} }",
    "{ 'darkMode': true, 'backgroudColor': { 'r': 0, 'g': 0, 'b': 0, emptyArray: [{x: 1}], emptyObject: {} }, emptyArray: [] }",
};

static void* general_thread_entry(void* arg)
{
    struct thread_arg *my_arg = (struct thread_arg *)arg;
    char runner_name[32];

    sprintf(runner_name, "thread%d", my_arg->nr);
    int th_no = my_arg->nr;

    // initial purc instance
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.purc.test",
            runner_name, NULL);
    assert(ret == PURC_ERROR_OK);

    if (ret == PURC_ERROR_OK) {
        purc_enable_log(false, false);
        other_inst[my_arg->nr] =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
        purc_log_info("purc_inst_create_move_buffer returns: %x\n",
                other_inst[my_arg->nr]);
    }
    sem_post(my_arg->wait);

    size_t n;
    do {
        ret = purc_inst_holding_messages_count(&n);

        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed: %d\n", ret);
        }
        else if (n > 0) {
            purc_log_info("purc_inst_holding_messages_count returns: %d\n", (int)n);

            pcrdr_msg *msg = purc_inst_take_away_message(0);
            purc_log_info("purc_inst_take_away_message returns a message:\n");
            purc_log_info("    type:        %d\n", msg->type);
            purc_log_info("    target:      %d\n", msg->target);
            purc_log_info("    targetValue: %d\n", (int)msg->targetValue);
            purc_log_info("    eventName:   %s\n",
                    purc_variant_get_string_const(msg->eventName));
            purc_log_info("    sourceURI: %s\n",
                    purc_variant_get_string_const(msg->sourceURI));

            purc_log_info("use the json: %s\n", jsons[th_no]);
            purc_variant_t data = purc_variant_make_from_json_string(
                    jsons[th_no], strlen(jsons[th_no]));

            msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
            msg->data = purc_variant_ref(data);
            purc_inst_move_message(main_inst, msg);
            purc_variant_unref(data);
            pcrdr_release_message(msg);
            break;
        }
        else {
            usleep(10000);  // 10m
        }

    } while (true);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %d messages discarded\n", (int)n);

    purc_cleanup();
    return NULL;
}

static int create_thread(int nr)
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

    other_threads[nr] = th;

    return ret;
}

TEST(instance, thread)
{
    int ret;

    // initial purc
    ret = purc_init_ex(PURC_MODULE_VARIANT, "cn.fmsoft.purc.test", "threads",
            NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_enable_log(true, false);

    main_inst =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    ASSERT_NE(main_inst, 0);

    create_thread(0);
    ASSERT_NE(other_inst[0], 0);

    pcrdr_msg *event;
    event = pcrdr_make_event_message(
            PCRDR_MSG_TARGET_INSTANCE,
            1,
            "test", NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

    if (purc_inst_move_message(other_inst[0], event) == 0) {
        purc_log_error("purc_inst_move_message: no recipient\n");
    }
    pcrdr_release_message(event);

    size_t n;
    do {

        ret = purc_inst_holding_messages_count(&n);
        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed\n");
            break;
        }
        else if (n > 0) {
            pcrdr_msg *msg = purc_inst_take_away_message(0);

            ASSERT_EQ(msg->target, PCRDR_MSG_TARGET_INSTANCE);
            ASSERT_EQ(msg->targetValue, 1);
            ASSERT_STREQ(purc_variant_get_string_const(msg->eventName), "test");

            pcrdr_release_message(msg);
            break;
        }
        else {
            usleep(10000);  // 10m
        }

    } while (true);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %d messages discarded\n", (int)n);

    pthread_join(other_threads[0], NULL);

    purc_cleanup();
}

TEST(instance, threads)
{
    int ret;

    // initial purc
    ret = purc_init_ex(PURC_MODULE_VARIANT, "cn.fmsoft.purc.test", "threads",
            NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_enable_log(true, false);

    main_inst =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    ASSERT_NE(main_inst, 0);

    for (int i = 1; i < NR_THREADS; i++) {
        create_thread(i);
        ASSERT_NE(other_inst[i], 0);
    }

    pcrdr_msg *event;
    event = pcrdr_make_event_message(
            PCRDR_MSG_TARGET_INSTANCE,
            1,
            "test", NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

    // broadcast the event
    if (purc_inst_move_message(PURC_EVENT_TARGET_BROADCAST, event) == 0) {
        purc_log_error("purc_inst_move_message: no recipient\n");
    }
    pcrdr_release_message(event);

    size_t n;
    int nr_got = 0;
    do {
        ret = purc_inst_holding_messages_count(&n);
        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed\n");
            break;
        }
        else if (n > 0) {
            pcrdr_msg *msg = purc_inst_take_away_message(0);

            ASSERT_EQ(msg->target, PCRDR_MSG_TARGET_INSTANCE);
            ASSERT_EQ(msg->targetValue, 1);
            ASSERT_STREQ(purc_variant_get_string_const(msg->eventName), "test");

            pcrdr_release_message(msg);

            nr_got++;
            if (nr_got == NR_THREADS)
                break;
        }
        else {
            usleep(10000);  // 10m
        }

    } while (true);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %d messages discarded\n", (int)n);

    for (int i = 1; i < NR_THREADS; i++) {
        pthread_join(other_threads[i], NULL);
    }

    purc_cleanup();
}

