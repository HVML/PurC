#include "purc.h"

#include <gtest/gtest.h>

TEST(observe, basic)
{
    const char *hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <update on=\"$TIMERS\" to=\"unite\">"
    "            ["
    "                { \"id\" : \"clock\", \"interval\" : 5000, \"active\" : \"yes\" }"
    "            ]"
    "        </update>"
    "    </head>"
    ""
    "    <body>"
    "        <div id=\"stream\">"
    ""
    "            <div id=\"c_title\">"
    "                <h2 id=\"c_title\">Stream observe<br/>"
    "                    <span id=\"clock\">$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S', null)</span>"
    "                    <span id=\"stream_content\"></span>"
    "                </h2>"
    "                <observe on=\"$STREAM.open('pipe:///var/tmp/stream_pipe', 'read create nonblock')\" for=\"event:read\">"
    "                    <update on=\"#stream_content\" at=\"textContent\" with=\"$STREAM.readlines($@, 1)\" />"
    "                </observe>"
    ""
    "                <observe on=\"$TIMERS\" for=\"expired:clock\">"
    "                    <update on=\"#clock\" at=\"textContent\" with=\"$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S', null)\" />"
    "                    <update on=\"#clock\" at=\"textContent\" with=\"$EJSON.stringify($STREAM.writelines($STREAM.open('pipe:///var/tmp/stream_pipe', 'write'), 'write line to pipe'))\" />"
    "                    <forget on=\"$TIMERS\" for=\"expired:clock\"/>"
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

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_stream_observe", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t stream = purc_variant_load_dvobj_from_so ("STREAM", "STREAM");
    ASSERT_NE(stream, nullptr);

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
    ASSERT_NE(vdom, nullptr);

    bool bind = purc_bind_document_variable(vdom, "STREAM", stream);
    ASSERT_EQ(bind, true);

    purc_run(PURC_VARIANT_INVALID, NULL);

    fprintf(stderr, "##################################### call unload\n");
    purc_variant_unload_dvobj(stream);
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

