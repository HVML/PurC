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
#include "private/tkz-helper.h"
#include "purc/purc-rwstream.h"
#include "hvml/hvml-token.h"

#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
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

struct hvml_token_test_data {
    char *name;
    char *hvml;
    char *comp;
    char *comp_path;
    int error;
};

static inline void
push_back(std::vector<hvml_token_test_data> &vec,
        const char *name, const char *hvml, const char *comp,
        const char *comp_path, int error)
{
    hvml_token_test_data data;
    memset(&data, 0, sizeof(data));
    data.name = MemCollector::strdup(name);
    data.hvml = MemCollector::strdup(hvml);
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


class hvml_parser_next_token : public testing::TestWithParam<hvml_token_test_data>
{
protected:
    void SetUp() {
        purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
                "hvml_token", NULL);
    }
    void TearDown() {
        purc_cleanup ();
    }
    const char* get_name() {
        return GetParam().name;
    }
    const char* get_hvml() {
        return GetParam().hvml;
    }
    const char* get_comp() {
        return GetParam().comp;
    }
    const char* get_comp_path() {
        return GetParam().comp_path;
    }
    int get_error() {
        return GetParam().error;
    }
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
    TO_ERROR(PCEJSON_ERROR_BAD_JSONEE_VARIABLE_NAME);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_UNESCAPED_CONTROL_CHARACTER);
    return -1;
}

TEST_P(hvml_parser_next_token, parse_and_serialize)
{
    const char* hvml = get_hvml();
    const char* comp = get_comp();
    const char* comp_path = get_comp_path();
    int error_code = get_error();
    PRINTF("test case : %s\n", get_name());

    struct pchvml_parser* parser = pchvml_create(0, 32);
    //fprintf(stderr, "hvml=%s|len=%ld\n", hvml, strlen(hvml));
    //fprintf(stderr, "comp=%s\n", comp);
    size_t sz = strlen (hvml);
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)hvml, sz);

    struct tkz_buffer* buffer = tkz_buffer_new();

    struct pchvml_token* token = NULL;
    while((token = pchvml_next_token(parser, rws)) != NULL) {
        struct tkz_buffer* token_buff = pchvml_token_to_string(token);
        if (token_buff) {
            const char* type_name = pchvml_token_get_type_name(token);
            //PRINTF("%s:%s\n", type_name, tkz_buffer_get_bytes(token_buff));
            tkz_buffer_append_bytes(buffer, type_name, strlen(type_name));
            tkz_buffer_append_bytes(buffer, "|", 1);
            tkz_buffer_append_another(buffer, token_buff);
            tkz_buffer_append_bytes(buffer, "\n", 1);
            tkz_buffer_destroy(token_buff);
        }
        enum pchvml_token_type type = pchvml_token_get_type(token);
        pchvml_token_destroy(token);
        token = NULL;
        if (type == PCHVML_TOKEN_EOF) {
            break;
        }
//        PRINTF("serial : %s|code=%d\n", tkz_buffer_get_bytes(buffer)
//                , purc_get_last_error());
    }
    int error = purc_get_last_error();
    ASSERT_EQ (error, error_code) << "Test Case : "<< get_name();

    if (error_code != PCHVML_SUCCESS)
    {
        purc_rwstream_destroy(rws);
        tkz_buffer_destroy(buffer);
        pchvml_destroy(parser);
        return;
    }

    const char* serial = tkz_buffer_get_bytes(buffer);
    char* result = strdup(serial);
//    PRINTF("serial : %s", serial);
    if (comp) {
        ASSERT_STREQ(trim(result), comp) << "Test Case : "<< get_name();
    }
    else {
        FILE* fp = fopen(comp_path, "w");
        fprintf(fp, "%s", serial);
        fclose(fp);
    }
    free(result);

    purc_rwstream_destroy(rws);
    tkz_buffer_destroy(buffer);
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

std::vector<hvml_token_test_data> read_hvml_token_test_data()
{
    std::vector<hvml_token_test_data> vec;

    const char* env = "HVML_TEST_TOKEN_FILES_PATH";
    char data_path[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(data_path, sizeof(data_path), env,
            "data");

    if (strlen(data_path)) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_list");

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
                    n = snprintf(file, sizeof(file), "%s/%s.hvml", data_path, name);
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
                    if (comp_buf) {
                        push_back(vec, name, buf, trim(comp_buf), file, error);
                    }
                    else {
                        push_back(vec, name, buf, NULL, file, error);
                    }

                    free (buf);
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
        push_back(vec, "hvml",
                "<hvml></hvml>",
                "PCHVML_TOKEN_START_TAG|<hvml>\nPCHVML_TOKEN_END_TAG|</hvml>",
                NULL,
                0);
    }
    return vec;
}

INSTANTIATE_TEST_SUITE_P(hvml_token, hvml_parser_next_token,
        testing::ValuesIn(read_hvml_token_test_data()));

