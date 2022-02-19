#include "purc.h"
#include "private/utils.h"
#include "../helpers.h"

#include <gtest/gtest.h>


TEST(samples, basic)
{
    PurCInstance purc;

    ASSERT_TRUE(purc);

    const char *input = "<hvml><head>hello world</head></hvml>";

    purc_vdom_t vdom = purc_load_hvml_from_string(input);
    ASSERT_NE(vdom, nullptr);

    purc_run(PURC_VARIANT_INVALID, NULL);
}


