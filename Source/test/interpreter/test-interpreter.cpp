#include "purc.h"

#include <gtest/gtest.h>

TEST(interpreter, basic)
{
    const char *calculator_1 =
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
    "        <div id=\"calculator\" test=\"$buttons\">"
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
    (void)calculator_1;

    const char *hvmls[] = {
        // "<hvml><head x=\"y\">hello<xinit a=\"b\">world<!--yes-->solid</xinit></head><body><timeout1/><timeout3/></body></hvml>",
        // "<hvml><head x=\"y\">hello<xinit a=\"b\">w<timeout3/>orld<!--yes-->solid</xinit></head><body><timeout1/></body></hvml>",
        // "<hvml><body><timeout1/><timeout9/><timeout2/></body></hvml>",
        // "<hvml><body><test a='b'>hello<!--yes--></test></body></hvml>",
        // "<hvml><body><archetype name=\"$?.button\"><li class=\"class\">letters</li></archetype></body></hvml>",
        // "<hvml><body><archetype name=\"button\"><li class=\"class\">letters</li></archetype></body></hvml>",
        // "<hvml><body><a><b><c></c></b></a></body></hvml>",
        calculator_1,
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

