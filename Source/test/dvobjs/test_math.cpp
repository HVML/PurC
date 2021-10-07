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
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <gtest/gtest.h>

extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv);
#define MAX_PARAM_NR    20

struct dvobjs_math_method_d
{
    const char * func;
    double param;
    double d;
};

struct dvobjs_math_method_ld
{
    const char * func;
    long double param;
    long double ld;
};

TEST(dvobjs, dvobjs_math_pi_e)
{
    static struct dvobjs_math_method_d math_d[] =
    {
        {
            "pi",
            0.0d,
            M_PI,
        },
        {
            "e",
            0.0d,
            M_E,
        }
    };

    static struct dvobjs_math_method_ld math_ld[] =
    {
        {
            "pi_l",
            0.0d,
            M_PIl
        },
        {
            "e_l",
            0.0d,
            M_El
        }
    };

    size_t i = 0;
    size_t size = PCA_TABLESIZE(math_d);
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    double number;
    long double numberl;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    purc_variant_t math = purc_variant_load_dvobj_from_so (
            "/usr/local/lib/purc-0.0/libpurc-dvobj-MATH.so", "MATH");
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    for (i = 0; i < size; i++) {
        // test double function
        purc_variant_t dynamic = purc_variant_object_get_by_ckey (math,
                math_d[i].func);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        purc_dvariant_method func = NULL;
        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        ret_var = func (NULL, 0, param);
        ASSERT_NE(ret_var, nullptr);

        ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER),
                true);

        purc_variant_cast_to_number (ret_var, &number, false);
        ASSERT_EQ(number, math_d[i].d);
        purc_variant_unref(ret_var);

        // test long double function
        dynamic = purc_variant_object_get_by_ckey (math, math_ld[i].func);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        ret_var = func (NULL, 0, param);
        ASSERT_NE(ret_var, nullptr);

        ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE),
                true);

        purc_variant_cast_to_long_double (ret_var, &numberl, false);
        ASSERT_EQ(numberl, math_ld[i].ld);
        purc_variant_unref(ret_var);
    }

    purc_variant_unload_dvobj (math);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_math_const)
{
    static struct dvobjs_math_method_d math_d[] =
    {
        {
            "e",
            0.0d,
            M_E,
        },
        {
            "log2e",
            0.0d,
            M_LOG2E,
        },
        {
            "log10e",
            0.0d,
            M_LOG10E,
        },
        {
            "ln2",
            0.0d,
            M_LN2,
        },
        {
            "ln10",
            0.0d,
            M_LN10,
        },
        {
            "pi",
            0.0d,
            M_PI,
        },
        {
            "pi/2",
            0.0d,
            M_PI_2,
        },
        {
            "pi/4",
            0.0d,
            M_PI_4,
        },
        {
            "1/pi",
            0.0d,
            M_1_PI,
        },
        {
            "1/sqrt(2)",
            0.0d,
            M_SQRT1_2,
        },
        {
            "2/pi",
            0.0d,
            M_2_PI,
        },
        {
            "sqrt(2)",
            0.0d,
            M_SQRT2,
        }
    };

    static struct dvobjs_math_method_ld math_ld[] =
    {
        {
            "e",
            0.0d,
            M_El
        },
        {
            "log2e",
            0.0d,
            M_LOG2El
        },
        {
            "log10e",
            0.0d,
            M_LOG10El
        },
        {
            "ln2",
            0.0d,
            M_LN2l
        },
        {
            "ln10",
            0.0d,
            M_LN10l
        },
        {
            "pi",
            0.0d,
            M_PIl
        },
        {
            "pi/2",
            0.0d,
            M_PI_2l
        },
        {
            "pi/4",
            0.0d,
            M_PI_4l
        },
        {
            "1/pi",
            0.0d,
            M_1_PIl
        },
        {
            "1/sqrt(2)",
            0.0d,
            M_SQRT1_2l
        },
        {
            "2/pi",
            0.0d,
            M_2_PIl
        },
        {
            "sqrt(2)",
            0.0d,
            M_SQRT2l
        }
    };

    size_t i = 0;
    size_t size = PCA_TABLESIZE(math_d);
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    double number;
    long double numberl;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;


    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    purc_variant_t math = purc_variant_load_dvobj_from_so (
            "/usr/local/lib/purc-0.0/libpurc-dvobj-MATH.so", "MATH");
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (math, "const");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    for (i = 0; i < size; i++) {
        param[0] = purc_variant_make_string (math_d[i].func, true);
        ret_var = func (NULL, 1, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER),
                true);
        purc_variant_cast_to_number (ret_var, &number, false);
        ASSERT_EQ(number, math_d[i].d);
        purc_variant_unref(ret_var);
        purc_variant_unref(param[0]);
    }

    dynamic = purc_variant_object_get_by_ckey (math, "const_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    for (i = 0; i < size; i++) {
        param[0] = purc_variant_make_string (math_ld[i].func, true);
        ret_var = func (NULL, 1, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE),
                true);
        purc_variant_cast_to_long_double (ret_var, &numberl, false);
        ASSERT_EQ(numberl, math_ld[i].ld);
        purc_variant_unref(ret_var);
        purc_variant_unref(param[0]);
    }

    param[0] = purc_variant_make_string ("abcd", true);
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, nullptr);
    purc_variant_unref(param[0]);

    purc_variant_unload_dvobj (math);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_math_func)
{
    static struct dvobjs_math_method_d math_d[] =
    {
        {
            "sin",
            M_PI / 2,
            1.0d,
        },
        {
            "cos",
            M_PI,
            -1.0d,
        },
        {
            "sqrt",
            9.0d,
            3.0d,
        }
    };

    static struct dvobjs_math_method_ld math_ld[] =
    {
        {
            "sin_l",
            M_PIl / 2,
            1.0d
        },
        {
            "cos_l",
            M_PIl,
            -1.0
        },
        {
            "sqrt_l",
            9.0d,
            3.0d
        }
    };

    size_t i = 0;
    size_t size = PCA_TABLESIZE(math_d);
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    double number;
    long double numberl;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    purc_variant_t math = purc_variant_load_dvobj_from_so (
            "/usr/local/lib/purc-0.0/libpurc-dvobj-MATH.so", "MATH");
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);


    for (i = 0; i < size; i++) {
        purc_variant_t dynamic = purc_variant_object_get_by_ckey (math,
                math_d[i].func);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        purc_dvariant_method func = NULL;
        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        param[0] = purc_variant_make_number (math_d[i].param);
        ret_var = func (NULL, 1, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER),
                true);
        purc_variant_cast_to_number (ret_var, &number, false);
        ASSERT_EQ(number, math_d[i].d);

        purc_variant_unref(ret_var);
        purc_variant_unref(param[0]);


        dynamic = purc_variant_object_get_by_ckey (math, math_ld[i].func);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        param[0] = purc_variant_make_longdouble (math_ld[i].param);
        ret_var = func (NULL, 1, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE),
                true);
        purc_variant_cast_to_long_double (ret_var, &numberl, false);
        ASSERT_EQ(numberl, math_ld[i].ld);

        purc_variant_unref(ret_var);
        purc_variant_unref(param[0]);
    }

    purc_variant_unload_dvobj (math);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_math_eval)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    double number;
    long double numberl;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    purc_variant_t math = purc_variant_load_dvobj_from_so (
            "/usr/local/lib/purc-0.0/libpurc-dvobj-MATH.so", "MATH");
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (math, "eval");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *exp = "(3 + 7) * (2 + 3 * 4)";
    param[0] = purc_variant_make_string (exp, false);
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST eval: param is \"%s\" = %lf\n", exp, number);
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);

    exp = "(3 + 7) / (2 - 2)";
    param[0] = purc_variant_make_string (exp, false);
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST eval: param is \"%s\" = %f\n", exp, number);
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);

    param[0] = purc_variant_make_string ("pi * r * r", false);
    param[1] = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                PURC_VARIANT_INVALID);
    purc_variant_t pi = purc_variant_make_number(M_PI);
    purc_variant_t one = purc_variant_make_number(1.0);
    purc_variant_object_set_by_static_ckey (param[1], "pi", pi);
    purc_variant_object_set_by_static_ckey (param[1], "r", one);
    purc_variant_unref(one);
    purc_variant_unref(pi);
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST eval: param is \"pi * r * r\", r = 1.0, value = %lf\n",
            number);
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);

    dynamic = purc_variant_object_get_by_ckey (math, "eval_l");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    exp = "(3 + 7) * (2 + 3)";
    param[0] = purc_variant_make_string (exp, false);
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE),
            true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST eval_l: param is \"%s\" = %Lf\n", exp, numberl);
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);

    exp = "(3 + 7) / (2 - 2)";
    param[0] = purc_variant_make_string (exp, false);
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE),
            true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST eval_l: param is \"%s\" = %Lf\n", exp, numberl);
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);

    param[0] = purc_variant_make_string ("pi * r * r", false);
    param[1] = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                PURC_VARIANT_INVALID);
    pi = purc_variant_make_longdouble(M_PI);
    one = purc_variant_make_longdouble (1.0);
    purc_variant_object_set_by_static_ckey (param[1], "pi", pi);
    purc_variant_object_set_by_static_ckey (param[1], "r", one);
    purc_variant_unref(one);
    purc_variant_unref(pi);
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_LONGDOUBLE),
            true);
    purc_variant_cast_to_long_double (ret_var, &numberl, false);
    printf("TEST eval_l: param is \"pi * r * r\", r = 1.0, value = %Lf\n",
            numberl);
    purc_variant_unref(ret_var);
    purc_variant_unref(param[0]);
    purc_variant_unref(param[1]);

    purc_variant_unload_dvobj (math);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_math_assignment)
{
    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;
    double number;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    purc_variant_t math = purc_variant_load_dvobj_from_so (
            "/usr/local/lib/purc-0.0/libpurc-dvobj-MATH.so", "MATH");
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (math, "eval");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *exp = "x = (3 + 7) * (2 + 3 * 4)\nx*3";
    param[0] = purc_variant_make_string (exp, false);
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_NUMBER), true);
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("TEST eval: param is \"%s\" = %lf\n", exp, number);

    purc_variant_unref(param[0]);

    purc_variant_unref(ret_var);

    purc_variant_unload_dvobj (math);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

struct test_sample {
    const char      *expr;
    const char      *result;
};

TEST(dvobjs, dvobjs_math_samples)
{
    struct test_sample samples[] = {
        {"1+2", "3"},
        {"-1", "-1"},
        {"1+-2", "-1"},
        {"1 + - 2", "-1"},
        {"x = (3 + 7) * (2 + 3 * 4)\nx*3", "420"},
        {"-(3+4)", "-7"},
        {"1+2\n", "3"},
        {"1+2\n\n", "3"},
        {"\n\n1+2\n\n", "3"},
        {"\n\n1+2", "3"},
        {"\n1+2", "3"},
    };
    purc_variant_t param[MAX_PARAM_NR];
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    purc_variant_t math = purc_variant_load_dvobj_from_so (
            "/usr/local/lib/purc-0.0/libpurc-dvobj-MATH.so", "MATH");
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (math, "eval");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    char buf[4096];
    purc_rwstream_t ws = purc_rwstream_new_from_mem(buf, sizeof(buf)-1);
    for (size_t i=0; i<sizeof(samples)/sizeof(samples[0]); ++i) {
        purc_rwstream_seek(ws, 0, SEEK_SET);
        buf[0] = '\0';
        buf[sizeof(buf)-1] = '\0';

        const char *expr= samples[i].expr;
        param[0] = purc_variant_make_string(expr, false);
        purc_variant_t ret_var = func(NULL, 1, param);
        purc_variant_unref(param[0]);

        if (!ret_var) {
            EXPECT_NE(ret_var, nullptr) << "eval failed: ["
                << expr << "]" << std::endl;
            continue;
        }
        ssize_t nr = purc_variant_serialize(ret_var, ws,
                        0, 0, NULL);
        EXPECT_GE(nr, 0) << "cast failed: ["
            << expr << "]" << std::endl;
        if (nr>=0)
            buf[nr] = '\0';
        EXPECT_STREQ(buf, samples[i].result) << "eval failed: ["
                << expr << "]" << std::endl;
        purc_variant_unref(ret_var);
    }

    purc_rwstream_destroy(ws);

    purc_variant_unload_dvobj (math);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before + (nr_reserved_after -
                nr_reserved_before) * sizeof(purc_variant));

    purc_cleanup ();
}

static void
_trim_tail_spaces(char *dest, size_t n)
{
    while (n>1) {
        if (!isspace(dest[n-1]))
            break;
        dest[--n] = '\0';
    }
}

static void
_eval(purc_dvariant_method func, const char *expr,
    char *dest, size_t dlen)
{
    size_t n = 0;
    dest[0] = '\0';

    purc_variant_t param[3];
    param[0] = purc_variant_make_string(expr, false);

    purc_variant_t ret_var = func(NULL, 1, param);
    purc_variant_unref(param[0]);

    if (!ret_var) {
        EXPECT_NE(ret_var, nullptr) << "eval failed: ["
            << expr << "]" << std::endl;
        return;
    }

    purc_rwstream_t ows;
    ows = purc_rwstream_new_from_mem(dest, dlen-1);
    if (!ows)
        goto end;

    purc_variant_serialize(ret_var, ows, 0, 0, &n);
    purc_rwstream_get_mem_buffer(ows, NULL);
    dest[n] = '\0';
    _trim_tail_spaces(dest, n);

    purc_rwstream_destroy(ows);

end:
    purc_variant_unref(ret_var);
}

static void
_eval_bc(const char *fn, char *dest, size_t dlen)
{
    FILE *fin = NULL;
    char cmd[8192];
    size_t n = 0;

    snprintf(cmd, sizeof(cmd), "cat '%s' | bc", fn);
    fin = popen(cmd, "r");
    EXPECT_NE(fin, nullptr) << "failed to execute: [" << cmd << "]"
        << std::endl;
    if (!fin)
        goto end;

    n = fread(dest, 1, dlen-1, fin);
    dest[n] = '\0';
    _trim_tail_spaces(dest, n);

end:
    if (fin)
        pclose(fin);
}

static void
_process_file(purc_dvariant_method func, const char *fn,
    char *dest, size_t dlen)
{
    FILE *fin = NULL;
    size_t sz = 0;
    char buf[8192];
    buf[0] = '\0';

    fin = fopen(fn, "r");
    if (!fin) {
        int err = errno;
        EXPECT_NE(fin, nullptr) << "Failed to open ["
            << fn << "]: [" << err << "]" << strerror(err) << std::endl;
        goto end;
    }

    sz = fread(buf, 1, sizeof(buf)-1, fin);
    buf[sz] = '\0';

    _eval(func, buf, dest, dlen);

end:
    if (fin)
        fclose(fin);
}

TEST(dvobjs, dvobjs_math_bc)
{
    int r = 0;
    DIR *d = NULL;
    struct dirent *dir = NULL;
    char path[1024] = {0};

    purc_instance_extra_info info = {0, 0};
    r = purc_init("cn.fmsoft.hybridos.test",
        "dvobjs_math_bc", &info);
    EXPECT_EQ(r, PURC_ERROR_OK);
    if (r)
        return;

    purc_variant_t math = purc_variant_load_dvobj_from_so (
            "/usr/local/lib/purc-0.0/libpurc-dvobj-MATH.so", "MATH");
    ASSERT_NE(math, nullptr);
    ASSERT_EQ(purc_variant_is_object (math), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (math, "eval");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    const char *math_path = getenv(env);
    std::cout << "env: " << env << "=" << math_path << std::endl;
    EXPECT_NE(math_path, nullptr) << "You shall specify via env `"
        << env << "`" << std::endl;
    if (!math_path)
        goto end;

    strcpy (path, math_path);
    strcat (path, "/math_bc");

    d = opendir(path);
    EXPECT_NE(d, nullptr) << "Failed to open dir @["
            << path << "]: [" << errno << "]" << strerror(errno)
            << std::endl;

    if (d) {
        if (chdir(path) != 0) {
            purc_variant_unref(math);
            return;
        }
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type & DT_REG) {
                char l[8192], r[8192];
                _process_file(func, dir->d_name, l, sizeof(l));
                _eval_bc(dir->d_name, r, sizeof(r));
                fprintf(stderr, "[%s] =?= [%s]\n", l, r);
                EXPECT_STREQ(l, r) << "Failed to parse bc file: ["
                    << dir->d_name << "]" << std::endl;
            }
        }
        closedir(d);
    }

end:
    if (math)
        purc_variant_unload_dvobj (math);

    purc_cleanup ();
}
