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

#include "config.h"
#include "purc/purc.h"

#include "TestDVObj.h"
#include "../helpers.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>


TEST(dvobjs, stream)
{
    const char *files_to_remove_before_testing[] = {
        "/tmp/test_stream_not_exist",
        "/tmp/test_stream_bytes",
        "/tmp/test_stream_lines",
        "/tmp/test_stream_struct",
        "/tmp/test_stream_seek",
        "/tmp/test_stream_readstring",
    };

    for (size_t i = 0; i < PCA_TABLESIZE(files_to_remove_before_testing); i++) {
        remove(files_to_remove_before_testing[i]);
    }

    TestDVObj tester;
    tester.run_testcases_in_file("stream");
}

#if OS(UNIX)
TEST(dvobjs, stream_pipe)
{
    if (access("/usr/bin/bc", F_OK)) {
        return;
    }
    TestDVObj tester;
    tester.run_testcases_in_file("stream_pipe");
}

TEST(dvobjs, stream_local)
{
    TestDVObj tester;
    tester.run_testcases_in_file("stream_local");
}
#endif

