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
#include <errno.h>
#include <gtest/gtest.h>

// it is the basic test
TEST(dvobjs, dvobjs_logical_not)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = NULL;

    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t logical = pcdvojbs_get_logical();
    ASSERT_NE(logical, nullptr);
    ASSERT_EQ(purc_variant_is_object (logical), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (logical, "not");
    ASSERT_NE(dynamic, nullptr);
    ASSERT_EQ(purc_variant_is_dynamic (dynamic), true);

    purc_dvariant_method func = NULL;
    func = purc_variant_dynamic_get_getter (dynamic);
    ASSERT_NE(func, nullptr);

    // undefined
    param[0] = purc_variant_make_undefined ();
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    // null
    param[0] = purc_variant_make_null ();
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    // boolean
    param[0] = purc_variant_make_boolean (true);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    param[0] = purc_variant_make_boolean (false);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    // number
    param[0] = purc_variant_make_number (0.0);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    param[0] = purc_variant_make_number (1.1);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    param[0] = purc_variant_make_number (-1.1);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);
   
    // ulongint
    param[0] = purc_variant_make_ulongint (1);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);
   
    param[0] = purc_variant_make_ulongint (0);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);
   
    // longint
    param[0] = purc_variant_make_longint(-1);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);
   
    param[0] = purc_variant_make_longint(0);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);
   
    // long double
    param[0] = purc_variant_make_longdouble(-1.2);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    param[0] = purc_variant_make_longdouble(0.0);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    // string
    param[0] = purc_variant_make_string("", false);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    param[0] = purc_variant_make_string("hello", false);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    // atom string
    param[0] = purc_variant_make_atom_string("", false);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    param[0] = purc_variant_make_atom_string("hello world", false);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    // byte sequence
    param[0] = purc_variant_make_byte_sequence ("hello world", 5);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    // native
    struct purc_native_ops ops;
    param[0] = purc_variant_make_native (ret_var, &ops);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    // object
    param[0] = purc_variant_make_object (0, PURC_VARIANT_INVALID, 
                                                    PURC_VARIANT_INVALID);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    param[0] = purc_variant_make_object (0, PURC_VARIANT_INVALID, 
                                                    PURC_VARIANT_INVALID);
    purc_variant_object_set (param[0], purc_variant_make_string("hello", false),
                            purc_variant_make_longdouble(-1.2));
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    // array
    param[0] = purc_variant_make_array (0, PURC_VARIANT_INVALID); 
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    param[0] = purc_variant_make_array (0, PURC_VARIANT_INVALID); 
    purc_variant_array_append (param[0], purc_variant_make_string("hello", false));
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    // set
    param[0] = purc_variant_make_set(0, PURC_VARIANT_INVALID, PURC_VARIANT_INVALID); 
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, true);

    param[0] = purc_variant_make_set(0, PURC_VARIANT_INVALID, PURC_VARIANT_INVALID); 
    purc_variant_set_add (param[0], purc_variant_make_string("hello", false), false);
    param[1] = NULL;
    ret_var = func (NULL, 0, param);
    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
    ASSERT_EQ(ret_var->b, false);

    purc_cleanup ();
}

purc_variant_t get_variant (char *buf)
{
    purc_variant_t ret_var = NULL;
    char *temp = NULL;
    char *temp_end = NULL;
    char tag[16];
    double d = 0.0d;
    int64_t i64;
    uint64_t u64;
    long double ld = 0.0d;

    temp = strchr (buf, ':');
    snprintf (tag, (temp - buf + 1), "%s", buf);

    switch (*tag) {
        case 's':
        case 'S':
            switch (*(tag + 1))  {
                case 't':
                case 'T':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_string (temp + 1, false);
                    break;
                default:
                    ret_var = PURC_VARIANT_INVALID; 
                    break;
            }
            break;
        case 'a':
        case 'A':
            switch (*(tag + 1))  {
                case 't':
                case 'T':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_atom_string (temp + 1, false);
                    break;
                default:
                    ret_var = PURC_VARIANT_INVALID; 
                    break;
            }
            break;
        case 'u':
        case 'U':
            switch (*(tag + 1))  {
                case 'n':
                case 'N':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_undefined ();
                    break;
                case 'l':
                case 'L':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    u64 = atoll (temp + 1);
                    ret_var = purc_variant_make_ulongint (u64);
                    break;
                default:
                    ret_var = PURC_VARIANT_INVALID; 
                    break;
            }
            break;
        case 'n':
        case 'N':
            switch (*(tag + 2))  {
                case 'l':
                case 'L':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_null ();
                    break;
                case 'm':
                case 'M':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    d = atof (temp + 1);
                    ret_var = purc_variant_make_number (d);
                    break;
                default:
                    ret_var = PURC_VARIANT_INVALID; 
                    break;
            }
            break;
        case 'b':
        case 'B':
            switch (*(tag + 1))  {
                case 'o':
                case 'O':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    if (strncasecmp (temp + 1, "true", 4) == 0)
                        ret_var = purc_variant_make_boolean (true);
                    else
                        ret_var = purc_variant_make_boolean (false);
                    break;
                case 'e':
                case 'E':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ret_var = purc_variant_make_byte_sequence (temp + 1, temp_end - temp - 1);
                    break;
                default:
                    ret_var = PURC_VARIANT_INVALID; 
                    break;
            }
            break;
        case 'l':
        case 'L':
            switch (*(tag + 4))  {
                case 'i':
                case 'I':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    i64 = atoll (temp + 1);
                    ret_var = purc_variant_make_longint (i64);
                    break;
                case 'd':
                case 'D':
                    temp_end = strchr (buf, ';');
                    *temp_end = 0x00;
                    ld = atof (temp + 1);
                    ret_var = purc_variant_make_longdouble (ld);
                    break;
                default:
                    ret_var = PURC_VARIANT_INVALID; 
                    break;
            }
            break;
        case 'i':
        case 'I':
            ret_var = PURC_VARIANT_INVALID; 
            break;
        default:
            ret_var = PURC_VARIANT_INVALID; 
            break;
    }

    return ret_var;
}

TEST(dvobjs, dvobjs_logical_and)
{
    purc_variant_t param[10];
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t ret_result = PURC_VARIANT_INVALID;

    // get and function
    purc_instance_extra_info info = {0, 0};
    int ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t logical = pcdvojbs_get_logical();
    ASSERT_NE(logical, nullptr);
    ASSERT_EQ(purc_variant_is_object (logical), true);

    purc_variant_t dynamic = purc_variant_object_get_by_ckey (logical, "and");
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
    strcat (file_path, "/and.test");

    FILE *fp = fopen(file_path, "r");   // open test_list
    ASSERT_NE(fp, nullptr);

    char *line = NULL;
    size_t sz = 0;
    ssize_t read = 0;
    int i = 0;
    int j = 0;

    while ((read = getline(&line, &sz, fp)) != -1) {
        *(line + read - 1) = 0;
        if (strcmp (line, "test_begin") == 0)  {    // begin a new test
            i ++;
            printf ("test case %d\n", i);

            // get parameters
            read = getline(&line, &sz, fp);
            *(line + read - 1) = 0;
            if (strcmp (line, "param_begin") == 0)  {   // begin param section
                j = 0;
                while (1) {
                    read = getline(&line, &sz, fp);
                    *(line + read - 1) = 0;
                    if (strcmp (line, "param_end") == 0)  {   // end param section
                        param[j] = NULL;
                        break;
                    }
                    param[j] = get_variant (line);
                    j++;
                }
                read = getline(&line, &sz, fp);
                *(line + read - 1) = 0;
                ret_result = get_variant(line);

                while (1) {
                    read = getline(&line, &sz, fp);
                    *(line + read - 1) = 0;
                    if (strcmp (line, "test_end") == 0)  {   // end test case 
                        break;
                    }
                }

                ret_var = func (NULL, j, param);

                if (ret_result == PURC_VARIANT_INVALID)  {
                    ASSERT_EQ(ret_var, PURC_VARIANT_INVALID);
                }
                else {
                    // it is custom, you only write here.
                    ASSERT_EQ(purc_variant_is_type (ret_var, PURC_VARIANT_TYPE_BOOLEAN), true);
                    ASSERT_EQ(ret_var->b, ret_result->b);
                }
            }
            else
                continue;
        }
        else
            continue;
    }

    fclose(fp);


    purc_cleanup ();
}
