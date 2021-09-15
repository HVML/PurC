#include "purc.h"

#include "private/hvml.h"
#include "private/utils.h"
#include "purc-rwstream.h"
#include "hvml/hvml-token.h"

#include <stdio.h>
#include <gtest/gtest.h>

using namespace std;

#define PRINTF(...)                                                       \
    do {                                                                  \
        fprintf(stderr, "\e[0;32m[          ] \e[0m");                    \
        fprintf(stderr, __VA_ARGS__);                                     \
    } while(false)

struct hvml_token_test_data {
    string name;
    string hvml;
    string comp;
    int error;
};

class hvml_parser_next_token : public testing::TestWithParam<hvml_token_test_data>
{
protected:
    void SetUp() {
        purc_init ("cn.fmsoft.hybridos.test", "hvml_token", NULL);
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
    TO_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER);
    TO_ERROR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER);
    TO_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_INFORMATIONS);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
    TO_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM_INFORMATION);
    TO_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM_INFORMATION);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_INFORMATION);
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
    TO_ERROR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_INFORMATION);
    TO_ERROR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE);
    TO_ERROR(PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_NULL_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE);
    return -1;
}

TEST_P(hvml_parser_next_token, parse_and_serialize)
{
    const char* hvml = get_hvml();
    const char* comp = get_comp();
    int error_code = get_error();
    PRINTF("test case : %s\n", get_name());

    struct pchvml_parser* parser = pchvml_create(0, 32);
    //fprintf(stderr, "hvml=%s|len=%ld\n", hvml, strlen(hvml));
    //fprintf(stderr, "comp=%s\n", comp);
    // read end of string as eof
    size_t sz = strlen (hvml) + 1;
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)hvml, sz);

    struct pchvml_temp_buffer* buffer = pchvml_temp_buffer_new();

    struct pchvml_token* token = NULL;
    while((token = pchvml_next_token(parser, rws)) != NULL) {
        struct pchvml_temp_buffer* token_buff = pchvml_token_to_string(token);
        if (token_buff) {
            pchvml_temp_buffer_append_temp_buffer(buffer, token_buff);
            pchvml_temp_buffer_destroy(token_buff);
        }
        enum pchvml_token_type type = pchvml_token_get_type(token);
        pchvml_token_destroy(token);
        if (type == PCHVML_TOKEN_EOF) {
            break;
        }
        //PRINTF("serial : %s\n", pchvml_temp_buffer_get_buffer(buffer));
    }
    int error = purc_get_last_error();
    ASSERT_EQ (error, error_code) << "Test Case : "<< get_name();

    if (error_code != PCHVML_SUCCESS)
    {
        ASSERT_EQ (token, nullptr) << "Test Case : "<< get_name();
        purc_rwstream_destroy(rws);
        pchvml_temp_buffer_destroy(buffer);
        pchvml_destroy(parser);
        return;
    }
    else {
        ASSERT_NE (token, nullptr) << "Test Case : "<< get_name();
    }


    const char* serial = pchvml_temp_buffer_get_buffer(buffer);
    ASSERT_STREQ(serial, comp) << "Test Case : "<< get_name();

    purc_rwstream_destroy(rws);
    pchvml_temp_buffer_destroy(buffer);
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

std::vector<hvml_token_test_data> read_hvml_token_test_data()
{
    std::vector<hvml_token_test_data> vec;

    char* data_path = getenv("HVML_TEST_TOKEN_FILES_PATH");

    if (data_path) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_token_list");

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

                    sprintf(file, "%s/%s.hvml", data_path, name);
                    char* buf = read_file (file);

                    if (!buf) {
                        continue;
                    }

                    sprintf(file, "%s/%s.serial", data_path, name);
                    char* comp_buf = read_file (file);
                    if (!comp_buf) {
                        free (buf);
                        continue;
                    }

                    vec.push_back(
                            hvml_token_test_data {
                                name, buf, trim(comp_buf), error
                                });

                    free (buf);
                    free (comp_buf);
                }
            }
            free (line);
            fclose(fp);
        }
    }

    if (vec.empty()) {
        vec.push_back(hvml_token_test_data {"hvml", "<hvml></hvml>",
                "<hvml></hvml>", 0});
    }
    return vec;
}

INSTANTIATE_TEST_SUITE_P(hvml_token, hvml_parser_next_token,
        testing::ValuesIn(read_hvml_token_test_data()));

