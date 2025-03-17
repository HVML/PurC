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

static void
my_after_first_run_plain(purc_coroutine_t cor, struct purc_cor_run_info *info)
{
    (void)cor;
    (void)info;

    // create client threads
    create_client_threads(1, "ws://localhost:8080/renderer");
}

TEST(renderer, plain_websocket)
{
    PurCInstance purc(false);

    purc_set_local_data(FN_AFTER_FIRST_RUN,
            (uintptr_t)my_after_first_run_plain, NULL);

    char *query = make_query_with_base("secure=false&base=%s&docLoadingMethod=direct");
    run_one_comp_test("renderer/hvml/websocket-based-server.hvml", query);
    free(query);

}

static void
my_after_first_run_secure(purc_coroutine_t cor, struct purc_cor_run_info *info)
{
    (void)cor;
    (void)info;

    // create client threads
    create_client_threads(1, "wss://localhost:8080/renderer");
}

TEST(renderer, secure_websocket)
{
    PurCInstance purc(false);

    purc_set_local_data(FN_AFTER_FIRST_RUN,
            (uintptr_t)my_after_first_run_secure, NULL);

    char *query = make_query_with_base("secure=true&base=%s&docLoadingMethod=direct");
    run_one_comp_test("renderer/hvml/websocket-based-server.hvml", query);
    free(query);
}

