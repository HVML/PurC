#include "purc.h"

#include <gtest/gtest.h>

TEST(interpreter, basic)
{
    const char *observer_hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <link rel=\"stylesheet\" type=\"text/css\" href=\"calculator.css\" />"
    "    </head>"
    ""
    "    <body>"
    "        <div id=\"calculator\">"
    ""
    "            <div id=\"c_title\">"
    "                <h2 id=\"c_title\">$T.get('Calculator')<br/>"
    "                    <span id=\"clock\">$SYSTEM.time('%H:%m')</span>"
    "                </h2>"
    "                <observe on=\"$TIMERS\" for=\"expired:clock\">"
    "                    <update on=\"#clock\" at=\"textContent\" with=\"$SYSTEM.time('%H:%m')\" />"
    "                </observe>"
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
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    // get statitics information
    struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    for (size_t i=0; i<PCA_TABLESIZE(hvmls); ++i) {
        const char *hvml = hvmls[i];
        purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
        ASSERT_NE(vdom, nullptr);
    }

    purc_run(PURC_VARIANT_INVALID, NULL);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

