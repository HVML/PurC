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

#include "purc/purc.h"

#include "hvml/hvml-token.h"
#include "private/dvobjs.h"
#include "private/hvml.h"
#include "private/interpreter.h"
#include "private/utils.h"
#include "purc/purc-rwstream.h"
#include "purc/purc-variant.h"

#include "../helpers.h"
#include "../tools.h"

#include <gtest/gtest.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <vector>

using namespace std;

struct TestCase {
    char *name;
    char *hvml;
    unsigned long run_idx;
};

static const char* request_json =
    "{ names: 'PurC', OS: ['Linux', 'macOS', 'HybridOS', 'Windows'] }";

std::vector<TestCase*> g_test_cases;
std::vector<char*> g_test_cases_name;

void destroy_test_case(struct TestCase* tc)
{
    if (tc->name) {
        free(tc->name);
    }
    if (tc->hvml) {
        free(tc->hvml);
    }
    free(tc);
}

std::vector<TestCase*>& read_test_cases();

class TestCaseEnv : public ::testing::Environment {
public:
    ~TestCaseEnv() override { }

    void SetUp() override
    {
    }

    void TearDown() override
    {
        size_t nr = g_test_cases.size();
        for (size_t i = 0; i < nr; i++) {
            TestCase* tc = g_test_cases[i];
            destroy_test_case(tc);
        }
    }
};

testing::Environment* const _env =
    testing::AddGlobalTestEnvironment(new TestCaseEnv);

static inline void
add_test_case(std::vector<struct TestCase*> &test_cases,
    std::vector<char*> &test_cases_name,
    const char *name, const char* hvml,
    unsigned long run_idx)
{
    struct TestCase *data = (struct TestCase *)calloc(1, sizeof(*data));
    data->name = strdup(name);
    data->hvml = strdup(hvml);
    data->run_idx = run_idx;

    test_cases.push_back(data);
    test_cases_name.push_back(data->name);
}

class TestRunIdx : public testing::TestWithParam<struct TestCase*> {
protected:
    void SetUp()
    {
        purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_run_idx", NULL);
    }
    void TearDown()
    {
        purc_cleanup();
    }
};

struct user_data {
    unsigned long run_idx;
};

static int my_cond_handler(purc_cond_k event, purc_coroutine_t cor,
    void *data)
{
    if (event == PURC_COND_COR_ONE_RUN) {
        struct purc_cor_run_info *run_info = (struct purc_cor_run_info *)data;

        struct user_data *ud = (struct user_data *)
            purc_coroutine_get_user_data(cor);
        if (!ud) {
            goto out;
        }
        ud->run_idx = run_info->run_idx;
    }

out:
    return 0;
}

TEST_P(TestRunIdx, tags)
{
    struct TestCase* test_case = GetParam();
    PRINTF("test case : %s\n", test_case->name);

    struct user_data ud = {0};

    purc_vdom_t vdom = purc_load_hvml_from_string(test_case->hvml);
    ASSERT_NE(vdom, nullptr);

    purc_variant_t request = purc_variant_make_from_json_string(request_json,
        strlen(request_json));
    ASSERT_NE(request, nullptr);

    purc_renderer_extra_info rdr_info = {};
    rdr_info.title = "def_page_title";
    purc_coroutine_t co = purc_schedule_vdom(vdom,
        0, request, PCRDR_PAGE_TYPE_NULL,
        "main", /* target_workspace */
        NULL, /* target_group */
        NULL, /* page_name */
        &rdr_info, "test", NULL);
    ASSERT_NE(co, nullptr);
    purc_variant_unref(request);

    purc_coroutine_set_user_data(co, &ud);

    purc_run((purc_cond_handler)my_cond_handler);

    ASSERT_EQ(ud.run_idx, test_case->run_idx);
}

std::vector<TestCase*>& read_test_cases()
{
    add_test_case(g_test_cases, g_test_cases_name,
            "test_case_only_hvml_tag",
            "<hvml></hvml>",
            0
            );

    add_test_case(g_test_cases, g_test_cases_name,
            "test_case_without_observe",
            "<hvml target='void'>"
            "    <!-- initialize some runner-level variables for the request handler -->"
            "    <init as MATH at '_runner' from 'MATH' via='LOAD' />"
            "    <init as FS at '_runner' from 'FS' for 'FS' via='LOAD' />"
            "</hvml>",
            0
            );

    add_test_case(g_test_cases, g_test_cases_name,
            "test_case_without_observe_and_have_exit",
            "<hvml target='void'>"
            "    <!-- initialize some runner-level variables for the request handler -->"
            "    <init as MATH at '_runner' from 'MATH' via='LOAD' />"
            "    <init as FS at '_runner' from 'FS' for 'FS' via='LOAD' />"
            "    <exit with 0/>"
            "</hvml>",
            0
            );

    add_test_case(g_test_cases, g_test_cases_name,
            "test_case_with_observe_and_exit_on_the_end",
            "<hvml target='void'>"
            "    <!-- initialize some runner-level variables for the request handler -->"
            "    <init as MATH at '_runner' from 'MATH' via='LOAD' />"
            "    <init as FS at '_runner' from 'FS' for 'FS' via='LOAD' />"
            ""
            "    <observe on $TIMERS for 'expired:gogogo'>"
            "    </observe>"
            ""
            "    <exit with 0/>"
            "</hvml>",
            0
            );

    add_test_case(g_test_cases, g_test_cases_name,
            "test_case_with_observe_timer_1",
            "<hvml target='void'>"
            "    <!-- initialize some runner-level variables for the request handler -->"
            "    <init as MATH at '_runner' from 'MATH' via='LOAD' />"
            "    <init as FS at '_runner' from 'FS' for 'FS' via='LOAD' />"
            ""
            "    <update on=$TIMERS to='unite'>"
            "        ["
            "            { 'id' : 'gogogo', 'interval' : 1000, 'active' : 'yes' },"
            "        ]"
            "    </update>"
            "    <init as 'progress' with 0UL />"
            "    <observe on $TIMERS for 'expired:gogogo'>"
            "        <init as 'progress' at '_grandparent' with ($progress + 10UL) />"
            "        <test with $L.ge($progress, 10UL) >"
            "            <update on $TIMERS to 'subtract' with [ { id: 'gogogo' } ] />"
            "            <forget on $TIMERS for 'expired:gogogo' />"
            "        </test>"
            "    </observe>"
            ""
            "</hvml>",
            1
            );

    add_test_case(g_test_cases, g_test_cases_name,
            "test_case_with_observe_timer_5",
            "<hvml target='void'>"
            "    <!-- initialize some runner-level variables for the request handler -->"
            "    <init as MATH at '_runner' from 'MATH' via='LOAD' />"
            "    <init as FS at '_runner' from 'FS' for 'FS' via='LOAD' />"
            ""
            "    <update on=$TIMERS to='unite'>"
            "        ["
            "            { 'id' : 'gogogo', 'interval' : 1000, 'active' : 'yes' },"
            "        ]"
            "    </update>"
            "    <init as 'progress' with 0UL />"
            "    <observe on $TIMERS for 'expired:gogogo'>"
            "        <init as 'progress' at '_grandparent' with ($progress + 10UL) />"
            "        <test with $L.ge($progress, 50UL) >"
            "            <update on $TIMERS to 'subtract' with [ { id: 'gogogo' } ] />"
            "            <forget on $TIMERS for 'expired:gogogo' />"
            "        </test>"
            "    </observe>"
            ""
            "</hvml>",
            5
            );

    add_test_case(g_test_cases, g_test_cases_name,
            "test_case_with_observe_timer_5_and_exit_on_observe",
            "<hvml target='void'>"
            "    <!-- initialize some runner-level variables for the request handler -->"
            "    <init as MATH at '_runner' from 'MATH' via='LOAD' />"
            "    <init as FS at '_runner' from 'FS' for 'FS' via='LOAD' />"
            ""
            "    <update on=$TIMERS to='unite'>"
            "        ["
            "            { 'id' : 'gogogo', 'interval' : 1000, 'active' : 'yes' },"
            "        ]"
            "    </update>"
            "    <init as 'progress' with 0UL />"
            "    <observe on $TIMERS for 'expired:gogogo'>"
            "        <init as 'progress' at '_grandparent' with ($progress + 10UL) />"
            "        <test with $L.ge($progress, 50UL) >"
            "            <update on $TIMERS to 'subtract' with [ { id: 'gogogo' } ] />"
            "            <forget on $TIMERS for 'expired:gogogo' />"
            "            <exit with 0/>"
            "        </test>"
            "    </observe>"
            ""
            "</hvml>",
            5
            );

    return g_test_cases;
}

INSTANTIATE_TEST_SUITE_P(tags, TestRunIdx,
    testing::ValuesIn(read_test_cases()));

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
