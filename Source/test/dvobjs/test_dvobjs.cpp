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
#include <gtest/gtest.h>

TEST(dvobjs, dvobjs_sys_uname)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    const char * result = NULL;
    size_t i = 0;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = pcdvojbs_get_system();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_c (sys, "uname");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    printf ("TEST get_uname: nr_args = 0, param = \"  beijing  shanghai\" :\n");
    param[0] = purc_variant_make_string ("  beijing shanghai", true);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_NE(ret_var, nullptr);

    purc_variant_object_iterator* it = purc_variant_object_make_iterator_begin(ret_var);
    for (i = 0; i < purc_variant_object_get_size (ret_var); i++) {
        const char     *key = purc_variant_object_iterator_get_key(it);
        purc_variant_t  val = purc_variant_object_iterator_get_value(it);

        result = purc_variant_get_string_const (val);

        printf("\t\t%s: %s\n", key, result);

        bool having = purc_variant_object_iterator_next(it);
        if (!having) {
            purc_variant_object_release_iterator(it);
            break;
        }
    }

    purc_cleanup ();
}
TEST(dvobjs, dvobjs_sys_uname_prt)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;
    const char * result = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = pcdvojbs_get_system();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_c (sys, "uname_prt");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    printf ("TEST get_uname_prt: nr_args = 1, param = NULL:\n");
    param[0] = purc_variant_make_string ("  hello   world  ", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, NULL);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_uname_prt: nr_args = 1, param[0] type is number:\n");
    param[0] = purc_variant_make_number (3.1415926);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_uname_prt: nr_args = 1, param = \"  hello   world  \" :\n");
    param[0] = purc_variant_make_string ("  hello   world  ", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");


    printf ("TEST get_uname_prt: nr_args = 0, param = \"hello world\" :\n");
    ret_var = func (NULL, 0, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);

    printf ("TEST get_uname_prt: nr_args = 1, param = \"all default\" :\n");
    param[0] = purc_variant_make_string ("all default", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);


    printf ("TEST get_uname_prt: nr_args = 1, param = \"default all\" :\n");
    param[0] = purc_variant_make_string ("default all", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);


    printf ("TEST get_uname_prt: nr_args = 1, param = \"hardware-platform kernel-version\" :\n");
    param[0] = purc_variant_make_string ("hardware-platform kernel-version", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);


    printf ("TEST get_uname_prt: nr_args = 1, param = \"   nodename   wrong-word   kernel-release   \" :\n");
    param[0] = purc_variant_make_string ("   nodename   wrong-word   kernel-release   ", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_sys_get_local)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = pcdvojbs_get_system();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_c (sys, "locale");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    printf ("TEST get_locale: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, NULL);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\t0 : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = NULL:\n");
    param[0] = purc_variant_make_string ("  hello   world  ", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, NULL);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_locale: nr_args = 1, param = \"hello world\":\n");
    param[0] = purc_variant_make_string ("hello world", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_locale: nr_args = 1, param[0] type is number:\n");
    param[0] = purc_variant_make_number (3.1415926);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_locale: nr_args = 1, param = ctype:\n");
    param[0] = purc_variant_make_string ("ctype", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tctype : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = numeric:\n");
    param[0] = purc_variant_make_string ("numeric", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tnumeric : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = time:\n");
    param[0] = purc_variant_make_string ("time", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\ttime : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = collate:\n");
    param[0] = purc_variant_make_string ("collate", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tcollate : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = monetary:\n");
    param[0] = purc_variant_make_string ("monetary", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tmonetary : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = messages:\n");
    param[0] = purc_variant_make_string ("messages", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tmessages : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = paper:\n");
    param[0] = purc_variant_make_string ("paper", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tpaper : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = name:\n");
    param[0] = purc_variant_make_string ("name", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tname : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = address:\n");
    param[0] = purc_variant_make_string ("address", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\taddress : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = telephone:\n");
    param[0] = purc_variant_make_string ("telephone", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\ttelephone : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = measurement:\n");
    param[0] = purc_variant_make_string ("measurement", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tmeasurement : %s\n", purc_variant_get_string_const (ret_var));

    printf ("TEST get_locale: nr_args = 1, param = identification:\n");
    param[0] = purc_variant_make_string ("identification", true);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tidentification : %s\n", purc_variant_get_string_const (ret_var));

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_sys_set_local)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = pcdvojbs_get_system();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_c (sys, "locale");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_setter (dynamic);
    ASSERT_NE(func, nullptr);

    printf ("TEST set_locale: nr_args = 1, param1 = \"all\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("all", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"all\", param2 type is number:\n");
    param[0] = purc_variant_make_string ("all", true);
    param[1] = purc_variant_make_number (3.1415926);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST set_locale: nr_args = 2, param1 type is number, param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_number (3.1415926);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 22, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"china\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("china", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"all\", param2 = \"china\":\n");
    param[0] = purc_variant_make_string ("china", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"all\", param2 = \"\":\n");
    param[0] = purc_variant_make_string ("all", true);
    param[1] = purc_variant_make_string ("", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"ctype\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("ctype", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"numeric\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("numeric", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"time\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("time", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"collate\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("collate", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"monetary\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("monetary", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"messages\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("messages", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"paper\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("paper", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"name\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("name", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"address\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("address", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"telephone\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("telephone", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"measurement\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("measurement", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    printf ("TEST set_locale: nr_args = 2, param1 = \"identification\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("identification", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    param[2] = NULL;
    ret_var = func (NULL, 2, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(ret_var, PURC_VARIANT_TRUE);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");

    purc_cleanup ();
}


TEST(dvobjs, dvobjs_sys_get_random)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = pcdvojbs_get_system();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_c (sys, "random");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    printf ("TEST get_random: nr_args = 0, param = 125.0d:\n");
    param[0] = purc_variant_make_number (125.0d);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_random: nr_args = 1, param = 1E-11:\n");
    param[0] = purc_variant_make_number (1E-11);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_random: nr_args = 1, param = 125.0d:\n");
    param[0] = purc_variant_make_number (125.0d);
    param[1] = NULL;
    ret_var = func (NULL, 1, param);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_number (ret_var), true);
    double number = 0.0d;
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("\t\tReturn random: %lf\n", number);

    purc_cleanup ();
}
