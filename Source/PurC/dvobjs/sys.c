/*
 * @file dvobjs.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of public part for html parser.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"
#include "dvobjs/parser.h"
#include "dvobjs/sys.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <locale.h>
#include <time.h>

static const char * get_next_option (const char * data, int * length)
{
    int size = strlen (data);
    const char * head = NULL;
    int i = 0;

    * length = 0;

    // get the first char which is not space
    for (i = 0; i < size; i++)
        if (*(data + i) != ' ')
            break;

    if (i == size)        // do not find any option 
        head = NULL;
    else {
        char * temp = NULL;

        head = data + i;

        // find next space
        temp = strchr (head, ' ');

        if (temp)
            * length = temp - head; 
        else
            * length = size - i;
    }

    return head;
}


purc_variant_t
get_uname (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    struct utsname name;
    int length = 0;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) && (nr_args != 0))
        return PURC_VARIANT_FALSE;

    if ((argv != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_FALSE;
        
    if (uname (&name) < 0) {
        return PURC_VARIANT_FALSE;
    }

    // create an empty object
    ret_var = purc_variant_make_object (0, PURC_VARIANT_INVALID, 
            PURC_VARIANT_INVALID);
    if(ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_FALSE;

    if (nr_args) {
        const char* option = purc_variant_get_string_const (argv[0]);
        const char * head = get_next_option (option, &length);

        while (head) {
            if (strncmp (head, "all", length) == 0) {
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
                break;
            }
            else if (strncmp (head, "default", length) == 0) {
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
                break;
            }
            else if (strncmp (head, "kernel-name", length) == 0) {
                purc_variant_object_set_c (ret_var, "kernel-name",
                        purc_variant_make_string (name.sysname, true));
            }
            else if (strncmp (head, "kernel-release", length) == 0) {
                purc_variant_object_set_c (ret_var, "kernel-release",
                        purc_variant_make_string (name.release, true));
            }
            else if (strncmp (head, "kernel-version", length) == 0) {
                purc_variant_object_set_c (ret_var, "kernel-version",
                        purc_variant_make_string (name.version, true));
            }
            else if (strncmp (head, "nodename ", length) == 0) {
                purc_variant_object_set_c (ret_var, "nodename",
                        purc_variant_make_string (name.nodename, true));
            }
            else if (strncmp (head, "machine", length) == 0) {
                purc_variant_object_set_c (ret_var, "machine",
                        purc_variant_make_string (name.machine, true));
            }
            else if (strncmp (head, "processor", length) == 0) {
                purc_variant_object_set_c (ret_var, "processor",
                        purc_variant_make_string (name.machine, true));
            }
            else if (strncmp (head, "hardware-platform", length) == 0) {
                purc_variant_object_set_c (ret_var, "hardware-platform",
                        purc_variant_make_string (name.machine, true));
            }
            else if (strncmp (head, "operating-system", length) == 0) {
                purc_variant_object_set_c (ret_var, "operating-system",
                        purc_variant_make_string (name.sysname, true));
            }

            head = get_next_option (head + length, &length);
        }
    }
    else
        purc_variant_object_set_c (ret_var, "kernel-name",
                purc_variant_make_string (name.sysname, true));

    if (purc_variant_object_get_size (ret_var) == 0)
    {
        purc_variant_unref (ret_var);
        return PURC_VARIANT_FALSE;
    }

    return ret_var;
}


purc_variant_t
get_locale (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    int length = 0;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) && (nr_args != 0))
        return PURC_VARIANT_FALSE;

    if ((argv != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_FALSE;
        
    if (nr_args) {
        const char* option = purc_variant_get_string_const (argv[0]);
        const char * head = get_next_option (option, &length);

        while (head) {
            if (strncmp (head, "ctype", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_CTYPE, NULL), true);
                break;
            }
            else if (strncmp (head, "numeric", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_NUMERIC, NULL), true);
                break;
            }
            else if (strncmp (head, "time", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_TIME, NULL), true);
                break;
            }
            else if (strncmp (head, "collate", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_COLLATE, NULL), true);
                break;
            }
            else if (strncmp (head, "monetary", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_MONETARY, NULL), true);
                break;
            }
            else if (strncmp (head, "messages", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_MESSAGES, NULL), true);
                break;
            }
            else if (strncmp (head, "paper", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_PAPER, NULL), true);
                break;
            }
            else if (strncmp (head, "name", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_NAME, NULL), true);
                break;
            }
            else if (strncmp (head, "address", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_ADDRESS, NULL), true);
                break;
            }
            else if (strncmp (head, "telephone", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_TELEPHONE, NULL), true);
                break;
            }
            else if (strncmp (head, "measurement", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_MEASUREMENT, NULL), true);
                break;
            }
            else if (strncmp (head, "identification", length) == 0) {
                ret_var = purc_variant_make_string (
                                setlocale (LC_IDENTIFICATION, NULL), true);
                break;
            }

            head = get_next_option (head + length, &length);
        }
    }
    else
        ret_var = purc_variant_make_string (
                setlocale (LC_ALL, NULL), true);

    if (ret_var == NULL)
    {
        return PURC_VARIANT_FALSE;
    }

    return ret_var;
}


purc_variant_t
set_locale (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    int length = 0;
    purc_variant_t ret_var = PURC_VARIANT_FALSE;

    if ((argv == NULL) || (nr_args != 2))
        return PURC_VARIANT_FALSE;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_FALSE;
    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1])))
        return PURC_VARIANT_FALSE;
        
    const char* option = purc_variant_get_string_const (argv[0]);
    const char * head = get_next_option (option, &length);

    while (head) {
        if (strncmp (head, "all", length) == 0) {
            if (setlocale (LC_ALL, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            break;
        }
        else if (strncmp (head, "ctype", length) == 0) {
            if (setlocale (LC_CTYPE, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "numeric", length) == 0) {
            if (setlocale (LC_NUMERIC, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "time", length) == 0) {
            if (setlocale (LC_TIME, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "collate", length) == 0) {
            if (setlocale (LC_COLLATE, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "monetary", length) == 0) {
            if (setlocale (LC_MONETARY, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "messages", length) == 0) {
            if (setlocale (LC_MESSAGES, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "paper", length) == 0) {
            if (setlocale (LC_PAPER, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "name", length) == 0) {
            if (setlocale (LC_NAME, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "address", length) == 0) {
            if (setlocale (LC_ADDRESS, purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "telephone", length) == 0) {
            if (setlocale (LC_TELEPHONE, 
                        purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "measurement", length) == 0) {
            if (setlocale (LC_MEASUREMENT, 
                        purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }
        else if (strncmp (head, "identification", length) == 0) {
            if (setlocale (LC_IDENTIFICATION, 
                        purc_variant_get_string_const (argv[1])))
                ret_var = PURC_VARIANT_TRUE;
            else {
                ret_var = PURC_VARIANT_FALSE;
                break;
            }
        }

        head = get_next_option (head + length, &length);
    }

    return ret_var;
}


purc_variant_t
get_random (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    double random = 0.0d;
    double number = 0.0d;

    if ((argv == NULL) || (nr_args != 1))
        return PURC_VARIANT_FALSE;

    if ((argv[0] != NULL) && (!purc_variant_is_number (argv[0])))
        return PURC_VARIANT_FALSE;

    number = purc_variant_get_number (argv[0]);

    if (abs (number) < 1.0E-10)
        return PURC_VARIANT_FALSE;

    srand(time(NULL));
    random = number * rand() / (double)(RAND_MAX);

    return purc_variant_make_number (random); 

        
}

purc_variant_t
get_time (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;

    return purc_variant_make_string (name.sysname, true);
}

purc_variant_t
set_time (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;

    return purc_variant_make_string (name.sysname, true);
}
