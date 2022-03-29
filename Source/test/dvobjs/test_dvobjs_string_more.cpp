#include "TestDVObj.h"

#include "../helpers.h"

TEST(dvobjs, basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t dvobj;

    dvobj = purc_dvobj_string_new();
    ASSERT_EQ(purc_variant_is_object(dvobj), true);
    purc_variant_unref(dvobj);

    purc_cleanup();
}

TEST(dvobjs, contains)
{
    char data_path[4096 + 1];
    const char *env = "DVOBJS_TEST_PATH";

    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
            env, "test_files");

    TestDVObj::run_testcases_in_file("STR", data_path, "contains");
}
