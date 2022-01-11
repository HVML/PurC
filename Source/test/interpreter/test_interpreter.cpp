#include "purc.h"

#include <gtest/gtest.h>


static const char *calculator_1 =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <title>计算器</title>"
    "        <link rel=\"stylesheet\" type=\"text/css\" href=\"calculator.css\" />"
    ""
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
    "    </head>"
    ""
    "    <body>"
    "        <div id=\"calculator\">"
    ""
    "            <div id=\"c_title\">"
    "                <h2>计算器</h2>"
    "            </div>"
    ""
    "            <div id=\"c_text\">"
    "                <input type=\"text\" id=\"text\" value=\"0\" readonly=\"readonly\" />"
    "            </div>"
    ""
    "            <div id=\"c_value\">"
    "                <archetype name=\"button\">"
    "                    <li class=\"$?.class\">$?.letters</li>"
    "                </archetype>"
    ""
    "                <ul>"
    "                    <iterate on=\"$buttons\">"
    "                        <update on=\"$@\" to=\"append\" with=\"$button\" />"
    "                        <except type=\"NoData\" raw>"
    "                            <p>Bad data!</p>"
    "                        </except>"
    "                    </iterate>"
    "                </ul>"
    "            </div>"
    "        </div>"
    "    </body>"
    ""
    "</hvml>";

static const char *calculator_2 =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <base href=\"$HVML.base(! 'https://gitlab.fmsoft.cn/hvml/hvml-docs/raw/master/samples/calculator/' )\" />"
    ""
    "        <update on=\"$T.map\" from=\"assets/{$SYSTEM.locale}.json\" to=\"merge\" />"
    ""
    "        <init as=\"buttons\" from=\"assets/buttons.json\" />"
    ""
    "        <title>$T.get('HVML Calculator')</title>"
    ""
    "        <update on=\"$TIMERS\" to=\"displace\">"
    "            ["
    "                { \"id\" : \"clock\", \"interval\" : 1000, \"active\" : \"yes\" },"
    "            ]"
    "        </update>"
    ""
    "        <link rel=\"stylesheet\" type=\"text/css\" href=\"assets/calculator.css\" />"
    "    </head>"
    ""
    "    <body>"
    "        <div id=\"calculator\">"
    ""
    "            <div id=\"c_text\">"
    "                <input type=\"text\" id=\"text\" value=\"0\" readonly=\"readonly\" />"
    "            </div>"
    ""
    "            <div id=\"c_value\">"
    "                <archetype name=\"button\">"
    "                    <li class=\"$?.class\">$?.letters</li>"
    "                </archetype>"
    ""
    "                <ul>"
    "                    <iterate on=\"$buttons\">"
    "                        <update on=\"$@\" to=\"append\" with=\"$button\" />"
    "                        <except type=\"NoData\" raw>"
    "                            <p>Bad data!</p>"
    "                        </except>"
    "                    </iterate>"
    "                </ul>"
    "            </div>"

    "            <div id=\"c_title\">"
    "                <h2 id=\"c_title\">$T.get('HVML Calculator')"
    "                    <small>$T.get('Current Time: ')<span id=\"clock\">$SYSTEM.time('%H:%M:%S')</span></small>"
    "                </h2>"
    "                <observe on=\"$TIMERS\" for=\"expired:clock\">"
    "                    <update on=\"#clock\" at=\"textContent\" with=\"$SYSTEM.time('%H:%M:%S')\" />"
    "                </observe>"
    "            </div>"
    "        </div>"
    "    </body>"
    ""
    "</hvml>";

TEST(interpreter, basic)
{
    if (1)
        return;

    (void)calculator_1;
    (void)calculator_2;

    const char *hvmls[] = {
        // "<hvml><head x=\"y\">hello<xinit a=\"b\">world<!--yes-->solid</xinit></head><body><timeout1/><timeout3/></body></hvml>",
        // "<hvml><head x=\"y\">hello<xinit a=\"b\">w<timeout3/>orld<!--yes-->solid</xinit></head><body><timeout1/></body></hvml>",
        // "<hvml><body><timeout1/><timeout9/><timeout2/></body></hvml>",
        // "<hvml><body><test a='b'>hello<!--yes--></test></body></hvml>",
        // "<hvml><body><archetype name=\"$?.button\"><li class=\"class\">letters</li></archetype></body></hvml>",
        // "<hvml><body><archetype name=\"button\"><li class=\"class\">letters</li></archetype></body></hvml>",
        // "<hvml><body><a><b><c></c></b></a></body></hvml>",
        calculator_1,
        // calculator_2,
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

