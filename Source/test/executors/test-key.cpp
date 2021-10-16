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

enum sample_file_parsing_state {
    SAMPLE_IN_BEGIN,
    SAMPLE_IN_RULE,
};

enum sample_type {
    UNRECOGNIZED_SAMPLE,
    POSITIVE_SAMPLE,
    NEGATIVE_SAMPLE,
};

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

static inline void
test_str_append(char **rule, const char *line)
{
    size_t len = 0;
    if (*rule)
        len = strlen(*rule);

    size_t n = len + strlen(line);

    char *p = (char*)realloc(*rule, n + 1);
    if (!p) {
        free(*rule);
        FAIL() << "Out of memory";
        return; // never reached here
    }
    if (!*rule)
        p[0] = '\0';

    strcat(p, line);
    *rule = p;
}

static inline bool
is_blank_line(const char *line)
{
    for (; *line; ++line) {
        if (isblank(*line))
            continue;
        if (*line == '#')
            continue;
        return false;
    }
    return true;
}

static inline void
process_file(FILE *f,
    std::function<void(const char *rule, enum sample_type st)> on_rule)
{
    char    *linebuf = NULL;
    size_t   len  = 0;

    enum sample_file_parsing_state state = SAMPLE_IN_BEGIN;
    enum sample_type st = UNRECOGNIZED_SAMPLE;

    char *rule = NULL;

    int lineno = 0;
    while (!feof(f)) {
        if (linebuf)
            linebuf[0] = '0';
        ssize_t n = getline(&linebuf, &len, f);
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

        ++lineno;

        char *line = linebuf;

again:
        switch (state) {
            case SAMPLE_IN_BEGIN:
                {
                    // comment
                    if (line[0] == '#')
                        break;
                    if (strstr(line, "P:") == line) {
                        st = POSITIVE_SAMPLE;
                        state = SAMPLE_IN_RULE;
                    }
                    else if (strstr(line, "N:") == line) {
                        st = NEGATIVE_SAMPLE;
                        state = SAMPLE_IN_RULE;
                    }
                    else {
                        if (st != UNRECOGNIZED_SAMPLE) {
                            state = SAMPLE_IN_RULE;
                            goto again;
                        }
                        else if (!is_blank_line(line)) {
                            std::cout << "Unrecognized: @" << lineno
                                << "[" << line << "]" << std::endl;
                        }
                    }
                } break;
            case SAMPLE_IN_RULE:
                {
                    char *p = strstr(line, ";\n"); // FIXME: ';\r\n'
                    if (p) {
                        // remove ';\n';
                        *p = '\0';

                        test_str_append(&rule, line);
                        on_rule(rule, st);
                        rule[0] = '\0';
                        state = SAMPLE_IN_BEGIN;
                    } else {
                        test_str_append(&rule, line);
                    }
                } break;
            default:
                FAIL() << "Internal logic error";
        }
    }

    if (rule) {
        if (state == SAMPLE_IN_RULE) {
            if (!is_blank_line(rule)) {
                on_rule(rule, st);
            }
        }
    }

    if (rule)
        free(rule);

    if (linebuf)
        free(linebuf);
}

static inline void
process_sample_files(const char *pattern,
        std::function<void(const char *rule, enum sample_type st)> on_rule)
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
                process_file(f, on_rule);
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
parse(const char *rule, enum sample_type st)
{
    if (st == POSITIVE_SAMPLE) {
        if (rule[0] == '\0')
            return;

        if (!parse_positive(rule))
            ADD_FAILURE();
    } else if (st == NEGATIVE_SAMPLE) {
        if (!parse_negative(rule))
            ADD_FAILURE();
    } else {
        FAIL() << "Unrecognized sample: [" << rule << "]" << std::endl;
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

    process_sample_files(sample_files, 
            [](const char *rule, enum sample_type st){
        parse(rule, st);
    });

    bool ok = purc_cleanup ();

    std::cout << std::endl;
    get_option_from_env(rel, true); // print
    print_statics();
    std::cout << std::endl;

    ASSERT_TRUE(ok);
}

