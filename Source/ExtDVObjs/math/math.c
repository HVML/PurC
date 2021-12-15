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

#include "purc-variant.h"
#include "mathlib.h"
#include "purc-version.h"

#include <strings.h>

#ifndef __USE_GNU
#define __USE_GNU                       /* for M_PIl when using glibc */
#endif
#ifndef __MATH_LONG_DOUBLE_CONSTANTS
#define __MATH_LONG_DOUBLE_CONSTANTS    /* for M_PIl when using MacOSX SDK */
#endif
#include <math.h>
#include <fenv.h>

#define UNUSED_PARAM (void)
#define MATH_DVOBJ_VERSION  0
#define MATH_DESCRIPTION    "For MATH Operations in PURC"

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

    double number = 0.0;

    if (nr_args == 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    switch (*option) {
        case 'e':
        case 'E':
            if (strcasecmp (option, "e") == 0)
                number = M_E;
            else
                goto error;
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
            else
                goto error;
            break;
        case 'p':
        case 'P':
            if (strcasecmp (option, "pi") == 0)
                number = M_PI;
            else if (strcasecmp (option, "pi/2") == 0)
                number = M_PI_2;
            else if (strcasecmp (option, "pi/4") == 0)
                number = M_PI_4;
            else
                goto error;
            break;
        case '1':
            if (strcasecmp (option, "1/pi") == 0)
                number = M_1_PI;
            else if (strcasecmp (option, "1/sqrt(2)") == 0)
                number = M_SQRT1_2;
            else
                goto error;
            break;
        case '2':
            if (strcasecmp (option, "2/pi") == 0)
                number = M_2_PI;
            else if (strcasecmp (option, "2/sqrt(2)") == 0)
                number = M_2_SQRTPI;
            else
                goto error;
            break;
        case 's':
        case 'S':
            if (strcasecmp (option, "sqrt(2)") == 0)
                number = M_SQRT2;
            else
                goto error;
            break;
        default:
            goto error;
    }

    return purc_variant_make_number (number);

error:
    purc_set_error (PURC_ERROR_WRONG_ARGS);

    return PURC_VARIANT_INVALID;
}


static purc_variant_t
const_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    long double number = 0.0L;

    if (nr_args == 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    switch (*option) {
        case 'e':
        case 'E':
            if (strcasecmp (option, "e") == 0)
                number = (long double)M_El;
            else
                goto error;
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
            else
                goto error;
            break;
        case 'p':
        case 'P':
            if (strcasecmp (option, "pi") == 0)
                number = (long double)M_PIl;
            else if (strcasecmp (option, "pi/2") == 0)
                number = (long double)M_PI_2l;
            else if (strcasecmp (option, "pi/4") == 0)
                number = (long double)M_PI_4l;
            else
                goto error;
            break;
        case '1':
            if (strcasecmp (option, "1/pi") == 0)
                number = (long double)M_1_PIl;
            else if (strcasecmp (option, "1/sqrt(2)") == 0)
                number = (long double)M_SQRT1_2l;
            else
                goto error;
            break;
        case '2':
            if (strcasecmp (option, "2/pi") == 0)
                number = (long double)M_2_PIl;
            else if (strcasecmp (option, "2/sqrt(2)") == 0)
                number = (long double)M_2_SQRTPIl;
            else
                goto error;
            break;
        case 's':
        case 'S':
            if (strcasecmp (option, "sqrt(2)") == 0)
                number = (long double)M_SQRT2l;
            else
                goto error;
            break;
        default:
            goto error;
    }

    return purc_variant_make_longdouble (number);

error:
    purc_set_error (PURC_ERROR_WRONG_ARGS);

    return PURC_VARIANT_INVALID;
}

#define GET_EXCEPTION_DOUBLE(x) \
    if (isnan (x)) { \
        purc_set_error (PURC_ERROR_WRONG_ARGS); \
        ret_var = PURC_VARIANT_INVALID; \
    } \
    else if ((x) == HUGE_VAL) { \
        if (fetestexcept (FE_DIVBYZERO)) \
            purc_set_error (PURC_ERROR_DIVBYZERO); \
        else if (fetestexcept (FE_OVERFLOW)) \
            purc_set_error (PURC_ERROR_OVERFLOW); \
        else \
            purc_set_error (PURC_ERROR_UNKNOWN); \
        ret_var = PURC_VARIANT_INVALID; \
    } \
    else if (fetestexcept (FE_UNDERFLOW)) { \
        purc_set_error (PURC_ERROR_UNDERFLOW); \
        ret_var = PURC_VARIANT_INVALID; \
    } \
    else \
        ret_var = purc_variant_make_number (x);

#define GET_EXCEPTION_LONGDOUBLE(x) \
    if (isnan (x)) { \
        purc_set_error (PURC_ERROR_WRONG_ARGS); \
        ret_var = PURC_VARIANT_INVALID; \
    } \
    else if ((x) == HUGE_VALL) { \
        if (fetestexcept (FE_DIVBYZERO)) \
            purc_set_error (PURC_ERROR_DIVBYZERO); \
        else if (fetestexcept (FE_OVERFLOW)) \
            purc_set_error (PURC_ERROR_OVERFLOW); \
        else \
            purc_set_error (PURC_ERROR_UNKNOWN); \
        ret_var = PURC_VARIANT_INVALID; \
    } \
    else if (fetestexcept (FE_UNDERFLOW)) { \
        purc_set_error (PURC_ERROR_UNDERFLOW); \
        ret_var = PURC_VARIANT_INVALID; \
    } \
    else \
        ret_var = purc_variant_make_longdouble (x);

#define GET_VARIANT_NUMBER_TYPE(x) \
    if ((x == PURC_VARIANT_INVALID) || \
            !(purc_variant_is_type (x, PURC_VARIANT_TYPE_NUMBER)  || \
              purc_variant_is_type (x, PURC_VARIANT_TYPE_LONGINT) || \
              purc_variant_is_type (x, PURC_VARIANT_TYPE_ULONGINT) || \
              purc_variant_is_type (x, PURC_VARIANT_TYPE_LONGDOUBLE))) { \
        purc_set_error (PURC_ERROR_WRONG_ARGS); \
        return PURC_VARIANT_INVALID; \
    }

#define GET_PARAM_NUMBER(x) \
    if (nr_args < x) { \
        purc_set_error (PURC_ERROR_WRONG_ARGS); \
        return PURC_VARIANT_INVALID; \
    }


static purc_variant_t
sin_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = sin (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}


static purc_variant_t
cos_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = cos (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
tan_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = tan (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
asin_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = asin (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}


static purc_variant_t
acos_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = acos (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
atan_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = atan (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}


static purc_variant_t
sin_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}


static purc_variant_t
cos_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}


static purc_variant_t
tan_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
asin_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = asinl (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}


static purc_variant_t
acos_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = acosl (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}


static purc_variant_t
atan_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = atanl (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
sqrt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}


static purc_variant_t
sqrt_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
fmod_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number1 = 0.0;
    double number2 = 0.0;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    purc_variant_cast_to_number (argv[0], &number1, false);
    purc_variant_cast_to_number (argv[1], &number2, false);

    number1 = fmod (number1, number2);
    GET_EXCEPTION_DOUBLE(number1);

    return ret_var;
}

static purc_variant_t
fmod_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number1 = 0.0L;
    long double number2 = 0.0L;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    purc_variant_cast_to_long_double (argv[0], &number1, false);
    purc_variant_cast_to_long_double (argv[1], &number2, false);

    number1 = fmodl (number1, number2);
    GET_EXCEPTION_LONGDOUBLE(number1);

    return ret_var;
}

static purc_variant_t
fabs_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = fabs (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
fabs_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = fabsl (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
log_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = log (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
log_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = logl (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
log10_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = log10 (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
log10_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = log10l (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
pow_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number1 = 0.0;
    double number2 = 0.0;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    purc_variant_cast_to_number (argv[0], &number1, false);
    purc_variant_cast_to_number (argv[1], &number2, false);

    number1 = pow (number1, number2);
    GET_EXCEPTION_DOUBLE(number1);

    return ret_var;
}

static purc_variant_t
pow_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number1 = 0.0L;
    long double number2 = 0.0L;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    purc_variant_cast_to_long_double (argv[0], &number1, false);
    purc_variant_cast_to_long_double (argv[1], &number2, false);

    number1 = powl (number1, number2);
    GET_EXCEPTION_LONGDOUBLE(number1);

    return ret_var;
}

static purc_variant_t
exp_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = exp (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
exp_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = expl (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
floor_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = floor (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
floor_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = floorl (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
ceil_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = ceil (number);
    GET_EXCEPTION_DOUBLE(number);

    return ret_var;
}

static purc_variant_t
ceil_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_long_double (argv[0], &number, false);

    number = ceill (number);
    GET_EXCEPTION_LONGDOUBLE(number);

    return ret_var;
}

static purc_variant_t
internal_eval_getter (int is_long_double, purc_variant_t root,
    size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if (nr_args >= 2 && !purc_variant_is_object(argv[1])) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *input = purc_variant_get_string_const(argv[0]);
    if (!input) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t param = nr_args >=2 ? argv[1] : PURC_VARIANT_INVALID;

    if (!is_long_double) {
        double v = 0;
        int r = math_eval(input, &v, param);
        if (r) {
            purc_set_error (PURC_ERROR_UNKNOWN);
            return PURC_VARIANT_INVALID;
        }
        return purc_variant_make_number(v);
    }
    else {
        long double v = 0;
        int r = math_eval_l(input, &v, param);
        if (r) {
            purc_set_error (PURC_ERROR_UNKNOWN);
            return PURC_VARIANT_INVALID;
        }
        return purc_variant_make_longdouble(v);
    }
}

static purc_variant_t
eval_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    return internal_eval_getter(0, root, nr_args, argv);
}


static purc_variant_t
eval_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    return internal_eval_getter(1, root, nr_args, argv);
}

static purc_variant_t pcdvobjs_make_dvobjs (
        const struct pcdvobjs_dvobjs *method, size_t size)
{
    size_t i = 0;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t ret_var= purc_variant_make_object (0, PURC_VARIANT_INVALID,
                                                    PURC_VARIANT_INVALID);

    if (ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (i = 0; i < size; i++) {
        val = purc_variant_make_dynamic (method[i].getter, method[i].setter);
        if (val == PURC_VARIANT_INVALID) {
            goto error;
        }

        if (!purc_variant_object_set_by_static_ckey (ret_var,
                    method[i].name, val)) {
            goto error;
        }

        purc_variant_unref (val);
    }

    return ret_var;

error:
    purc_variant_unref (ret_var);

    return PURC_VARIANT_INVALID;
}


static purc_variant_t pcdvobjs_create_math (void)
{
    static struct pcdvobjs_dvobjs method [] = {
        {"pi",      pi_getter, NULL},
        {"pi_l",    pi_l_getter, NULL},
        {"e",       e_getter, NULL},
        {"e_l",     e_l_getter, NULL},
        {"const",   const_getter, NULL},
        {"const_l", const_l_getter, NULL},
        {"eval",    eval_getter, NULL},
        {"eval_l",  eval_l_getter, NULL},
        {"sin",     sin_getter, NULL},
        {"sin_l",   sin_l_getter, NULL},
        {"cos",     cos_getter, NULL},
        {"cos_l",   cos_l_getter, NULL},
        {"tan",     tan_getter, NULL},
        {"tan_l",   tan_l_getter, NULL},
        {"asin",    asin_getter, NULL},
        {"asin_l",  asin_l_getter, NULL},
        {"acos",    acos_getter, NULL},
        {"acos_l",  acos_l_getter, NULL},
        {"atan",    atan_getter, NULL},
        {"atan_l",  atan_l_getter, NULL},
        {"sqrt",    sqrt_getter, NULL},
        {"sqrt_l",  sqrt_l_getter, NULL},
        {"fmod",    fmod_getter, NULL},
        {"fmod_l",  fmod_l_getter, NULL},
        {"fabs",    fabs_getter, NULL},
        {"fabs_l",  fabs_l_getter, NULL},
        {"log",     log_getter, NULL},
        {"log_l",   log_l_getter, NULL},
        {"log10",   log10_getter, NULL},
        {"log10_l", log10_l_getter, NULL},
        {"pow",     pow_getter, NULL},
        {"pow_l",   pow_l_getter, NULL},
        {"exp",     exp_getter, NULL},
        {"exp_l",   exp_l_getter, NULL},
        {"floor",   floor_getter, NULL},
        {"floor_l", floor_l_getter, NULL},
        {"ceil",    ceil_getter, NULL},
        {"ceil_l",  ceil_l_getter, NULL},
    };

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}

purc_variant_t __purcex_load_dynamic_variant (const char *name, int *ver_code)
{
    UNUSED_PARAM(name);
    *ver_code = MATH_DVOBJ_VERSION;

    return pcdvobjs_create_math ();
}

size_t __purcex_get_number_of_dynamic_variants (void)
{
    return 1;
}

const char * __purcex_get_dynamic_variant_name (size_t idx)
{
    if (idx != 0)
        return NULL;

    return "MATH";
}

const char * __purcex_get_dynamic_variant_desc (size_t idx)
{
    if (idx != 0)
        return NULL;

    return MATH_DESCRIPTION;
}


/*
#undef div
#undef frexp
#undef ldexp
#undef modf

