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
#include "helper.h"

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

static purc_variant_t
uname_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    if (uname (&name) < 0) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    // create an empty object
    ret_var = purc_variant_make_object (0, 
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if(ret_var == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    val = purc_variant_make_string (name.sysname, true);
    if (val == PURC_VARIANT_INVALID)
        goto error;
    if (!purc_variant_object_set_by_static_ckey (ret_var, "kernel-name", val))
        goto error_unref;
    purc_variant_unref (val);

    val = purc_variant_make_string (name.nodename, true);
    if (val == PURC_VARIANT_INVALID)
        goto error;
    if (!purc_variant_object_set_by_static_ckey (ret_var, "nodename", val))
        goto error_unref;
    purc_variant_unref (val);

    val = purc_variant_make_string (name.release, true);
    if (val == PURC_VARIANT_INVALID)
        goto error;
    if (!purc_variant_object_set_by_static_ckey (ret_var, 
                "kernel-release", val))
        goto error_unref;
    purc_variant_unref (val);

    val = purc_variant_make_string (name.version, true);
    if (val == PURC_VARIANT_INVALID)
        goto error;
    if (!purc_variant_object_set_by_static_ckey (ret_var, 
                "kernel-version", val))
        goto error_unref;
    purc_variant_unref (val);

    val = purc_variant_make_string (name.machine, true);
    if (val == PURC_VARIANT_INVALID)
        goto error;
    if (!purc_variant_object_set_by_static_ckey (ret_var, "machine", val))
        goto error_unref;
    purc_variant_unref (val);

    val = purc_variant_make_string (name.machine, true);
    if (val == PURC_VARIANT_INVALID)
        goto error;
    if (!purc_variant_object_set_by_static_ckey (ret_var, "processor", val))
        goto error_unref;
    purc_variant_unref (val);

    val = purc_variant_make_string (name.machine, true);
    if (val == PURC_VARIANT_INVALID)
        goto error;
    if (!purc_variant_object_set_by_static_ckey (ret_var, 
                "hardware-platform", val))
        goto error_unref;
    purc_variant_unref (val);

    val = purc_variant_make_string (name.sysname, true);
    if (val == PURC_VARIANT_INVALID)
        goto error;
    if (!purc_variant_object_set_by_static_ckey (ret_var, 
                "operating-system", val))
        goto error_unref;
    purc_variant_unref (val);

    return ret_var;

error_unref:
    purc_variant_unref (val);
error:
    purc_variant_unref (ret_var);
    return PURC_VARIANT_INVALID;
}


static purc_variant_t
uname_prt_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    struct utsname name;
    size_t length = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    bool first = true;
    const char * delim = " ";
    purc_rwstream_t rwstream = NULL;
    size_t rw_size = 0;
    size_t content_size = 0;
    char * rw_string = NULL;
 
    if ((nr_args == 0) || 
        ((nr_args == 1) && (argv[0] != PURC_VARIANT_INVALID) && 
         (!purc_variant_is_string (argv[0])))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
        
    if (uname (&name) < 0) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);

    if (nr_args) {
        const char * option = purc_variant_get_string_const (argv[0]);
        const char * head = pcdvobjs_get_next_option (option, " ", &length);

        while (head) {
            switch (* head) {
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
                            purc_rwstream_write (rwstream, 
                                    delim, strlen (delim));

                        purc_rwstream_write (rwstream, 
                                name.sysname, strlen (name.sysname));
                    }
                    break;

                case 'h':
                case 'H':
                    if (strncasecmp (head, "hardware-platform", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, 
                                    delim, strlen (delim));

                        purc_rwstream_write (rwstream, 
                                name.machine, strlen (name.machine));
                    }
                    break;

                case 'p':
                case 'P':
                    if (strncasecmp (head, "processor", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, 
                                    delim, strlen (delim));

                        purc_rwstream_write (rwstream, 
                                name.machine, strlen (name.machine));
                    }
                    break;

                case 'm':
                case 'M':
                    if (strncasecmp (head, "machine", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, 
                                    delim, strlen (delim));

                        purc_rwstream_write (rwstream, 
                                name.machine, strlen (name.machine));
                    }
                    break;

                case 'n':
                case 'N':
                    if (strncasecmp (head, "nodename ", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, 
                                    delim, strlen (delim));

                        purc_rwstream_write (rwstream, 
                                name.nodename, strlen (name.nodename));
                    }
                    break;

                case 'k':
                case 'K':
                    if (strncasecmp (head, "kernel-name", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, 
                                    delim, strlen (delim));

                        purc_rwstream_write (rwstream, 
                                name.sysname, strlen (name.sysname));
                    } else if (strncasecmp (head, 
                                "kernel-release", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, 
                                    delim, strlen (delim));

                        purc_rwstream_write (rwstream, 
                                name.release, strlen (name.release));
                    } else if (strncasecmp (head, 
                                "kernel-version", length) == 0) {
                        if (first)
                            first = false;
                        else
                            purc_rwstream_write (rwstream, 
                                    delim, strlen (delim));

                        purc_rwstream_write (rwstream, name.version, 
                                strlen (name.version));
                    }
                    break;
            }

            head = pcdvobjs_get_next_option (head + length + 1, " ", &length);
        }
    } else {
        purc_rwstream_write (rwstream, name.sysname, strlen (name.sysname));
    }

    rw_string = purc_rwstream_get_mem_buffer_ex (rwstream, 
                                            &content_size, &rw_size, true);
    if ((content_size == 0) || (rw_string == NULL) || (rw_size == 0))  
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

    return ret_var;
}


static purc_variant_t
locale_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    size_t length = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((nr_args != 0) && (argv == NULL))  {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((nr_args == 1) && ((argv[0] != PURC_VARIANT_INVALID) && 
                (!purc_variant_is_string (argv[0])))) {
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
                    } else if (strncasecmp (head, "collate", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_COLLATE, NULL), true);
                    }
                    break;

                case 'n':
                case 'N':
                    if (strncasecmp (head, "numeric", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_NUMERIC, NULL), true);
                    } else if (strncasecmp (head, "name", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_NAME, NULL), true);
                    }
                    break;

                case 't':
                case 'T':
                    if (strncasecmp (head, "time", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_TIME, NULL), true);
                    } else if (strncasecmp (head, "telephone", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_TELEPHONE, NULL), true);
                    }
                    break;

                case 'm':
                case 'M':
                    if (strncasecmp (head, "monetary", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_MONETARY, NULL), true);
                    } else if (strncasecmp (head, "messages", length) == 0) {
                        ret_var = purc_variant_make_string (
                                setlocale (LC_MESSAGES, NULL), true);
                    } else if (strncasecmp (head, "measurement", length) == 0) {
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
    } else
        ret_var = purc_variant_make_string (
                setlocale (LC_MESSAGES, NULL), true);

    if (ret_var == PURC_VARIANT_INVALID)
    {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return ret_var;
}

static purc_variant_t
locale_setter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    size_t length = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args != 2) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

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
                    if (setlocale (LC_ALL, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                } else if (strncasecmp (head, "address", length) == 0) {
                    if (setlocale (LC_ADDRESS, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                }
                break;

            case 'c':
            case 'C':
                if (strncasecmp (head, "ctype", length) == 0) {
                    if (setlocale (LC_CTYPE, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                } else if (strncasecmp (head, "collate", length) == 0) {
                    if (setlocale (LC_COLLATE, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                }
                break;

            case 'n':
            case 'N':
                if (strncasecmp (head, "numeric", length) == 0) {
                    if (setlocale (LC_NUMERIC, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                } else if (strncasecmp (head, "name", length) == 0) {
                    if (setlocale (LC_NAME, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                }
                break;

            case 't':
            case 'T':
                if (strncasecmp (head, "time", length) == 0) {
                    if (setlocale (LC_TIME, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                } else if (strncasecmp (head, "telephone", length) == 0) {
                    if (setlocale (LC_TELEPHONE, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                }
                break;

            case 'm':
            case 'M':
                if (strncasecmp (head, "monetary", length) == 0) {
                    if (setlocale (LC_MONETARY, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                }
                else if (strncasecmp (head, "messages", length) == 0) {
                    if (setlocale (LC_MESSAGES, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                } else if (strncasecmp (head, "measurement", length) == 0) {
                    if (setlocale (LC_MEASUREMENT, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                }
                break;

            case 'p':
            case 'P':
                if (strncasecmp (head, "paper", length) == 0) {
                    if (setlocale (LC_PAPER, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                }
                break;

            case 'i':
            case 'I':
                if (strncasecmp (head, "identification", length) == 0) {
                    if (setlocale (LC_IDENTIFICATION, 
                                purc_variant_get_string_const (argv[1])))
                        ret_var = PURC_VARIANT_TRUE;
                    else 
                        ret_var = PURC_VARIANT_INVALID;
                }
                break;
        }

        head = pcdvobjs_get_next_option (head + length + 1, " ", &length);
    }

    return ret_var;
}


static purc_variant_t
random_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    double random = 0.0d;
    double number = 0.0d;

    if (nr_args == 0)  {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) && 
            (!purc_variant_is_number (argv[0]))) {
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
        } else {
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
    } else {
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
        } else {
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


static purc_variant_t
time_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    double epoch = 0.0d;
    const char *name = NULL;
    const char *timezone = NULL;
    time_t t_time;
    struct tm *t_tm = NULL;

    if ((nr_args >= 1) && (argv[0] != PURC_VARIANT_INVALID) && 
                    (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    } else
        name = purc_variant_get_string_const (argv[0]);

    if ((nr_args >= 2) && (argv[1] != PURC_VARIANT_INVALID) &&  
            (!((purc_variant_is_ulongint (argv[1]))   || 
               (purc_variant_is_longdouble (argv[1])) || 
               (purc_variant_is_longint (argv[1]))    || 
               (purc_variant_is_number (argv[1]))))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    } else
        purc_variant_cast_to_number (argv[1], &epoch, false);
        
        
    if ((nr_args >= 3) && (argv[2] != PURC_VARIANT_INVALID) && 
            (!purc_variant_is_string (argv[2]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    } else
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

        val = purc_variant_make_number (t_tm->tm_sec);
        purc_variant_object_set_by_static_ckey (ret_var, "sec", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_min);
        purc_variant_object_set_by_static_ckey (ret_var, "min", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_hour);
        purc_variant_object_set_by_static_ckey (ret_var, "hour", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_mday);
        purc_variant_object_set_by_static_ckey (ret_var, "mday", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_mon);
        purc_variant_object_set_by_static_ckey (ret_var, "mon", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_year);
        purc_variant_object_set_by_static_ckey (ret_var, "year", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_wday);
        purc_variant_object_set_by_static_ckey (ret_var, "wday", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_yday);
        purc_variant_object_set_by_static_ckey (ret_var, "yday", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_isdst);
        purc_variant_object_set_by_static_ckey (ret_var, "isdst", val);
        purc_variant_unref (val);
    } else if (strcasecmp (name, "iso8601") == 0) {
        get_time_format (FORMAT_ISO8601, epoch, timezone);
    } else if (strcasecmp (name, "rfc822") == 0) {
        get_time_format (FORMAT_RFC822, epoch, timezone);
    } else {
        /* replace 
           %Y: year
           %m: month
           %d: day
           %H: hour
           %m: minute
           %S: second
         */
        purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);
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
    static struct pcdvojbs_dvobjs method [] = {
        {"uname",     uname_getter,     NULL},
        {"uname_prt", uname_prt_getter, NULL},
        {"locale",    locale_getter,    locale_setter},
        {"random",    random_getter,    NULL},
        {"time",      time_getter,      time_setter} };

    size_t size = sizeof (method) / sizeof (struct pcdvojbs_dvobjs);
    return pcdvobjs_make_dvobjs (method, size);
}
