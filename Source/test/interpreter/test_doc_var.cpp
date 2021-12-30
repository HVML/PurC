#include "purc.h"

#include <gtest/gtest.h>

TEST(doc_var, basic)
{
    const char *test_hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "<head>"
    "    <base href=\"$HVML.base(! 'https://gitlab.fmsoft.cn/hvml/hvml-docs/raw/master/samples/calculator/' )\" />"
    ""
    "    <link rel=\"stylesheet\" type=\"text/css\" href=\"assets/calculator.css\" />"
    "</head>"
    ""
    "<body>"
    "    <div id=\"calculator\">"
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

