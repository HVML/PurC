#include "purc.h"

#include "private/hvml.h"
#include "private/utils.h"
#include "purc-rwstream.h"
#include "purc-variant.h"
#include "hvml/hvml-token.h"
#include "private/dvobjs.h"
#include "private/interpreter.h"

#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gtest/gtest.h>

using namespace std;

struct TestCase {
    char *name;
    char *hvml;
    char *html;
    char *html_path;
};

static inline void
add_test_case(std::vector<TestCase> &test_cases,
        const char *name, const char *hvml,
        const char *html, const char *html_path)
{
    TestCase data;
    memset(&data, 0, sizeof(data));
    data.name = MemCollector::strdup(name);
    data.hvml = MemCollector::strdup(hvml);

    if (html) {
        data.html = MemCollector::strdup(html);
        data.html_path = NULL;
    }
    else {
        data.html = NULL;
        data.html_path = MemCollector::strdup(html_path);
    }

    test_cases.push_back(data);
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

class TestHVMLTag : public testing::TestWithParam<TestCase>
{
protected:
    void SetUp() {
        purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
                "test_hvml_tag", NULL);
        purc_bind_session_variables();
    }
    void TearDown() {
        purc_cleanup ();
    }
};

struct buffer {
    char                   *dump_buff;

    ~buffer() {
        if (dump_buff) {
            free(dump_buff);
            dump_buff = nullptr;
        }
    }
};

TEST_P(TestHVMLTag, hvml_tags)
{
    TestCase test_case = GetParam();
    PRINTF("test case : %s\n", test_case.name);

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    setenv(PURC_ENVV_EXECUTOR_PATH, SOPATH, 1);

    struct buffer buf;
    buf.dump_buff = nullptr;

    purc_enable_log(true, false);

    purc_vdom_t vdom = purc_load_hvml_from_string(test_case.hvml);
    ASSERT_NE(vdom, nullptr);

    purc_coroutine_t co = purc_schedule_vdom_null(vdom);
    ASSERT_NE(co, nullptr);

    pcintr_coroutine_set_dump_buff(co, &buf.dump_buff);

    purc_run(NULL);

    ASSERT_NE(buf.dump_buff, nullptr);

    if (test_case.html) {
        if (strcmp(trim(buf.dump_buff), trim(test_case.html))) {
            fprintf(stderr, "============================\n");
            fprintf(stderr, "dump:\n%s\n", trim(buf.dump_buff));
            fprintf(stderr, "html:\n%s\n", trim(test_case.html));
            fprintf(stderr, "============================\n");
        }
        ASSERT_STREQ(trim(buf.dump_buff), trim(test_case.html));
    }
    else {
        FILE* fp = fopen(test_case.html_path, "w");
        fprintf(fp, "%s", buf.dump_buff);
        fclose(fp);
        fprintf(stderr, "html written to `%s`\n", test_case.html_path);
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

std::vector<TestCase> read_test_cases()
{
    std::vector<TestCase> test_cases;

    const char *env = "HVML_TAG_TEST_PATH";
    char data_path[PATH_MAX + 1] =  {0};
    char file_path[PATH_MAX + 1] = {0};
    char file[PATH_MAX+16] = {0};
    char *line = NULL;
    size_t sz = 0;
    ssize_t read = 0;

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

            n = snprintf(file, sizeof(file), "%s/%s.html", data_path, name);
            if ( n>= 0 && (size_t)n >= sizeof(file)) {
                // to circumvent format-truncation warning
                ;
            }
            char *html = read_file(file);
            add_test_case(test_cases, name, hvml, html, file);

            free (hvml);
            free (html);
        }
    }
    free (line);
    fclose(fp);

end:
    if (test_cases.empty()) {
        add_test_case(test_cases, "base",
                "<hvml></hvml>",
                "<html>\n  <head>\n  </head>\n  <body>\n  </body>\n</html>",
                NULL
                );
    }
    return test_cases;
}

INSTANTIATE_TEST_SUITE_P(hvml_tags, TestHVMLTag,
        testing::ValuesIn(read_test_cases()));

