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
#include "tools.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>

#define __USE_GNU 
#include <math.h>


static purc_variant_t
pi_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return purc_variant_make_number ((double)M_PI);
}


static purc_variant_t
pi_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return purc_variant_make_longdouble ((long double)M_PIl);
}


static purc_variant_t
e_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return purc_variant_make_number ((double)M_E);
}


static purc_variant_t
e_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return purc_variant_make_longdouble ((long double)M_El);
}

static purc_variant_t
const_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    double number = 0.0d;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char * option = purc_variant_get_string_const (argv[0]);
    switch (*option) {
        case 'e':
        case 'E':
            if (strcasecmp (option, "e") == 0)
                number = M_E;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case 'l':
        case 'L':
            if (strcasecmp (option, "log2e") == 0)
                number = M_LOG2E;
            else if (strcasecmp (option, "log10e") == 0)
                number = M_LOG10E;
            else if (strcasecmp (option, "ln2") == 0)
                number = M_LN2;
            else if (strcasecmp (option, "ln10") == 0)
                number = M_LN10;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case 'p':
        case 'P':
            if (strcasecmp (option, "pi") == 0)
                number = M_PI;
            else if (strcasecmp (option, "pi/2") == 0)
                number = M_PI_2;
            else if (strcasecmp (option, "pi/4") == 0)
                number = M_PI_4;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case '1':
            if (strcasecmp (option, "1/pi") == 0)
                number = M_1_PI;
            else if (strcasecmp (option, "1/sqrt(2)") == 0)
                number = M_SQRT1_2;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case '2':
            if (strcasecmp (option, "2/pi") == 0)
                number = M_2_PI;
            else if (strcasecmp (option, "2/sqrt(2)") == 0)
                number = M_2_SQRTPI;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case 's':
        case 'S':
            if (strcasecmp (option, "sqrt(2)") == 0)
                number = M_SQRT2;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        default:
            pcinst_set_error (PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
    }

    return purc_variant_make_number (number);
}


static purc_variant_t
const_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    
    long double number = 0.0d;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char * option = purc_variant_get_string_const (argv[0]);
    switch (*option) {
        case 'e':
        case 'E':
            if (strcasecmp (option, "e") == 0)
                number = (long double)M_El;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case 'l':
        case 'L':
            if (strcasecmp (option, "log2e") == 0)
                number = (long double)M_LOG2El;
            else if (strcasecmp (option, "log10e") == 0)
                number = (long double)M_LOG10El;
            else if (strcasecmp (option, "ln2") == 0)
                number = (long double)M_LN2l;
            else if (strcasecmp (option, "ln10") == 0)
                number = (long double)M_LN10l;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case 'p':
        case 'P':
            if (strcasecmp (option, "pi") == 0)
                number = (long double)M_PIl;
            else if (strcasecmp (option, "pi/2") == 0)
                number = (long double)M_PI_2l;
            else if (strcasecmp (option, "pi/4") == 0)
                number = (long double)M_PI_4l;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case '1':
            if (strcasecmp (option, "1/pi") == 0)
                number = (long double)M_1_PIl;
            else if (strcasecmp (option, "1/sqrt(2)") == 0)
                number = (long double)M_SQRT1_2l;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case '2':
            if (strcasecmp (option, "2/pi") == 0)
                number = (long double)M_2_PIl;
            else if (strcasecmp (option, "2/sqrt(2)") == 0)
                number = (long double)M_2_SQRTPIl;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        case 's':
        case 'S':
            if (strcasecmp (option, "sqrt(2)") == 0)
                number = (long double)M_SQRT2l;
            else {
                pcinst_set_error (PURC_ERROR_WRONG_ARGS);
                return PURC_VARIANT_INVALID;
            }
            break;
        default:
            pcinst_set_error (PURC_ERROR_WRONG_ARGS);
            return PURC_VARIANT_INVALID;
    }

    return purc_variant_make_longdouble (number);

}


static purc_variant_t
sin_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0d;

    purc_variant_cast_to_number (argv[0], &number, false);
    ret_var = purc_variant_make_number (sin (number));

    return ret_var;
}


static purc_variant_t
cos_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0d;

    purc_variant_cast_to_number (argv[0], &number, false);
    ret_var = purc_variant_make_number (cos (number));

    return ret_var;
}


static purc_variant_t
sqrt_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0d;

    purc_variant_cast_to_number (argv[0], &number, false);
    ret_var = purc_variant_make_number (sqrt (number));

    return ret_var;
}


static purc_variant_t
sin_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0d;

    purc_variant_cast_to_long_double (argv[0], &number, false);
    ret_var = purc_variant_make_longdouble (sinl (number));

    return ret_var;
}


static purc_variant_t
cos_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0d;

    purc_variant_cast_to_long_double (argv[0], &number, false);
    ret_var = purc_variant_make_longdouble (cosl (number));

    return ret_var;
}


static purc_variant_t
sqrt_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0d;

    purc_variant_cast_to_long_double (argv[0], &number, false);
    ret_var = purc_variant_make_longdouble (sqrtl (number));

    return ret_var;
}


static purc_variant_t
eval_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    int result = 0;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_object (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

#if 0
    size_t length = purc_variant_string_length (argv[0]);
    struct pcdvobjs_math_param myparam = {0.0d, argv[1]};
    yyscan_t lexer;

    if (mathlex_init_extra (&myparam, &lexer)) {
        return PURC_VARIANT_INVALID;
    }

    YY_BUFFER_STATE buffer = math_scan_bytes (
                    purc_variant_get_string_const (argv[0]), length, lexer);
    math_switch_to_buffer (buffer, lexer);
    result = mathparse(&myparam, lexer);
    math_delete_buffer(buffer, lexer);
    mathlex_destroy (lexer);
#else // ! 0
    struct pcdvobjs_math_param myparam = {0.0d, argv[1]};
    result = math_parse(purc_variant_get_string_const(argv[0]), &myparam);
#endif // 0

    if (result != 0) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    return purc_variant_make_number (myparam.result);
}


static purc_variant_t
eval_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    int result = 0;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_object (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

#if 0
    size_t length = purc_variant_string_length (argv[0]);
    struct pcdvobjs_mathld_param myparam = {0.0d, argv[1]};
    yyscan_t lexer;

    if (mathldlex_init_extra (&myparam, &lexer)) {
        return PURC_VARIANT_INVALID;
    }

    YY_BUFFER_STATE buffer = mathld_scan_bytes (
                    purc_variant_get_string_const (argv[0]), length, lexer);
    mathld_switch_to_buffer (buffer, lexer);
    result = mathldparse(&myparam, lexer);
    mathld_delete_buffer(buffer, lexer);
    mathldlex_destroy (lexer);
#else // ! 0
    struct pcdvobjs_mathld_param myparam = {0.0d, argv[1]};
    result = mathld_parse(purc_variant_get_string_const(argv[0]), &myparam);
#endif // 0

    if (result != 0) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    return purc_variant_make_longdouble (myparam.result);
}


// only for test now.
purc_variant_t pcdvojbs_get_math (void)
{
    purc_variant_t math = purc_variant_make_object_c (14,
            "pi",       purc_variant_make_dynamic (pi_getter, NULL),
            "pi_l",     purc_variant_make_dynamic (pi_l_getter, NULL),
            "e",        purc_variant_make_dynamic (e_getter, NULL),
            "e_l",      purc_variant_make_dynamic (e_l_getter, NULL),
            "const",    purc_variant_make_dynamic (const_getter, NULL),
            "const_l",  purc_variant_make_dynamic (const_l_getter, NULL),
            "eval",     purc_variant_make_dynamic (eval_getter, NULL),
            "eval_l",   purc_variant_make_dynamic (eval_l_getter, NULL),
            "sin",      purc_variant_make_dynamic (sin_getter, NULL),
            "sin_l",    purc_variant_make_dynamic (sin_l_getter, NULL),
            "cos",      purc_variant_make_dynamic (cos_getter, NULL),
            "cos_l",    purc_variant_make_dynamic (cos_l_getter, NULL),
            "sqrt",     purc_variant_make_dynamic (sqrt_getter, NULL),
            "sqrt_l",   purc_variant_make_dynamic (sqrt_l_getter, NULL)
       );
    return math;
}

