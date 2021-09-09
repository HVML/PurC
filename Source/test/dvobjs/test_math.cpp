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

TEST(dvobjs, dvobjs_math_pi)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t math = pcdvojbs_get_math();
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_c (math, "pi");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    ret_var = func (NULL, 0, param);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);

    double number;
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST pi: %lf\n", number);

    dynamic = purc_variant_object_get_c (math, "pi_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    ret_var = func (NULL, 0, param);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);

    long double numberl;
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST pi_l: %Lf\n", numberl);

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_math_e)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t math = pcdvojbs_get_math();
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_c (math, "e");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    ret_var = func (NULL, 0, param);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);

    double number;
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST e: %lf\n", number);

    dynamic = purc_variant_object_get_c (math, "e_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    ret_var = func (NULL, 0, param);
    ASSERT_NE(ret_var, nullptr);

    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);

    long double numberl;
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST e_l: %Lf\n", numberl);

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_math_const)
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

    purc_variant_t dynamic = purc_variant_object_get_c (math, "const");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = purc_variant_make_string ("e", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"e\": %lf\n", number);

    param[0] = purc_variant_make_string ("log2e", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"log2e\": %lf\n", number);

    param[0] = purc_variant_make_string ("log10e", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"log10e\": %lf\n", number);

    param[0] = purc_variant_make_string ("ln2", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"ln2\": %lf\n", number);

    param[0] = purc_variant_make_string ("ln10", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"ln10\": %lf\n", number);

    param[0] = purc_variant_make_string ("pi", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"pi\": %lf\n", number);

    param[0] = purc_variant_make_string ("pi/2", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"pi/2\": %lf\n", number);

    param[0] = purc_variant_make_string ("pi/4", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"pi/4\": %lf\n", number);

    param[0] = purc_variant_make_string ("1/pi", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"1/pi\": %lf\n", number);

    param[0] = purc_variant_make_string ("2/pi", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"2/pi\": %lf\n", number);

    param[0] = purc_variant_make_string ("sqrt(2)", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST const: param is \"sqrt(2)\": %lf\n", number);


    dynamic = purc_variant_object_get_c (math, "const_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = purc_variant_make_string ("e", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"e\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("log2e", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"log2e\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("log10e", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const-L: param is \"log10e\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("ln2", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"ln2\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("ln10", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"ln10\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("pi", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"pi\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("pi/2", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"pi/2\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("pi/4", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"pi/4\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("1/pi", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"1/pi\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("2/pi", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"2/pi\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("sqrt(2)", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST const_l: param is \"sqrt(2)\": %Lf\n", numberl);

    param[0] = purc_variant_make_string ("abcd", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, nullptr);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_math_sin)
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

    purc_variant_t dynamic = purc_variant_object_get_c (math, "sin");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = purc_variant_make_number (M_PI / 2);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST sin: param is M_PI / 2 : %lf\n", number);


    dynamic = purc_variant_object_get_c (math, "sin_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = purc_variant_make_longdouble (M_PI / 2);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST sin_l: param is M_PI / 2 : %Lf\n", numberl);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_math_cos)
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

    purc_variant_t dynamic = purc_variant_object_get_c (math, "cos");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = purc_variant_make_number (M_PI);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST cos: param is M_PI : %lf\n", number);


    dynamic = purc_variant_object_get_c (math, "cos_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = purc_variant_make_longdouble (M_PI);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST cos_l: param is M_PI : %Lf\n", numberl);

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_math_sqrt)
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

    purc_variant_t dynamic = purc_variant_object_get_c (math, "sqrt");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = purc_variant_make_number (2.0d);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST sqrt: param is 2.0 : %lf\n", number);


    dynamic = purc_variant_object_get_c (math, "sqrt_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    param[0] = purc_variant_make_longdouble (2.0d);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE), true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST sqrt_l: param is 2.0 : %Lf\n", numberl);

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

