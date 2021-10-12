#include "purc.h"

#include "purc-executor.h"

#include "private/utils.h"

extern "C" {
#include "key.tab.h"
}

#include <gtest/gtest.h>
#include <glob.h>
#include <limits.h>

#define ENV(env)      #env

TEST(executor, basic)
{
    purc_instance_extra_info info = {0, 0};
    bool cleanup = false;

    // initial purc
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    bool ok;

    struct purc_exec_ops ops;
    memset(&ops, 0, sizeof(ops));
    ok = purc_register_executor("KEY", &ops);
    EXPECT_FALSE(ok);
    EXPECT_EQ(purc_get_last_error(), PCEXECUTOR_ERROR_ALREAD_EXISTS);

    cleanup = purc_cleanup();
    ASSERT_EQ(cleanup, true);
}

TEST(executor, positive)
{
    purc_instance_extra_info info = {0, 0};
    bool cleanup = false;

    const char *rules[] = {
        "KEY: ALL",
        "KEY: 'zh_CN', 'zh_HK'",
        "KEY: LIKE 'zh_*'",
        "KEY: LIKE /zh_[A-Z][A-Z]/i",
        "KEY: 'zh_CN', LIKE 'zh_*'",
        "KEY: ALL, FOR VALUE",
        "KEY: 'zh_CN', 'zh_HK', FOR VALUE",
        "KEY: LIKE 'zh_*', FOR VALUE",
        "KEY: LIKE /zh_[A-Z][A-Z]/i, FOR VALUE",
        "KEY: 'zh_CN', LIKE 'zh_*', FOR VALUE",
    };

    // initial purc
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    bool ok;

    struct purc_exec_ops ops;
    memset(&ops, 0, sizeof(ops));
    ok = purc_register_executor("KEY", &ops);
    EXPECT_FALSE(ok);
    EXPECT_EQ(purc_get_last_error(), PCEXECUTOR_ERROR_ALREAD_EXISTS);

    for (size_t i=0; i<PCA_TABLESIZE(rules); ++i) {
        const char *rule = rules[i];
        int r = key_parse(rule, NULL);
        if (r==0) {
            continue;
        } else {
            EXPECT_EQ(r, 0) << "Failed to parse: ["
                << rule << "]" << std::endl;
        }
    }

    cleanup = purc_cleanup();
    ASSERT_EQ(cleanup, true);
}

static inline void
parse(const char *rule)
{
    int r = key_parse(rule, NULL);
    EXPECT_EQ(r, 0) << "Failed to parse: ["
        << rule << "]" << std::endl;
}

static inline void
process_file(FILE *f)
{
    char    *line = NULL;
    size_t   len  = 0;

    while (!feof(f)) {
        ssize_t n = getline(&line, &len, f);
        if (n<0) {
            int err = errno;
            if (feof(f))
                break;

            std::cout << "Failed reading file: [" << err << "]"
                << strerror(err) << std::endl;
            break;
        }
        if (n==0)
            continue;
        if (line[0] == '#')
            continue;

        line[n-1] = '\0';

        parse(line);
    }
}

TEST(executor, glob)
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

    char files[PATH_MAX+1];
    const char *env = ENV(EXECUTOR_FILES);
    pcutils_getpath_from_env_or_rel(files, sizeof(files),
        env, "data/*.rule");
    std::cout << "env: export " << env << "=" << files << std::endl;

    globbuf.gl_offs = 0;
    r = glob(files, GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
    EXPECT_EQ(r, 0) << "Failed to globbing @["
            << files << "]: [" << errno << "]" << strerror(errno)
            << std::endl;

    if (r == 0) {
        for (size_t i=0; i<globbuf.gl_pathc; ++i) {
            const char *file = globbuf.gl_pathv[i];
            std::cout << "file: [" << file << "]" << std::endl;

            FILE *f = fopen(file, "r");
            EXPECT_NE(f, nullptr) << "Failed to open file: ["
                << file << "]" << std::endl;
            if (!f)
                continue;

            process_file(f);
            fclose(f);
        }
    }
    globfree(&globbuf);

    bool ok = purc_cleanup ();
    ASSERT_TRUE(ok);
}

