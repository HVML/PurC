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

#include "private/vdom.h"
#include <gtest/gtest.h>

TEST(observe, basic)
{
    const char *observer_hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <link rel=\"stylesheet\" type=\"text/css\" href=\"calculator.css\" />"
    ""
    "        <update on=\"$TIMERS\" to=\"displace\">"
    "            ["
    "                { \"id\" : \"clock\", \"interval\" : 1000, \"active\" : \"yes\" },"
    "            ]"
    "        </update>"
    "    </head>"
    ""
    "    <body>"
    "        <div id=\"calculator\">"
    ""
    "            <div id=\"c_title\">"
    "                <h2 id=\"c_title\">Calculator-2<br/>"
    "                    <span id=\"clock\">00:00</span>"
    "                </h2>"
    "                <observe on=\"$TIMERS\" for=\"expired:clock\">"
    "                    <update on=\"#clock\" at=\"textContent\" with=\"$DATETIME.time_prt()\" />"
    "                    <update on=\"$TIMERS\" to=\"overwrite\">"
    "                       { \"id\" : \"clock\", \"active\" : \"no\" }"
    "                    </update>"
    "                    <forget on=\"$TIMERS\" for=\"expired:clock\"/>"
    "                </observe>"
    "                <p>this is after observe</p>"
    "            </div>"
    ""
    "        </div>"
    "    </body>"
    ""
    "</hvml>";
    (void)observer_hvml;

    const char *hvmls[] = {
        observer_hvml,
    };

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);
    purc_bind_runner_variables();

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

