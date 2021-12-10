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
#include <libgen.h>
#include <gtest/gtest.h>

using namespace std;

#define PRINTF(...)                                                       \
    do {                                                                  \
        fprintf(stderr, "\e[0;32m[          ] \e[0m");                    \
        fprintf(stderr, __VA_ARGS__);                                     \
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

struct vcm_eval_test_data {
    char *name;
    char *hvml;
    char *comp;
    int error;
};

static inline void
push_back(std::vector<vcm_eval_test_data> &vec,
        const char *name, const char *hvml, const char *comp, int error)
{
    vcm_eval_test_data data;
    memset(&data, 0, sizeof(data));
    data.name = MemCollector::strdup(name);
    data.hvml = MemCollector::strdup(hvml);
    data.comp = MemCollector::strdup(comp);
    data.error = error;

    vec.push_back(data);
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


class test_vcm_eval : public testing::TestWithParam<vcm_eval_test_data>
{
protected:
    void SetUp() {
        purc_init ("cn.fmsoft.hybridos.test", "vcm_eval", NULL);
        name = GetParam().name;
        hvml = GetParam().hvml;
        comp = GetParam().comp;
        error = GetParam().error;
    }
    void TearDown() {
        purc_cleanup ();
    }
    const char* get_name() {
        return name.c_str();
    }
    const char* get_hvml() {
        return hvml.c_str();
    }
    const char* get_comp() {
        return comp.c_str();
    }
    int get_error() {
        return error;
    }
private:
    string name;
    string hvml;
    string comp;
    int error;
};

#define TO_ERROR(err_name)                                 \
    if (strcmp (err, #err_name) == 0) {                  \
        return err_name;                                 \
    }

int to_error(const char* err)
{
    TO_ERROR(PCHVML_SUCCESS);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_NULL_CHARACTER);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME);
    TO_ERROR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
    TO_ERROR(PCHVML_ERROR_MISSING_END_TAG_NAME);
    TO_ERROR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
    TO_ERROR(PCHVML_ERROR_EOF_IN_TAG);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG);
    TO_ERROR(PCHVML_ERROR_CDATA_IN_HTML_CONTENT);
    TO_ERROR(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT);
    TO_ERROR(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
    TO_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
    TO_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME);
    TO_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_NAME);
    TO_ERROR(PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
    TO_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_ID);
    TO_ERROR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_ID);
    TO_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_ID);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUB_AND_SYS);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
    TO_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM);
    TO_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM);
    TO_ERROR(PCHVML_ERROR_EOF_IN_CDATA);
    TO_ERROR(PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEY_NAME);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_COMMA);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_BASE64);
    TO_ERROR(PCHVML_ERROR_BAD_JSON_NUMBER);
    TO_ERROR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_ESCAPE_ENTITY);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
    TO_ERROR(PCHVML_ERROR_EMPTY_JSONEE_NAME);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_NAME);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
    TO_ERROR(PCHVML_ERROR_EMPTY_JSONEE_KEYWORD);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_COMMA);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_PARENTHESIS);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_LEFT_ANGLE_BRACKET);
    TO_ERROR(PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE);
    TO_ERROR(PCHVML_ERROR_NESTED_COMMENT);
    TO_ERROR(PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT);
    TO_ERROR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    TO_ERROR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE);
    TO_ERROR(PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_NULL_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_INVALID_UTF8_CHARACTER);
    return -1;
}

purc_variant_t find_var(void* ctxt, const char* name)
{
    if (strcmp(name, "SYSTEM") == 0) {
        return (purc_variant_t) ctxt;
    }
    return purc_variant_make_string(name, false);
}

TEST_P(test_vcm_eval, parse_and_serialize)
{
    const char* hvml = get_hvml();
    const char* comp = get_comp();
    int error_code = get_error();
    PRINTF("test case : %s\n", get_name());

    struct pchvml_parser* parser = pchvml_create(0, 32);
    size_t sz = strlen (hvml);
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)hvml, sz);


    parser->state = PCHVML_EJSON_DATA_STATE;
    struct pchvml_token* token = pchvml_next_token(parser, rws);

    int error = purc_get_last_error();
    ASSERT_EQ (error, error_code) << "Test Case : "<< get_name();

    if (error_code != PCHVML_SUCCESS)
    {
        purc_rwstream_destroy(rws);
        pchvml_destroy(parser);
        return;
    }

    purc_variant_t sys = pcdvobjs_get_system();
    struct pcvcm_node* root = pchvml_token_get_vcm(token);

    purc_variant_t vt = pcvcm_eval_ex (root, find_var, sys);
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

    purc_variant_unref(sys);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);
    pchvml_destroy(parser);
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

std::vector<vcm_eval_test_data> read_vcm_eval_test_data()
{
    std::vector<vcm_eval_test_data> vec;

    const char* env = "VCM_EVAL_TEST_FILES_PATH";
    char data_path[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(data_path, sizeof(data_path), env,
            "test_vcm_eval_files");

    if (strlen(data_path)) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_files_list");

        FILE* fp = fopen(file_path, "r");
        if (fp) {
            char file[PATH_MAX+16] = {0};

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

                    int n;
                    n = snprintf(file, sizeof(file), "%s/%s.json", data_path, name);
                    if (n>=0 && (size_t)n>=sizeof(file)) {
                        // to circumvent format-truncation warning
                        ;
                    }
                    char* buf = read_file (file);

                    if (!buf) {
                        continue;
                    }

                    n = snprintf(file, sizeof(file), "%s/%s.serial", data_path, name);
                    if (n>=0 && (size_t)n>=sizeof(file)) {
                        // to circumvent format-truncation warning
                        ;
                    }
                    char* comp_buf = read_file (file);
                    if (!comp_buf) {
                        free (buf);
                        continue;
                    }

                    push_back(vec, name, buf, trim(comp_buf), error);

                    free (buf);
                    free (comp_buf);
                }
            }
            free (line);
            fclose(fp);
        }
    }

    if (vec.empty()) {
        push_back(vec, "array", "[123]\n", "[123]", 0);
        push_back(vec, "unquoted_key", "{key:1}\n", "{\"key\":1}", 0);
        push_back(vec, "single_quoted_key", "{'key':'2'}\n", "{\"key\":\"2\"}", 0);
    }
    return vec;
}

INSTANTIATE_TEST_SUITE_P(vcm_eval, test_vcm_eval,
        testing::ValuesIn(read_vcm_eval_test_data()));

