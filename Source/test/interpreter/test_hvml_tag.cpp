#include "purc.h"

#include "private/hvml.h"
#include "private/utils.h"
#include "purc-rwstream.h"
#include "purc-variant.h"
#include "hvml/hvml-token.h"
#include "private/dvobjs.h"

#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gtest/gtest.h>

using namespace std;

struct TestCase {
    char *name;
    char *hvml;
    char *comp;
};

static inline void
add_test_case(std::vector<TestCase> &test_cases,
        const char *name, const char *hvml, const char *comp)
{
    TestCase data;
    memset(&data, 0, sizeof(data));
    data.name = MemCollector::strdup(name);
    data.hvml = MemCollector::strdup(hvml);
    data.comp = MemCollector::strdup(comp);

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
    }
    void TearDown() {
        purc_cleanup ();
    }
};

TEST_P(TestHVMLTag, hvml_tags)
{
    TestCase test_case = GetParam();
    PRINTF("test case : %s\n", test_case.name);

    char *dump_buff = NULL;
    purc_vdom_t vdom = purc_load_hvml_from_string(test_case.hvml);
    ASSERT_NE(vdom, nullptr);

    pcvdom_document_set_dump_buff(vdom, &dump_buff);

    purc_run(PURC_VARIANT_INVALID, NULL);

    ASSERT_STREQ(trim(dump_buff), test_case.comp);
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
    strcat(file_path, "hvml_tags.cases");

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

            char *buf = read_file(file);
            if (!buf) {
                continue;
            }

            n = snprintf(file, sizeof(file), "%s/%s.html", data_path, name);
            if ( n>= 0 && (size_t)n >= sizeof(file)) {
                // to circumvent format-truncation warning
                ;
            }
            char *comp_buf = read_file(file);
            if (!comp_buf) {
                free (buf);
                continue;
            }

            add_test_case(test_cases, name, buf, trim(comp_buf));

            free (buf);
            free (comp_buf);
        }
    }
    free (line);
    fclose(fp);

end:
    if (test_cases.empty()) {
        add_test_case(test_cases, "base", "<hvml></hvml>", "<html>\n  <head>\n  </head>\n  <body>\n  </body>\n</html>\n");
    }
    return test_cases;
}

INSTANTIATE_TEST_SUITE_P(hvml_tags, TestHVMLTag,
        testing::ValuesIn(read_test_cases()));

