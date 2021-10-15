#include "purc.h"

#include "purc-executor.h"

#include "private/utils.h"

extern "C" {
#include "key.tab.h"
}

#include <gtest/gtest.h>
#include <glob.h>
#include <libgen.h>
#include <limits.h>

#include "../helpers.h"

#define ENV(env)      #env

static bool debug_flex  = false;       // trace flex
static bool debug_bison = false;       // trace bison
static bool verbose_neg = false;       // if print err_msg when neg 'succeeds'
static char sample_files[PATH_MAX+1];  // sample files searching pattern

struct statistics {
    int        positives;
    int        negatives;
    int        positives_fail;
    int        negatives_fail;
};
static struct statistics counter;

static inline void print_statics(void)
{
    std::cout << "positives/failes: ("
        << counter.positives << "/" << counter.positives_fail << ")"
        << std::endl;
    std::cout << "negatives/failes: ("
        << counter.negatives << "/" << counter.negatives_fail << ")"
        << std::endl;
}

static inline char*
test_str_append(char *rule, const char *line)
{
    size_t len = 0;
    if (rule)
        len = strlen(rule);

    size_t n = len + strlen(line);

    char *p = (char*)realloc(rule, n + 1);
    if (!p) {
        free(rule);
        return NULL;
    }
    if (!rule)
        p[0] = '\0';

    strcat(p, line);
    return p;
}

static inline void
process_file(FILE *f, const char *file,
    std::function<void(const char *rule, bool neg)> on_rule)
{
    char    *line = NULL;
    size_t   len  = 0;

    const char *fn = basename((char*)file);
    bool neg = (strstr(fn, "N.")==fn) ? true : false;

    char *rule = NULL;

    while (!feof(f)) {
        if (line)
            line[0] = '0';
        ssize_t n = getline(&line, &len, f);
        if (n<0) {
            int err = errno;
            if (feof(f))
                break;

            std::cout << "Failed reading file: [" << err << "]"
                << strerror(err) << std::endl;
            break;
        }
        if (n<=0)
            continue;

        // comment
        if (line[0] == '#')
            continue;

        char *p = strstr(line, ";\n"); // FIXME: ';\r\n'
        if (p) {
            // remove ';\n';
            *p = '\0';

            rule = test_str_append(rule, line);
            if (rule) {
                on_rule(rule, neg);
                rule[0] = '\0';
                continue;
            }
            // warning?
        } else {
            rule = test_str_append(rule, line);
            if (!rule) {
                // warning?
            }
        }
    }

    if (rule) {
        on_rule(rule, neg);
    }

    if (rule)
        free(rule);

    if (line)
        free(line);
}

static inline void
process_sample_files(const char *pattern,
        std::function<void(const char *rule, bool neg)> on_rule)
{
    glob_t gbuf;
    memset(&gbuf, 0, sizeof(gbuf));

    gbuf.gl_offs = 0;
    int r = glob(pattern, GLOB_DOOFFS | GLOB_APPEND, NULL, &gbuf);
    EXPECT_EQ(r, 0) << "Failed to globbing @["
            << pattern << "]: [" << errno << "]" << strerror(errno)
            << std::endl;

    if (r == 0) {
        for (size_t i=0; i<gbuf.gl_pathc; ++i) {
            const char *file = gbuf.gl_pathv[i];
            std::cout << "file: [" << file << "]" << std::endl;

            FILE *f = fopen(file, "r");
            EXPECT_NE(f, nullptr) << "Failed to open file: ["
                << file << "]" << std::endl;
            if (f) {
                process_file(f, file, on_rule);
            }

            fclose(f);
        }
    }
    globfree(&gbuf);
}

static inline void get_option_from_env(const char *rel, bool print)
{
    const char *env;

    env = ENV(SAMPLE_FILES);
    test_getpath_from_env_or_rel(sample_files, sizeof(sample_files),
            env, rel);
    if (print) {
        std::cout << "env: export " << env << "=" << sample_files << std::endl;
    }

    env = ENV(DEBUG_FLEX);
    debug_flex = test_getbool_from_env_or_default(env, debug_flex);
    if (print) {
        std::cout << "env: export " << env << "=" << debug_flex << std::endl;
    }

    env = ENV(DEBUG_BISON);
    debug_bison = test_getbool_from_env_or_default(env, debug_bison);
    if (print) {
        std::cout << "env: export " << env << "=" << debug_bison << std::endl;
    }

    env = ENV(VERBOSE_NEG);
    verbose_neg = test_getbool_from_env_or_default(env, verbose_neg);
    if (print) {
        std::cout << "env: export " << env << "=" << verbose_neg << std::endl;
    }
}

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
parse_positive(const char *rule)
{
    struct key_param param = {
        .err_msg            = nullptr,
        .debug_flex         = debug_flex,
        .debug_bison        = debug_bison,
    };
    bool r;
    r = key_parse(rule, strlen(rule), &param) == 0;
    const char *err_msg = param.err_msg ? param.err_msg : "";

    ++counter.positives;

    if (!r) {
        ++counter.positives_fail;
        std::cout << "Not postive:[" << rule << "]:"
            << err_msg << std::endl;
    }

    if (param.err_msg)
        free(param.err_msg);

    return r;
}

static inline bool
parse_negative(const char *rule)
{
    struct key_param param = {
        .err_msg            = nullptr,
        .debug_flex         = debug_flex,
        .debug_bison        = debug_bison,
    };
    bool r;
    r = key_parse(rule, strlen(rule), &param) != 0;
    const char *err_msg = param.err_msg ? param.err_msg : "";

    ++counter.negatives;


    if (r && verbose_neg) {
        std::cout << "As expected:[" << rule << "]:"
            << err_msg << std::endl;
    } else if (!r) {
        ++counter.negatives_fail;
        std::cout << "Not negative:[" << rule << "]" << std::endl;
    }

    if (param.err_msg)
        free(param.err_msg);

    return r;
}

static inline void
parse(const char *rule, bool neg)
{
    if (!neg) {
        if (rule[0] == '\0')
            return;

        if (!parse_positive(rule))
            ADD_FAILURE();
    } else {
        if (!parse_negative(rule))
            ADD_FAILURE();
    }
}

TEST(executor, files)
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

    const char *rel = "data/*.rule";
    get_option_from_env(rel, false);

    process_sample_files(sample_files, [](const char *rule, bool neg){
        parse(rule, neg);
    });

    bool ok = purc_cleanup ();

    std::cout << std::endl;
    get_option_from_env(rel, true); // print
    print_statics();
    std::cout << std::endl;

    ASSERT_TRUE(ok);
}

