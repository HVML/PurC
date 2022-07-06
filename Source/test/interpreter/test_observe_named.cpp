#include "purc.h"

#include <gtest/gtest.h>

TEST(observe, basic)
{
    if (1)
        return;

    const char *observer_hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <link rel=\"stylesheet\" type=\"text/css\" href=\"calculator.css\" />"
    ""
    "        <init as=\"buttons\" uniquely against=\"letters\">"
    "            ["
    "                { \"letters\": \"7\", \"class\": \"number\" },"
    "                { \"letters\": \"8\", \"class\": \"number\" },"
    "            ]"
    "        </init>"
    "        <update on=\"$TIMERS\" to=\"unite\">"
    "            ["
    "                { \"id\" : \"clock\", \"interval\" : 1000, \"active\" : \"yes\" }"
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
    "                <observe on=\"$buttons[0]\" for=\"change\">"
    "                    <update on=\"#clock\" at=\"textContent\" with=\"$buttons[0].class\" />"
    "                </observe>"
    "                <observe on=\"$TIMERS\" for=\"expired:clock\">"
    "                    <update on=\"$buttons\" to=\"overwrite\">"
    "                        { \"letters\" : \"7\", \"class\" : \"update by timers $SYSTEM.random(10)\" }"
    "                    </update>"
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
    purc_bind_session_variables();

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    for (size_t i=0; i<PCA_TABLESIZE(hvmls); ++i) {
        const char *hvml = hvmls[i];
        purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
        ASSERT_NE(vdom, nullptr);
    }

    purc_run(NULL);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

