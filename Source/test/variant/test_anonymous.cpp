#include "purc.h"
#include "purc-variant.h"

#include "private/variant.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtest/gtest.h>

#define MARK_ANONYM(v) purc_variant_tag_as_anonymous(v)

TEST(anonymous, basic)
{
    int r;
    purc_instance_extra_info info = {0, 0};
    r = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(r, PURC_ERROR_OK);

    struct purc_variant_stat * stat = NULL;
    stat = purc_variant_usage_stat();

    // normal way
    purc_variant_t arr;
    {
        purc_variant_t v1 = purc_variant_make_array(0, NULL);
        purc_variant_t v2 = purc_variant_make_object(0, NULL, NULL);
        purc_variant_t v3 = purc_variant_make_string("hello", true);
        arr = purc_variant_make_array(3, v1, v2, v3);
        // you have to unref explicitly
        purc_variant_unref(v3);
        purc_variant_unref(v2);
        purc_variant_unref(v1);
    }
    ASSERT_EQ(2, stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    ASSERT_EQ(1, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(1, stat->nr_values[PURC_VARIANT_TYPE_STRING]);
    purc_variant_unref(arr);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_STRING]);

    // anonymous way
    arr = purc_variant_make_array(3,
        MARK_ANONYM(purc_variant_make_array(0, NULL)),
        MARK_ANONYM(purc_variant_make_object(0, NULL, NULL)),
        MARK_ANONYM(purc_variant_make_string("hello", true)));
    ASSERT_EQ(2, stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    ASSERT_EQ(1, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(1, stat->nr_values[PURC_VARIANT_TYPE_STRING]);
    purc_variant_unref(arr);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_STRING]);

    // normal_way
    purc_variant_t obj;
    {
        purc_variant_t k1 = purc_variant_make_string("hello", true);
        purc_variant_t v1 = purc_variant_make_string("world", true);
        obj = purc_variant_make_object(1, k1, v1);
        purc_variant_unref(v1);
        purc_variant_unref(k1);
    }
    ASSERT_EQ(1, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(2, stat->nr_values[PURC_VARIANT_TYPE_STRING]);
    purc_variant_unref(obj);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_STRING]);

    // anonymous way
    obj = purc_variant_make_object(1,
        MARK_ANONYM(purc_variant_make_string("hello", true)),
        MARK_ANONYM(purc_variant_make_string("world", true)));
    ASSERT_EQ(1, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(2, stat->nr_values[PURC_VARIANT_TYPE_STRING]);
    purc_variant_unref(obj);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_STRING]);

    bool b = purc_cleanup ();
    ASSERT_EQ (b, true);
}

TEST(anonymous, complex)
{
    int r;
    purc_instance_extra_info info = {0, 0};
    r = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(r, PURC_ERROR_OK);

    struct purc_variant_stat * stat = NULL;
    stat = purc_variant_usage_stat();

    // this will introduce mem-leak
    // purc_variant_t data = purc_variant_make_object_by_static_ckey (1,
    //     "data", purc_variant_make_object_by_static_ckey (1,
    //         "todolist", purc_variant_make_array (2,
    //             purc_variant_make_object_by_static_ckey (2,
    //                 "title", purc_variant_make_string ("刷题", true),
    //                 "date", purc_variant_make_string ("2021-12-31 10:00:00", true)),
    //             purc_variant_make_object_by_static_ckey (2,
    //                 "title", purc_variant_make_string ("刷题", true),
    //                 "date", purc_variant_make_string ("2021-12-31 10:00:00", true))
    //             )
    //         )
    //     );

    // mark all intermediates as anonymous variant
    purc_variant_t data = purc_variant_make_object_by_static_ckey (1,
        "data", MARK_ANONYM(purc_variant_make_object_by_static_ckey (1,
            "todolist", MARK_ANONYM(purc_variant_make_array (2,
                MARK_ANONYM(purc_variant_make_object_by_static_ckey (2,
                    "title", MARK_ANONYM(purc_variant_make_string ("刷题", true)),
                    "date", MARK_ANONYM(purc_variant_make_string ("2021-12-31 10:00:00", true)))),
                MARK_ANONYM(purc_variant_make_object_by_static_ckey (2,
                    "title", MARK_ANONYM(purc_variant_make_string ("刷题", true)),
                    "date", MARK_ANONYM(purc_variant_make_string ("2021-12-31 10:00:00", true))))
                ))
            ))
        );

    ASSERT_EQ(1, stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    ASSERT_EQ(4, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(10, stat->nr_values[PURC_VARIANT_TYPE_STRING]);
    purc_variant_unref(data);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    ASSERT_EQ(0, stat->nr_values[PURC_VARIANT_TYPE_STRING]);

    bool b = purc_cleanup ();
    ASSERT_EQ (b, true);
}

