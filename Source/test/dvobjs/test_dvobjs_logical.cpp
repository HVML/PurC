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
#include "private/avl.h"
#include "private/hashtable.h"
#include "private/variant.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dvobjs.h"

#include "../helpers.h"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <gtest/gtest.h>

extern purc_variant_t get_variant (char *buf, size_t *length);
extern void get_variant_total_info (size_t *mem, size_t *value, size_t *resv_ord, size_t *resv_out);
#define MAX_PARAM_NR    20

TEST(dvobjs, dvobjs_logical)
{
    const char *function[] = {
        "not",
        "and",
        "or",
        "xor",
        "eq",
        "ne",
        "gt",
        "ge",
        "lt",
        "le",
        "streq",
        "strne",
        "strgt",
        "strge",
        "strlt",
        "strle",
    };
    purc_variant_t param[MAX_PARAM_NR] = {0};
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t ret_result = PURC_VARIANT_INVALID;
    size_t function_size = PCA_TABLESIZE(function);
    size_t i = 0;
    size_t line_number = 0;
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_ord_before = 0;
    size_t nr_reserved_out_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_ord_after = 0;
    size_t nr_reserved_out_after = 0;
    char file_path[1024];
    char data_path[PATH_MAX+1];
    const char *env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(data_path, sizeof(data_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << data_path << std::endl;

    // get and function
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t logical = purc_dvobj_logical_new();
    ASSERT_NE(logical, nullptr);
    ASSERT_EQ(purc_variant_is_object (logical), true);

    for (i = 0; i < function_size; i++) {
        printf ("test _L.%s:\n", function[i]);

        purc_variant_t dynamic = purc_variant_object_get_by_ckey_ex (logical,
                function[i], true);
        ASSERT_NE(dynamic, nullptr);
        ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

        purc_dvariant_method func = NULL;
        func = purc_variant_dynamic_get_getter (dynamic);
        ASSERT_NE(func, nullptr);

        // get test file
        strcpy (file_path, data_path);
        strcat (file_path, "/");
        strcat (file_path, function[i]);
        strcat (file_path, ".test");

        fprintf(stderr, "file_path: %s\n", file_path);
        FILE *fp = fopen(file_path, "r");   // open test_list
        ASSERT_NE(fp, nullptr) << "Failed to open file: ["
                               << file_path
                               << "]"
                               << std::endl;

        char *line = NULL;
        if (1) {
            size_t sz = 0;
            ssize_t read = 0;
            size_t j = 0;
            size_t length_sub = 0;

            line_number = 0;

            get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
                    &nr_reserved_ord_before, &nr_reserved_out_before);

            while ((read = getline(&line, &sz, fp)) != -1) {
                *(line + read - 1) = 0;
                line_number ++;

                if (strncasecmp (line, "test_begin", 10) == 0) {
                    printf ("\ttest case on line %ld [%s] (_L.%s)\n",
                            line_number, file_path, function[i]);

                    // get parameters
                    read = getline(&line, &sz, fp);
                    *(line + read - 1) = 0;
                    line_number ++;

                    if (strcmp (line, "param_begin") == 0) {
                        j = 0;

                        // get param
                        while (1) {
                            read = getline(&line, &sz, fp);
                            *(line + read - 1) = 0;
                            line_number ++;

                            if (strcmp (line, "param_end") == 0) {
                                break;
                            }
                            param[j] = get_variant (line, &length_sub);
                            j++;
                            ASSERT_LE(j, MAX_PARAM_NR);
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

                            if (strcmp (line, "test_end") == 0) {
                                break;
                            }
                        }

                        ret_var = func (NULL, j, param, false);

                        if (ret_result == PURC_VARIANT_INVALID) {
                            ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
                        } else {
                            // USER MODIFIED HERE.
                            ASSERT_EQ(purc_variant_is_type (ret_var,
                                        PURC_VARIANT_TYPE_BOOLEAN), true);
                            ASSERT_EQ(ret_var->b, ret_result->b);
                        }

                        PURC_VARIANT_SAFE_CLEAR(ret_var);
                        PURC_VARIANT_SAFE_CLEAR(ret_result);

                        for (size_t i = 0; i < j; ++i)
                            PURC_VARIANT_SAFE_CLEAR(param[i]);

                        get_variant_total_info (&sz_total_mem_after,
                                 &sz_total_values_after, &nr_reserved_ord_after, &nr_reserved_out_after);
                        ASSERT_EQ(sz_total_values_before, sz_total_values_after);
                        ASSERT_EQ(sz_total_mem_after,
                                 sz_total_mem_before +
                                 (nr_reserved_ord_after - nr_reserved_ord_before) * sizeof(purc_variant_ord) +
                                 (nr_reserved_out_after - nr_reserved_out_before) * sizeof(purc_variant));
                    } else
                        continue;
                } else
                    continue;
            }

            length_sub++;
        }

        fclose(fp);
        if (line)
            free(line);
    }

    PURC_VARIANT_SAFE_CLEAR(ret_var);
    PURC_VARIANT_SAFE_CLEAR(ret_result);

    for (size_t i = 0; i < PCA_TABLESIZE(param); ++i)
        PURC_VARIANT_SAFE_CLEAR(param[i]);

    PURC_VARIANT_SAFE_CLEAR(logical);

    purc_cleanup ();
}

struct test_sample {
    const char      *expr;
    const int       result;
};

TEST(dvobjs, dvobjs_logical_eval)
{
    size_t sz_total_mem_before = 0;
    size_t sz_total_values_before = 0;
    size_t nr_reserved_ord_before = 0;
    size_t nr_reserved_out_before = 0;
    size_t sz_total_mem_after = 0;
    size_t sz_total_values_after = 0;
    size_t nr_reserved_ord_after = 0;
    size_t nr_reserved_out_after = 0;

    struct test_sample samples[] = {
        {"1 < 2", 1},
        {"(1 < 2) && (2 > 4)", 0},
        {"(1 < 2) || (2 > 4)", 1}
    };

    purc_variant_t param[MAX_PARAM_NR];
    purc_variant_t ret_var = NULL;

    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t logical = purc_dvobj_logical_new();
    ASSERT_NE(logical, nullptr);
    ASSERT_EQ(purc_variant_is_object (logical), true);

    purc_variant_t dynamic =
        purc_variant_object_get_by_ckey_ex (logical, "eval", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    for (size_t i = 0; i < PCA_TABLESIZE (samples); i++) {
        get_variant_total_info (&sz_total_mem_before, &sz_total_values_before,
                &nr_reserved_ord_before, &nr_reserved_out_before);

        param[0] = purc_variant_make_string (samples[i].expr, false);
        param[1] = PURC_VARIANT_INVALID;
        std::cout << "parsing [" << samples[i].expr << "]" << std::endl;
        ret_var = func (NULL, 1, param, false);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_BOOLEAN), true);
        ASSERT_EQ(samples[i].result, ret_var->b);

        purc_variant_unref(ret_var);
        purc_variant_unref(param[0]);

        get_variant_total_info (&sz_total_mem_after,
                &sz_total_values_after, &nr_reserved_ord_after, &nr_reserved_out_after);
        ASSERT_EQ(sz_total_values_before, sz_total_values_after);
        ASSERT_EQ(sz_total_mem_after,
                sz_total_mem_before +
                (nr_reserved_ord_after - nr_reserved_ord_before) * sizeof(purc_variant_ord) +
                (nr_reserved_out_after - nr_reserved_out_before) * sizeof(purc_variant));
    }

    purc_variant_unref(logical);

    purc_cleanup ();
}

static void
_trim_tail_spaces(char *dest, size_t n)
{
    while (n>1) {
        if (!isspace(dest[n-1]))
            break;
        dest[--n] = '\0';
    }
}

static void
_eval(purc_dvariant_method func, const char *expr,
    char *dest, size_t dlen)
{
    size_t n = 0;
    dest[0] = '\0';

    purc_variant_t param[3];
    param[0] = purc_variant_make_string(expr, false);
    param[1] = PURC_VARIANT_INVALID;

    purc_variant_t ret_var = func(NULL, 1, param, false);
    purc_variant_unref(param[0]);
    if (param[1])
        purc_variant_unref(param[1]);

    if (!ret_var) {
        EXPECT_NE(ret_var, nullptr) << "eval failed: ["
            << expr << "]" << std::endl;
        return;
    }

    purc_rwstream_t ows;
    ows = purc_rwstream_new_from_mem(dest, dlen-1);
    if (!ows)
        goto end;

    purc_variant_serialize(ret_var, ows, 0, 0, &n);
    purc_rwstream_get_mem_buffer(ows, NULL);
    dest[n] = '\0';
    _trim_tail_spaces(dest, n);

    purc_rwstream_destroy(ows);

end:
    purc_variant_unref(ret_var);
}

static void
_eval_bc(const char *fn, char *dest, size_t dlen)
{
    FILE *fin = NULL;
    char cmd[8192];
    size_t n = 0;

    snprintf(cmd, sizeof(cmd), 
            "cat '%s' | bc | sed 's/1/true/g' | sed 's/0/false/g'", fn);
    fin = popen(cmd, "r");
    EXPECT_NE(fin, nullptr) << "failed to execute: [" << cmd << "]"
        << std::endl;
    if (!fin)
        goto end;

    n = fread(dest, 1, dlen-1, fin);
    dest[n] = '\0';
    _trim_tail_spaces(dest, n);

end:
    if (fin)
        pclose(fin);
}

static void
_process_file(purc_dvariant_method func, const char *fn,
    char *dest, size_t dlen)
{
    FILE *fin = NULL;
    size_t sz = 0;
    char buf[8192];
    buf[0] = '\0';

    fin = fopen(fn, "r");
    if (!fin) {
        int err = errno;
        EXPECT_NE(fin, nullptr) << "Failed to open ["
            << fn << "]: [" << err << "]" << strerror(err) << std::endl;
        goto end;
    }

    sz = fread(buf, 1, sizeof(buf)-1, fin);
    buf[sz] = '\0';

    _eval(func, buf, dest, dlen);

end:
    if (fin)
        fclose(fin);
}

TEST(dvobjs, dvobjs_logical_bc)
{
#if OS(LINUX) || OS(DARWIN)
    int r = 0;
    DIR *d = NULL;
    struct dirent *dir = NULL;
    char path[1024] = {0};

    if (access("/usr/bin/bc", F_OK)) {
        return;
    }

    purc_instance_extra_info info = {};
    r = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.hvml.test", "dvobjs", &info);
    EXPECT_EQ(r, PURC_ERROR_OK);
    if (r)
        return;

    purc_variant_t logical = purc_dvobj_logical_new();
    ASSERT_NE(logical, nullptr);
    ASSERT_EQ(purc_variant_is_object (logical), true);

    purc_variant_t dynamic =
        purc_variant_object_get_by_ckey_ex (logical, "eval", true);
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    char logical_path[PATH_MAX+1];
    const char *env = "DVOBJS_TEST_PATH";
    test_getpath_from_env_or_rel(logical_path, sizeof(logical_path),
        env, "test_files");
    std::cerr << "env: " << env << "=" << logical_path << std::endl;
    EXPECT_NE(logical_path, nullptr) << "You shall specify via env `"
        << env << "`" << std::endl;
    strcpy (path, logical_path);
    strcat (path, "/logical_bc");

    d = opendir(path);
    EXPECT_NE(d, nullptr) << "Failed to open dir @["
            << path << "]: [" << errno << "]" << strerror(errno)
            << std::endl;

    if (d) {
        if (chdir(path) != 0) {
            purc_variant_unref(logical);
            return;
        }
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type & DT_REG) {
                char l[8192], r[8192];
                _process_file(func, dir->d_name, l, sizeof(l));
                _eval_bc(dir->d_name, r, sizeof(r));
                fprintf(stderr, "[%s] =?= [%s]\n", l, r);
                EXPECT_STREQ(l, r) << "Failed to compare with bc result: ["
                    << dir->d_name << "]" << std::endl;
            }
        }
        closedir(d);
    }

    if (logical)
        purc_variant_unref(logical);

    purc_cleanup ();
#endif
}
