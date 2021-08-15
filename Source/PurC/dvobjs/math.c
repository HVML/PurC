/*
 * @file math.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of math dynamic variant object.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <math.h>


purc_variant_t
get_pi (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    if ((argv == NULL) && (nr_args != 0))
        return PURC_VARIANT_INVALID;

    if ((argv != NULL) && (!purc_variant_is_number (argv[0])))
        return PURC_VARIANT_INVALID;
        
    double number = 0.0d;

    if (nr_args == 0) 
        number = 6;    
    else {
        purc_variant_cast_to_number (argv[0], &number, false);
        if (number < 0)
            return PURC_VARIANT_INVALID;
    }

    double pi = M_PI;
    double multi = pow (10.0d, number);

    pi = (pi * multi + 0.5) / multi;

    return purc_variant_make_number (pi);
}


purc_variant_t
math_eval (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);


    return NULL;
}

purc_variant_t
math_sin (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    if ((argv == NULL) || (nr_args != 1))
        return PURC_VARIANT_INVALID;

    purc_variant_t ret_var = NULL;

    if (argv[0] != NULL) {
        if (purc_variant_is_number (argv[0])) {
            double number = 0.0d;
            purc_variant_cast_to_number (argv[0], &number, false);
            ret_var = purc_variant_make_number (sin (number));
        }
        else if (purc_variant_is_longint (argv[0])) {
            int64_t number = 0;
            purc_variant_cast_to_longint (argv[0], &number, false);
            ret_var = purc_variant_make_number (sin ((double)number));
        }
        else if (purc_variant_is_ulongint (argv[0])) {
            uint64_t number = 0;
            purc_variant_cast_to_ulongint (argv[0], &number, false);
            ret_var = purc_variant_make_number (sin ((double)number));
        }
        else if (purc_variant_is_longdouble (argv[0])) {
            long double number = 0;
            purc_variant_cast_to_long_double (argv[0], &number, false);
            ret_var = purc_variant_make_longdouble (sinl (number));
        }
        else
            ret_var = PURC_VARIANT_INVALID;
    }
    else
        ret_var = PURC_VARIANT_INVALID;

    return ret_var;
}

purc_variant_t
math_cos (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    if ((argv == NULL) || (nr_args != 1))
        return PURC_VARIANT_INVALID;

    purc_variant_t ret_var = NULL;

    if (argv[0] != NULL) {
        if (purc_variant_is_number (argv[0])) {
            double number = 0.0d;
            purc_variant_cast_to_number (argv[0], &number, false);
            ret_var = purc_variant_make_number (cos (number));
        }
        else if (purc_variant_is_longint (argv[0])) {
            int64_t number = 0;
            purc_variant_cast_to_longint (argv[0], &number, false);
            ret_var = purc_variant_make_number (cos ((double)number));
        }
        else if (purc_variant_is_ulongint (argv[0])) {
            uint64_t number = 0;
            purc_variant_cast_to_ulongint (argv[0], &number, false);
            ret_var = purc_variant_make_number (cos ((double)number));
        }
        else if (purc_variant_is_longdouble (argv[0])) {
            long double number = 0;
            purc_variant_cast_to_long_double (argv[0], &number, false);
            ret_var = purc_variant_make_longdouble (cosl (number));
        }
        else
            ret_var = PURC_VARIANT_INVALID;
    }
    else
        ret_var = PURC_VARIANT_INVALID;

    return ret_var;
}

purc_variant_t
math_sqrt (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    if ((argv == NULL) || (nr_args != 1))
        return PURC_VARIANT_INVALID;

    purc_variant_t ret_var = NULL;

    if (argv[0] != NULL) {
        if (purc_variant_is_number (argv[0])) {
            double number = 0.0d;
            purc_variant_cast_to_number (argv[0], &number, false);
            ret_var = purc_variant_make_number (sqrt (number));
        }
        else if (purc_variant_is_longint (argv[0])) {
            int64_t number = 0;
            purc_variant_cast_to_longint (argv[0], &number, false);
            ret_var = purc_variant_make_number (sqrt ((double)number));
        }
        else if (purc_variant_is_ulongint (argv[0])) {
            uint64_t number = 0;
            purc_variant_cast_to_ulongint (argv[0], &number, false);
            ret_var = purc_variant_make_number (sqrt ((double)number));
        }
        else if (purc_variant_is_longdouble (argv[0])) {
            long double number = 0;
            purc_variant_cast_to_long_double (argv[0], &number, false);
            ret_var = purc_variant_make_longdouble (sqrtl (number));
        }
        else
            ret_var = PURC_VARIANT_INVALID;
    }
    else
        ret_var = PURC_VARIANT_INVALID;

    return ret_var;
}
