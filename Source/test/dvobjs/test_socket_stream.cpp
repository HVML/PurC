/*
 * @file test-socket-stream.c
 * @author Vincent Wei
 * @date 2025/02/13
 * @brief The program tests $SOCKET and $STREAM
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

TEST(socket, local_stream_default)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    purc_atom_t client_inst = purc_inst_create_or_get(APP_NAME,
            "client", client_cond_handler, NULL);
    assert(client_inst != 0);

    run_one_comp_test("dvobjs/socket/local-stream.hvml", "mode=default");

    purc_inst_ask_to_shutdown(client_inst);

    unsigned int seconds = 0;
    while (purc_atom_to_string(client_inst)) {
        purc_log_info("Wait for termination of client instance...\n");
        sleep(1);
        seconds++;
        ASSERT_LT(seconds, 10);
    }
}

/* TODO: A test for blocked socket, and use sockopt() */
TEST(socket, local_stream_blocked)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    purc_atom_t client_inst = purc_inst_create_or_get(APP_NAME,
            "client", client_cond_handler, NULL);
    assert(client_inst != 0);

    run_one_comp_test("dvobjs/socket/local-stream-blocked.hvml", NULL);

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
TEST(socket, local_stream_multiple_clients)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/socket/local-stream-multiple-clients.hvml");
}

/* using call within */
TEST(socket, local_stream_multiple_clients_blocked)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/socket/local-stream-multiple-clients-blocked.hvml");
}

/* using call within */
TEST(socket, inet_stream_multiple_clients)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/socket/inet-stream-multiple-clients.hvml", "scheme=inet");
}

/* using call within */
TEST(socket, inet4_stream_multiple_clients)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/socket/inet-stream-multiple-clients.hvml", "scheme=inet4");
}

/* using call within */
TEST(socket, inet6_stream_multiple_clients)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/socket/inet-stream-multiple-clients.hvml", "scheme=inet6");
}

/* using call within */
TEST(socket, inet_stream_multiple_clients_blocked)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/socket/inet-stream-multiple-clients-blocked.hvml", "scheme=inet");
}

/* using call within */
TEST(socket, inet4_stream_multiple_clients_blocked)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/socket/inet-stream-multiple-clients-blocked.hvml", "scheme=inet4");
}

/* using call within */
TEST(socket, inet6_stream_multiple_clients_blocked)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/socket/inet-stream-multiple-clients-blocked.hvml", "scheme=inet6");
}

