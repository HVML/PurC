/*
 * @file system.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of system dynamic variant object.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"
#include "tools.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <locale.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>

#define FORMAT_ISO8601  1
#define FORMAT_RFC822   2

https://www.doxygen.nl/manual/output.html

/**
 * @brief       Get system information 
 *
 * @param[in]   root    : The context variant
 * @param[in]   nr_args : The number of elements in argv array, it should be 0
 * @param[in]   argv    : A varaint array
 *                        argv[0], it should be PURC_VARIANT_INVALID 
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_BAD_SYSTEM_CALL when invokes uname error.
 *              - A PURC_VARIANT_TYPE_OBJECT varint with:
 *                  {
 *                      "kernel-name"       : "xxxxxxx",
 *                      "nodename"          : "xxxxxxx",
 *                      "kernel-release"    : "xxxxxxx",
 *                      "kernel-version"    : "xxxxxxx",
 *                      "machine"           : "xxxxxxx",
 *                      "processor"         : "xxxxxxx",
 *                      "hardware-platform" : "xxxxxxx",
 *                      "operating-system"  : "xxxxxxx"
 *                  }
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "uname");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_getter (dynamic);
 *
 *              purc_variant_t param[1];
 *              param[0] = PURC_VARIANT_INVALID;
 *              purc_variant_t var = func (root, 0, param);
 * @endcode
 *
 * @note        The function does not check the validity of parameters,
 *              you can simplify the code as:
 *              purc_variant_t var = func (root, 0, PURC_VARIANT_INVALID);
*/
static purc_variant_t
uname_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (uname (&name) < 0) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    // create an empty object
    ret_var = purc_variant_make_object (0, PURC_VARIANT_INVALID, 
            PURC_VARIANT_INVALID);
    if(ret_var == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_object_set_c (ret_var, "kernel-name",
            purc_variant_make_string (name.sysname, true));
    purc_variant_object_set_c (ret_var, "nodename",
            purc_variant_make_string (name.nodename, true));
    purc_variant_object_set_c (ret_var, "kernel-release",
            purc_variant_make_string (name.release, true));
    purc_variant_object_set_c (ret_var, "kernel-version",
            purc_variant_make_string (name.version, true));
    purc_variant_object_set_c (ret_var, "machine",
            purc_variant_make_string (name.machine, true));
    purc_variant_object_set_c (ret_var, "processor",
            purc_variant_make_string (name.machine, true));
    purc_variant_object_set_c (ret_var, "hardware-platform",
            purc_variant_make_string (name.machine, true));
    purc_variant_object_set_c (ret_var, "operating-system",
            purc_variant_make_string (name.sysname, true));

    return ret_var;
}


/**
 * @brief       Get system information according to user's input 
 *
 * @param[in]   root    : The context variant
 * @param[in]   nr_args : The number of elements in argv array, it should be 1
 * @param[in]   argv    : A varaint array
 *                        argv[0], a PURC_VARIANT_TYPE_STRING type, can be one
 *                            or more items as below:
 *                            "kernel-name", "kernel-release", "kernel-version", 
 *                            "nodename", "machine", "processor", 
 *                            "hardware-platform", "operating-system".
 *                            or :
 *                            "all"
 *                            or :
 *                            "default"
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_BAD_SYSTEM_CALL when invokes uname error,
 *                  PURC_ERROR_WRONG_ARGS when input parameter is error.
 *              - A PURC_VARIANT_TYPE_STRING varint, accroding to the 
 *                  input orders with white space as delimiter.
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "uname_prt");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_getter (dynamic);
 *
 *              purc_variant_t param[2];
 *              param[0] = purc_variant_make_string ("kernel-name processor");
 *              param[1] = NULL;
 *              purc_variant_t var = func (root, 1, param);
 * @endcode
 *
 * @note        If input multiple items, use white space as delimiter.
 *              If input "all", you will get all system information.
 *              If input "default", only get "kernel-name", "kernel-release", 
 *                  "kernel-version", "nodename", "machine".
*/
static purc_variant_t
uname_prt_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    struct utsname name;
    size_t length = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    bool first = true;
    const char * delim = " ";

    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, 1024);

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                                (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
        
    if (uname (&name) < 0) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    if (nr_args) {
        const char * option = purc_variant_get_string_const (argv[0]);
        const char * head = pcdvobjs_get_next_option (option, " ", &length);

        while (head) {
            switch (* head)
            {
                case 'a':
                case 'A':
                    if (strncasecmp (head, "all", length) == 0) {
                        purc_rwstream_seek (rwstream, 0, SEEK_SET); 

                        purc_rwstream_write (rwstream, name.sysname, 
                                                        strlen (name.sysname));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.nodename, 
                                                    strlen (name.nodename));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.release, 
                                                    strlen (name.release));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.version, 
                                                    strlen (name.version));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.machine,
                                                    strlen (name.machine));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        // process
                        purc_rwstream_write (rwstream, name.machine,
                                                    strlen (name.machine));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        // hardware
                        purc_rwstream_write (rwstream, name.machine,
                                                    strlen (name.machine));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        // os
                        purc_rwstream_write (rwstream, name.sysname, 
                                                        strlen (name.sysname));
                    }
                    break;
                case 'd':
                case 'D':
                    if (strncasecmp (head, "default", length) == 0) {
                        purc_rwstream_seek (rwstream, 0, SEEK_SET); 

                        purc_rwstream_write (rwstream, name.sysname, 
                                                        strlen (name.sysname));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.nodename, 
                                                    strlen (name.nodename));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.release, 
                                                    strlen (name.release));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.version, 
                                                    strlen (name.version));
                        purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.machine,
                                                    strlen (name.machine));
                    }
                    break;
            
                case 'o':
                case 'O':
                    if (strncasecmp (head, "operating-system", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.sysname, 
                                                        strlen (name.sysname));
                    }
                    break;

                case 'h':
                case 'H':
                    if (strncasecmp (head, "hardware-platform", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.machine,
                                                    strlen (name.machine));
                    }
                    break;

                case 'p':
                case 'P':
                    if (strncasecmp (head, "processor", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.machine,
                                                    strlen (name.machine));
                    }
                    break;

                case 'm':
                case 'M':
                    if (strncasecmp (head, "machine", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.machine,
                                                    strlen (name.machine));
                    }
                    break;

                case 'n':
                case 'N':
                    if (strncasecmp (head, "nodename ", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.nodename, 
                                                    strlen (name.nodename));
                    }
                    break;

                case 'k':
                case 'K':
                    if (strncasecmp (head, "kernel-name", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.sysname, 
                                                        strlen (name.sysname));
                    }
                    else if (strncasecmp (head, "kernel-release", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.release, 
                                                    strlen (name.release));
                    }
                    else if (strncasecmp (head, "kernel-version", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.version, 
                                                    strlen (name.version));
                    }
                    break;
            }

            head = pcdvobjs_get_next_option (head + length + 1, " ", &length);
        }
    }
    else
        purc_rwstream_write (rwstream, name.sysname, strlen (name.sysname));

    size_t rw_size = 0;
    size_t content_size = 0;
    char * rw_string = purc_rwstream_get_mem_buffer_ex (rwstream, 
                                            &content_size, &rw_size, true);
    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        ret_var = purc_variant_make_string_reuse_buff (rw_string, rw_size, false); 
        if(ret_var == PURC_VARIANT_INVALID) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
}


/**
 * @brief       Get locale information according to user's input 
 *
 * @param[in]   root    : The context variant
 * @param[in]   nr_args : The number of elements in argv array, it should be 1
 * @param[in]   argv    : A varaint array
 *                        argv[0], a PURC_VARIANT_TYPE_STRING type, can be one
 *                            items as below:
 *                            "ctype", "numeric", "time", "collate", "monetary", 
 *                            "messages", "paper", "name", "address", "telephone", 
 *                            "measurement", "identification"
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_WRONG_ARGS when input parameter is error.
 *              - A PURC_VARIANT_TYPE_STRING varint, accroding to the input. 
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "locale");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_getter (dynamic);
 *
 *              purc_variant_t param[2];
 *              param[0] = purc_variant_make_string ("ctype");
 *              param[1] = NULL;
 *              purc_variant_t var = func (root, 1, param);
 * @endcode
 *
 * @note
*/
static purc_variant_t
locale_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    size_t length = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
        
    if (nr_args) {
        const char* option = purc_variant_get_string_const (argv[0]);
        const char * head = pcdvobjs_get_next_option (option, " ", &length);

        while (head) {
            switch (* head)
            {
                case 'c':
                case 'C':
                    if (strncasecmp (head, "ctype", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_CTYPE, NULL), true);
                    }
                    else if (strncasecmp (head, "collate", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_COLLATE, NULL), true);
                    }
                    break;

                case 'n':
                case 'N':
                    if (strncasecmp (head, "numeric", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_NUMERIC, NULL), true);
                    }
                    else if (strncasecmp (head, "name", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_NAME, NULL), true);
                    }
                    break;

                case 't':
                case 'T':
                    if (strncasecmp (head, "time", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_TIME, NULL), true);
                    }
                    else if (strncasecmp (head, "telephone", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_TELEPHONE, NULL), true);
                    }
                    break;

                case 'm':
                case 'M':
                    if (strncasecmp (head, "monetary", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_MONETARY, NULL), true);
                    }
                    else if (strncasecmp (head, "messages", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_MESSAGES, NULL), true);
                    }
                    if (strncasecmp (head, "measurement", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_MEASUREMENT, NULL), true);
                    }
                    break;

                case 'p':
                case 'P':
                    if (strncasecmp (head, "paper", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_PAPER, NULL), true);
                    }
                    break;

                case 'a':
                case 'A':
                    if (strncasecmp (head, "address", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_ADDRESS, NULL), true);
                    }
                    break;

                case 'i':
                case 'I':
                    if (strncasecmp (head, "identification", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_IDENTIFICATION, NULL), true);
                    }
                    break;
            }

            head = pcdvobjs_get_next_option (head + length + 1, " ", &length);
        }
    }
    else
        ret_var = purc_variant_make_string (setlocale (LC_ALL, NULL), true);

    if (ret_var == PURC_VARIANT_INVALID)
    {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return ret_var;
}

/**
 * @brief       Set locale information according to user's input 
 *
 * @param[in]   root    : The context variant
 * @param[in]   nr_args : The number of elements in argv array, it should be 2
 * @param[in]   argv    : A varaint array
 *                        argv[0], a PURC_VARIANT_TYPE_STRING type, can be one
 *                            or more items as below:
 *                            "ctype", "numeric", "time", "collate", "monetary", 
 *                            "messages", "paper", "name", "address", "telephone", 
 *                            "measurement", "identification"
 *                            or :
 *                            "all"
 *                        argv[1], a PURC_VARIANT_TYPE_STRING type, the locale 
 *                            to be set.
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_WRONG_ARGS when input parameter is error.
 *              - A PURC_VARIANT_TYPE_BOOLEAN varint.
 *                  PURC_VARIANT_TRUE for successful, otherwise 
 *                  PURC_VARIANT_FALSE.
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "locale");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_setter (dynamic);
 *
 *              purc_variant_t param[3];
 *              param[0] = purc_variant_make_string ("ctype");
 *              param[1] = purc_variant_make_string ("zh_CN");
 *              param[2] = NULL;
 *              purc_variant_t var = func (root, 1, param);
 * @endcode
 *
 * @note        If input is "all", the function will set all items.
*/
static purc_variant_t
locale_setter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    size_t length = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                    (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) && 
                    (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
        
    const char* option = purc_variant_get_string_const (argv[0]);
    const char * head = pcdvobjs_get_next_option (option, " ", &length);

    while (head) {
        switch (* head)
        {
            case 'a':
            case 'A':
                if (strncasecmp (head, "all", length) == 0) {
                    if (setlocale (LC_ALL, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                else if (strncasecmp (head, "address", length) == 0) {
                    if (setlocale (LC_ADDRESS, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                break;

            case 'c':
            case 'C':
                if (strncasecmp (head, "ctype", length) == 0) {
                    if (setlocale (LC_CTYPE, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                else if (strncasecmp (head, "collate", length) == 0) {
                    if (setlocale (LC_COLLATE, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                break;

            case 'n':
            case 'N':
                if (strncasecmp (head, "numeric", length) == 0) {
                    if (setlocale (LC_NUMERIC, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                else if (strncasecmp (head, "name", length) == 0) {
                    if (setlocale (LC_NAME, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                break;

            case 't':
            case 'T':
                if (strncasecmp (head, "time", length) == 0) {
                    if (setlocale (LC_TIME, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                else if (strncasecmp (head, "telephone", length) == 0) {
                    if (setlocale (LC_TELEPHONE, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                break;

            case 'm':
            case 'M':
                if (strncasecmp (head, "monetary", length) == 0) {
                    if (setlocale (LC_MONETARY, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                else if (strncasecmp (head, "messages", length) == 0) {
                    if (setlocale (LC_MESSAGES, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                else if (strncasecmp (head, "measurement", length) == 0) {
                    if (setlocale (LC_MEASUREMENT, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                break;

            case 'p':
            case 'P':
                if (strncasecmp (head, "paper", length) == 0) {
                    if (setlocale (LC_PAPER, purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                    }
                }
                break;

            case 'i':
            case 'I':
                if (strncasecmp (head, "identification", length) == 0) {
                    if (setlocale (LC_IDENTIFICATION, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else {
                        ret_var = PURC_VARIANT_INVALID;
                        break;
                    }
                }
        }

        head = pcdvobjs_get_next_option (head + length + 1, " ", &length);
    }

    return ret_var;
}


/**
 * @brief       Get a random, the range is from 0 to user defined. 
 *
 * @param[in]   root    : The context variant
 * @param[in]   nr_args : The number of elements in argv array, it should be 1 
 * @param[in]   argv    : A varaint array
 *                        argv[0], a PURC_VARIANT_TYPE_NUMBER type, the maxium
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_WRONG_ARGS when input parameter is error.
 *              - A PURC_VARIANT_TYPE_NUMBER varint for the random.
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "random");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_getter (dynamic);
 *
 *              purc_variant_t param[2];
 *              param[0] = purc_variant_make_string (34.0d);
 *              param[1] = NULL;
 *              purc_variant_t var = func (root, 1, param);
 * @endcode
 *
 * @note        You can use MRAND_MAX for the maxium value the system supports.
*/
static purc_variant_t
random_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    double random = 0.0d;
    double number = 0.0d;

    if ((argv[0] != PURC_VARIANT_INVALID) 
                       && (!purc_variant_is_number (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_cast_to_number (argv[0], &number, false);

    if (fabs (number) < 1.0E-10) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    srand(time(NULL));      // todo initial
    random = number * rand() / (double)(RAND_MAX);

    return purc_variant_make_number (random); 
}


static purc_variant_t
get_time_format (int type, double epoch, const char *timezone)
{
    time_t t_time;
    struct tm *t_tm = NULL;
    char local_time[32] = {0};
    char *tz_now = getenv ("TZ");
    purc_variant_t ret_var = NULL;
    char str_format[32] = {0};

    if (type == FORMAT_ISO8601)
        sprintf (str_format, "%%FT%%T%%z");
    else if (type == FORMAT_RFC822)
        sprintf (str_format, "%%a, %%d %%b %%y %%T %%z");
    else
        sprintf (str_format, "%%FT%%T%%z");

    if (epoch == 0.0d) {
        if (timezone == NULL) {
            t_time = time (NULL);
            t_tm = localtime(&t_time);
            if (t_tm == NULL) {
                pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if(strftime(local_time, 32, str_format, t_tm) == 0) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }

            // create a string variant
            ret_var = purc_variant_make_string (local_time, false); 
            if(ret_var == PURC_VARIANT_INVALID) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
        }
        else {
            setenv ("TZ", timezone, 0);
            t_time = time (NULL);
            t_tm = localtime(&t_time);
            if (t_tm == NULL) {
                pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if(strftime(local_time, 32, str_format, t_tm) == 0) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }

            if (tz_now)
                setenv ("TZ", tz_now, 1);
            else
                unsetenv ("TZ");

            // create a string variant
            ret_var = purc_variant_make_string (local_time, false); 
            if(ret_var == PURC_VARIANT_INVALID) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
        }
    }
    else {
        if (timezone == NULL) {
            t_time = epoch;
            t_tm = localtime(&t_time);
            if (t_tm == NULL) {
                pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if(strftime(local_time, 32, str_format, t_tm) == 0) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }

            // create a string variant
            ret_var = purc_variant_make_string (local_time, false); 
            if(ret_var == PURC_VARIANT_INVALID) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
        }
        else {
            setenv ("TZ", timezone, 0);

            t_time = epoch;
            t_tm = localtime(&t_time);

            if (t_tm == NULL) {
                pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if(strftime(local_time, 32, str_format, t_tm) == 0) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }

            if (tz_now)
                setenv ("TZ", tz_now, 1);
            else
                unsetenv ("TZ");

            // create a string variant
            ret_var = purc_variant_make_string (local_time, false); 
            if(ret_var == PURC_VARIANT_INVALID) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
        }
    }

    return ret_var;
}


/**
 * @brief       Get time information. 
 *
 * @param[in]   root    : The context variant
 * @param[in]   nr_args : The number of elements in argv array, it should be 3 
 * @param[in]   argv    : A varaint array
 *                        argv[0], a PURC_VARIANT_TYPE_STRING type, time format
 *                            -"tm": get the time information as struct tm;
 *                            -"iso8601": get the time in iso8601 format;
 *                            -"RFC822": get the time in RFC822 format;
 *                            -format string: get the time with string user defined
 *                        argv[1], seconds since the Epoch, can be 
 *                            PURC_VARIANT_TYPE_NUMBER, 
 *                            PURC_VARIANT_TYPE_ULONGINT,
 *                            PURC_VARIANT_TYPE_LONGDOUBLE, 
 *                            PURC_VARIANT_TYPE_LONGINT type
 *                        argv[2], a PURC_VARIANT_TYPE_STRING type, timezone
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_WRONG_ARGS when input parameter is error.
 *              - A PURC_VARIANT_TYPE_STRING varint for the time information.
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "time");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_getter (dynamic);
 *
 *              // get the current time in iso8601 format
 *              purc_variant_t param[4];
 *              param[0] = purc_variant_make_string ("iso8601");
 *              param[1] = PURC_VARIANT_UNDEFINED;
 *              param[2] = PURC_VARIANT_UNDEFINED;
 *              param[3] = NULL;
 *              purc_variant_t var1 = func (root, 3, param);
 *
 *              // get time in Asia/Shanghai, and Epoch is 1234567,
 *              // and return string format is iso8601
 *              param[0] = purc_variant_make_string ("iso8601");
 *              param[1] = purc_variant_make_number (1234567);
 *              param[2] = purc_variant_make_string ("Asia/Shanghai");
 *              param[3] = NULL;
 *              purc_variant_t var2 = func (root, 3, param);
 *
 *              // get time in Asia/Shanghai, and Epoch is 1234567,
 *              // and return string is in user defined format
 *              param[0] = purc_variant_make_string ("The Shanghai time is %H:%m");
 *              param[1] = purc_variant_make_number (1234567);
 *              param[2] = purc_variant_make_string ("Asia/Shanghai");
 *              param[3] = NULL;
 *              purc_variant_t var3 = func (root, 3, param);
 * @endcode
 *
 * @note        If do not indicate seconds since the Epoch, or time zone, 
 *              argv[1] and argv[2] should be PURC_VARIANT_UNDEFINED.
 *              Support user define format, as below:
 *                  %Y: the year
 *                  %m: the month
 *                  %d: the day
 *                  %H: the hour
 *                  %M: the minute
 *                  %S: the second
*/
static purc_variant_t
time_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double epoch = 0.0d;
    const char *name = NULL;
    const char *timezone = NULL;
    time_t t_time;
    struct tm *t_tm = NULL;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                    (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    else
        name = purc_variant_get_string_const (argv[0]);

    if ((argv[1] != PURC_VARIANT_INVALID) && 
                            (!((purc_variant_is_ulongint (argv[1]))   || 
                               (purc_variant_is_longdouble (argv[1])) || 
                               (purc_variant_is_longint (argv[1]))    || 
                               (purc_variant_is_number (argv[1]))))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    else
        purc_variant_cast_to_number (argv[1], &epoch, false);
        
        
    if ((argv[2] != PURC_VARIANT_INVALID) 
                            && (!purc_variant_is_string (argv[2]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    else
        timezone = purc_variant_get_string_const (argv[2]);

    if (strcasecmp (name, "tm") == 0) {
        t_time = time (NULL);
        t_tm = localtime(&t_time);

        ret_var = purc_variant_make_object (0, PURC_VARIANT_INVALID, 
                PURC_VARIANT_INVALID);
        if(ret_var == PURC_VARIANT_INVALID) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            return PURC_VARIANT_INVALID;
        }

        purc_variant_object_set_c (ret_var, "sec",
                purc_variant_make_number (t_tm->tm_sec));
        purc_variant_object_set_c (ret_var, "min",
                purc_variant_make_number (t_tm->tm_min));
        purc_variant_object_set_c (ret_var, "hour",
                purc_variant_make_number (t_tm->tm_hour));
        purc_variant_object_set_c (ret_var, "mday",
                purc_variant_make_number (t_tm->tm_mday));
        purc_variant_object_set_c (ret_var, "mon",
                purc_variant_make_number (t_tm->tm_mon));
        purc_variant_object_set_c (ret_var, "year",
                purc_variant_make_number (t_tm->tm_year));
        purc_variant_object_set_c (ret_var, "wday",
                purc_variant_make_number (t_tm->tm_wday));
        purc_variant_object_set_c (ret_var, "yday",
                purc_variant_make_number (t_tm->tm_yday));
        purc_variant_object_set_c (ret_var, "isdst",
                purc_variant_make_number (t_tm->tm_isdst));
    }
    else if (strcasecmp (name, "iso8601") == 0) {
        get_time_format (FORMAT_ISO8601, epoch, timezone);
    }
    else if (strcasecmp (name, "rfc822") == 0) {
        get_time_format (FORMAT_RFC822, epoch, timezone);
    }
    else {
        /* replace 
           %Y: year
           %m: month
           %d: day
           %H: hour
           %m: minute
           %S: second
         */
        purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, 1024);
        char buffer[16];
        int start = 0;
        int i = 0;

        while (*(name + i) != 0x00) {
            if (*(name + i) == '%') {
                switch (*(name + i + 1)) {
                    case 0x00:
                        break;
                    case '%':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        purc_rwstream_write (rwstream, "%", 1);
                        i++;
                        start = i + 1;
                        break;
                    case 'Y':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_year);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'm':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_mon);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'd':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_mday);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'H':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_hour);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'M':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_min);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'S':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_sec);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                }
            }
            i++;
        }

        if (i != start)
            purc_rwstream_write (rwstream, name + start, strlen (name + start));

        size_t rw_size = 0;
        size_t content_size = 0;
        char * rw_string = purc_rwstream_get_mem_buffer_ex (rwstream, 
                                            &content_size, &rw_size, true);

        if ((rw_size == 0) || (rw_string == NULL))
            ret_var = PURC_VARIANT_INVALID;
        else {
            ret_var = purc_variant_make_string_reuse_buff (rw_string, 
                                                        rw_size, false);
            if(ret_var == PURC_VARIANT_INVALID) {
                pcinst_set_error (PURC_ERROR_INVALID_VALUE);
                ret_var = PURC_VARIANT_INVALID;
            }
        }

        purc_rwstream_destroy (rwstream);

    }

    return ret_var;
}


/**
 * @brief       Set time. 
 *
 * @param[in]   root    : The context variant
 * @param[in]   nr_args : The number of elements in argv array, it should be 1
 * @param[in]   argv    : A varaint array
 *                        argv[0], seconds since the Epoch, it should be 
 *                            PURC_VARIANT_TYPE_NUMBER type.
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_WRONG_ARGS when input parameter is error.
 *              - A PURC_VARIANT_TYPE_BOOLEAN varint.
 *                  PURC_VARIANT_TRUE for successful, otherwise 
 *                  PURC_VARIANT_FALSE.
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "time");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_setter (dynamic);
 *
 *              // get the current time in iso8601 format
 *              purc_variant_t param[2];
 *              param[0] = purc_variant_make_number (1234567);
 *              param[1] = NULL;
 *              purc_variant_t var = func (root, 1, param);
 * @endcode
 *
 * @note 
*/
static purc_variant_t
time_setter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    struct timeval stime;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                    (!purc_variant_is_number (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    double epoch = 0.0d;
    purc_variant_cast_to_number (argv[0], &epoch, false);

    gettimeofday (&stime, NULL);
    stime.tv_sec = epoch;
    if (settimeofday (&stime, NULL))
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;
}

// only for test now.
purc_variant_t pcdvojbs_get_system (void)
{
    purc_variant_t sys = purc_variant_make_object_c (6,
            "uname",        purc_variant_make_dynamic (uname_getter, NULL),
            "uname_prt",    purc_variant_make_dynamic (uname_prt_getter, NULL),
            "locale",       purc_variant_make_dynamic (locale_getter, locale_setter),
            "random",       purc_variant_make_dynamic (random_getter, NULL),
            "time",         purc_variant_make_dynamic (time_getter, time_setter)
       );
    return sys;
}

