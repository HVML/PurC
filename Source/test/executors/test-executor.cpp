#include "purc.h"

#include "purc-executor.h"

#include "private/utils.h"

#include "../helpers.h"

extern "C" {
#include "key.tab.h"
}

#include <gtest/gtest.h>
#include <glob.h>
#include <libgen.h>
#include <limits.h>

#define ENV(env)      #env
#define PRINTF(_ok, _fmt, ...)                                            \
    do {                                                                  \
        int _clr = 32;                                                    \
        fprintf(stderr, "\e[0;%dm%s \e[0m" _fmt,                          \
            _clr, _ok ? "[  SUCCESS ]" : "[     FAIL ]", __VA_ARGS__);    \
    } while(false)

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

static inline bool
parse_positive(const char *rule, char **err_msg)
{
    return key_parse(rule, err_msg, NULL) == 0;
}

static inline bool
parse_negative(const char *rule, char **err_msg)
{
    return !parse_positive(rule, err_msg);
}

static inline void
parse(const char *rule, bool neg)
{
    char *err_msg = NULL;
    if (neg) {
        EXPECT_PRED2(parse_negative, rule, &err_msg);
        if (err_msg) {
            std::cout << "As expected:[" << rule << "]:"
                << err_msg << std::endl;
        }
    } else {
        EXPECT_PRED2(parse_positive, rule, &err_msg)
            << err_msg;
    }
    if (err_msg) {
        free(err_msg);
    }
}

static inline void
process_file(FILE *f, const char *file)
{
    char    *line = NULL;
    size_t   len  = 0;

    const char *fn = basename((char*)file);
    bool neg = (strstr(fn, "N.")==fn) ? true : false;

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

        if (neg) {
            if (line[0] == '\0')
                continue;
        }
        parse(line, neg);
    }

    if (line)
        free(line);
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
    test_getpath_from_env_or_rel(files, sizeof(files),
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

            process_file(f, file);
            fclose(f);
        }
    }
    globfree(&globbuf);

    bool ok = purc_cleanup ();
    ASSERT_TRUE(ok);
}

TEST(executor, foo)
{
    SUCCEED();
    ADD_FAILURE();
    ADD_FAILURE() << "damn";
    ADD_FAILURE() << "xdamn";
}

