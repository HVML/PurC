#include "purc.h"
#include "private/utils.h"
#include "../helpers.h"

#include <gtest/gtest.h>


TEST(interpreter, purc_init)
{
    unsigned int modules[] = {
        // PURC_MODULE_UTILS,
        // PURC_MODULE_DOM,
        // PURC_MODULE_HTML,
        // PURC_MODULE_XML,
        // PURC_MODULE_VARIANT,
        // PURC_MODULE_EJSON,
        // PURC_MODULE_XGML,
        // PURC_MODULE_HVML,
        PURC_MODULE_PCRDR,
        // PURC_MODULE_ALL,
    };
    const char *app_name = "foo";
    const char *runner_name = "bar";


    for (size_t i=0; i<sizeof(modules)/sizeof(modules[0]); ++i) {
        if (modules[i] != PURC_MODULE_EJSON)
            continue;

        const purc_instance_extra_info extra_info = {};

        int r;
        r = purc_init_ex(modules[i], app_name, runner_name, &extra_info);

        ASSERT_EQ(r, 0);

        purc_cleanup();
    }
}



