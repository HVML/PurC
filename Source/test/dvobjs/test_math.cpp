#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <gtest/gtest.h>

struct dvobjs_math_item
{
    const char * func_d;
    enum purc_variant_type type_d;
    double param_d;
    double d;
    const char * func_ld;
    enum purc_variant_type type_ld;
    long double param_ld;
    long double ld;
};

TEST(dvobjs, dvobjs_math_pi_e)
{
    struct dvobjs_math_item math_item[] = 
    {
        {
            "pi",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_PI,
            "pi_l",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_PIl
        },
        {
            "e",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_E,
            "e_l",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_El
        }
    };

    size_t i = 0;
    size_t size = sizeof (math_item) / sizeof (struct dvobjs_math_item);
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    double number;
    long double numberl;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t math = pcdvojbs_get_math();
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    for (i = 0; i < size; i++) {
        // test double function
        purc_variant_t dynamic = purc_variant_object_get_c (math, math_item[i].func_d);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        purc_dvariant_method func = NULL;
        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        ret_var = func (NULL, 0, param);
        ASSERT_NE(ret_var, nullptr);

        ASSERT_EQ(purc_variant_is_type (ret_var, math_item[i].type_d), true);

        purc_variant_cast_to_number (ret_var, &number, false);
        ASSERT_EQ(number, math_item[i].d);

        // test long double function
        dynamic = purc_variant_object_get_c (math, math_item[i].func_ld);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        ret_var = func (NULL, 0, param);
        ASSERT_NE(ret_var, nullptr);

        ASSERT_EQ(purc_variant_is_type (ret_var, math_item[i].type_ld), true);

        purc_variant_cast_to_long_double (ret_var, &numberl, false);
        ASSERT_EQ(numberl, math_item[i].ld);
    }

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_math_const)
{
    struct dvobjs_math_item math_item[] = 
    {
        {
            "e",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_E,
            "e",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_El
        },
        {
            "log2e",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_LOG2E,
            "log2e",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_LOG2El
        },
        {
            "log10e",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_LOG10E,
            "log10e",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_LOG10El
        },
        {
            "ln2",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_LN2,
            "ln2",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_LN2l
        },
        {
            "ln10",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_LN10,
            "ln10",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_LN10l
        },
        {
            "pi",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_PI,
            "pi",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_PIl
        },
        {
            "pi/2",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_PI_2,
            "pi/2",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_PI_2l
        },
        {
            "pi/4",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_PI_4,
            "pi/4",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_PI_4l
        },
        {
            "1/pi",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_1_PI,
            "1/pi",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_1_PIl
        },
        {
            "1/sqrt(2)",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_SQRT1_2,
            "1/sqrt(2)",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_SQRT1_2l
        },
        {
            "2/pi",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_2_PI,
            "2/pi",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_2_PIl
        },
        {
            "sqrt(2)",
            PURC_VARIANT_TYPE_NUMBER,
            0.0d,
            M_SQRT2,
            "sqrt(2)",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            0.0d,
            M_SQRT2l
        }
    };

    size_t i = 0;
    size_t size = sizeof (math_item) / sizeof (struct dvobjs_math_item);
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    double number;
    long double numberl;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t math = pcdvojbs_get_math();
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_c (math, "const");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    for (i = 0; i < size; i++) {
        param[0] = purc_variant_make_string (math_item[i].func_d, true);
        param[1] = NULL;
        ret_var = func (NULL, 1, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var, math_item[i].type_d), true);
        purc_variant_cast_to_number (ret_var, &number, false);
        ASSERT_EQ(number, math_item[i].d);
    }

    dynamic = purc_variant_object_get_c (math, "const_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    for (i = 0; i < size; i++) {
        param[0] = purc_variant_make_string (math_item[i].func_ld, true);
        param[1] = NULL;
        ret_var = func (NULL, 1, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var, math_item[i].type_ld), true);
        purc_variant_cast_to_long_double (ret_var, &numberl, false);
        ASSERT_EQ(numberl, math_item[i].ld);
    }

    param[0] = purc_variant_make_string ("abcd", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, nullptr);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_math_func)
{
    struct dvobjs_math_item math_item[] = 
    {
        {
            "sin",
            PURC_VARIANT_TYPE_NUMBER,
            M_PI / 2,
            1.0d,
            "sin_l",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            M_PIl / 2,
            1.0d
        },
        {
            "cos",
            PURC_VARIANT_TYPE_NUMBER,
            M_PI,
            -1.0d,
            "cos_l",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            M_PIl,
            -1.0
        },
        {
            "sqrt",
            PURC_VARIANT_TYPE_NUMBER,
            9.0d,
            3.0d,
            "sqrt_l",
            PURC_VARIANT_TYPE_LONGDOUBLE,
            9.0d,
            3.0d
        }
    };

    size_t i = 0;
    size_t size = sizeof (math_item) / sizeof (struct dvobjs_math_item);
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    double number;
    long double numberl;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t math = pcdvojbs_get_math();
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);


    for (i = 0; i < size; i++) {
        purc_variant_t dynamic = purc_variant_object_get_c (math, math_item[i].func_d);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        purc_dvariant_method func = NULL;
        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        param[0] = purc_variant_make_number (math_item[i].param_d);
        param[1] = NULL;
        ret_var = func (NULL, 1, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var, math_item[i].type_d), true);
        purc_variant_cast_to_number (ret_var, &number, false);
        ASSERT_EQ(number, math_item[i].d);


        dynamic = purc_variant_object_get_c (math, math_item[i].func_ld);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        param[0] = purc_variant_make_longdouble (math_item[i].param_ld);
        param[1] = NULL;
        ret_var = func (NULL, 1, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var, math_item[i].type_ld), true);
        purc_variant_cast_to_long_double (ret_var, &numberl, false);
        ASSERT_EQ(numberl, math_item[i].ld);
    }

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_math_eval)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    double number;
    long double numberl;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t math = pcdvojbs_get_math();
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_c (math, "eval");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *exp = "(3 + 7) * (2 + 3 * 4)";
    param[0] = purc_variant_make_string (exp, false);
    param[1] = PURC_VARIANT_INVALID;
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST eval: param is \"%s\" = %lf\n", exp, number);

    exp = "(3 + 7) / (2 - 2)";
    param[0] = purc_variant_make_string (exp, false);
    param[1] = PURC_VARIANT_INVALID;
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST eval: param is \"%s\" = %f\n", exp, number);

    param[0] = purc_variant_make_string ("pi * r * r", false);
    param[1] = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                PURC_VARIANT_INVALID);
    purc_variant_object_set_c (param[1], "pi", purc_variant_make_number (M_PI));
    purc_variant_object_set_c (param[1], "r", purc_variant_make_number (1.0));
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST eval: param is \"pi * r * r\", r = 1.0, value = %lf\n", number);

    dynamic = purc_variant_object_get_c (math, "eval_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    exp = "(3 + 7) * (2 + 3)";
    param[0] = purc_variant_make_string (exp, false);
    param[1] = PURC_VARIANT_INVALID;
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST eval_l: param is \"%s\" = %Lf\n", exp, numberl);

    exp = "(3 + 7) / (2 - 2)";
    param[0] = purc_variant_make_string (exp, false);
    param[1] = PURC_VARIANT_INVALID;
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST eval_l: param is \"%s\" = %Lf\n", exp, numberl);

    param[0] = purc_variant_make_string ("pi * r * r", false);
    param[1] = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                PURC_VARIANT_INVALID);
    purc_variant_object_set_c (param[1], "pi", purc_variant_make_longdouble (M_PI));
    purc_variant_object_set_c (param[1], "r", purc_variant_make_longdouble (1.0));
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST eval_l: param is \"pi * r * r\", r = 1.0, value = %Lf\n", numberl);

    purc_cleanup ();
}

