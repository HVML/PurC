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
#include <gtest/gtest.h>

#define ATOM_BITS_NR        (sizeof(purc_atom_t) << 3)
#define BUCKET_BITS(bucket)       \
    ((purc_atom_t)bucket << (ATOM_BITS_NR - PURC_ATOM_BUCKET_BITS))

TEST(instance, syslog)
{
    // initial purc
    int ret = purc_init_ex(PURC_MODULE_VARIANT, "cn.fmsoft.hvml.purc", "test",
            NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_atom_t endpoint_atom = BUCKET_BITS(PURC_ATOM_BUCKET_USER) | 1;
    const char *endpoint = purc_atom_to_string(endpoint_atom);

    char host_name[PURC_LEN_HOST_NAME + 1];
    purc_extract_host_name(endpoint, host_name);

    char app_name[PURC_LEN_APP_NAME + 1];
    purc_extract_app_name(endpoint, app_name);

    char runner_name[PURC_LEN_RUNNER_NAME + 1];
    purc_extract_runner_name(endpoint, runner_name);

    ASSERT_STREQ(host_name, "localhost");
    ASSERT_STREQ(app_name, "cn.fmsoft.hvml.purc");
    ASSERT_STREQ(runner_name, "test");

    purc_enable_log(true, true);
    purc_log_debug("You will see this message in syslog with full endpoint name\n");

    purc_cleanup();
}

