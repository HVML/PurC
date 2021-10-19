#include "purc.h"

#include "purc-executor.h"

#include "private/utils.h"

#include <gtest/gtest.h>
#include <glob.h>
#include <libgen.h>
#include <limits.h>

#include "../helpers.h"

extern "C" {
#include "exe_sql.tab.h"
}

#include "utils.cpp.in"

TEST(exe_sql, basic)
{
    purc_instance_extra_info info = {0, 0};
    bool cleanup = false;

    // initial purc
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    bool ok;

    struct purc_exec_ops ops;
    memset(&ops, 0, sizeof(ops));
    ok = purc_register_executor("SQL", &ops);
    EXPECT_FALSE(ok);
    EXPECT_EQ(purc_get_last_error(), PCEXECUTOR_ERROR_ALREAD_EXISTS);

    cleanup = purc_cleanup();
    ASSERT_EQ(cleanup, true);
}

static inline bool
parse(const char *rule, char **err_msg)
{
    struct exe_sql_param param = {
        .err_msg            = nullptr,
        .debug_flex         = debug_flex,
        .debug_bison        = debug_bison,
    };
    bool r;
    r = exe_sql_parse(rule, strlen(rule), &param) == 0;
    *err_msg = param.err_msg;
    return r;
}

TEST(exe_sql, files)
{
    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    purc_instance_extra_info info = {0, 0};
    r = purc_init("cn.fmsoft.hybridos.test",
        "vdom_gen", &info);
    EXPECT_EQ(r, PURC_ERROR_OK);
    if (r)
        return;

    const char *rel = "data/sql.*.rule";
    get_option_from_env(rel, false);

    process_sample_files(sample_files,
            [](const char *rule, char **err_msg) -> bool {
        return parse(rule, err_msg);
    });

    bool ok = purc_cleanup ();

    std::cout << std::endl;
    get_option_from_env(rel, true); // print
    print_statics();
    std::cout << std::endl;

    ASSERT_TRUE(ok);
}

