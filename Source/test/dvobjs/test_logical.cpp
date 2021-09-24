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

static purc_variant_t getter(
        purc_variant_t root, size_t nr_args, purc_variant_t * argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    purc_variant_t value = purc_variant_make_number (3.1415926);
    return value;
}

static purc_variant_t setter(
        purc_variant_t root, size_t nr_args, purc_variant_t * argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    purc_variant_t value = purc_variant_make_number (2.71828828);
    return value;
}

static bool rws_releaser (void* entity)
{
    UNUSED_PARAM(entity);
    return true;
}

static struct purc_native_ops rws_ops = {
    .property_getter       = NULL,
    .property_setter       = NULL,
    .property_eraser       = NULL,
    .property_cleaner      = NULL,
    .cleaner               = NULL,
    .eraser                = rws_releaser,
    .observe               = NULL,
};

static void replace_for_bsequence(char *buf, size_t *length_sub)
{
    size_t tail = 0;
    size_t head = 0;
    char chr = 0;
    unsigned char number = 0;
    unsigned char temp = 0;

    for (tail = 0; tail < *length_sub; tail++)  {
        if (*(buf + tail) == '\\')  {
            tail++;
            chr = *(buf + tail);
            if ((chr >= '0') && (chr <= '9'))
                number = chr - '0';
            else if ((chr >= 'a') && (chr <= 'z'))
                number = chr - 'a';
            else if ((chr >= 'A') && (chr <= 'Z'))
                number = chr - 'A';
            number = number << 4;

            tail++;
            chr = *(buf + tail);
            if ((chr >= '0') && (chr <= '9'))
                temp = chr - '0';
            else if ((chr >= 'a') && (chr <= 'z'))
                temp = chr - 'a';
            else if ((chr >= 'A') && (chr <= 'Z'))
                temp = chr - 'A';
            number |= temp;

            *(buf + head) = number;
            head++;
        } else {
            *(buf + head) = *(buf + tail);
            head++;
        }
    }

    *length_sub = head;

    return;
}

purc_variant_t get_variant (char *buf, size_t *length)
{
    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;
    char *temp = NULL;
    char *temp_end = NULL;
    char tag[64];
    double d = 0.0d;
    int64_t i64;
    uint64_t u64;
    long double ld = 0.0d;
    int number = 0;
    int i = 0;
    size_t length_sub = 0;

    *length = 0;

    temp = strchr (buf, ':');
    snprintf (tag, (temp - buf + 1), "%s", buf);

    switch (*tag) {
        case 'a':
        case 'A':
            switch (*(tag + 1))  {
                case 'r':       // array
                case 'R':
                    temp_end = strchr (temp + 1, ':');
                    snprintf (tag, (temp_end - temp), "%s", temp + 1);
                    number = atoi (tag);
                    temp = temp_end + 1;

                    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
                    for (i = 0; i < number; i++) {
                        val = get_variant (temp, &length_sub);
                        purc_variant_array_append (ret_var, val);
                        purc_variant_unref (val);
                        if (i < number - 1)
                            temp += (length_sub + 1);
                    }
                    *length = temp - buf + length_sub;
                    break;
                case 't':       // atomstring
                case 'T':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_atom_string (temp + 1, false);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'b':
        case 'B':
            switch (*(tag + 1))  {
                case 'o':       // boolean
                case 'O':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    if (strncasecmp (temp + 1, "true", 4) == 0)
                        ret_var = purc_variant_make_boolean (true);
                    else
                        ret_var = purc_variant_make_boolean (false);
                    *length = temp_end - buf;
                    break;
                case 's':       // byte sequence
                case 'S':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    length_sub = temp_end - temp - 1;
                    replace_for_bsequence(temp + 1, &length_sub);
                    ret_var = purc_variant_make_byte_sequence (temp + 1,
                            length_sub);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'd':               // dynamic
        case 'D':
            temp_end = strchr (buf, ';');
            *temp_end = 0x00;
            ret_var = purc_variant_make_dynamic (getter, setter);
            *length = temp_end - buf;
            break;
        case 'i':
        case 'I':
            temp_end = strchr (buf, ';');
            *length = temp_end - buf;
            ret_var = PURC_VARIANT_INVALID;
            break;
        case 'l':
        case 'L':
            switch (*(tag + 4))  {
                case 'd':       // long double
                case 'D':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ld = atof (temp + 1);
                    ret_var = purc_variant_make_longdouble (ld);
                    *length = temp_end - buf;
                    break;
                case 'i':       // long int
                case 'I':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    i64 = atoll (temp + 1);
                    ret_var = purc_variant_make_longint (i64);
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'n':
        case 'N':
            switch (*(tag + 2))  {
                case 't':       // native;
                case 'T':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_native ((void *)"hello world",
                            &rws_ops);
                    *length = temp_end - buf;
                    break;
                case 'l':       // null;
                case 'L':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_null ();
                    *length = temp_end - buf;
                    break;
                case 'm':       // number
                case 'M':
                    temp_end = strchr (temp + 1, ';');
                    *temp_end = 0x00;
                    d = atof (temp + 1);
                    ret_var = purc_variant_make_number (d);
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'o':               // object
        case 'O':
            temp_end = strchr (temp + 1, ':');
            snprintf (tag, (temp_end - temp), "%s", temp + 1);
            number = atoi (tag);
            temp = temp_end + 1;

            ret_var = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                                                    PURC_VARIANT_INVALID);
            for (i = 0; i < number; i++) {
                // get key
                purc_variant_t key = PURC_VARIANT_INVALID;
                temp = strchr (temp, '\"');
                temp_end = strchr (temp + 1, '\"');
                snprintf (tag, temp_end - temp, "%s", temp + 1);
                key = purc_variant_make_string(tag, true);

                // get value
                temp = temp_end + 2;
                *length = temp - buf;
                val = get_variant (temp, &length_sub);
                purc_variant_object_set (ret_var, key, val);

                purc_variant_unref (key);
                purc_variant_unref (val);
                if (i < number - 1)
                    temp += (length_sub + 1);
            }
            *length = temp - buf + length_sub;
            break;
        case 's':
        case 'S':
            switch (*(tag + 1))  {
                case 'e':       // set
                case 'E':
                    temp_end = strchr (temp + 1, ':');
                    snprintf (tag, (temp_end - temp), "%s", temp + 1);
                    number = atoi (tag);
                    temp = temp_end + 1;

                    ret_var = purc_variant_make_set_by_ckey(0, "key1", NULL);
                    for (i = 0; i < number; i++) {
                        val = get_variant (temp, &length_sub);
                        purc_variant_set_add (ret_var, val, false);
                        purc_variant_unref (val);
                        if (i < number - 1)
                            temp += (length_sub + 1);
                    }
                    *length = temp - buf + length_sub;
                    break;
                case 't':       // sting
                case 'T':
                    temp = strchr (temp + 1, '\"');
                    temp_end = strchr (temp + 1, '\"');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_string (temp + 1, false);
                    *length = temp_end + 1 - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        case 'u':
        case 'U':
            switch (*(tag + 1))  {
                case 'l':       // unsigned long int
                case 'L':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    u64 = atoll (temp + 1);
                    ret_var = purc_variant_make_ulongint (u64);
                    *length = temp_end - buf;
                    break;
                case 'n':       // undefined
                case 'N':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_undefined ();
                    *length = temp_end - buf;
                    break;
                default:
                    temp_end = strchr (buf, ';');
                    *length = temp_end - buf;
                    ret_var = PURC_VARIANT_INVALID;
                    break;
            }
            break;
        default:
            temp_end = strchr (buf, ';');
            *length = temp_end - buf;
            ret_var = PURC_VARIANT_INVALID;
            break;
    }

    return ret_var;
}

TEST(dvobjs, dvobjs_logical)
{
    const char *function[] = {"not", "and", "or", "xor", "eq", "ne", "gt",
                              "ge", "lt", "le", "streq", "strne", "strgt",
                              "strge", "strlt", "strle"};
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

    purc_variant_t logical = pcdvojbs_get_logical();
    ASSERT_NE(logical, nullptr);
    ASSERT_EQ(purc_variant_is_object (logical), true);

    for (i = 0; i < function_size; i++)  {
        printf ("test _L.%s:\n", function[i]);

        purc_variant_t dynamic = purc_variant_object_get_by_ckey (logical,
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
                                    PURC_VARIANT_TYPE_BOOLEAN), true);
                        ASSERT_EQ(ret_var->b, ret_result->b);
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
    purc_variant_unref(logical);
    purc_cleanup ();
}

struct test_sample {
    const char      *expr;
    const int       result;
};

TEST(dvobjs, dvobjs_logical_eval)
{
    struct test_sample samples[] = {
        {"1 < 2", 1},
        {"(1 < 2) && (2 > 4)", 0},
        {"(1 < 2) || (2 > 4)", 1}
    };

    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t logical = pcdvojbs_get_logical();
    ASSERT_NE(logical, nullptr);
    ASSERT_EQ(purc_variant_is_object (logical), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (logical, "eval");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    for (size_t i = 0; i < PCA_TABLESIZE (samples); i++)  {
        param[0] = purc_variant_make_string (samples[i].expr, false);
        param[1] = PURC_VARIANT_INVALID;
        param[2] = NULL;
        std::cout << "parsing [" << samples[i].expr << "]" << std::endl;
        ret_var = func (NULL, 2, param);
        ASSERT_NE(ret_var, nullptr);
        ASSERT_EQ(purc_variant_is_type (ret_var,
                    PURC_VARIANT_TYPE_BOOLEAN), true);
        ASSERT_EQ(samples[i].result, ret_var->b);

        purc_variant_unref(ret_var);
        purc_variant_unref(param[0]);
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
    param[2] = NULL;

    purc_variant_t ret_var = func(NULL, 2, param);
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

    snprintf(cmd, sizeof(cmd), "cat '%s' | bc | sed 's/1/true/g' | sed 's/0/false/g'", fn);
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
    int r = 0;
    DIR *d = NULL;
    struct dirent *dir = NULL;
    char path[1024] = {0};

    purc_instance_extra_info info = {0, 0};
    r = purc_init("cn.fmsoft.hybridos.test",
        "test_init", &info);
    EXPECT_EQ(r, PURC_ERROR_OK);
    if (r)
        return;

    purc_variant_t logical = pcdvojbs_get_logical();
    ASSERT_NE(logical, nullptr);
    ASSERT_EQ(purc_variant_is_object (logical), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (logical, "eval");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    const char *env = "DVOBJS_TEST_PATH";
    const char *logical_path = getenv(env);
    std::cout << "env: " << env << "=" << logical_path << std::endl;
    EXPECT_NE(logical_path, nullptr) << "You shall specify via env `"
        << env << "`" << std::endl;
    if (!logical_path)
        goto end;

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
                EXPECT_STREQ(l, r) << "Failed to parse bc file: ["
                    << dir->d_name << "]" << std::endl;
            }
        }
        closedir(d);
    }

end:
    if (logical)
        purc_variant_unref(logical);

    purc_cleanup ();
}
