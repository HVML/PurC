#include "purc.h"

#include "private/ejson.h"
#include "private/instance.h"
#include "purc-rwstream.h"

#include <gtest/gtest.h>

TEST(ejson, ref_manual_good)
{
    // add child bottom-up
    purc_init("cn.fmsoft.hybridos.test", "ejson", NULL);

    purc_variant_t root = purc_variant_make_array(0, NULL);
    purc_variant_t arr1 = purc_variant_make_array(0, NULL);

    purc_variant_t str = purc_variant_make_string("hello", true);
    EXPECT_EQ(str->refc, 1);  // 1 for `str`
    purc_variant_array_append(arr1, str);
    EXPECT_EQ(str->refc, 2);  // 1 for `str`, 1 for in `arr1`

    purc_variant_array_append(root, arr1);
    EXPECT_EQ(str->refc, 3);  // 1 for `str`, 1 for in `arr1`, 1 for in `root`

    purc_variant_unref(root);
    EXPECT_EQ(str->refc, 2);  // 1 for `str`, 1 for in `arr1`
    purc_variant_unref(arr1);
    EXPECT_EQ(str->refc, 1);  // 1 for `str`
    purc_variant_unref(str);  // 0 and clean

    purc_cleanup();
}

TEST(ejson, ref_manual_bad)
{
    // add child top-down
    purc_init("cn.fmsoft.hybridos.test", "ejson", NULL);

    purc_variant_t root = purc_variant_make_array(0, NULL);
    purc_variant_t arr1 = purc_variant_make_array(0, NULL);
    purc_variant_array_append(root, arr1);

    purc_variant_t str = purc_variant_make_string("hello", true);
    EXPECT_EQ(str->refc, 1); // 1 for `str`
    purc_variant_array_append(arr1, str);
    // in _array_append, we don't know `arr1` is in `root`
    EXPECT_EQ(str->refc, 2); // 1 for `str`, 1 for `arr1`,

    purc_variant_unref(root);
    EXPECT_EQ(str->refc, 1); // 1 unref'd recursively from `root`
    purc_variant_unref(arr1);
    EXPECT_EQ(str->refc, 0); // 1 unref`d recursively from `arr1`
    fprintf(stderr, "================= you are still good now! =================\n");
    purc_variant_unref(str);
    fprintf(stderr, "================= you can not reach here! =================\n");

    purc_cleanup();
}

