/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc.h"

#include "private/variant.h"
#include "private/ejson-parser.h"
#include "private/executor.h"
#include "private/utils.h"

#include <gtest/gtest.h>
#include <glob.h>
#include <limits.h>


#include "../helpers.h"


#define ENV(env)      #env

struct statistics
{
    size_t                 nr_success;
    size_t                 nr_failure;
};

struct config {
    bool debug_flex;                // trace flex
    bool debug_bison;               // trace bison
    bool verbose_neg;               // if print err_msg when neg 'succeeds'
    char sample_files[PATH_MAX+1];  // sample files searching pattern

    struct statistics stat;
};

static inline void
config_from_env(struct config *cfg, const char *rel)
{
    const char *env;

    env = ENV(SAMPLE_FILES);
    test_getpath_from_env_or_rel(cfg->sample_files, sizeof(cfg->sample_files),
            env, rel);

    env = ENV(DEBUG_FLEX);
    cfg->debug_flex = test_getbool_from_env_or_default(env, 0);

    env = ENV(DEBUG_BISON);
    cfg->debug_bison = test_getbool_from_env_or_default(env, 0);

    env = ENV(VERBOSE_NEG);
    cfg->verbose_neg = test_getbool_from_env_or_default(env, 0);
}

static inline void
config_print(struct config *cfg)
{
    const char *env;

    env = ENV(SAMPLE_FILES);
    std::cerr << "env: export " << env << "=" << cfg->sample_files;
    std::cerr << std::endl;

    env = ENV(DEBUG_FLEX);
    std::cerr << "env: export " << env << "=" << cfg->debug_flex;
    std::cerr << std::endl;

    env = ENV(DEBUG_BISON);
    std::cerr << "env: export " << env << "=" << cfg->debug_bison;
    std::cerr << std::endl;

    env = ENV(VERBOSE_NEG);
    std::cerr << "env: export " << env << "=" << cfg->verbose_neg;
    std::cerr << std::endl;

    std::cerr << "test result(total/success/failure):"
        << cfg->stat.nr_success + cfg->stat.nr_failure << "/"
        << cfg->stat.nr_success << "/"
        << cfg->stat.nr_failure << std::endl;
}

enum parser_state {
    IN_BEGIN,
    IN_INPUT,
    IN_RULE,
    IN_OUTPUT,
};

struct string
{
    char                 *str;
    size_t                len;
    size_t                sz;
};

static inline int
string_append(struct string *str, const char *s, size_t len)
{
    const size_t n = str->len + len;
    if (n >= str->sz) {
        char *p = (char*)realloc(str->str, (n+15)/8 * 8);
        if (!p)
            return -1;
        str->str = p;
        str->sz  = n;
    }
    memcpy(str->str + str->len, s, len);
    str->len = n;
    str->str[str->len] = '\0';
    return 0;
}

static inline void
string_clear(struct string *str)
{
    str->len = 0;
    if (str->str)
        str->str[0] = '\0';
}

static inline void
string_reset(struct string *str)
{
    if (str->str) {
        free(str->str);
        str->str = NULL;
    }
    str->len = str->sz = 0;
}

struct parser_ctx
{
    enum parser_state      state;

    struct string          input;
    struct string          rule;
    struct string          output;

    purc_variant_t         v_input;
    purc_variant_t         v_output;

    unsigned int           has_input:1;
    unsigned int           has_rule:1;
    unsigned int           has_output:1;

    unsigned int           result:1;
};

static inline void
parser_ctx_clear_v_input(struct parser_ctx *ctx)
{
    if (ctx->v_input) {
        purc_variant_unref(ctx->v_input);
        ctx->v_input = PURC_VARIANT_INVALID;
    }
}

static inline void
parser_ctx_clear_v_output(struct parser_ctx *ctx)
{
    if (ctx->v_output) {
        purc_variant_unref(ctx->v_output);
        ctx->v_output = PURC_VARIANT_INVALID;
    }
}

static inline void
parser_ctx_reset(struct parser_ctx *ctx)
{
    ctx->state          = IN_BEGIN;
    string_reset(&ctx->input);
    string_reset(&ctx->rule);
    string_reset(&ctx->output);

    parser_ctx_clear_v_input(ctx);
    parser_ctx_clear_v_output(ctx);
}

static inline int
parser_ctx_append_input(struct parser_ctx *ctx, const char *s, size_t len)
{
    if (string_append(&ctx->input, s, len)==0) {
        ctx->has_input = 1;
        return 0;
    }
    return -1;
}

static inline int
parser_ctx_append_rule(struct parser_ctx *ctx, const char *s, size_t len)
{
    if (string_append(&ctx->rule, s, len)==0) {
        ctx->has_rule = 1;
        return 0;
    }
    return -1;
}

static inline int
parser_ctx_append_output(struct parser_ctx *ctx, const char *s, size_t len)
{
    if (string_append(&ctx->output, s, len)==0) {
        ctx->has_output = 1;
        return 0;
    }
    return -1;
}

static inline void
parser_ctx_clear_input(struct parser_ctx *ctx)
{
    string_clear(&ctx->input);
    ctx->has_input = 0;
}

static inline void
parser_ctx_clear_rule(struct parser_ctx *ctx)
{
    string_clear(&ctx->rule);
    ctx->has_rule = 0;
}

static inline void
parser_ctx_clear_output(struct parser_ctx *ctx)
{
    string_clear(&ctx->output);
    ctx->has_output = 0;
}

static inline bool
is_blank_line(const char *line)
{
    for (; *line; ++line) {
        if (isspace(*line))
            continue;
        return false;
    }
    return true;
}

static inline void
make_variant_from_json(purc_variant_t *v, const char *s, size_t len,
        struct config *cfg)
{
    if (s[len] != '\0')
        abort();

    if (strcmp(s, "undefined")==0) {
        *v = purc_variant_make_undefined();
    }
    else if (strcmp(s, "null")==0) {
        *v = purc_variant_make_null();
    }
    else {
        // *v = purc_variant_make_from_json_string(s, len);
        *v = pcejson_parser_parse_string(s,
                cfg->debug_flex, cfg->debug_bison);
    }
}

static void
process_input_output(struct config *cfg, const char *fn, struct parser_ctx *ctx)
{
    (void)cfg;
    (void)fn;

    int r = 0;
    r = purc_variant_compare_ex(ctx->v_input, ctx->v_output,
            PCVARIANT_COMPARE_OPT_AUTO);
    if (r) {
        const char *src_file = pcutils_basename((char*)__FILE__);
        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "Failed to compare output/input:" << std::endl
            << "[" << ctx->output.str << "]" << std::endl
            << "[" << ctx->input.str << "]" << std::endl;
    }
    ctx->result = (r == 0) ? 1 : 0;
}

static void
process_rule_output_do_choose(struct config *cfg, const char *fn,
    struct parser_ctx *ctx, purc_exec_ops_t ops, purc_exec_inst_t inst)
{
    (void)cfg;
    (void)fn;

    const char *src_file = pcutils_basename((char*)__FILE__);

    purc_variant_t v = ops->choose(inst, ctx->rule.str);
    if (v == PURC_VARIANT_INVALID) {
        const char *err = purc_get_error_message(purc_get_last_error());
        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "Failed to do choose operation for input/rule/err:" << std::endl
            << "[" << ctx->input.str << "]" << std::endl
            << "[" << ctx->rule.str << "]" << std::endl
            << "[" << err << "]" << std::endl;
        ctx->result = 0;
        return;
    }

    int r = 0;
    r = purc_variant_compare_ex(v, ctx->v_output, PCVARIANT_COMPARE_OPT_AUTO);
    if (r) {
        purc_rwstream_t rws = purc_rwstream_new_buffer(1024, -1);
        purc_variant_serialize(v, rws, 0, 0, NULL);
        const char *p = (const char*)purc_rwstream_get_mem_buffer(rws, NULL);

        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "Failed to compare input/rule/output/actual:" << std::endl
            << "input:   [" << ctx->input.str << "]" << std::endl
            << "rule:    [" << ctx->rule.str << "]" << std::endl
            << "output:  [" << ctx->output.str << "]" << std::endl
            << "expected:[" << (const char*)p << "]" << std::endl;
        purc_rwstream_destroy(rws);

        ctx->result = 0;
    }

    purc_variant_unref(v);
}

static void
process_rule_output_with_choose(struct config *cfg, const char *fn,
    struct parser_ctx *ctx, const char *name)
{
    const char *src_file = pcutils_basename((char*)__FILE__);

    purc_exec_ops_t ops;
    bool ok;
    ok = purc_get_executor(name, &ops);

    if (!ok) {
        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "No executor is found for name/rule:"
            << "[" << name << "]" << std::endl
            << "[" << ctx->rule.str << "]" << std::endl;
        ctx->result = 0;
        return;
    }

    purc_exec_inst_t inst;
    inst = ops->create(PURC_EXEC_TYPE_CHOOSE, ctx->v_input, true);
    if (!inst) {
        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "Failed to create choose instance for:"
            << "[" << ctx->rule.str << "]" << std::endl;
        ctx->result = 0;
        return;
    }

    process_rule_output_do_choose(cfg, fn, ctx, ops, inst);

    ops->destroy(inst);
}

static void
process_rule_output(struct config *cfg, const char *fn, struct parser_ctx *ctx)
{
    const char *src_file = pcutils_basename((char*)__FILE__);

    const char *end = ctx->rule.str + ctx->rule.len;
    const char *head = NULL;
    const char *tail = NULL;
    const char *p = ctx->rule.str;
    for (; p < end; ++p) {
        if (isspace(*p))
            continue;
        head = p;
        ++p;
        break;
    }

    if (*head == ':') {
        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "Bad rule:"
            << "[" << ctx->rule.str << "]" << std::endl;
        ctx->result = 0;
        return;
    }

    for (; p < end; ++p) {
        if (isspace(*p) || *p == ':') {
            tail = p;
            break;
        }
    }

    if (!tail) {
        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "Bad rule:"
            << "[" << ctx->rule.str << "]" << std::endl;
        ctx->result = 0;
        return;
    }

    char *name = strndup(head, tail - head);
    if (!name) {
        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "No executor is found for "
            << "[" << head << "]" << std::endl;
        ctx->result = 0;
        return;
    }

    process_rule_output_with_choose(cfg, fn, ctx, name);

    free(name);
}

static void
process_output(struct config *cfg, const char *fn, struct parser_ctx *ctx)
{
    if (ctx->has_rule) {
        process_rule_output(cfg, fn, ctx);
    } else {
        process_input_output(cfg, fn, ctx);
    }
}

static void
process_sample_file(struct config *cfg, FILE *file, const char *fn)
{
    struct parser_ctx ctx;
    bool    ok = true;
    char    *linebuf = NULL;
    size_t   len  = 0;

    const char *src_file = pcutils_basename((char*)__FILE__);

    (void)cfg;

    memset(&ctx, 0, sizeof(ctx));
    ctx.state = IN_BEGIN;

    int lineno = 0;
    while (!feof(file) && ok) {
        if (linebuf)
            linebuf[0] = '0';
        ssize_t n = getline(&linebuf, &len, file);
        if (n<0) {
            int err = errno;
            if (feof(file))
                break;

            std::cerr << src_file << "[" << __LINE__ << "]:"
                << fn << "[" << lineno + 1 << "]:"
                << "Failed reading file: [" << err << "]"
                << strerror(err) << std::endl;
            break;
        }
        if (n<=0)
            continue;

        ++lineno;

        char *line = linebuf;

        switch (ctx.state) {
            case IN_BEGIN:
                {
                    // comment
                    if (line[0] == '#')
                        break;
                    if (is_blank_line(line))
                        break;
                    if (strstr(line, "I:") == line) {
                        parser_ctx_clear_input(&ctx);
                        parser_ctx_clear_v_input(&ctx);
                        ctx.state = IN_INPUT;
                    }
                    else if (strstr(line, "R:") == line) {
                        if (!ctx.has_input) {
                            std::cerr << src_file << "[" << __LINE__ << "]:"
                                << fn << "[" << lineno << "]:"
                                << "No input value has been specified yet:"
                                << "[" << line << "]"
                                << std::endl;
                            ok = false;
                            break;
                        }
                        if (ctx.has_rule) {
                            std::cerr << src_file << "[" << __LINE__ << "]:"
                                << fn << "[" << lineno << "]:"
                                << "No consecutive rules are allowed:"
                                << "[" << line << "]"
                                << std::endl;
                            ok = false;
                            break;
                        }
                        ctx.state = IN_RULE;
                    }
                    else if (strstr(line, "O:") == line) {
                        if (!ctx.has_input) {
                            std::cerr << src_file << "[" << __LINE__ << "]:"
                                << fn << "[" << lineno << "]:"
                                << "No input value has been specified yet:"
                                << "[" << line << "]"
                                << std::endl;
                            ok = false;
                            break;
                        }
                        ctx.state = IN_OUTPUT;
                    }
                    else {
                        std::cerr << src_file << "[" << __LINE__ << "]:"
                            << fn << "[" << lineno << "]:"
                            << "Unrecognized:"
                            << "[" << line << "]"
                            << std::endl;
                    }
                } break;
            case IN_INPUT:
                {
                    size_t len = strlen(line);
                    char *p = strstr(line, ";\n"); // FIXME: ';\r\n'
                    if (p) {
                        // remove ';\n';
                        *p = '\0';
                        len -= 2;
                    }
                    if (parser_ctx_append_input(&ctx, line, len)) {
                        std::cerr << src_file << "[" << __LINE__ << "]:"
                            << "OOM: @"
                            << lineno
                            << "[" << line << "]"
                            << std::endl;
                        ok = false;
                        break;
                    }
                    if (!p)
                        break;

                    purc_variant_t v;
                    make_variant_from_json(&v, ctx.input.str, ctx.input.len,
                        cfg);
                    if (v == PURC_VARIANT_INVALID) {
                        std::cerr << src_file << "[" << __LINE__ << "]:"
                            << "Failed to parse input: @"
                            << lineno
                            << "[" << ctx.input.str << "]"
                            << std::endl;
                        ok = false;
                        break;
                    }
                    parser_ctx_clear_v_input(&ctx);
                    ctx.v_input = v;
                    ctx.state = IN_BEGIN;
                } break;
            case IN_RULE:
                {
                    size_t len = strlen(line);
                    char *p = strstr(line, ";\n"); // FIXME: ';\r\n'
                    if (p) {
                        // remove ';\n';
                        *p = '\0';
                        len -= 2;
                    }
                    if (parser_ctx_append_rule(&ctx, line, len)) {
                        std::cerr << src_file << "[" << __LINE__ << "]:"
                            << "OOM: @"
                            << lineno
                            << "[" << line << "]"
                            << std::endl;
                        ok = false;
                        break;
                    }
                    if (!p)
                        break;

                    ctx.state = IN_BEGIN;
                } break;
            case IN_OUTPUT:
                {
                    size_t len = strlen(line);
                    char *p = strstr(line, ";\n"); // FIXME: ';\r\n'
                    if (p) {
                        // remove ';\n';
                        *p = '\0';
                        len -= 2;
                    }
                    if (parser_ctx_append_output(&ctx, line, len)) {
                        std::cerr << src_file << "[" << __LINE__ << "]:"
                            << "OOM: @"
                            << lineno
                            << "[" << line << "]"
                            << std::endl;
                        ok = false;
                        break;
                    }
                    if (!p)
                        break;

                    purc_variant_t v;
                    make_variant_from_json(&v, ctx.output.str, ctx.output.len,
                            cfg);
                    if (v == PURC_VARIANT_INVALID) {
                        std::cerr << src_file << "[" << __LINE__ << "]:"
                            << "Failed to parse output: @"
                            << lineno
                            << "[" << ctx.output.str << "]"
                            << std::endl;
                        ok = false;
                        break;
                    }

                    parser_ctx_clear_v_output(&ctx);
                    ctx.v_output = v;
                    ctx.result = 1;
                    process_output(cfg, fn, &ctx);
                    if (ctx.result) {
                        ++cfg->stat.nr_success;
                    } else {
                        ++cfg->stat.nr_failure;
                        std::cerr << "@" << fn << "[" << lineno << "]"
                                  << std::endl;
                    }

                    parser_ctx_clear_rule(&ctx);
                    parser_ctx_clear_output(&ctx);
                    parser_ctx_clear_v_output(&ctx);

                    ctx.state = IN_BEGIN;
                } break;
            default:
                FAIL() << src_file << "[" << __LINE__ << "]:"
                    << "Internal logic error";
        }
    }

    if (ok && ctx.has_rule) {
        std::cerr << src_file << "[" << __LINE__ << "]:"
            << "No output has been specified yet"
            << std::endl;
        ok = false;
    }

    if (linebuf)
        free(linebuf);

    parser_ctx_reset(&ctx);

    ASSERT_EQ(ok, true);
}

static void
process(struct config *cfg)
{
    const char *src_file = pcutils_basename((char*)__FILE__);

    const char *pattern = cfg->sample_files;
    glob_t gbuf;
    memset(&gbuf, 0, sizeof(gbuf));

    gbuf.gl_offs = 0;
    int r = glob(pattern, GLOB_DOOFFS | GLOB_APPEND, NULL, &gbuf);
    EXPECT_TRUE(r==0) << "failed to search files with pattern: ["
        << pattern << "]";

    if (r == 0) {
        for (size_t i=0; i<gbuf.gl_pathc; ++i) {
            const char *file = gbuf.gl_pathv[i];
            std::cerr << src_file << "[" << __LINE__ << "]:"
                << "file: [" << file << "]" << std::endl;

            FILE *f = fopen(file, "r");
            EXPECT_NE(f, nullptr)
                << src_file << "[" << __LINE__ << "]:"
                << "Failed to open file: ["
                << file << "]";
            if (f) {
                const char *fn = pcutils_basename((char*)file);
                process_sample_file(cfg, f, fn);
            }

            fclose(f);
        }
        globfree(&gbuf);
    }
}

TEST(executors, full)
{
    int r = 0;
    bool ok = false;
    const char *rel = "data/*.full";
    struct config cfg;
    memset(&cfg, 0, sizeof(cfg));

    purc_instance_extra_info info = {};
    r = purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hvml.test", "executors",
            &info);
    ASSERT_EQ(r, PURC_ERROR_OK);

    config_from_env(&cfg, rel);

    process(&cfg);

    ok = purc_cleanup ();

    std::cerr << std::endl;
    config_print(&cfg);
    std::cerr << std::endl;

    ASSERT_TRUE(ok);

    ASSERT_EQ(cfg.stat.nr_failure, 0);
}

struct ejson_parser_record
{
    bool                     positive;
    const char              *in;
    const char              *out;
};

static inline int
do_serialize(purc_variant_t value, char *buf, size_t sz)
{
    purc_rwstream_t out = purc_rwstream_new_from_mem(buf, sz);
    if (out == NULL)
        return -1;

    size_t len = 0;
    ssize_t r = purc_variant_serialize(value, out, 0, 0, &len);
    purc_rwstream_destroy(out);

    if (r>=0)
        buf[r] = '\0';

    return r<0 ? -1 : 0;
}

static inline void
do_ejson_parser_parse(struct ejson_parser_record *record, struct config *cfg)
{
    int debug_flex = cfg->debug_flex;
    int debug_bison = cfg->debug_bison;
    bool positive = record->positive;
    const char *in = record->in;
    const char *out = record->out;
    purc_variant_t v;
    v = pcejson_parser_parse_string(in, debug_flex, debug_bison);
    if (v == PURC_VARIANT_INVALID) {
        if (positive) {
            FAIL() << "failed to parse positive: [" << in << "]";
        }
        return;
    }

    if (!positive) {
        FAIL() << "unexpected success parse for negative: [" << in << "]";
        purc_variant_unref(v);
        return;
    }

    if (1) {
        purc_variant_t vo;
        vo = pcejson_parser_parse_string(out, debug_flex, debug_bison);
        if (vo == PURC_VARIANT_INVALID) {
            purc_variant_unref(v);
            FAIL() << "failed to parse positive: [" << out << "]";
            return;
        }

        int r = purc_variant_compare_ex(v, vo, PCVARIANT_COMPARE_OPT_AUTO);

        purc_variant_unref(v);
        purc_variant_unref(vo);

        if (r) {
            FAIL() << "compare failed: in/serialize/out" << std::endl
                << "[" << in << "]" << std::endl
                << "[" << out << "]";
        }
        return;
    }

    char buf[8192];
    int r = do_serialize(v, buf, sizeof(buf)-1);
    purc_variant_unref(v);

    if (r) {
        FAIL() << "serialize failed:" << std::endl
            << "[" << in << "]";
        return;
    }

    if (strcmp(buf, out)) {
        FAIL() << "compare failed: in/serialize/out" << std::endl
            << "[" << in << "]" << std::endl
            << "[" << buf << "]" << std::endl
            << "[" << out << "]";
        return;
    }
}

TEST(executors, ejson_parser)
{
    const char *rel = "dummy";
    struct config cfg;
    purc_instance_extra_info info = {};
    int r = purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hvml.test", "executors",
            &info);
    ASSERT_EQ(r, PURC_ERROR_OK);

    config_from_env(&cfg, rel);

    struct ejson_parser_record records[] = {
        { true, "undefined", "undefined" },
        { true, "null", "null" },
        { true, "true", "true" },
        { true, "false", "false" },
        { true, "''", "\"\"" },
        { true, "[]", "[]" },
        { true, "{}", "{}" },
        { true, "0", "0FL" },
        { true, "0.0", "0FL" },
        { true, "0FL", "0FL" },
        { true, "0.0FL", "0FL" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "undefined", "undefined" },
        { true, "[0]", "[0FL]" },
        { true, "['ab']", "[\"ab\"]" },
        { true, "{'hello':'world'}", "{\"hello\":\"world\"}" },
        { true, "'hello'", "'hello'"},
    };

    for (size_t i=0; i<PCA_TABLESIZE(records); ++i) {
        struct ejson_parser_record *record = records + i;
        do_ejson_parser_parse(record, &cfg);
    }

    bool ok = purc_cleanup ();

    ASSERT_TRUE(ok);
}

TEST(executors, utf8_wchar)
{
}

