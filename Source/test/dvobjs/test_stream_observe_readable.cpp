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

bool is_file_exists(const char* file)
{
    struct stat filestat;
    return (0 == stat(file, &filestat));
}

TEST(observe, basic)
{
    const char *hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <update on=\"$TIMERS\" to=\"unite\">"
    "            ["
    "                { \"id\" : \"clock\", \"interval\" : 1000, \"active\" : \"yes\" }"
    "            ]"
    "        </update>"
    "    </head>"
    ""
    "    <body>"
    "        <div id=\"stream\">"
    ""
    "            <div id=\"c_title\">"
    "                <h2 id=\"c_title\">Stream observe<br/>"
    "                    <span id=\"content\">$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S', null)</span>"
    "                </h2>"
    ""
    "                <init as 'reader' with $STREAM.open('fifo:///tmp/stream_fifo', 'read create nonblock') />"
    ""
    "                <observe on $reader for 'stream:readable' >"
    "                    <inherit>"
    "                        $STREAM.stdout.writelines('readable event')"
    "                    </inherit>"
    "                    <update on '#content' at 'textContent' with $DATA.stringify($reader.readlines(1)) />"
    "                    <forget on $reader for 'stream:readable' />"
    "                    <choose on $reader.close() />"
    "                    <choose on $writter.close() />"
    "                </observe>"
    ""
    ""
    "                <observe on $TIMERS for 'expired:clock'>"
    "                    <inherit>"
    "                        $STREAM.stdout.writelines('timer event')"
    "                    </inherit>"
    "                    <forget on $TIMERS for 'expired:clock' />"
    "                    <init as 'writter' at '#stream' with $STREAM.open('fifo:///tmp/stream_fifo', 'write') />"
    "                    <choose on $writter.writelines('message write to fifo') >"
    "                       <update on '#content' at 'textContent' with $DATA.stringify($?) />"
    "                    </choose>"
    "                </observe>"
    ""
    "                <p>this is after observe</p>"
    "            </div>"
    ""
    "        </div>"
    "    </body>"
    ""
    "</hvml>";

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    const char *file = "/tmp/stream_fifo";

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_stream_observe", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    if(!is_file_exists(file)) {
        ret = mkfifo(file, 0777);
        ASSERT_EQ(ret, 0);
    }

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
    ASSERT_NE(vdom, nullptr);

    purc_coroutine_t cor = purc_schedule_vdom_null(vdom);
    ASSERT_NE(cor, nullptr);

    purc_run(NULL);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);

    unlink(file);
}

