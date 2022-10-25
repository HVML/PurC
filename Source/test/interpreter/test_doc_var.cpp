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

#include <gtest/gtest.h>

TEST(doc_var, basic)
{
    const char *test_hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "<head>"
    "    <base href=\"$CRTN.base(! 'https://gitlab.fmsoft.cn/hvml/hvml-docs/raw/master/samples/calculator/' )\" />"
    ""
    "    <link rel=\"stylesheet\" type=\"text/css\" href=\"assets/calculator.css\" />"
    "        <init as=\"buttons\" uniquely>"
    "            ["
    "                { \"letters\": \"7\", \"class\": \"number\" },"
    "                { \"letters\": \"8\", \"class\": \"number\" },"
    "                { \"letters\": \"9\", \"class\": \"number\" },"
    "                { \"letters\": \"←\", \"class\": \"c_blue backspace\" },"
    "                { \"letters\": \"C\", \"class\": \"c_blue clear\" },"
    "                { \"letters\": \"4\", \"class\": \"number\" },"
    "                { \"letters\": \"5\", \"class\": \"number\" },"
    "                { \"letters\": \"6\", \"class\": \"number\" },"
    "                { \"letters\": \"×\", \"class\": \"c_blue multiplication\" },"
    "                { \"letters\": \"÷\", \"class\": \"c_blue division\" },"
    "                { \"letters\": \"1\", \"class\": \"number\" },"
    "                { \"letters\": \"2\", \"class\": \"number\" },"
    "                { \"letters\": \"3\", \"class\": \"number\" },"
    "                { \"letters\": \"+\", \"class\": \"c_blue plus\" },"
    "                { \"letters\": \"-\", \"class\": \"c_blue subtraction\" },"
    "                { \"letters\": \"0\", \"class\": \"number\" },"
    "                { \"letters\": \"00\", \"class\": \"number\" },"
    "                { \"letters\": \".\", \"class\": \"number\" },"
    "                { \"letters\": \"%\", \"class\": \"c_blue percent\" },"
    "                { \"letters\": \"=\", \"class\": \"c_yellow equal\" },"
    "            ]"
    "        </init>"
    "</head>"
    ""
    "<body>"
    "    <div id=\"calculator\">"
    ""
    "        <div value=\"assets/{$SYS.locale}.json\">"
    "        </div>"
    ""
    "        <div value=\"$T.get('HVML Calculator')\">"
    "        </div>"
    ""
    "        <div>"
    "            $T.get('HVML Calculator')"
    "        </div>"
    ""
    "        <div value=\"$SYS.time()\">"
    "        </div>"
    ""
    "        <div value=\"$SYS.cwd\">"
    "        </div>"
    ""
    "        <div value=\"$SYS.cwd(!'/tmp/')\">"
    "              set cwd to /tmp/"
    "        </div>"
    ""
    "        <div value=\"$SYS.cwd\">"
    "        </div>"
    ""
    "        <div value=\"$RUNNER.user\">"
    "        </div>"
    ""
    "        <div value=\"test set SESSION.user(!'abc', 123)\">"
    "            $RUNNER.user(!'abc', 123)"
    "        </div>"
    ""
    "        <div value=\"$RUNNER.user\">"
    "        </div>"
    ""
    "        <div value=\"$RUNNER.user('abc')\">"
    "        </div>"
    ""
    "        <div value=\"$RUNNER.user('abc')\">"
    "        </div>"
    ""
    "        <div value=\"$buttons[0].letters\">"
    "            <init as=\"buttons\" uniquely temporarily>"
    "                ["
    "                    { \"letters\": \"777\", \"class\": \"number\" },"
    "                ]"
    "            </init>"
    "            <div value=\"$buttons[0].letters\">"
    "            </div>"
    "        </div>"
    ""
    "        <div>"
    "            $buttons[0].letters"
    "        </div>"
    ""
    "        <div>"
    "            $buttons[0]"
    "        </div>"
    ""
    "    </div>"
    "</body>"
    ""
    "</hvml>";
    (void)test_hvml;

    const char *hvmls[] = {
        test_hvml,
    };

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    for (size_t i=0; i<PCA_TABLESIZE(hvmls); ++i) {
        const char *hvml = hvmls[i];
        purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
        purc_schedule_vdom_null(vdom);
        ASSERT_NE(vdom, nullptr);
    }

    purc_run(NULL);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

