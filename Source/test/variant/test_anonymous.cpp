#include "purc.h"
#include "purc-variant.h"

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
    purc_variant_unref(arr);

    // anonymous way
    arr = purc_variant_make_array(3,
        MARK_ANONYM(purc_variant_make_array(0, NULL)),
        MARK_ANONYM(purc_variant_make_object(0, NULL, NULL)),
        MARK_ANONYM(purc_variant_make_string("hello", true)));
    purc_variant_unref(arr);

    // normal_way
    purc_variant_t obj;
    {
        purc_variant_t k1 = purc_variant_make_string("hello", true);
        purc_variant_t v1 = purc_variant_make_string("world", true);
        obj = purc_variant_make_object(1, k1, v1);
        purc_variant_unref(v1);
        purc_variant_unref(k1);
    }
    purc_variant_unref(obj);

    // anonymous way
    obj = purc_variant_make_object(1,
        MARK_ANONYM(purc_variant_make_string("hello", true)),
        MARK_ANONYM(purc_variant_make_string("world", true)));
    purc_variant_unref(obj);

    bool b = purc_cleanup ();
    ASSERT_EQ (b, true);
}

TEST(anonymous, complex)
{
    int r;
    purc_instance_extra_info info = {0, 0};
    r = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(r, PURC_ERROR_OK);

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

    purc_variant_unref(data);

    bool b = purc_cleanup ();
    ASSERT_EQ (b, true);
}

