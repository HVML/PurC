#include "purc.h"

#include "private/ejson.h"
#include "private/utils.h"
#include "purc-rwstream.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
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
    string name;
    string json;
    string comp;
    int error;
};

class ejson_parser_vcm_eval : public testing::TestWithParam<ejson_test_data>
{
protected:
    void SetUp() {
        purc_init ("cn.fmsoft.hybridos.test", "ejson", NULL);
        name = GetParam().name;
        json = GetParam().json;
        comp = GetParam().comp;
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
        return comp.c_str();
    }
    int get_error() {
        return error;
    }
private:
    string name;
    string json;
    string comp;
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

TEST_P(ejson_parser_vcm_eval, parse_and_serialize)
{
    const char* json = get_json();
    const char* comp = get_comp();
    int error_code = get_error();
    //fprintf(stderr, "json=%s|len=%ld\n", json, strlen(json));
    //fprintf(stderr, "comp=%s\n", comp);
    // read end of string as eof
    size_t sz = strlen (json) + 1;
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)json, sz);

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws, 32);
    int error = purc_get_last_error();
    ASSERT_EQ (error, error_code) << "Test Case : "<< get_name();

    if (error_code != PCEJSON_SUCCESS)
    {
        ASSERT_EQ (root, nullptr) << "Test Case : "<< get_name();
        purc_rwstream_destroy(rws);
        pcvcm_node_destroy (root);
        pcejson_destroy(parser);
        return;
    }
    else {
        ASSERT_NE (root, nullptr) << "Test Case : "<< get_name();
    }


    purc_variant_t vt = pcvcm_eval (root, NULL);
    ASSERT_NE(vt, PURC_VARIANT_INVALID) << "Test Case : "<< get_name();

    char buf[1024] = {0};
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr) << "Test Case : "<< get_name();


    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0) << "Test Case : "<< get_name();

    buf[n] = 0;
    //fprintf(stderr, "buf=%s\n", buf);
    //fprintf(stderr, "com=%s\n", comp);
    ASSERT_STREQ(buf, comp) << "Test Case : "<< get_name();

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);

    pcvcm_node_destroy (root);

    pcejson_destroy(parser);
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

    const char* env = "EJSON_DATA_PATH";
    char data_path[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(data_path, sizeof(data_path), env, "data");

    if (data_path) {
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

                    sprintf(file, "%s/%s.json", data_path, name);
                    char* json_buf = read_file (file);
                    if (!json_buf) {
                        continue;
                    }

                    sprintf(file, "%s/%s.serial", data_path, name);
                    char* comp_buf = read_file (file);
                    if (!comp_buf) {
                        free (json_buf);
                        continue;
                    }

                    vec.push_back(
                            ejson_test_data {
                                name, json_buf, trim(comp_buf), error
                                });

                    free (json_buf);
                    free (comp_buf);
                }
            }
            free (line);
            fclose(fp);
        }
    }

    if (vec.empty()) {
        vec.push_back(ejson_test_data {"array", "[123]", "[123]", 0});
        vec.push_back(ejson_test_data {
                "unquoted_key", "{key:1}", "{\"key\":1}", 0});
        vec.push_back(ejson_test_data {
                "single_quoted_key", "{'key':'2'}", "{\"key\":\"2\"}", 0});
    }
    return vec;
}

INSTANTIATE_TEST_SUITE_P(ejson, ejson_parser_vcm_eval,
        testing::ValuesIn(read_ejson_test_data()));

