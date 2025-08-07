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
#include "../helpers.h"
#include "../tools.h"

#include "TestDVObj.h"

#include <gtest/gtest.h>

TEST(dvobjs, js_basic)
{
#if ENABLE(QUICKJS)
    TestDVObj tester;
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);
    tester.run_testcases_in_file("js");
#endif
}

TEST(dvobjs, js_hello)
{
#if ENABLE(QUICKJS)
    PurCInstance purc(false);
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);
    run_one_comp_test("dvobjs/js/hello.hvml");
#endif
}


TEST(dvobjs, js_pi)
{
#if ENABLE(QUICKJS)
    PurCInstance purc(false);
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    char path[PATH_MAX];
    const char *env = "SOURCE_FILES";
    const char *rel = "js/pi_bigint.js";
    test_getpath_from_env_or_rel(path, sizeof(path), env, rel);
    char *url = purc_url_encode_alloc(path, true);
    char query[PATH_MAX];
    snprintf(query, PATH_MAX, "script=%s", url);
    free(url);

    run_one_comp_test("dvobjs/js/pi.hvml", query);
#endif
}

