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


#include "purc/purc.h"

#include "private/hvml.h"
#include "private/utils.h"
#include "purc/purc-rwstream.h"
#include "purc/purc-variant.h"
#include "hvml/hvml-token.h"
#include "private/dvobjs.h"
#include "private/interpreter.h"

#include "../helpers.h"
#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gtest/gtest.h>

#include <vector>

using namespace std;

struct TestCase {
    char *name;
    char *hvml;
    char *html;
    char *html_path;
};

static const char *request_json = "{ names: 'PurC', OS: ['Linux', 'macOS', 'HybridOS', 'Windows'] }";

const char *dest_tag = NULL;

std::vector<TestCase*> g_test_cases;
std::vector<char*> g_test_cases_name;

void destroy_test_case(struct TestCase *tc)
{
    if (tc->name) {
        free(tc->name);
    }
    if (tc->hvml) {
        free(tc->hvml);
    }
    if (tc->html) {
        free(tc->html);
    }
    if (tc->html_path) {
        free(tc->html_path);
    }
    free(tc);
}

std::vector<TestCase*>& read_test_cases();

class TestCaseEnv : public ::testing::Environment {
    public:
        ~TestCaseEnv() override {}

        void SetUp() override {
        }

        void TearDown() override {
            size_t nr = g_test_cases.size();
            for (size_t i = 0; i < nr; i++) {
                TestCase *tc = g_test_cases[i];
                destroy_test_case(tc);
            }
        }
};

testing::Environment* const _env = testing::AddGlobalTestEnvironment(new TestCaseEnv);

static purc_variant_t
eval_expected_result(const char *code)
{
    char *ejson = NULL;
    size_t ejson_len = 0;
    const char *line = code;

    while (*line == '#') {

        line++;

        // skip blank character: space or tab
        while (isblank(*line)) {
            line++;
        }

        if (strncmp(line, "RESULT:", strlen("RESULT:")) == 0) {
            line += strlen("RESULT:");

            const char *eol = line;
            while (*eol != '\n') {
                eol++;
            }

            ejson_len = eol - line;
            if (ejson_len > 0) {
                ejson = strndup(line, ejson_len);
            }

            break;
        }
        else {
            // skip left characters in the line
            while (*line != '\n') {
                line++;
            }
            line++;

            // skip blank character: space or tab
            while (isblank(*line) || *line == '\n') {
                line++;
            }
        }
    }

    purc_variant_t result = PURC_VARIANT_INVALID;
    if (ejson) {
        result = purc_variant_make_from_json_string(ejson, ejson_len);
    }
#if 0
    else {
        result = purc_variant_make_undefined();
    }
#endif

    if (ejson)
        free(ejson);

    /* purc_log_debug("result type: %s\n",
            purc_variant_typename(purc_variant_get_type(result))); */

    return result;
}


static inline void
add_test_case(std::vector<struct TestCase*> &test_cases,
        std::vector<char*> &test_cases_name,
        const char *name, const char *hvml,
        const char *html, const char *html_path)
{
    struct TestCase *data = (struct TestCase*) calloc(sizeof(struct TestCase), 1);
    data->name = strdup(name);
    data->hvml = strdup(hvml);

    if (html) {
        data->html = strdup(html);
        data->html_path = NULL;
    }
    else {
        data->html = NULL;
        data->html_path = strdup(html_path);
    }

    test_cases.push_back(data);
    test_cases_name.push_back(data->name);
}

char *trim(char *str)
{
    if (!str) {
        return NULL;
    }
    char *end;

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if(*str == 0) {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';
    return str;
}

class TestHVMLTag : public testing::TestWithParam<struct TestCase*>
{
protected:
    void SetUp() {
        purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
                "test_hvml_tag", NULL);
    }
    void TearDown() {
        purc_cleanup ();
    }
};



class TestHVMLTagName : public testing::TestWithParam<struct TestCase*>
{
public:
    struct PrintToStringParamName
    {
        template <class ParamType>
        std::string operator()( const testing::TestParamInfo<ParamType>& info ) const
        {
            auto tc = static_cast<struct TestCase*>(info.param);
            return std::string(tc->name);
        }
    };
};



struct buffer {
    char                   *dump_buff;

    purc_variant_t          expected_result;
    ~buffer() {
        if (dump_buff) {
            free(dump_buff);
            dump_buff = nullptr;
        }
        if (expected_result) {
            purc_variant_unref(expected_result);
        }
    }
};

static int my_cond_handler(purc_cond_k event, purc_coroutine_t cor,
        void *data)
{
    if (event == PURC_COND_COR_ONE_RUN) {
        struct purc_cor_run_info *run_info = (struct purc_cor_run_info *)data;

        purc_document_t doc = run_info->doc;

        struct buffer *buf =
            (struct buffer *)purc_coroutine_get_user_data(cor);
        if (!buf) {
            goto out;
        }

        if (buf->dump_buff) {
            free(buf->dump_buff);
            buf->dump_buff = nullptr;
        }
        buf->dump_buff = intr_util_dump_doc(doc, NULL);
    }
    else if (event == PURC_COND_COR_TERMINATED) {
        purc_rwstream_t rws = purc_rwstream_new_buffer(1024, 0);
        purc_coroutine_dump_stack(cor, rws);
        size_t nr_buf = 0;
        void *buf = purc_rwstream_get_mem_buffer(rws, &nr_buf);
        fprintf(stderr, "%s\n", (char*)buf);
        purc_rwstream_destroy(rws);
        fprintf(stderr, "recv term co=%d\n", cor->cid);
    }
    else if (event == PURC_COND_COR_EXITED) {
        fprintf(stderr, "recv exited co=%d\n", cor->cid);
        struct purc_cor_exit_info *info = (struct purc_cor_exit_info *)data;
        struct buffer *buf =
            (struct buffer *)purc_coroutine_get_user_data(cor);
        if (!buf) {
            goto out;
        }
        if (buf->expected_result &&
                !purc_variant_is_equal_to(buf->expected_result, info->result)) {
            char exe_result[1024];
            char exp_result[1024];
            purc_rwstream_t my_rws;

            my_rws = purc_rwstream_new_from_mem(exp_result,
                    sizeof(exp_result) - 1);
            size_t len_expected = 0;
            ssize_t n = purc_variant_serialize(buf->expected_result, my_rws,
                    0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
            exp_result[n] = 0;
            purc_rwstream_destroy(my_rws);

            my_rws = purc_rwstream_new_from_mem(exe_result,
                    sizeof(exe_result) - 1);
            if (info->result) {
                size_t len_expected = 0;
                ssize_t n = purc_variant_serialize(info->result, my_rws,
                        0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
                exe_result[n] = 0;

            }
            else {
                strcpy(exe_result, "INVALID VALUE");
            }
            purc_rwstream_destroy(my_rws);

            ADD_FAILURE()
                << "The execute result does not match to the expected result: "
                << std::endl
                << TCS_YELLOW
                << exe_result
                << TCS_NONE
                << " vs. "
                << TCS_YELLOW
                << exp_result
                << TCS_NONE
                << std::endl;

        }
        else {
            std::cout
                << TCS_GREEN
                << "Passed"
                << TCS_NONE
                << std::endl;
        }
    }

out:
    return 0;
}

TEST_P(TestHVMLTag, tags)
{
    struct TestCase *test_case = GetParam();
    PRINTF("test case : %s\n", test_case->name);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    setenv(PURC_ENVV_EXECUTOR_PATH, SOPATH, 1);

    struct buffer buf;
    buf.dump_buff = nullptr;
    buf.expected_result = eval_expected_result(test_case->hvml);

//    purc_enable_log(true, false);

    purc_vdom_t vdom = purc_load_hvml_from_string(test_case->hvml);
    ASSERT_NE(vdom, nullptr);

    purc_variant_t request =
        purc_variant_make_from_json_string(request_json,
                strlen(request_json));
    ASSERT_NE(request, nullptr);

    purc_renderer_extra_info rdr_info = {};
    rdr_info.title = "def_page_title";
    purc_coroutine_t co = purc_schedule_vdom(vdom,
            0, request, PCRDR_PAGE_TYPE_NULL,
            "main",   /* target_workspace */
            NULL,     /* target_group */
            NULL,     /* page_name */
            &rdr_info, "test", NULL);
    ASSERT_NE(co, nullptr);
    purc_variant_unref(request);

    purc_coroutine_set_user_data(co, &buf);

    purc_run((purc_cond_handler)my_cond_handler);

    ASSERT_NE(buf.dump_buff, nullptr);

    if (test_case->html) {
        std::string left = buf.dump_buff;
        left.erase(remove(left.begin(), left.end(), ' '), left.end());
        left.erase(remove(left.begin(), left.end(), '\n'), left.end());

        std::string right = test_case->html;
        right.erase(remove(right.begin(), right.end(), ' '), right.end());
        right.erase(remove(right.begin(), right.end(), '\n'), right.end());
        ASSERT_EQ(left, right);
    }
    else {
        FILE* fp = fopen(test_case->html_path, "w");
        fprintf(fp, "%s", buf.dump_buff);
        fclose(fp);
        fprintf(stderr, "html written to `%s`\n", test_case->html_path);
        fprintf(stderr, "html:\n%s\n", buf.dump_buff);
    }
}

char *read_file(const char *file)
{
    FILE *fp = fopen (file, "r");
    if (fp == NULL) {
        return NULL;
    }

    fseek (fp, 0, SEEK_END);
    size_t sz = ftell (fp);

    char *buf = (char*) malloc(sz + 1);
    if (!buf) {
        return NULL;
    }

    fseek (fp, 0, SEEK_SET);
    sz = fread (buf, 1, sz, fp);
    fclose (fp);
    buf[sz] = 0;
    return buf;
}

#if OS(DARWIN)
#define OS_POSTFIX  "darwin"
#else
#define OS_POSTFIX  "unknown"
#endif

std::vector<TestCase*>& read_test_cases()
{
    const char *env = "HVML_TAG_TEST_PATH";
    char data_path[PATH_MAX + 1] =  {0};
    char file_path[PATH_MAX + 1] = {0};
    char file[PATH_MAX+16] = {0};
    char file_os[PATH_MAX+16] = {0};
    char *line = NULL;
    size_t sz = 0;
    ssize_t read = 0;
    size_t nr_dest_tag = dest_tag ? strlen(dest_tag) : 0;

    test_getpath_from_env_or_rel(data_path, sizeof(data_path), env,
            "test_tags");

    strcpy(file_path, data_path);
    strcat(file_path, "/");
    strcat(file_path, "tags.cases");

    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        goto end;
    }

    while ((read = getline(&line, &sz, fp)) != -1) {
        if (line && line[0] != '#') {
            char *name = strtok(trim(line), " ");
            if (!name) {
                continue;
            }

            if (nr_dest_tag) {
                if (strncmp(name, dest_tag, nr_dest_tag) != 0) {
                    continue;
                }
            }

            int n;
            n = snprintf(file, sizeof(file), "%s/%s.hvml", data_path, name);
            if ( n>= 0 && (size_t)n >= sizeof(file)) {
                // to circumvent format-truncation warning
                ;
            }

            char *hvml = read_file(file);
            if (!hvml) {
                continue;
            }

            n = snprintf(file_os, sizeof(file_os), "%s/%s-%s.html",
                    data_path, name, OS_POSTFIX);
            if (n >= 0 && (size_t)n >= sizeof(file_os)) {
                // to circumvent format-truncation warning
                ;
            }
            n = snprintf(file, sizeof(file), "%s/%s.html", data_path, name);
            if ( n>= 0 && (size_t)n >= sizeof(file)) {
                // to circumvent format-truncation warning
                ;
            }

            char *html;
            if ((html = read_file(file_os)) == NULL) {
                html = read_file(file);
            }

            add_test_case(g_test_cases, g_test_cases_name, name, hvml, html, file);

            free (hvml);
            free (html);
        }
    }
    free (line);
    fclose(fp);

end:
    if (g_test_cases.empty()) {
        add_test_case(g_test_cases, g_test_cases_name, "base",
                "<hvml></hvml>",
                "<!DOCTYPE html><html>\n  <head>\n  </head>\n  <body>\n  </body>\n</html>",
                NULL
                );
    }
    return g_test_cases;
}

INSTANTIATE_TEST_SUITE_P(tags, TestHVMLTag,
        testing::ValuesIn(read_test_cases()),
        TestHVMLTagName::PrintToStringParamName());


int main(int argc, char **argv) {
    if (argc > 1 && strncmp(argv[1], "--gtest", 7) != 0) {
        dest_tag = argv[1];
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

