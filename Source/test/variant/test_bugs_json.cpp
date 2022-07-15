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

#include "private/hvml.h"
#include "private/utils.h"
#include "purc-rwstream.h"
#include "hvml/hvml-token.h"
#include "private/ejson-parser.h"
#include "private/debug.h"

#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gtest/gtest.h>

#include <dirent.h>
#include <glob.h>

using namespace std;

#define PRINTF(...)                                                       \
    do {                                                                  \
        fprintf(stdout, "\e[0;32m[          ] \e[0m");                    \
        fprintf(stdout, __VA_ARGS__);                                     \
    } while(false)

#if OS(LINUX) || OS(UNIX)
// get path from env or __FILE__/../<rel> otherwise
#define getpath_from_env_or_rel(_path, _len, _env, _rel) do {  \
    const char *p = getenv(_env);                                      \
    if (p) {                                                           \
        snprintf(_path, _len, "%s", p);                                \
    } else {                                                           \
        char tmp[PATH_MAX+1];                                          \
        snprintf(tmp, sizeof(tmp), __FILE__);                          \
        const char *folder = dirname(tmp);                             \
        snprintf(_path, _len, "%s/%s", folder, _rel);                  \
    }                                                                  \
} while (0)

#endif // OS(LINUX) || OS(UNIX)

#define MIN_BUFFER     512
#define MAX_BUFFER     1024  *1024  *1024

char *variant_to_string(purc_variant_t v)
{
    purc_rwstream_t my_rws = purc_rwstream_new_buffer(MIN_BUFFER, MAX_BUFFER);
    size_t len_expected = 0;
    ssize_t ret = purc_variant_serialize(v, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    if (ret == -1) {
        return NULL;
    }
    char *buf = (char*)purc_rwstream_get_mem_buffer_ex(my_rws, NULL, NULL, true);
    purc_rwstream_destroy(my_rws);
    return buf;
}


enum container_ops_type {
    CONTAINER_OPS_TYPE_DISPLACE,
    CONTAINER_OPS_TYPE_APPEND,
    CONTAINER_OPS_TYPE_PREPEND,
    CONTAINER_OPS_TYPE_MERGE,
    CONTAINER_OPS_TYPE_REMOVE,
    CONTAINER_OPS_TYPE_INSERT_BEFORE,
    CONTAINER_OPS_TYPE_INSERT_AFTER,
    CONTAINER_OPS_TYPE_UNITE,
    CONTAINER_OPS_TYPE_INTERSECT,
    CONTAINER_OPS_TYPE_SUBTRACT,
    CONTAINER_OPS_TYPE_XOR,
    CONTAINER_OPS_TYPE_OVERWRITE
};

struct test_case {
    char *name;
    char *json;
    char *serial;
    char *serial_path;
};

static inline void
add_test_case(std::vector<test_case> &test_cases,
        const char *name, const char *json, const char *serial,
        const char *serial_path)
{
    struct test_case test;
    test.name = MemCollector::strdup(name);
    test.json = MemCollector::strdup(json);
    if (serial) {
        test.serial = MemCollector::strdup(serial);
        test.serial_path = NULL;
    }
    else {
        test.serial = NULL;
        test.serial_path = MemCollector::strdup(serial_path);
    }
    test_cases.push_back(test);
}

char *trim(char *str)
{
    if (!str)
    {
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

class TestCaseData : public testing::TestWithParam<test_case>
{
protected:
    void SetUp() {
        purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
                "purc_variant", NULL);
    }
    void TearDown() {
        purc_cleanup ();
    }
};


TEST_P(TestCaseData, bugs_json)
{
    struct test_case data = GetParam();
    PRINTF("name=%s\n", data.name);

    purc_variant_t vt = purc_variant_make_from_json_string(
            data.json, strlen(data.json));
    ASSERT_NE(vt, PURC_VARIANT_INVALID);

    char *result = variant_to_string(vt);
    ASSERT_NE(result, nullptr);

    if (data.serial) {
//        PRINTF("src=%s\n", result);
//        PRINTF("cmp=%s\n", data.serial);
        ASSERT_STREQ(trim(result), trim(data.serial));
    }
    else {
        FILE *fp = fopen(data.serial_path, "w");
        if (fp) {
            fwrite(result, 1, strlen(result), fp);
            fclose(fp);
        }
    }

    // clear
    free(result);
    purc_variant_unref(vt);
}

char *read_file (const char *file)
{
    FILE *fp = fopen (file, "r");
    if (fp == NULL) {
        return NULL;
    }
    fseek (fp, 0, SEEK_END);
    size_t sz = ftell (fp);
    char *buf = (char*) malloc(sz + 1);
    fseek (fp, 0, SEEK_SET);
    sz = fread (buf, 1, sz, fp);
    fclose (fp);
    buf[sz] = 0;
    return buf;
}

std::vector<test_case> load_test_case()
{
    int r = 0;
    std::vector<test_case> test_cases;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    char base_path[PATH_MAX + 1];
    char json_path[PATH_MAX + 1];
    char serial_path[PATH_MAX + 1];
    const char *env = "TEST_BUGS_JSON_PATH";
    getpath_from_env_or_rel(base_path, sizeof(base_path), env, "bugs");

    if (!base_path[0]) {
        goto end;
    }

    strcpy(json_path, base_path);
    strcat(json_path, "/*.json");

    globbuf.gl_offs = 0;
    r = glob(json_path, GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);

    if (r == 0) {
        for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
            char *name = basename((char *)globbuf.gl_pathv[i]);
            strcpy(serial_path, base_path);
            strcat(serial_path, "/");

            char *suffix = strrchr(name, '.');
            if (suffix == NULL)
                continue;

            strncat(serial_path, name, suffix - name);
            strcat(serial_path, ".serial");

            char *json  = read_file(globbuf.gl_pathv[i]);
            if (json == NULL) {
                continue;
            }

            char *serial  = read_file(serial_path);
            add_test_case(test_cases, name, json, serial, serial_path);
            free(json);

            if (serial) {
                free(serial);
            }
        }
    }
    globfree(&globbuf);

    if (test_cases.empty()) {
        add_test_case(test_cases, "inner_test", "[123]", "[123]", NULL);
    }

end:
    return test_cases;
}

INSTANTIATE_TEST_SUITE_P(test_bugs_json, TestCaseData,
        testing::ValuesIn(load_test_case()));


