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

#include "private/ejson.h"
#include "private/utils.h"
#include "purc/purc-rwstream.h"

#include "../helpers.h"

#include <stdio.h>
#include <gtest/gtest.h>

using namespace std;

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

struct ejson_test_data {
    char *name;
    char *json;
    char *comp;
    char *comp_path;
    int error;
};

static inline void
push_back(std::vector<ejson_test_data> &vec,
        const char *name, const char *json, const char *comp,
        const char *comp_path, int error)
{
    ejson_test_data data;
    memset(&data, 0, sizeof(data));
    data.name = MemCollector::strdup(name);
    data.json = MemCollector::strdup(json);
    if (comp) {
        data.comp = MemCollector::strdup(comp);
        data.comp_path = NULL;
    }
    else {
        data.comp = NULL;
        data.comp_path = MemCollector::strdup(comp_path);
    }
    data.error = error;

    vec.push_back(data);
}

class variant_load_from_json : public testing::TestWithParam<ejson_test_data>
{
protected:
    void SetUp() {
        purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test", "variant",
                NULL);
        name = GetParam().name;
        json = GetParam().json;
        error = GetParam().error;
    }
    void TearDown() {
        purc_cleanup ();
    }
    const char* get_name() {
        return name.c_str();
    }
    const char* get_json() {
        return json.c_str();
    }
    const char* get_comp() {
        return GetParam().comp;
    }
    const char* get_comp_path() {
        return GetParam().comp_path;
    }
    int get_error() {
        return error;
    }
private:
    string name;
    string json;
    int error;
};

#define TO_ERROR(err_name)                                 \
    if (strcmp (err, #err_name) == 0) {                  \
        return err_name;                                 \
    }

int to_error(const char* err)
{
    TO_ERROR(PCRWSTREAM_SUCCESS);
    TO_ERROR(PCRWSTREAM_ERROR_FAILED);
    TO_ERROR(PCRWSTREAM_ERROR_FILE_TOO_BIG);
    TO_ERROR(PCRWSTREAM_ERROR_IO);
    TO_ERROR(PCRWSTREAM_ERROR_IS_DIR);
    TO_ERROR(PCRWSTREAM_ERROR_NO_SPACE);
    TO_ERROR(PCRWSTREAM_ERROR_NO_DEVICE_OR_ADDRESS);
    TO_ERROR(PCRWSTREAM_ERROR_OVERFLOW);
    TO_ERROR(PCRWSTREAM_ERROR_PIPE);
    TO_ERROR(PURC_ERROR_BAD_ENCODING);
    TO_ERROR(PCEJSON_SUCCESS);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_CHARACTER);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_NULL_CHARACTER);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_JSON_NUMBER);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACE);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_RIGHT_BRACKET);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_JSON_KEY_NAME);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_COMMA);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_JSON_KEYWORD);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_BASE64);
    TO_ERROR(PCEJSON_ERROR_BAD_JSON_NUMBER);
    TO_ERROR(PCEJSON_ERROR_BAD_JSON);
    TO_ERROR(PCEJSON_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
    TO_ERROR(PCEJSON_ERROR_UNEXPECTED_EOF);
    TO_ERROR(PCEJSON_ERROR_MAX_DEPTH_EXCEEDED);
    return -1;
}

TEST_P(variant_load_from_json, load_and_serialize)
{
    const char* json = get_json();
    const char* comp = get_comp();
    int error_code = get_error();
    //fprintf(stderr, "json=%s|len=%ld\n", json, strlen(json));
    //fprintf(stderr, "comp=%s\n", comp);
    // read end of string as eof
    size_t sz = strlen (json) + 1;
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)json, sz);

    purc_variant_t vt = purc_variant_load_from_json_stream(rws);

    int error = purc_get_last_error();
    ASSERT_EQ (error, error_code) << "Test Case : "<< get_name();
    if (error_code != PCEJSON_SUCCESS)
    {
        purc_rwstream_destroy(rws);
        return;
    }

    ASSERT_NE(vt, PURC_VARIANT_INVALID) << "Test Case : "<< get_name();

    char buf[1024] = {0};
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr) << "Test Case : "<< get_name();


    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0) << "Test Case : "<< get_name();

    buf[n] = 0;
    if (comp) {
        fprintf(stderr, "com=%s\n", comp);
        ASSERT_STREQ(buf, comp) << "Test Case : "<< get_name();
    }
    else {
        const char* comp_path = get_comp_path();
        FILE* fp = fopen(comp_path, "w");
        fprintf(fp, "%s", buf);
        fclose(fp);
    }

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);
}

char* read_file (const char* file)
{
    FILE* fp = fopen (file, "r");
    if (fp == NULL) {
        return NULL;
    }
    fseek (fp, 0, SEEK_END);
    size_t sz = ftell (fp);
    char* buf = (char*) malloc(sz + 1);
    fseek (fp, 0, SEEK_SET);
    sz = fread (buf, 1, sz, fp);
    fclose (fp);
    buf[sz] = 0;
    return buf;
}

char* trim(char *str)
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

std::vector<ejson_test_data> read_ejson_test_data()
{
    std::vector<ejson_test_data> vec;

    const char* env = "LOAD_FROM_JSON_DATA_PATH";
    char data_path[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(data_path, sizeof(data_path), env, "data/ejson");

    if (strlen(data_path)) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_list");

        FILE* fp = fopen(file_path, "r");
        if (fp) {
            char file[1024] = {0};

            char* line = NULL;
            size_t sz = 0;
            ssize_t read = 0;
            while ((read = getline(&line, &sz, fp)) != -1) {
                if (line && line[0] != '#') {
                    char* name = strtok (trim(line), " ");
                    if (!name) {
                        continue;
                    }

                    char* err = strtok (NULL, " ");
                    int error = PCEJSON_SUCCESS;
                    if (err != NULL) {
                        error = to_error (err);
                    }

#if 0
                    sprintf(file, "%s/%s.json", data_path, name);
#else
                    strcpy(file, data_path);
                    strcat(file, "/");
                    strcat(file, name);
                    strcat(file, ".json");
#endif
                    char* json_buf = read_file (file);
                    if (!json_buf) {
                        continue;
                    }

#if 0
                    sprintf(file, "%s/%s.serial", data_path, name);
#else
                    strcpy(file, data_path);
                    strcat(file, "/");
                    strcat(file, name);
                    strcat(file, ".serial");
#endif
                    char* comp_buf = read_file (file);
                    if (comp_buf) {
                        push_back(vec, name, json_buf, trim(comp_buf), file, error);
                    }
                    else {
                        push_back(vec, name, json_buf, NULL, file, error);
                    }


                    free (json_buf);
                    if (comp_buf) {
                        free (comp_buf);
                    }
                }
            }
            free (line);
            fclose(fp);
        }
    }

    if (vec.empty()) {
        push_back(vec, "array", "[123]", "[123]", NULL, 0);
        push_back(vec, "unquoted_key", "{key:1}", "{\"key\":1}", NULL, 0);
        push_back(vec,
                "single_quoted_key", "{'key':'2'}", "{\"key\":\"2\"}", NULL, 0);
    }
    return vec;
}

INSTANTIATE_TEST_SUITE_P(ejson, variant_load_from_json,
        testing::ValuesIn(read_ejson_test_data()));

