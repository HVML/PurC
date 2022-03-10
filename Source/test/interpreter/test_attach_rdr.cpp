#include "purc.h"
#include "private/utils.h"
#include "../helpers.h"

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


TEST(interpreter, basic)
{
    (void)calculator_1;

    const char *hvmls[] = {
        calculator_1,
    };

    PurCInstance purc("cn.fmsoft.hybridos.test", "test_attach_rdr", false);

    ASSERT_TRUE(purc);

    // get statitics information
    struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    for (size_t i=0; i<PCA_TABLESIZE(hvmls); ++i) {
        const char *hvml = hvmls[i];
        purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
        ASSERT_NE(vdom, nullptr);
    }

    purc_run(PURC_VARIANT_INVALID, NULL);
}

