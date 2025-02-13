/*
 * @file test-socket-dgram.c
 * @author Vincent Wei
 * @date 2025/02/07
 * @brief The program tests $SOCKET and $dgramSocket
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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

#undef NDEBUG

#include "purc/purc.h"
#include "../helpers.h"
#include "../tools.h"

#include <gtest/gtest.h>

static const char *cond_names[] = {
    "PURC_COND_STARTED",
    "PURC_COND_STOPPED",
    "PURC_COND_NOCOR",
    "PURC_COND_IDLE",
    "PURC_COND_COR_CREATED",
    "PURC_COND_COR_ONE_RUN",
    "PURC_COND_COR_EXITED",
    "PURC_COND_COR_TERMINATED",
    "PURC_COND_COR_DESTROYED",
    "PURC_COND_UNK_REQUEST",
    "PURC_COND_UNK_EVENT",
    "PURC_COND_SHUTDOWN_ASKED",
};

static int client_cond_handler(purc_cond_k event, void *arg, void *data)
{
    (void)data;
    purc_log_info("condition: %s\n", cond_names[event]);

    if (event == PURC_COND_STARTED) {
        purc_atom_t sid = (purc_atom_t)(uintptr_t)arg;
        // purc_instance_extra_info *info = (purc_instance_extra_info *)data;

        const char *endpoint = purc_atom_to_string(sid);
        assert(endpoint);

        char host_name[PURC_LEN_HOST_NAME + 1];
        purc_extract_host_name(endpoint, host_name);
        assert(strcmp(host_name, PCRDR_LOCALHOST) == 0);

        char app_name[PURC_LEN_APP_NAME + 1];
        purc_extract_app_name(endpoint, app_name);
        assert(strcmp(app_name, APP_NAME) == 0);

        char run_name[PURC_LEN_RUNNER_NAME + 1];
        purc_extract_runner_name(endpoint, run_name);
        assert(strncmp(run_name, "client", 6) == 0);
    }
    else if (event == PURC_COND_STOPPED) {
        purc_atom_t sid = (purc_atom_t)(uintptr_t)arg;
        assert(sid != 0);

        const char *endpoint = purc_atom_to_string(sid);
        assert(endpoint);

        char host_name[PURC_LEN_HOST_NAME + 1];
        purc_extract_host_name(endpoint, host_name);
        assert(strcmp(host_name, PCRDR_LOCALHOST) == 0);

        char app_name[PURC_LEN_APP_NAME + 1];
        purc_extract_app_name(endpoint, app_name);
        assert(strcmp(app_name, APP_NAME) == 0);

        char run_name[PURC_LEN_RUNNER_NAME + 1];
        purc_extract_runner_name(endpoint, run_name);
        assert(strncmp(run_name, "client", 6) == 0);
    }
    else if (event == PURC_COND_SHUTDOWN_ASKED) {
        return 0;
    }

    return 0;
}

/* using load within */
TEST(socket, basic)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    purc_atom_t client_inst = purc_inst_create_or_get(APP_NAME,
            "client", client_cond_handler, NULL);
    assert(client_inst != 0);

    run_one_comp_test("dvobjs/socket/load-within.hvml");

    purc_inst_ask_to_shutdown(client_inst);

    unsigned int seconds = 0;
    while (purc_atom_to_string(client_inst)) {
        purc_log_info("Wait for termination of client instance...\n");
        sleep(1);
        seconds++;
        ASSERT_LT(seconds, 10);
    }
}

/* using load within */
TEST(socket, local_dgram)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    purc_atom_t client_inst = purc_inst_create_or_get(APP_NAME,
            "client", client_cond_handler, NULL);
    assert(client_inst != 0);

    run_one_comp_test("dvobjs/socket/local-dgram.hvml");

    purc_inst_ask_to_shutdown(client_inst);

    unsigned int seconds = 0;
    while (purc_atom_to_string(client_inst)) {
        purc_log_info("Wait for termination of client instance...\n");
        sleep(1);
        seconds++;
        ASSERT_LT(seconds, 10);
    }
}

/* using load within */
TEST(socket, local_dgram_multiple_datagrams)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    purc_atom_t client_inst = purc_inst_create_or_get(APP_NAME,
            "client", client_cond_handler, NULL);
    assert(client_inst != 0);

    run_one_comp_test("dvobjs/socket/local-dgram-multiple-datagrams.hvml");

    purc_inst_ask_to_shutdown(client_inst);

    unsigned int seconds = 0;
    while (purc_atom_to_string(client_inst)) {
        purc_log_info("Wait for termination of client instance...\n");
        sleep(1);
        seconds++;
        ASSERT_LT(seconds, 10);
    }
}

/* using call within */
TEST(socket, local_dgram_multiple_clients)
{
    PurCInstance purc(false);

    run_one_comp_test("dvobjs/socket/local-dgram-multiple-clients.hvml");
}

/* using call within */
TEST(socket, inet_dgram_multiple_clients)
{
    PurCInstance purc(false);

    run_one_comp_test("dvobjs/socket/inet-dgram-multiple-clients.hvml");
}

/* using load within */
TEST(socket, inet6_dgram_multiple_datagrams)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    purc_atom_t client_inst = purc_inst_create_or_get(APP_NAME,
            "client", client_cond_handler, NULL);
    assert(client_inst != 0);

    run_one_comp_test("dvobjs/socket/inet6-dgram-multiple-datagrams.hvml");

    purc_inst_ask_to_shutdown(client_inst);

    unsigned int seconds = 0;
    while (purc_atom_to_string(client_inst)) {
        purc_log_info("Wait for termination of client instance...\n");
        sleep(1);
        seconds++;
        ASSERT_LT(seconds, 10);
    }
}

