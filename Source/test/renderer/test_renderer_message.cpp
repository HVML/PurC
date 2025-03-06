/*
 * @file test-renderer-good-client.c
 * @author Vincent Wei
 * @date 2025/02/27
 * @brief The program tests renderer 
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
#include "client_thread.h"

#include <gtest/gtest.h>

/* using load within */

static purc_atom_t client_inst;
void my_after_first_run(purc_coroutine_t cor, struct purc_cor_run_info *info)
{
    (void)cor;
    (void)info;

    // create client threads
    create_client_threads(1, "local:///var/tmp/hvml-test-renderer.sock");
}

TEST(renderer, message)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    after_first_run = my_after_first_run;

    char *query = make_query_with_base("base=%s&client=plain");
    run_one_comp_test("renderer/hvml/message-based-server.hvml", query);
    free(query);

#if 0
    purc_inst_ask_to_shutdown(client_inst);
    unsigned int seconds = 0;
    while (purc_atom_to_string(client_inst)) {
        purc_log_info("Wait for termination of client instance...\n");
        sleep(1);
        seconds++;
        ASSERT_LT(seconds, 10);
    }
#endif
}

