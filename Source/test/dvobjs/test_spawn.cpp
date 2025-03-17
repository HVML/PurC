/*
 * @file test-spawn.c
 * @author Vincent Wei
 * @date 2025/03/11
 * @brief The program tests $SYS.spawn(), $SYS.pipe(), $SYS.close(),
 *      and $STREAM.from()
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

TEST(spawn, bc)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/spawn/spawn-bc.hvml");
}

TEST(spawn, plain_websocket_worker)
{
    PurCInstance purc(false);

    purc_atom_t client_inst = purc_inst_create_or_get(APP_NAME,
            "client", client_cond_handler, NULL);
    assert(client_inst != 0);

    char *query = make_query_with_base("client=plain&secure=false&base=%s");
    run_one_comp_test("dvobjs/spawn/spawn-websocket-worker.hvml", query);
    free(query);

    purc_inst_ask_to_shutdown(client_inst);

    unsigned int seconds = 0;
    while (purc_atom_to_string(client_inst)) {
        purc_log_info("Wait for termination of client instance...\n");
        sleep(1);
        seconds++;
        ASSERT_LT(seconds, 10);
    }
}

#if HAVE(OPENSSL)
TEST(spawn, secure_websocket_worker)
{
    PurCInstance purc(false);

    purc_atom_t client_inst = purc_inst_create_or_get(APP_NAME,
            "client", client_cond_handler, NULL);
    assert(client_inst != 0);

    char *query = make_query_with_base("client=secure&secure=true&base=%s");
    run_one_comp_test("dvobjs/spawn/spawn-websocket-worker.hvml", query);
    free(query);

    purc_inst_ask_to_shutdown(client_inst);

    unsigned int seconds = 0;
    while (purc_atom_to_string(client_inst)) {
        purc_log_info("Wait for termination of client instance...\n");
        sleep(1);
        seconds++;
        ASSERT_LT(seconds, 10);
    }
}
#endif // HAVE(OPENSSL)
