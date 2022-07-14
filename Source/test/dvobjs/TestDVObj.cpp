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

#include "TestDVObj.h"

#include "../helpers.h"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <gtest/gtest.h>

TestDVObj::TestDVObj()
{
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", NULL);

    if (ret != PURC_ERROR_OK) {
        purc_log_error("purc_init_ex returns error (%d), please check appName and runnerName\n", ret);
        exit(1);
    }

    const struct purc_variant_stat *stat;
    stat = purc_variant_usage_stat();
    m_init_stat = *stat;
}

TestDVObj::~TestDVObj()
{
    for (dvobj_map_t::iterator i = m_dvobjs.begin();
            i != m_dvobjs.end(); i++) {
        purc_variant_unref((*i).second);
    }

    const struct purc_variant_stat *stat;
    stat = purc_variant_usage_stat();

    if (m_init_stat.nr_total_values != stat->nr_total_values ||
            stat->sz_total_mem != (m_init_stat.sz_total_mem + (stat->nr_reserved -
                    m_init_stat.nr_reserved) * purc_variant_wrapper_size())) {
        purc_log_error("Memory leak found\n");
    }

    purc_cleanup();
}

purc_variant_t TestDVObj::dvobj_new(const char *name)
{
    purc_variant_t dvobj = PURC_VARIANT_INVALID;

    if (strcmp(name, "SYSTEM") == 0) {
        dvobj = purc_dvobj_system_new();
    }
    else if (strcmp(name, "DATETIME") == 0) {
        dvobj = purc_dvobj_datetime_new();
    }
    else if (strcmp(name, "HVML") == 0) {
        dvobj = purc_dvobj_hvml_new(NULL);
    }
    else if (strcmp(name, "EJSON") == 0) {
        dvobj = purc_dvobj_ejson_new();
    }
    else if (strcmp(name, "SESSION") == 0) {
        dvobj = purc_dvobj_session_new();
    }
    else if (strcmp(name, "L") == 0) {
        dvobj = purc_dvobj_logical_new();
    }
    else if (strcmp(name, "T") == 0) {
        dvobj = purc_dvobj_text_new();
    }
    else if (strcmp(name, "STR") == 0) {
        dvobj = purc_dvobj_string_new();
    }
    else if (strcmp(name, "URL") == 0) {
        dvobj = purc_dvobj_url_new();
    }
    else if (strcmp(name, "STREAM") == 0) {
        dvobj = purc_dvobj_stream_new(0);
    }

    if (dvobj != PURC_VARIANT_INVALID) {
        m_dvobjs[name] = dvobj;
    }

    return dvobj;
}

purc_variant_t TestDVObj::get_dvobj(void* ctxt, const char* name)
{
    purc_variant_t dvobj = PURC_VARIANT_INVALID;

    TestDVObj *p = static_cast<TestDVObj *>(ctxt);

    dvobj_map_t::iterator i = p->m_dvobjs.find(name);
    if (i == p->m_dvobjs.end()) {
        dvobj = p->dvobj_new(name);
    }
    else
        dvobj = (*i).second;

    return dvobj;
}

void TestDVObj::run_testcases(const struct dvobj_result *test_cases, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        struct purc_ejson_parse_tree *ptree;
        purc_variant_t result, expected;

        purc_log_info("Evaluting: %s\n", test_cases[i].jsonee);

        ptree = purc_variant_ejson_parse_string(test_cases[i].jsonee,
                strlen(test_cases[i].jsonee));
        result = purc_variant_ejson_parse_tree_evalute(ptree,
                TestDVObj::get_dvobj, this, true);
        purc_variant_ejson_parse_tree_destroy(ptree);

        /* FIXME: purc_variant_string_parse_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(this, test_cases[i].name);

            if (purc_variant_get_type(result) != purc_variant_get_type(expected)) {
                purc_log_error("result type: %s, error message: %s\n",
                        purc_variant_typename(purc_variant_get_type(result)),
                        purc_get_error_message(purc_get_last_error()));
            }

            if (test_cases[i].vrtcmp) {
                ASSERT_EQ(test_cases[i].vrtcmp(result, expected), true);
            }
            else {
                ASSERT_EQ(purc_variant_is_equal_to(result, expected), true);
            }

            if (test_cases[i].errcode) {
                ASSERT_EQ(purc_get_last_error(), test_cases[i].errcode);
            }

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }
}

/**
 * Creates a new purc_rwstream_t which is dedicated for dump only,
 * that is, the new purc_rwstream_t is write-only and not seekable.
 *
 * @param ctxt: the buffer
 *
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_OUT_OF_MEMORY: Out of memory
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t
purc_rwstream_new_for_dump (void *ctxt, pcrws_cb_write fn);
void TestDVObj::run_testcases_in_file(const char *file_name)
{
    char file_path[4096 + 1];
    const char *env = "DVOBJS_TEST_PATH";

    test_getpath_from_env_or_rel(file_path, sizeof(file_path),
            env, "test_files");

    // get test file
    strcat(file_path, "/");
    strcat(file_path, file_name);
    strcat(file_path, ".cases");

    FILE *fp = fopen(file_path, "r");   // open test_list
    ASSERT_NE(fp, nullptr) << "Failed to open file: ["
        << file_path << "]" << std::endl;

    purc_log_info("Run test cases from file: %s\n", file_path);

    size_t line_number = 0;
    size_t case_number = 0;

    char *line = NULL;
    size_t sz = 0;
    ssize_t read = 0;

    line_number = 0;
    while ((read = getline(&line, &sz, fp)) != -1) {
        *(line + read - 1) = 0;
        line_number++;

        struct purc_ejson_parse_tree *ptree;

        if (line[0] == '#') {
            // ignore
        }
        else if (strncasecmp(line, "negative", 8) == 0) {
            purc_log_info("Negative case #%ld, on line #%ld\n", case_number, line_number);

            // read expression
            read = getline(&line, &sz, fp);
            line[read - 1] = 0;
            line_number++;

            const char* exp;
            size_t exp_len = read - 1;
            exp = pcutils_trim_spaces(line, &exp_len);

            purc_variant_t result;
            purc_log_info("Evaluating: `%s`\n", exp);
            ptree = purc_variant_ejson_parse_string(exp, exp_len);
            result = purc_variant_ejson_parse_tree_evalute(ptree,
                    TestDVObj::get_dvobj, this, true);
            purc_variant_ejson_parse_tree_destroy(ptree);

            // read exception name
            read = getline(&line, &sz, fp);
            *(line + read - 1) = 0;
            line_number++;

            const char* exc;
            size_t exc_len = read - 1;
            exc = pcutils_trim_spaces(line, &exc_len);
            purc_log_info("Exception `%s` expected\n", exc);

            purc_atom_t except_atom = purc_get_error_exception(purc_get_last_error());

            EXPECT_EQ(except_atom, purc_atom_try_string_ex(PURC_ATOM_BUCKET_EXCEPT, exc));

            // read expected result
            read = getline(&line, &sz, fp);
            *(line + read - 1) = 0;
            line_number++;

            exp_len = read - 1;
            exp = pcutils_trim_spaces(line, &exp_len);
            if (exp_len > 0) {
                purc_log_info("Silent result `%s` expected\n", exp);

                purc_variant_t expected;
                ptree = purc_variant_ejson_parse_string(exp, exp_len);
                expected = purc_variant_ejson_parse_tree_evalute(ptree,
                        NULL, NULL, true);
                purc_variant_ejson_parse_tree_destroy(ptree);

                bool check = purc_variant_is_equal_to(result, expected);
                if (!check) {
                    char buf[4096];
                    purc_rwstream_t stm;
                    stm = purc_rwstream_new_from_mem(buf, sizeof(buf)-1);
                    ssize_t n = purc_variant_serialize(result, stm, 0, 0, NULL);
                    ASSERT_GT(n, 0);
                    buf[n] = '\0';
                    purc_log_info("Serialized result: %s\n", buf);
                    purc_rwstream_destroy(stm);
                }

                EXPECT_EQ(check, true);
                purc_variant_unref(expected);
            }

            purc_variant_unref(result);
            case_number++;
        }
        else if (strncasecmp(line, "positive", 8) == 0) {
            purc_variant_t result, expected;

            purc_log_info("Positive case #%ld on line #%ld\n", case_number, line_number);

            // read expression
            read = getline(&line, &sz, fp);
            line[read - 1] = 0;
            line_number++;

            const char* exp;
            size_t exp_len = read - 1;
            exp = pcutils_trim_spaces(line, &exp_len);

            purc_log_info("Evaluting: `%s`\n", exp);
            ptree = purc_variant_ejson_parse_string(exp, exp_len);
            result = purc_variant_ejson_parse_tree_evalute(ptree,
                    TestDVObj::get_dvobj, this, true);
            purc_variant_ejson_parse_tree_destroy(ptree);

            // read expected result
            read = getline(&line, &sz, fp);
            *(line + read - 1) = 0;
            line_number++;

            exp_len = read - 1;
            exp = pcutils_trim_spaces(line, &exp_len);

            purc_log_info("Result `%s` expected\n", exp);

            ptree = purc_variant_ejson_parse_string(exp, exp_len);
            expected = purc_variant_ejson_parse_tree_evalute(ptree,
                    NULL, NULL, true);
            purc_variant_ejson_parse_tree_destroy(ptree);

            bool check = purc_variant_is_equal_to(result, expected);
            if (!check) {
                char buf[4096];
                purc_rwstream_t stm;
                stm = purc_rwstream_new_from_mem(buf, sizeof(buf)-1);
                ssize_t n = purc_variant_serialize(result, stm, 0, 0, NULL);
                ASSERT_GT(n, 0);
                buf[n] = '\0';
                purc_log_info("Serialized result: %s\n", buf);
                purc_rwstream_destroy(stm);
            }

            EXPECT_EQ(check, true);

            purc_variant_unref(result);
            purc_variant_unref(expected);
            case_number++;
        }

    }

    fclose(fp);
    if (line)
        free(line);
}

