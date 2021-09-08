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
    purc_variant_array_append(arr1, str);

    purc_variant_array_append(root, arr1);

    purc_variant_unref(root);
    purc_variant_unref(arr1);
    purc_variant_unref(str);

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
    purc_variant_array_append(arr1, str);

    purc_variant_unref(root);
    purc_variant_unref(arr1);
    purc_variant_unref(str);

    purc_cleanup();
}

