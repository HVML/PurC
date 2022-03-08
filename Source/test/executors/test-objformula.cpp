#include "purc.h"

#include "purc-executor.h"

#include "private/utils.h"

#include <gtest/gtest.h>
#include <glob.h>
#include <libgen.h>
#include <limits.h>

#include "../helpers.h"

extern "C" {
#include "pcexe-helper.h"
#include "exe_objformula.h"
#include "exe_objformula.tab.h"
}

#include "utils.cpp.in"

TEST(exe_objformula, basic)
{
    purc_instance_extra_info info = {};
    bool cleanup = false;

    // initial purc
    int ret = purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hvml.test",
            "exe_objformula", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    bool ok;

    struct purc_exec_ops ops;
    memset(&ops, 0, sizeof(ops));
    ok = purc_register_executor("OBJFORMULA", &ops);
    EXPECT_FALSE(ok);
    EXPECT_EQ(purc_get_last_error(), PCEXECUTOR_ERROR_ALREAD_EXISTS);

    cleanup = purc_cleanup();
    ASSERT_EQ(cleanup, true);
}

static inline bool
parse(const char *rule, char *err_msg, size_t sz_err_msg)
{
    struct exe_objformula_param param;
    memset(&param, 0, sizeof(param));
    param.debug_flex         = debug_flex;
    param.debug_bison        = debug_bison;

    bool r;
    r = exe_objformula_parse(rule, strlen(rule), &param) == 0;
    if (param.err_msg) {
        snprintf(err_msg, sz_err_msg, "%s", param.err_msg);
        free(param.err_msg);
        param.err_msg = NULL;
    }

    exe_objformula_param_reset(&param);

    return r;
}

TEST(exe_objformula, files)
{
    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    purc_instance_extra_info info = {};
    r = purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hvml.test", "exe_objformula",
            &info);
    EXPECT_EQ(r, PURC_ERROR_OK);
    if (r)
        return;

    const char *rel = "data/objformula.*.rule";
    get_option_from_env(rel, false);

    process_sample_files(sample_files,
            [](const char *rule, char *err_msg, size_t sz_err_msg) -> bool {
        return parse(rule, err_msg, sz_err_msg);
    });

    bool ok = purc_cleanup ();

    std::cerr << std::endl;
    get_option_from_env(rel, true); // print
    print_statics();
    std::cerr << std::endl;

    ASSERT_TRUE(ok);
}

