/*
** Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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

#if OS(UNIX)
TEST(dvobjs, socket)
{
    TestDVObj tester;
    tester.run_testcases_in_file("socket");
}

TEST(dvobjs, socket_local_stream)
{
    TestDVObj tester;
    tester.run_testcases_in_file("socket_local_stream");
}

TEST(dvobjs, socket_inet_stream)
{
    TestDVObj tester;
    tester.run_testcases_in_file("socket_inet_stream");
}

#else

TEST(dvobjs, foo)
{
}
#endif

