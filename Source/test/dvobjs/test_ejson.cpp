#include "purc.h"
#include "private/avl.h"
#include "private/arraylist.h"
#include "private/hashtable.h"
#include "purc-variant.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <gtest/gtest.h>

extern purc_variant_t get_variant (char *buf, size_t *length);
extern void get_variant_total_info (size_t *mem, size_t *value);
#define MAX_PARAM_NR    20

TEST(dvobjs, dvobjs_ejson_type)
{
    const char *function[] = {"type"};
    purc_variant_t param[10] = {0};
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t ret_result = PURC_VARIANT_INVALID;
    size_t function_size = sizeof(function) / sizeof(char *);
    size_t i = 0;
    size_t line_number = 0;

    // get and function
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t ejson = pcdvojbs_get_ejson();
    ASSERT_NE(ejson, nullptr);
    ASSERT_EQ(purc_variant_is_object (ejson), true);

    for (i = 0; i < function_size; i++)  {
        printf ("test _L.%s:\n", function[i]);

        purc_variant_t dynamic = purc_variant_object_get_by_ckey (ejson,
                function[i]);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        purc_dvariant_method func = NULL;
        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        // get test file
        char* data_path = getenv("DVOBJS_TEST_PATH");
        ASSERT_NE(data_path, nullptr);

        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/");
        strcat (file_path, function[i]);
        strcat (file_path, ".test");

        FILE *fp = fopen(file_path, "r");   // open test_list
        ASSERT_NE(fp, nullptr);

        char *line = NULL;
        size_t sz = 0;
        ssize_t read = 0;
        int j = 0;
        size_t length_sub = 0;

        line_number = 0;

        while ((read = getline(&line, &sz, fp)) != -1) {
            *(line + read - 1) = 0;
            line_number ++;

            if (strncasecmp (line, "test_begin", 10) == 0)  {
                printf ("\ttest case on line %ld\n", line_number);

                // get parameters
                read = getline(&line, &sz, fp);
                *(line + read - 1) = 0;
                line_number ++;

                if (strcmp (line, "param_begin") == 0)  {
                    j = 0;

                    // get param
                    while (1) {
                        read = getline(&line, &sz, fp);
                        *(line + read - 1) = 0;
                        line_number ++;

                        if (strcmp (line, "param_end") == 0)  {
                            if (param[j]) {
                                purc_variant_unref(param[j]);
                                param[j] = NULL;
                            }
                            param[j] = NULL;
                            break;
                        }
                        param[j] = get_variant (line, &length_sub);
                        j++;
                    }

                    // get result
                    read = getline(&line, &sz, fp);
                    *(line + read - 1) = 0;
                    line_number ++;

                    ret_result = get_variant(line, &length_sub);

                    // test case end
                    while (1) {
                        read = getline(&line, &sz, fp);
                        *(line + read - 1) = 0;
                        line_number ++;

                        if (strcmp (line, "test_end") == 0)  {
                            break;
                        }
                    }

                    ret_var = func (NULL, j, param);

                    if (ret_result == PURC_VARIANT_INVALID)  {
                        ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
                    }
                    else {
                        // USER MODIFIED HERE.
                        ASSERT_EQ(purc_variant_is_type (ret_var,
                                    PURC_VARIANT_TYPE_STRING), true);
                        ASSERT_STREQ(purc_variant_get_string_const (ret_var),
                                purc_variant_get_string_const (ret_result));
                        purc_variant_unref(ret_var);
                        ret_var = PURC_VARIANT_INVALID;
                        purc_variant_unref(ret_result);
                        ret_result = PURC_VARIANT_INVALID;
                    }

                    for (size_t i=0; i<PCA_TABLESIZE(param); ++i) {
                        if (param[i]) {
                            purc_variant_unref(param[i]);
                            param[i] = NULL;
                        }
                    }
                }
                else
                    continue;
            }
            else
                continue;
        }

        length_sub++;
        fclose(fp);
        if (line)
            free(line);
    }
    purc_variant_unref(ejson);
    purc_cleanup ();
}

TEST(dvobjs, dvobjs_ejson_number)
{
    const char *function[] = {"number"};
    purc_variant_t param[10] = {0};
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t ret_result = PURC_VARIANT_INVALID;
    size_t function_size = sizeof(function) / sizeof(char *);
    size_t i = 0;
    size_t line_number = 0;

    // get and function
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t ejson = pcdvojbs_get_ejson();
    ASSERT_NE(ejson, nullptr);
    ASSERT_EQ(purc_variant_is_object (ejson), true);

    for (i = 0; i < function_size; i++)  {
        printf ("test _L.%s:\n", function[i]);

        purc_variant_t dynamic = purc_variant_object_get_by_ckey (ejson,
                function[i]);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        purc_dvariant_method func = NULL;
        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        // get test file
        char* data_path = getenv("DVOBJS_TEST_PATH");
        ASSERT_NE(data_path, nullptr);

        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/");
        strcat (file_path, function[i]);
        strcat (file_path, ".test");

        FILE *fp = fopen(file_path, "r");   // open test_list
        ASSERT_NE(fp, nullptr);

        char *line = NULL;
        size_t sz = 0;
        ssize_t read = 0;
        int j = 0;
        size_t length_sub = 0;

        line_number = 0;

        while ((read = getline(&line, &sz, fp)) != -1) {
            *(line + read - 1) = 0;
            line_number ++;

            if (strncasecmp (line, "test_begin", 10) == 0)  {
                printf ("\ttest case on line %ld\n", line_number);

                // get parameters
                read = getline(&line, &sz, fp);
                *(line + read - 1) = 0;
                line_number ++;

                if (strcmp (line, "param_begin") == 0)  {
                    j = 0;

                    // get param
                    while (1) {
                        read = getline(&line, &sz, fp);
                        *(line + read - 1) = 0;
                        line_number ++;

                        if (strcmp (line, "param_end") == 0)  {
                            if (param[j]) {
                                purc_variant_unref(param[j]);
                                param[j] = NULL;
                            }
                            param[j] = NULL;
                            break;
                        }
                        param[j] = get_variant (line, &length_sub);
                        j++;
                    }

                    // get result
                    read = getline(&line, &sz, fp);
                    *(line + read - 1) = 0;
                    line_number ++;

                    ret_result = get_variant(line, &length_sub);

                    // test case end
                    while (1) {
                        read = getline(&line, &sz, fp);
                        *(line + read - 1) = 0;
                        line_number ++;

                        if (strcmp (line, "test_end") == 0)  {
                            break;
                        }
                    }

                    ret_var = func (NULL, j, param);

                    if (ret_result == PURC_VARIANT_INVALID)  {
                        ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
                    }
                    else {
                        // USER MODIFIED HERE.
                        ASSERT_EQ(purc_variant_is_type (ret_var,
                                    PURC_VARIANT_TYPE_ULONGINT), true);
                        ASSERT_EQ(ret_var->u64, ret_result->u64);

                        purc_variant_unref(ret_var);
                        ret_var = PURC_VARIANT_INVALID;
                        purc_variant_unref(ret_result);
                        ret_result = PURC_VARIANT_INVALID;
                    }

                    for (size_t i=0; i<PCA_TABLESIZE(param); ++i) {
                        if (param[i]) {
                            purc_variant_unref(param[i]);
                            param[i] = NULL;
                        }
                    }
                }
                else
                    continue;
            }
            else
                continue;
        }

        length_sub++;
        fclose(fp);
        if (line)
            free(line);
    }
    purc_variant_unref(ejson);
    purc_cleanup ();
}

