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
#include "dvobjs/parser.h"
#include "dvobjs/system.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <locale.h>
#include <time.h>


// https://gitlab.fmsoft.cn/hvml/docs-undisclosed/blob/master/design/purc-architecture-zh.md#351-%E6%9E%84%E9%80%A0%E5%8A%A8%E6%80%81%E5%8F%98%E4%BD%93%E5%AF%B9%E8%B1%A1

// https://gitlab.fmsoft.cn/hvml/docs-undisclosed/blob/master/design/purc-architecture-zh.md#410-%E5%B8%B8%E7%94%A8%E5%8A%A8%E6%80%81%E5%8F%98%E4%BD%93%E5%AF%B9%E8%B1%A1

static const char* get_next_option (const char* data, const char* end,
                                          const char* delim, size_t* length)
{
    const char* head = NULL;
    int i = 1;

    UNUSED_PARAM(end);

    *length = 0;

    // get the first char which is not space
    while ((*data != ' ') && (*data != 0x00)) {
        data++;
    }

    if (*data == 0x00)
        head = NULL;
    else {
        char* temp = NULL;

        head = data;

        // find next space
        temp = strchr (head, delim[0]);

        if (temp) {
            i =  temp - head;
            while ((*(head + i - 1) != ' ') && (*(head + i - 1) != 0x00)) {
                i--;
            }
            *length = i; 
        }
        else {
            i = 1;
            while ((*(head + i) != ' ') && (*(head + i) != 0x00)) {
                i++;
            }
            
            *length = i;
        }
    }

    return head;
}

purc_variant_t
get_uname_all (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;
    purc_variant_t ret_var = NULL;

    if (uname (&name) < 0) {
        return PURC_VARIANT_INVALID;
    }

    // create an empty object
    ret_var = purc_variant_make_object (0, PURC_VARIANT_INVALID, 
            PURC_VARIANT_INVALID);
    if(ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

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


purc_variant_t
get_uname (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    struct utsname name;
    size_t length = 0;
    purc_variant_t ret_var = NULL;
    bool first = true;
    const char * delim = " ";

    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, 1024);

    if ((argv == NULL) && (nr_args != 0))
        return PURC_VARIANT_INVALID;

    if ((argv != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
        
    if (uname (&name) < 0) {
        return PURC_VARIANT_INVALID;
    }

    if (nr_args) {
        const char * option = purc_variant_get_string_const (argv[0]);
        const char * end = option + strlen (option);
        const char * head = get_next_option (option, end, " ", &length);

        while (head) {
            switch (* head)
            {
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

            head = get_next_option (head + length, end, " ", &length);
        }
    }
    else
        purc_rwstream_write (rwstream, name.sysname, strlen (name.sysname));

    size_t rw_size = 0;
    const char * rw_string = purc_rwstream_get_mem_buffer (rwstream, &rw_size);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        ret_var = purc_variant_make_string (rw_string, false); 
        if(ret_var == PURC_VARIANT_INVALID)
            ret_var = PURC_VARIANT_INVALID;
    }

    purc_rwstream_close (rwstream);
    purc_rwstream_destroy (rwstream);

    return ret_var;
}


purc_variant_t
get_locale (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    size_t length = 0;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) && (nr_args != 0))
        return PURC_VARIANT_INVALID;

    if ((argv != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
        
    if (nr_args) {
        const char* option = purc_variant_get_string_const (argv[0]);
        const char * end = option + strlen (option);
        const char * head = get_next_option (option, end, " ", &length);

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

            head = get_next_option (head + length, end, " ", &length);
        }
    }
    else
        ret_var = purc_variant_make_string (setlocale (LC_ALL, NULL), true);

    if (ret_var == NULL)
    {
        return PURC_VARIANT_INVALID;
    }

    return ret_var;
}


purc_variant_t
set_locale (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    size_t length = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 2))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1])))
        return PURC_VARIANT_INVALID;
        
    const char* option = purc_variant_get_string_const (argv[0]);
    const char * end = option + strlen (option);
    const char * head = get_next_option (option, end, " ", &length);

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

        head = get_next_option (head + length, end, " ", &length);
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
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_number (argv[0])))
        return PURC_VARIANT_INVALID;

    purc_variant_cast_to_number (argv[0], &number, false);

    if (abs (number) < 1.0E-10)
        return PURC_VARIANT_INVALID;

    srand(time(NULL));
    random = number * rand() / (double)(RAND_MAX);

    return purc_variant_make_number (random); 
}

purc_variant_t
get_time (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args == 0))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;

    if ((argv[1] != NULL) && (!((purc_variant_is_ulongint (argv[1]))   || 
                              (purc_variant_is_longdouble (argv[1])) || 
                              (purc_variant_is_number (argv[1]))))) 
        return PURC_VARIANT_INVALID;

        
    if ((argv[2] != NULL) && (!purc_variant_is_string (argv[2])))
        return PURC_VARIANT_INVALID;

    // create an empty object
    ret_var = purc_variant_make_string ("", false); 
    if(ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    return ret_var;
}

purc_variant_t
set_time (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_number (argv[0])))
        return PURC_VARIANT_INVALID;

    return ret_var;
}
