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
}

TestDVObj::~TestDVObj()
{
    for (dvobj_map_t::iterator i = m_dvobjs.begin();
            i != m_dvobjs.end(); i++) {
        purc_variant_unref((*i).second);
    }

    purc_cleanup();
}

purc_variant_t TestDVObj::dvobj_new(const char *dvobj_name)
{
    purc_variant_t dvobj = PURC_VARIANT_INVALID;

    if (strcmp(dvobj_name, "SYSTEM") == 0) {
        dvobj = purc_dvobj_system_new();
    }
    else if (strcmp(dvobj_name, "DATETIME") == 0) {
        dvobj = purc_dvobj_datetime_new();
    }
    else if (strcmp(dvobj_name, "HVML") == 0) {
        dvobj = purc_dvobj_hvml_new(NULL);
    }
    else if (strcmp(dvobj_name, "EJSON") == 0) {
        dvobj = purc_dvobj_ejson_new();
    }
    else if (strcmp(dvobj_name, "SESSION") == 0) {
        dvobj = purc_dvobj_session_new();
    }
    else if (strcmp(dvobj_name, "L") == 0) {
        dvobj = purc_dvobj_logical_new();
    }
    else if (strcmp(dvobj_name, "T") == 0) {
        dvobj = purc_dvobj_text_new();
    }
    else if (strcmp(dvobj_name, "STR") == 0) {
        dvobj = purc_dvobj_string_new();
    }
#if 0
    else if (strcmp(dvobj_name, "URL") == 0) {
        dvobj = purc_dvobj_url_new();
    }
#endif

    if (dvobj != PURC_VARIANT_INVALID) {
        m_dvobjs[dvobj_name] = dvobj;
    }

    return dvobj;
}

purc_variant_t TestDVObj::get_dvobj(void* ctxt, const char* name)
{
    TestDVObj *p = static_cast<TestDVObj *>(ctxt);
    purc_log_info("members in map: %lu\n", p->m_dvobjs.size());
    purc_log_info("dvobj: %p for %s in map (%p)\n", p->m_dvobjs[name], name, p);
    return p->m_dvobjs[name];
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

void TestDVObj::run_testcases_in_file(const char *path_name, const char *file_name)
{
    size_t line_number = 0;
    size_t case_number = 0;
    char file_path[4096 + 1];

    // get test file
    strcpy(file_path, path_name);
    strcat(file_path, "/");
    strcat(file_path, file_name);
    strcat(file_path, ".cases");

    FILE *fp = fopen(file_path, "r");   // open test_list
    ASSERT_NE(fp, nullptr) << "Failed to open file: ["
        << file_path << "]" << std::endl;

    char *line = NULL;
    size_t sz = 0;
    ssize_t read = 0;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_after = 0;
    get_variant_total_info(&sz_total_mem_before, &sz_total_values_before,
            &nr_reserved_before);

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
            purc_variant_unref(result);

            // read exception name
            read = getline(&line, &sz, fp);
            *(line + read - 1) = 0;
            line_number++;

            const char* exc;
            size_t exc_len = read - 1;
            exc = pcutils_trim_spaces(line, &exc_len);
            purc_log_info("Exception `%s` expected\n", exc);

            purc_atom_t except_atom = purc_get_error_exception(purc_get_last_error());

            ASSERT_EQ(except_atom, purc_atom_try_string_ex(1, exc));
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
            ASSERT_EQ(check, true);

            purc_variant_unref(result);
            purc_variant_unref(expected);
            case_number ++;
        }

        get_variant_total_info(&sz_total_mem_after,
                &sz_total_values_after, &nr_reserved_after);
        ASSERT_EQ(sz_total_values_before, sz_total_values_after);
        ASSERT_EQ(sz_total_mem_after,
                sz_total_mem_before + (nr_reserved_after -
                    nr_reserved_before) * purc_variant_wrapper_size());
    }

    fclose(fp);
    if (line)
        free(line);
}

