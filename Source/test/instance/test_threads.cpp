#include "purc.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <gtest/gtest.h>

static purc_atom_t other_inst = 0;
static purc_atom_t main_inst = 0;

static void* general_thread_entry(void* arg)
{
    // initial purc instance
    int ret = purc_init_ex(PURC_MODULE_VARIANT,
            "cn.fmsoft.hvml.purc", "thread", NULL);
    if (ret == PURC_ERROR_OK) {
        purc_enable_log(true, true);
        other_inst =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
        purc_log_info("purc_inst_create_move_buffer returns: %x\n", other_inst);
    }
    sem_post((sem_t *)arg);

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
            purc_log_info("    event:       %s\n",
                    purc_variant_get_string_const(msg->event));

            purc_inst_move_message(main_inst, msg);
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

TEST(instance, threads)
{
    pthread_t th;
    int ret;

    // initial purc
    ret = purc_init_ex(PURC_MODULE_VARIANT, "cn.fmsoft.hvml.purc", "test",
            NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_enable_log(true, true);

    main_inst =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    ASSERT_NE(main_inst, 0);

    // start the other thread
    sem_t wait;
    sem_init(&wait, 0, 0);

    ret = pthread_create(&th, NULL, general_thread_entry, &wait);
    ASSERT_EQ(ret, 0);

    sem_wait(&wait);
    sem_destroy(&wait);

    ASSERT_NE(other_inst, 0);

    size_t n;
    do {
        pcrdr_msg *event;
        event = pcrdr_make_event_message(
                PCRDR_MSG_TARGET_THREAD,
                1,
                "test",
                PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
                PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

        if (purc_inst_move_message(other_inst, event) == 0) {
            purc_log_error("purc_inst_move_message: no recipient\n");
            break;
        }

        pcrdr_release_message(event);

        ret = purc_inst_holding_messages_count(&n);
        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed\n");
            break;
        }
        else if (n > 0) {
            pcrdr_msg *msg = purc_inst_take_away_message(0);

            ASSERT_EQ(msg->target, PCRDR_MSG_TARGET_THREAD);
            ASSERT_EQ(msg->targetValue, 1);
            ASSERT_STREQ(purc_variant_get_string_const(msg->event), "test");

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
}

