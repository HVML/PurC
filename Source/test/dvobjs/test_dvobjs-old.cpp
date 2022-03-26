#include "purc.h"
#include "purc-variant.h"

#include "private/avl.h"
#include "private/hashtable.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"

#include "../helpers.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv);
#define MAX_PARAM_NR    20

TEST(dvobjs, basic)
{
    char buff[PATH_MAX + 1];
    char *cwd;
    cwd = getcwd(buff, sizeof(buff));

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);
    bool ok;
    purc_variant_t v;

    if (0) {
        fprintf(stderr, "cwd: %s\n", cwd);
        v = purc_variant_load_dvobj_from_so("/usr/local/lib/purc-0.0/libpurc-dvobj-MATH.so", "MATH");
        ASSERT_NE(v, PURC_VARIANT_INVALID);
        ok = purc_variant_unload_dvobj(v);
        ASSERT_TRUE(ok);

        v = purc_variant_load_dvobj_from_so("../../../../../../usr/local/lib/purc-0.0/libpurc-dvobj-FS.so", "FS");
        ASSERT_NE(v, PURC_VARIANT_INVALID);
        ok = purc_variant_unload_dvobj(v);
        ASSERT_TRUE(ok);

        v = purc_variant_load_dvobj_from_so("libpurc-dvobj-FS.so", "FS");
        ASSERT_NE(v, PURC_VARIANT_INVALID);
        ok = purc_variant_unload_dvobj(v);
        ASSERT_TRUE(ok);
    }

    v = purc_variant_load_dvobj_from_so(NULL, "MATH");
    ASSERT_NE(v, PURC_VARIANT_INVALID);
    ok = purc_variant_unload_dvobj(v);
    ASSERT_TRUE(ok);

    purc_cleanup ();
}

TEST(dvobjs, dvobjs_sys_uname)
{
    purc_variant_t param[MAX_PARAM_NR] = {PURC_VARIANT_INVALID};
    purc_variant_t ret_var = NULL;
    const char *result = NULL;
    size_t i = 0;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (sys, "uname",
            false);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    printf ("TEST get_uname: nr_args = 0, param = \"  beijing  shanghai\" :\n");
    param[0] = purc_variant_make_string ("  beijing shanghai", true);
    ret_var = func (NULL, 0, param, false);
    ASSERT_NE(ret_var, nullptr);

    purc_variant_object_iterator *it =
        purc_variant_object_make_iterator_begin (ret_var);
    for (ssize_t i = 0; i < purc_variant_object_get_size (ret_var); i++) {
        const char     *key = purc_variant_object_iterator_get_ckey (it);
        purc_variant_t  val = purc_variant_object_iterator_get_value (it);

        result = purc_variant_get_string_const (val);

        printf("\t\t%s: %s\n", key, result);

        bool having = purc_variant_object_iterator_next (it);
        if (!having) {
            purc_variant_object_release_iterator (it);
            break;
        }
    }

    for (i = 0; i < 10; i++) {
        if (param[i])
            purc_variant_unref (param[i]);
    }
    purc_variant_unref (ret_var);
    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before +
            (nr_reserved_after - nr_reserved_before) * sizeof(purc_variant));

    purc_variant_unref (sys);
    purc_cleanup ();
}

TEST(dvobjs, dvobjs_sys_uname_prt)
{
    purc_variant_t param[MAX_PARAM_NR] = {PURC_VARIANT_INVALID};
    purc_variant_t ret_var = NULL;
    const char *result = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (sys, "uname_prt",
            false);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    printf ("TEST get_uname_prt: nr_args = 1, param[0] type is number:\n");
    param[0] = purc_variant_make_number (3.1415926);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    purc_variant_unref (param[0]);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_uname_prt: nr_args = 1, \
            param = \"  hello   world  \" :\n");
    param[0] = purc_variant_make_string ("  hello   world  ", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    purc_variant_unref (param[0]);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");

    printf ("TEST get_uname_prt: nr_args = 0, param = \"hello world\" :\n");
    ret_var = func (NULL, 0, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);
    purc_variant_unref (ret_var);

    printf ("TEST get_uname_prt: nr_args = 1, param = \"all default\" :\n");
    param[0] = purc_variant_make_string ("all default", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

    printf ("TEST get_uname_prt: nr_args = 1, param = \"default all\" :\n");
    param[0] = purc_variant_make_string ("default all", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

    printf ("TEST get_uname_prt: nr_args = 1, \
            param = \"hardware-platform kernel-version\" :\n");
    param[0] = purc_variant_make_string ("hardware-platform kernel-version",
            true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);


    printf ("TEST get_uname_prt: nr_args = 1, \
            param = \"   nodename   wrong-word   kernel-release   \" :\n");
    param[0] = purc_variant_make_string (
            "   nodename   wrong-word   kernel-release   ", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    result = purc_variant_get_string_const (ret_var);
    printf("\t\tReturn : %s\n", result);
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before +
            (nr_reserved_after - nr_reserved_before) * sizeof(purc_variant));

    purc_variant_unref (sys);
    purc_cleanup ();
}

TEST(dvobjs, dvobjs_sys_get_locale)
{
    purc_variant_t param[MAX_PARAM_NR] = {PURC_VARIANT_INVALID};
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (sys, "locale", false);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    printf ("TEST get_locale: nr_args = 0, param = NULL:\n");
    ret_var = func (NULL, 0, NULL, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tmessages : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);

    printf ("TEST get_locale: nr_args = 1, param = NULL:\n");
    param[0] = purc_variant_make_string ("  hello   world  ", true);
    ret_var = func (NULL, 1, NULL, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);

    printf ("TEST get_locale: nr_args = 1, param = \"hello world\":\n");
    param[0] = purc_variant_make_string ("hello world", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);

    printf ("TEST get_locale: nr_args = 1, param[0] type is number:\n");
    param[0] = purc_variant_make_number (3.1415926);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);

    printf ("TEST get_locale: nr_args = 1, param = ctype:\n");
    param[0] = purc_variant_make_string ("ctype", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tctype : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

    printf ("TEST get_locale: nr_args = 1, param = numeric:\n");
    param[0] = purc_variant_make_string ("numeric", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tnumeric : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

    printf ("TEST get_locale: nr_args = 1, param = time:\n");
    param[0] = purc_variant_make_string ("time", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\ttime : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

    printf ("TEST get_locale: nr_args = 1, param = collate:\n");
    param[0] = purc_variant_make_string ("collate", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tcollate : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

    printf ("TEST get_locale: nr_args = 1, param = monetary:\n");
    param[0] = purc_variant_make_string ("monetary", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tmonetary : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

    printf ("TEST get_locale: nr_args = 1, param = messages:\n");
    param[0] = purc_variant_make_string ("messages", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tmessages : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);

#ifdef LC_PAPER
    printf ("TEST get_locale: nr_args = 1, param = paper:\n");
    param[0] = purc_variant_make_string ("paper", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tpaper : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);
#endif // LC_PAPER

#ifdef LC_NAME
    printf ("TEST get_locale: nr_args = 1, param = name:\n");
    param[0] = purc_variant_make_string ("name", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tname : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);
#endif // LC_NAME

#ifdef LC_ADDRESS
    printf ("TEST get_locale: nr_args = 1, param = address:\n");
    param[0] = purc_variant_make_string ("address", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\taddress : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);
#endif // LC_ADDRESS

#ifdef LC_TELEPHONE
    printf ("TEST get_locale: nr_args = 1, param = telephone:\n");
    param[0] = purc_variant_make_string ("telephone", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\ttelephone : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);
#endif // LC_TELEPHONE

#ifdef LC_MEASUREMENT
    printf ("TEST get_locale: nr_args = 1, param = measurement:\n");
    param[0] = purc_variant_make_string ("measurement", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tmeasurement : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);
#endif // LC_MEASUREMENT

#ifdef LC_IDENTIFICATION
    printf ("TEST get_locale: nr_args = 1, param = identification:\n");
    param[0] = purc_variant_make_string ("identification", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_string (ret_var), true);
    printf("\t\tidentification : %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (ret_var);
    purc_variant_unref (param[0]);
#endif // LC_IDENTIFICATION

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before +
            (nr_reserved_after - nr_reserved_before) * sizeof(purc_variant));

    purc_variant_unref (sys);
    purc_cleanup ();
}


TEST(dvobjs, dvobjs_sys_set_locale)
{
    purc_variant_t param[MAX_PARAM_NR] = {PURC_VARIANT_INVALID};
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (sys, "locale", false);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_setter (dynamic);
    ASSERT_NE(func, nullptr);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    printf ("TEST set_locale: nr_args = 1, \
            param1 = \"all\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("all", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"all\", param2 type is number:\n");
    param[0] = purc_variant_make_string ("all", true);
    param[1] = purc_variant_make_number (3.1415926);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);

    printf ("TEST set_locale: nr_args = 2, \
            param1 type is number, param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_number (3.1415926);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"china\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("china", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"all\", param2 = \"china\":\n");
    param[0] = purc_variant_make_string ("china", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"all\", param2 = \"\":\n");
    param[0] = purc_variant_make_string ("all", true);
    param[1] = purc_variant_make_string ("", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"ctype\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("ctype", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"numeric\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("numeric", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"time\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("time", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"collate\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("collate", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"monetary\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("monetary", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"messages\", param2 = \"en_US.UTF-8\":\n");
    param[0] = purc_variant_make_string ("messages", true);
    param[1] = purc_variant_make_string ("en_US.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"paper\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("paper", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"name\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("name", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"address\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("address", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"telephone\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("telephone", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"measurement\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("measurement", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    printf ("TEST set_locale: nr_args = 2, \
            param1 = \"identification\", param2 = \"zh_CN.UTF-8\":\n");
    param[0] = purc_variant_make_string ("identification", true);
    param[1] = purc_variant_make_string ("zh_CN.UTF-8", true);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, nullptr);
    printf("\t\tReturn PURC_VARIANT_TRUE\n");
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before +
            (nr_reserved_after - nr_reserved_before) * sizeof(purc_variant));

    purc_variant_unref (sys);
    purc_cleanup ();
}


TEST(dvobjs, dvobjs_sys_get_random)
{
    purc_variant_t param[MAX_PARAM_NR] = {PURC_VARIANT_INVALID};
    purc_variant_t ret_var = NULL;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (sys, "random", false);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    printf ("TEST get_random: nr_args = 0, param = 125.0d:\n");
    param[0] = purc_variant_make_number (125.0);
    ret_var = func (NULL, 0, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);

    printf ("TEST get_random: nr_args = 1, param = 1E-11:\n");
    param[0] = purc_variant_make_number (1E-11);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
    printf("\t\tReturn PURC_VARIANT_INVALID\n");
    purc_variant_unref (param[0]);

    printf ("TEST get_random: nr_args = 1, param = 125.0d:\n");
    param[0] = purc_variant_make_number (125.0);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, nullptr);
    ASSERT_EQ(purc_variant_is_number (ret_var), true);
    double number = 0.0;
    purc_variant_cast_to_number (ret_var, &number, false);
    printf("\t\tReturn random: %lf\n", number);
    purc_variant_unref (param[0]);
    purc_variant_unref (ret_var);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before +
            (nr_reserved_after - nr_reserved_before) * sizeof(purc_variant));

    purc_variant_unref (sys);
    purc_cleanup ();
}


TEST(dvobjs, dvobjs_sys_gettime)
{
    purc_variant_t param[MAX_PARAM_NR] = {PURC_VARIANT_INVALID};
    purc_variant_t ret_var = NULL;
    double number = 0;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t sys = purc_dvobj_system_new();
    ASSERT_NE(sys, nullptr);
    ASSERT_EQ(purc_variant_is_object (sys), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (sys, "time", false);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

    printf ("TEST get_time: nr_args = 0 :\n");
    time_t t_time;
    t_time = time (NULL);
    ret_var = func (NULL, 0, param, false);
    ASSERT_NE(ret_var, PURC_VARIANT_INVALID);
    ASSERT_EQ(t_time, ret_var->u64);
    purc_variant_unref (ret_var);

    printf ("TEST get_time: nr_args = 1, param = \"tm\":\n");
    param[0] = purc_variant_make_string ("tm", false);
    ret_var = func (NULL, 1, param, false);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                PURC_VARIANT_TYPE_OBJECT), true);
    purc_variant_object_iterator *it =
        purc_variant_object_make_iterator_begin (ret_var);
    for (ssize_t i = 0; i < purc_variant_object_get_size (ret_var); i++) {
        const char     *key = purc_variant_object_iterator_get_ckey (it);
        purc_variant_t  val = purc_variant_object_iterator_get_value (it);

        purc_variant_cast_to_number (val, &number, false);

        printf("\t\t%s: %d\n", key, (int)number);

        bool having = purc_variant_object_iterator_next (it);
        if (!having) {
            purc_variant_object_release_iterator (it);
            break;
        }
    }
    purc_variant_unref (param[0]);
    purc_variant_unref (ret_var);

    printf ("TEST get_time: nr_args = 1, param = \"iso8601\":\n");
    param[0] = purc_variant_make_string ("iso8601", false);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                PURC_VARIANT_TYPE_STRING), true);
    printf("\t\tReturn: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (param[0]);
    purc_variant_unref (ret_var);

    printf ("TEST get_time: nr_args = 1, param = \"rfc822\":\n");
    param[0] = purc_variant_make_string ("rfc822", false);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                PURC_VARIANT_TYPE_STRING), true);
    printf("\t\tReturn: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (param[0]);
    purc_variant_unref (ret_var);

    printf ("TEST get_time: nr_args = 1, param = \"abcdefg\":\n");
    param[0] = purc_variant_make_string ("abcdefg", false);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                PURC_VARIANT_TYPE_STRING), true);
    ASSERT_STREQ ("abcdefg", purc_variant_get_string_const (ret_var));
    purc_variant_unref (param[0]);
    purc_variant_unref (ret_var);

    printf ("TEST get_time: nr_args = 1, \
            param = \"beijing time %%Y-%%m-%%d, %%H:%%M:%%S, shenzhen\"\n");
    param[0] = purc_variant_make_string (
            "beijing time %Y-%m-%d, %H:%M:%S, shenzhen", false);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                PURC_VARIANT_TYPE_STRING), true);
    printf("\t\tReturn: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (param[0]);
    purc_variant_unref (ret_var);

    printf ("TEST get_time: nr_args = 1, \
            param = \"beijing time %%Y-%%m-%%d, shenzhen\"\n");
    param[0] = purc_variant_make_string (
            "beijing time %Y-%m-%d, shenzhen", false);
    ret_var = func (NULL, 1, param, false);
    ASSERT_NE(ret_var, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                PURC_VARIANT_TYPE_STRING), true);
    printf("\t\tReturn: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (param[0]);
    purc_variant_unref (ret_var);

    printf ("TEST get_time: nr_args = 2, \
            param = \"beijing time %%Y-%%m-%%d, %%H:%%M:%%S, shenzhen\", \
            %ld\n", t_time - 24 * 60 * 60);
    param[0] = purc_variant_make_string (
            "beijing time %Y-%m-%d, %H:%M:%S, shenzhen", false);
    param[1] = purc_variant_make_number (t_time - 24 * 60 * 60);
    ret_var = func (NULL, 2, param, false);
    ASSERT_NE(ret_var, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                PURC_VARIANT_TYPE_STRING), true);
    printf("\t\tReturn: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (ret_var);


    printf ("TEST get_time: nr_args = 2, \
            param = \"beijing time %%Y-%%m-%%d, %%H:%%M:%%S, shenzhen\", \
            %ld, Europe/Belgrade\n", t_time - 24 * 60 * 60);
    param[0] = purc_variant_make_string (
            "beijing time %Y-%m-%d, %H:%M:%S, shenzhen", false);
    param[1] = purc_variant_make_number (t_time - 24 * 60 * 60);
    param[2] = purc_variant_make_string ("Europe/Belgrade", false);
    ret_var = func (NULL, 3, param, false);
    ASSERT_NE(ret_var, PURC_VARIANT_INVALID);
    ASSERT_EQ(purc_variant_is_type (ret_var,
                PURC_VARIANT_TYPE_STRING), true);
    printf("\t\tReturn: %s\n", purc_variant_get_string_const (ret_var));
    purc_variant_unref (param[0]);
    purc_variant_unref (param[1]);
    purc_variant_unref (param[2]);
    purc_variant_unref (ret_var);

    get_variant_total_info (&sz_total_mem_after,
            &sz_total_values_after, &nr_reserved_after);
    ASSERT_EQ(sz_total_values_before, sz_total_values_after);
    ASSERT_EQ(sz_total_mem_after, sz_total_mem_before +
            (nr_reserved_after - nr_reserved_before) * sizeof(purc_variant));

    purc_variant_unref (sys);
    purc_cleanup ();
}

TEST(dvobjs, reuse_buff)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_rwstream_t rws;
    rws = purc_rwstream_new_buffer (32, 1024);
    purc_rwstream_write(rws, "hello", 5);
    purc_rwstream_write(rws, "\0", 1);

    size_t content_size, raw_size;
    char *s;
    s = (char*)purc_rwstream_get_mem_buffer_ex(rws,
            &content_size, &raw_size, true);

    ASSERT_NE(s, nullptr);
    ASSERT_EQ(content_size, 6);
    ASSERT_GT(raw_size, content_size);
    ASSERT_EQ(memcmp("hello", s, 5), 0);

    purc_rwstream_destroy(rws);

    purc_variant_t v;
    v = purc_variant_make_string_reuse_buff(s, content_size, false);
    purc_variant_unref(v);

    purc_cleanup ();
}

