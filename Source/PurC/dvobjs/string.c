/*
 * @file string.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of string dynamic variant object.
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
#include "dvobjs/string.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <locale.h>
#include <time.h>
#include <math.h>


static const char* get_next_segment (const char* data, const char* delim, 
                                                            size_t* length)
{
    const char* head = NULL;
    char* temp = NULL;

    *length = 0;

    if ((*data == 0x00) || (*delim == 0x00))
        return NULL;

    head = data;

    temp = strstr (head, delim);

    if (temp) {
        *length =  temp - head;
    }
    else {
        *length = strlen (head);
    }

    return head;
}

purc_variant_t
string_contains (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 2))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1])))
        return PURC_VARIANT_INVALID;

    const char* source = purc_variant_get_string_const (argv[0]);
    const char* sub = purc_variant_get_string_const (argv[1]);

    if (strstr (source, sub))
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}


purc_variant_t
string_ends_with (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 2))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1])))
        return PURC_VARIANT_INVALID;

    const char* source = purc_variant_get_string_const (argv[0]);
    size_t len_source = purc_variant_string_length (argv[0]) - 1;
    const char* sub = purc_variant_get_string_const (argv[1]);
    size_t len_sub = purc_variant_string_length (argv[1]) -1;

    size_t i = 0;
    bool find = true;

    if ((len_source == 0) || (len_sub == 0) || (len_source < len_sub)) {
        find = false;
    }
    else {
        for (i = 0; i < len_sub; i++)
        {
            if (*(source + len_source - len_sub + i) != *(sub + i)) {
                find = false;
                break;
            }
        }
    }

    if (find)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}


purc_variant_t
string_explode (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 2))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1])))
        return PURC_VARIANT_INVALID;

    const char* source = purc_variant_get_string_const (argv[0]);
    const char* delim = purc_variant_get_string_const (argv[1]);
    size_t len_delim = purc_variant_string_length (argv[1]) - 1;
    size_t length = 0;
    const char* head = get_next_segment (source, delim, &length);

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    while (head) {
        purc_variant_array_append (ret_var, 
                            purc_variant_make_string (head, true));
    
        if (*(head + length) != 0x00)
            head = get_next_segment (head + length + len_delim, 
                                                    delim, &length);
        else
            break;
    }

    return ret_var;
}


purc_variant_t
string_shuffle (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;

    size_t size = purc_variant_string_length (argv[0]);
    if (size < 2)       // it is an empty string
        return PURC_VARIANT_INVALID;

    char * src = malloc (size);
    strcpy (src, purc_variant_get_string_const (argv[0]));
    *(src + size - 1) = 0x00;

    srand(time(NULL));

    size_t i = 0;
    int random = 0;
    for(i =  0; i < size - 2; i++)
    {
        random = i + (rand () % (size - 1 - i));
        int tmp = *(src + random);
        *(src + random) = *(src + i);
        *(src + i) = tmp;
    }

    ret_var = purc_variant_make_string (src, false);
    
    free(src);

    return ret_var;
}


purc_variant_t
string_replace (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    double random = 0.0d;
    double number = 0.0d;

    if ((argv == NULL) || (nr_args != 1))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_number (argv[0])))
        return PURC_VARIANT_INVALID;

    purc_variant_cast_to_number (argv[0], &number, false);

    if (fabs (number) < 1.0E-10)
        return PURC_VARIANT_INVALID;

    srand(time(NULL));
    random = number * rand() / (double)(RAND_MAX);

    return purc_variant_make_number (random); 
}

purc_variant_t
string_format_c (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    char local_time[32] = {0};
    purc_variant_t ret_var = NULL;
    time_t t_time;
    struct tm *t_tm = NULL;

    t_time = time (NULL);
    t_tm = localtime(&t_time);
    if (t_tm == NULL)
        return PURC_VARIANT_INVALID;

    if(strftime(local_time, 32, "%FT%T%z", t_tm) == 0)
        return PURC_VARIANT_INVALID;

    // create a string variant
    ret_var = purc_variant_make_string (local_time, false); 
    if(ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    return ret_var;
}

purc_variant_t
string_format_p (purc_variant_t root, int nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

#if 0
    purc_variant_t ret_var = NULL;
    double epoch = 0.0d;
    const char* name = NULL;
    const char* locale = NULL;

    if ((argv == NULL) || (nr_args == 0))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
    else
        name = purc_variant_get_string_const (argv[0]);

    if ((argv[1] != NULL) && (!((purc_variant_is_ulongint (argv[1]))   || 
                              (purc_variant_is_longdouble (argv[1])) || 
                              (purc_variant_is_number (argv[1]))))) 
        return PURC_VARIANT_INVALID;
    else
        purc_variant_cast_to_number (argv[1], &epoch, false);
        
        
    if ((argv[2] != NULL) && (!purc_variant_is_string (argv[2])))
        return PURC_VARIANT_INVALID;
    else
        locale = purc_variant_get_string_const (argv[2]);


    return ret_var;
#endif
    return NULL;
}
