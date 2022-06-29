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

#include "purc-errors.h"
#include "purc-variant.h"
#include "purc-version.h"
#include "purc-dvobjs.h"

#include "private/map.h"
#include "mathlib.h"

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

/*                              error_number            exception
invalid param type and number   PURC_ERROR_ARGUMENT_MISSED   
output                          PURC_ERROR_DIVBYZERO    FloatingPoint
                                PURC_ERROR_OVERFLOW     Overflow
                                PURC_ERROR_UNDERFLOW    Underflow
                                PURC_ERROR_INVALID_FLOAT    FloatingPoint
*/

// map for const and const_l
// char* :: struct const_value*
static pcutils_map *const_map = NULL;

struct const_value {
    double d;
    long double ld;
};

struct const_struct {
    const char *key;
    struct const_value value;
};

#define GET_EXCEPTION_OR_CREATE_VARIANT(x, y) \
    if (isnan (x)) { \
        purc_set_error (PURC_ERROR_INVALID_FLOAT); \
        ret_var = PURC_VARIANT_INVALID; \
    } \
    else { \
        if (fetestexcept (FE_DIVBYZERO)) {\
            purc_set_error (PURC_ERROR_DIVBYZERO); \
            ret_var = PURC_VARIANT_INVALID; \
        } \
        else if (fetestexcept (FE_OVERFLOW)) { \
            purc_set_error (PURC_ERROR_OVERFLOW); \
            ret_var = PURC_VARIANT_INVALID; \
        } \
        else if (fetestexcept (FE_UNDERFLOW)) { \
            purc_set_error (PURC_ERROR_UNDERFLOW); \
            ret_var = PURC_VARIANT_INVALID; \
        } \
        else if (fetestexcept (FE_INVALID)) { \
            purc_set_error (PURC_ERROR_INVALID_FLOAT); \
            ret_var = PURC_VARIANT_INVALID; \
        } \
        else { \
            if (y) \
                ret_var = purc_variant_make_longdouble (x); \
            else \
                ret_var = purc_variant_make_number (x); \
        } \
    }

#define GET_EXCEPTION(x) \
    if (isnan (x)) { \
        purc_set_error (PURC_ERROR_INVALID_FLOAT); \
        return PURC_VARIANT_INVALID; \
    } \
    else { \
        if (fetestexcept (FE_DIVBYZERO)) {\
            purc_set_error (PURC_ERROR_DIVBYZERO); \
            return PURC_VARIANT_INVALID; \
        } \
        else if (fetestexcept (FE_OVERFLOW)) {\
            purc_set_error (PURC_ERROR_OVERFLOW); \
            return PURC_VARIANT_INVALID; \
        } \
        else if (fetestexcept (FE_UNDERFLOW)) { \
            purc_set_error (PURC_ERROR_UNDERFLOW); \
            return PURC_VARIANT_INVALID; \
        } \
        else if (fetestexcept (FE_INVALID)) { \
            purc_set_error (PURC_ERROR_INVALID_FLOAT); \
            return PURC_VARIANT_INVALID; \
        } \
    }

#define GET_VARIANT_NUMBER_TYPE(x) \
    if ((x == PURC_VARIANT_INVALID) || \
            !(purc_variant_is_type (x, PURC_VARIANT_TYPE_NUMBER)  || \
              purc_variant_is_type (x, PURC_VARIANT_TYPE_LONGINT) || \
              purc_variant_is_type (x, PURC_VARIANT_TYPE_ULONGINT) || \
              purc_variant_is_type (x, PURC_VARIANT_TYPE_LONGDOUBLE))) { \
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE); \
        return PURC_VARIANT_INVALID; \
    } \
    feclearexcept(FE_ALL_EXCEPT);

#define GET_PARAM_NUMBER(x) \
    if (nr_args < x) { \
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED); \
        return PURC_VARIANT_INVALID; \
    }

static purc_variant_t
pi_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (const_map) {
        pcutils_map_entry *entry = pcutils_map_find (const_map, "pi");
        if (entry)
            ret_var = purc_variant_make_number (
                    ((struct const_value *)entry->val)->d);
        else
            ret_var = purc_variant_make_number ((double)M_PI);
    }
    else
        ret_var = purc_variant_make_number ((double)M_PI);

    return ret_var;
}

static purc_variant_t
pi_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (const_map) {
        pcutils_map_entry *entry = pcutils_map_find (const_map, "pi");
        if (entry)
            ret_var = purc_variant_make_longdouble (
                    ((struct const_value *)entry->val)->ld);
        else
            ret_var = purc_variant_make_longdouble ((long double)M_PIl);
    }
    else
        ret_var = purc_variant_make_longdouble ((long double)M_PIl);

    return ret_var;
}

static purc_variant_t
e_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (const_map) {
        pcutils_map_entry *entry = pcutils_map_find (const_map, "e");
        if (entry)
            ret_var = purc_variant_make_number (
                    ((struct const_value *)entry->val)->d);
        else
            ret_var = purc_variant_make_number ((double)M_E);
    }
    else
        ret_var = purc_variant_make_number ((double)M_E);

    return ret_var;
}

static purc_variant_t
e_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (const_map) {
        pcutils_map_entry *entry = pcutils_map_find (const_map, "e");
        if (entry)
            ret_var = purc_variant_make_longdouble (
                    ((struct const_value *)entry->val)->ld);
        else
            ret_var = purc_variant_make_longdouble ((long double)M_El);
    }
    else
        ret_var = purc_variant_make_longdouble ((long double)M_El);

    return ret_var;
}

static purc_variant_t
const_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    GET_PARAM_NUMBER(1);

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    if (const_map) {
        pcutils_map_entry *entry = pcutils_map_find (const_map, option);
        if (entry)
            ret_var = purc_variant_make_number (
                    ((struct const_value *)entry->val)->d);
        else
            purc_set_error (PURC_ERROR_INVALID_VALUE);
    }
    else
        purc_set_error (PURC_ERROR_INVALID_VALUE);

    return ret_var;
}

static purc_variant_t
const_setter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    if (const_map == NULL) {
        purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    GET_PARAM_NUMBER(2);

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            !purc_variant_is_number (argv[1])) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if ((nr_args > 2) && (argv[2] == PURC_VARIANT_INVALID ||
            !purc_variant_is_longdouble (argv[2]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    // empty string
    if (purc_variant_string_size (argv[0]) < 2) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    double number = 0.0;
    long double ld = 0.0;
    purc_variant_cast_to_number (argv[1], &number, false);
    if (nr_args > 2)
        purc_variant_cast_to_longdouble (argv[2], &ld, false);
    else
        ld = (long double)number;

    // get the key
    const char *option = purc_variant_get_string_const (argv[0]);
    pcutils_map_entry *entry = pcutils_map_find (const_map, option);

    if (entry) {    // replace
        ((struct const_value *)(entry->val))->d = number;
        ((struct const_value *)(entry->val))->ld = ld;
    }
    else {          // insert
        // create key
        char *key = strdup(option);
        if (key == NULL) {
            purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        // create the entry
        struct const_value *val = malloc (sizeof(*val));
        if (val == NULL) {
            free (key);
            purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }
        val->d = number;
        val->ld = ld;

        if (pcutils_map_find_replace_or_insert (const_map, key, val, NULL)) {
            free(key);
            free(val);
        }
    }

    return purc_variant_make_boolean (true);;
}


static purc_variant_t
const_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    GET_PARAM_NUMBER(1);

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    if (const_map) {
        pcutils_map_entry *entry = pcutils_map_find (const_map, option);
        if (entry)
            ret_var = purc_variant_make_longdouble (
                    ((struct const_value *)entry->val)->ld);
        else
            purc_set_error (PURC_ERROR_INVALID_VALUE);
    }
    else
        purc_set_error (PURC_ERROR_INVALID_VALUE);

    return ret_var;
}

static purc_variant_t
add_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    int type = PURC_VARIANT_TYPE_NUMBER;
    long double number1 = 0.0;
    long double number2 = 0.0;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    if (nr_args > 2) {
        GET_VARIANT_NUMBER_TYPE (argv[2]);
        type = purc_variant_get_type (argv[2]);
    }

    purc_variant_cast_to_longdouble (argv[0], &number1, false);
    purc_variant_cast_to_longdouble (argv[1], &number2, false);
    number1 += number2;
    GET_EXCEPTION(number1);

    switch (type) {
        case PURC_VARIANT_TYPE_NUMBER:
            ret_var = purc_variant_make_number ((double)number1);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            ret_var = purc_variant_make_longint ((int64_t)number1);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            ret_var = purc_variant_make_ulongint ((uint64_t)number1);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            ret_var = purc_variant_make_longdouble (number1);
            break;
    }

    return ret_var;
}

static purc_variant_t
sub_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    int type = PURC_VARIANT_TYPE_NUMBER;
    long double number1 = 0.0;
    long double number2 = 0.0;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    if (nr_args > 2) {
        GET_VARIANT_NUMBER_TYPE (argv[2]);
        type = purc_variant_get_type (argv[2]);
    }

    purc_variant_cast_to_longdouble (argv[0], &number1, false);
    purc_variant_cast_to_longdouble (argv[1], &number2, false);
    number1 -= number2;
    GET_EXCEPTION(number1);

    switch (type) {
        case PURC_VARIANT_TYPE_NUMBER:
            ret_var = purc_variant_make_number ((double)number1);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            ret_var = purc_variant_make_longint ((int64_t)number1);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            ret_var = purc_variant_make_ulongint ((uint64_t)number1);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            ret_var = purc_variant_make_longdouble (number1);
            break;
    }

    return ret_var;
}

static purc_variant_t
mul_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    int type = PURC_VARIANT_TYPE_NUMBER;
    long double number1 = 0.0;
    long double number2 = 0.0;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    if (nr_args > 2) {
        GET_VARIANT_NUMBER_TYPE (argv[2]);
        type = purc_variant_get_type (argv[2]);
    }

    purc_variant_cast_to_longdouble (argv[0], &number1, false);
    purc_variant_cast_to_longdouble (argv[1], &number2, false);
    number1 *= number2;
    GET_EXCEPTION(number1);

    switch (type) {
        case PURC_VARIANT_TYPE_NUMBER:
            ret_var = purc_variant_make_number ((double)number1);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            ret_var = purc_variant_make_longint ((int64_t)number1);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            ret_var = purc_variant_make_ulongint ((uint64_t)number1);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            ret_var = purc_variant_make_longdouble (number1);
            break;
    }

    return ret_var;
}

static purc_variant_t
div_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    int type = PURC_VARIANT_TYPE_NUMBER;
    long double number1 = 0.0;
    long double number2 = 0.0;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    if (nr_args > 2) {
        GET_VARIANT_NUMBER_TYPE (argv[2]);
        type = purc_variant_get_type (argv[2]);
    }

    purc_variant_cast_to_longdouble (argv[0], &number1, false);
    purc_variant_cast_to_longdouble (argv[1], &number2, false);
    number1 /= number2;
    GET_EXCEPTION(number1);

    switch (type) {
        case PURC_VARIANT_TYPE_NUMBER:
            ret_var = purc_variant_make_number ((double)number1);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            ret_var = purc_variant_make_longint ((int64_t)number1);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            ret_var = purc_variant_make_ulongint ((uint64_t)number1);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            ret_var = purc_variant_make_longdouble (number1);
            break;
    }

    return ret_var;
}


static purc_variant_t
sin_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = sin (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}


static purc_variant_t
cos_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = cos (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
tan_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = tan (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
sinh_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = sinh (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}


static purc_variant_t
cosh_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = cosh (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
tanh_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = tanh (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
asin_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = asin (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}


static purc_variant_t
acos_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = acos (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
atan_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = atan (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}


static purc_variant_t
asinh_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = asinh (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}


static purc_variant_t
acosh_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = acosh (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
atanh_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = atanh (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}


static purc_variant_t
sin_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);
    number = sinl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}


static purc_variant_t
cos_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);
    number = cosl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}


static purc_variant_t
tan_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);
    number = tanl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
sinh_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);
    number = sinhl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}


static purc_variant_t
cosh_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);
    number = coshl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}


static purc_variant_t
tanh_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);
    number = tanhl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
asin_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = asinl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}


static purc_variant_t
acos_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = acosl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}


static purc_variant_t
atan_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = atanl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
asinh_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = asinhl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}


static purc_variant_t
acosh_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = acoshl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}


static purc_variant_t
atanh_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = atanhl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
sqrt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);
    number = sqrt (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}


static purc_variant_t
sqrt_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);
    number = sqrtl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
fmod_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number1 = 0.0;
    double number2 = 0.0;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    purc_variant_cast_to_number (argv[0], &number1, false);
    purc_variant_cast_to_number (argv[1], &number2, false);

    number1 = fmod (number1, number2);
    GET_EXCEPTION_OR_CREATE_VARIANT(number1, 0);

    return ret_var;
}

static purc_variant_t
fmod_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number1 = 0.0L;
    long double number2 = 0.0L;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    purc_variant_cast_to_longdouble (argv[0], &number1, false);
    purc_variant_cast_to_longdouble (argv[1], &number2, false);

    number1 = fmodl (number1, number2);
    GET_EXCEPTION_OR_CREATE_VARIANT(number1, 1);

    return ret_var;
}

static purc_variant_t
fabs_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    int type = purc_variant_get_type (argv[0]);

    double d = 0.0;
    long double ld = 0.0L;
    int64_t i64 = 0;
    uint64_t u64 = 0;

    switch (type) {
        case PURC_VARIANT_TYPE_NUMBER:
            purc_variant_cast_to_number (argv[0], &d, false);
            d = fabsl (d);
            GET_EXCEPTION(d);
            ret_var = purc_variant_make_number (d);
            break;
        case PURC_VARIANT_TYPE_LONGINT:
            purc_variant_cast_to_longint (argv[0], &i64, false);
            i64 = fabsl ((long double)i64);
            ret_var = purc_variant_make_longint (i64);
            break;
        case PURC_VARIANT_TYPE_ULONGINT:
            purc_variant_cast_to_ulongint (argv[0], &u64, false);
            ret_var = purc_variant_make_ulongint (u64);
            break;
        case PURC_VARIANT_TYPE_LONGDOUBLE:
            purc_variant_cast_to_longdouble (argv[0], &ld, false);
            ld = fabsl (ld);
            GET_EXCEPTION(ld);
            ret_var = purc_variant_make_longdouble (ld);
            break;
    }

    return ret_var;
}

static purc_variant_t
log_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = log (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
log_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = logl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
log10_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = log10 (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
log10_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = log10l (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
pow_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number1 = 0.0;
    double number2 = 0.0;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    purc_variant_cast_to_number (argv[0], &number1, false);
    purc_variant_cast_to_number (argv[1], &number2, false);

    number1 = pow (number1, number2);
    GET_EXCEPTION_OR_CREATE_VARIANT(number1, 0);

    return ret_var;
}

static purc_variant_t
pow_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number1 = 0.0L;
    long double number2 = 0.0L;

    GET_PARAM_NUMBER(2);
    GET_VARIANT_NUMBER_TYPE (argv[0]);
    GET_VARIANT_NUMBER_TYPE (argv[1]);

    purc_variant_cast_to_longdouble (argv[0], &number1, false);
    purc_variant_cast_to_longdouble (argv[1], &number2, false);

    number1 = powl (number1, number2);
    GET_EXCEPTION_OR_CREATE_VARIANT(number1, 1);

    return ret_var;
}

static purc_variant_t
exp_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = exp (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
exp_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = expl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
floor_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = floor (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
floor_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = floorl (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
ceil_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double number = 0.0;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_number (argv[0], &number, false);

    number = ceil (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 0);

    return ret_var;
}

static purc_variant_t
ceil_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    long double number = 0.0L;

    GET_PARAM_NUMBER(1);
    GET_VARIANT_NUMBER_TYPE (argv[0]);

    purc_variant_cast_to_longdouble (argv[0], &number, false);

    number = ceill (number);
    GET_EXCEPTION_OR_CREATE_VARIANT(number, 1);

    return ret_var;
}

static purc_variant_t
internal_eval_getter (int is_long_double, purc_variant_t root,
    size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (nr_args >= 2 && (argv[1] == PURC_VARIANT_INVALID ||
                !purc_variant_is_object(argv[1]))) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *input = purc_variant_get_string_const(argv[0]);
    if (!input) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t param = nr_args >=2 ? argv[1] : PURC_VARIANT_INVALID;

    if (!is_long_double) {
        double v = 0;
        int r = math_eval(input, &v, param);
        if (r)
            return PURC_VARIANT_INVALID;
        return purc_variant_make_number(v);
    }
    else {
        long double v = 0;
        int r = math_eval_l(input, &v, param);
        if (r)
            return PURC_VARIANT_INVALID;
        return purc_variant_make_longdouble(v);
    }
}

static purc_variant_t
eval_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    return internal_eval_getter(0, root, nr_args, argv, silently);
}


static purc_variant_t
eval_l_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    return internal_eval_getter(1, root, nr_args, argv, silently);
}

static void * map_copy_key(const void *key)
{
    return (void*)key;
}

static void map_free_key(void *key)
{
    free(key);
}

static void *map_copy_val(const void *val)
{
    return (void *)val;
}

static int map_comp_key(const void *key1, const void *key2)
{
    return strcmp ((const char*)key1, (const char*)key2);
}

static void map_free_val(void *val)
{
    struct const_value *value = (struct const_value*)val;
    free(value);
}

void __attribute__ ((destructor)) math_fini(void)
{
    if (const_map) {
        pcutils_map_destroy (const_map);
        const_map = NULL;
    }
}

// todo: release const_map
static purc_variant_t pcdvobjs_create_math (void)
{
    // set const map
    size_t i = 0;
    static struct const_struct const_key_value [] = {
        {"e",         {M_E,        M_El}},
        {"log2e",     {M_LOG2E,    M_LOG2El}},
        {"log10e",    {M_LOG10E,   M_LOG10El}},
        {"ln2",       {M_LN2,      M_LN2l}},
        {"ln10",      {M_LN10,     M_LN10l}},
        {"pi",        {M_PI,       M_PIl}},
        {"pi/2",      {M_PI_2,     M_PI_2l}},
        {"pi/4",      {M_PI_4,     M_PI_4l}},
        {"1/pi",      {M_1_PI,     M_1_PIl}},
        {"1/sqrt(2)", {M_SQRT1_2,  M_SQRT1_2l}},
        {"2/pi",      {M_2_PI,     M_2_PIl}},
        {"2/sqrt(2)", {M_2_SQRTPI, M_2_SQRTPIl}},
        {"sqrt(2)",   {M_SQRT2,    M_SQRT2l}},
    };

    if (const_map == NULL) {
        const_map = pcutils_map_create (map_copy_key, map_free_key,
                map_copy_val, map_free_val, map_comp_key, false);
        if (!const_map)
            return PURC_VARIANT_INVALID;

        bool ok = true;
        for (i = 0; i < PCA_TABLESIZE(const_key_value); i++) {
            ok = false;
            struct const_struct *p = const_key_value + i;

            char *k = strdup(p->key);
            if (!p)
                break;

            struct const_value *v;
            v = (struct const_value*)malloc(sizeof(*v));
            if (!v) {
                free(k);
                break;
            }
            *v = const_key_value[i].value;

            if (pcutils_map_find_replace_or_insert (const_map, k, v, NULL)) {
                free(k);
                free(v);
                break;
            }

            ok = true;
        }

        if (!ok) {
            pcutils_map_destroy (const_map);
            const_map = NULL;
            return PURC_VARIANT_INVALID;
        }
    }

    // set dynamic
    static struct purc_dvobj_method method [] = {
        {"pi",      pi_getter, NULL},
        {"pi_l",    pi_l_getter, NULL},
        {"e",       e_getter, NULL},
        {"e_l",     e_l_getter, NULL},
        {"const",   const_getter, const_setter},
        {"const_l", const_l_getter, NULL},
        {"eval",    eval_getter, NULL},
        {"eval_l",  eval_l_getter, NULL},
        {"sin",     sin_getter, NULL},
        {"sin_l",   sin_l_getter, NULL},
        {"cos",     cos_getter, NULL},
        {"cos_l",   cos_l_getter, NULL},
        {"tan",     tan_getter, NULL},
        {"sinh",    sinh_getter, NULL},
        {"sinh_l",  sinh_l_getter, NULL},
        {"cosh",    cosh_getter, NULL},
        {"cosh_l",  cosh_l_getter, NULL},
        {"tanh",    tanh_getter, NULL},
        {"tanh_l",  tanh_l_getter, NULL},
        {"tan_l",   tan_l_getter, NULL},
        {"asin",    asin_getter, NULL},
        {"asin_l",  asin_l_getter, NULL},
        {"acos",    acos_getter, NULL},
        {"acos_l",  acos_l_getter, NULL},
        {"atan",    atan_getter, NULL},
        {"atan_l",  atan_l_getter, NULL},
        {"asinh",   asinh_getter, NULL},
        {"asinh_l", asinh_l_getter, NULL},
        {"acosh",   acosh_getter, NULL},
        {"acosh_l", acosh_l_getter, NULL},
        {"atanh",   atanh_getter, NULL},
        {"atanh_l", atanh_l_getter, NULL},
        {"sqrt",    sqrt_getter, NULL},
        {"sqrt_l",  sqrt_l_getter, NULL},
        {"fmod",    fmod_getter, NULL},
        {"fmod_l",  fmod_l_getter, NULL},
        {"fabs",    fabs_getter, NULL},
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
        {"add",     add_getter, NULL},
        {"sub",     sub_getter, NULL},
        {"mul",     mul_getter, NULL},
        {"div",     div_getter, NULL},
    };

    return purc_dvobj_make_from_methods (method, PCA_TABLESIZE(method));
}

purc_variant_t __purcex_load_dynamic_variant (const char *name, int *ver_code)
{
    UNUSED_PARAM(name);
    *ver_code = MATH_DVOBJ_VERSION;

    if (pcutils_strcasecmp(name, "MATH") == 0)
        return pcdvobjs_create_math ();

    return PURC_VARIANT_INVALID;
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
*/
