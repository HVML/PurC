/*
 * @file logical.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of logical dynamic variant object.
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
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "helper.h"

#include <math.h>
#include <regex.h>

static bool reg_cmp (const char *buf1, const char *buf2)
{
    regex_t reg;
    int err = 0;
    int number = 10;
    regmatch_t pmatch[number];

    if ((buf1 == NULL) || (buf2 == NULL))
        return false;

    if (regcomp (&reg, buf1, REG_EXTENDED) < 0) {
        goto error;
    }

    err = regexec(&reg, buf2, number, pmatch, 0);

    if (err == REG_NOMATCH) {
        goto error_free;
    }
    else if (err) {
        goto error_free;
    }

    if (pmatch[0].rm_so == -1) {
        goto error_free;
    }

    regfree (&reg);

    return true;

error_free:
    regfree (&reg);
error:
    return false;
}

static purc_variant_t
not_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (purc_variant_booleanize (argv[0]))
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;
}

static purc_variant_t
and_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    bool judge = true;
    size_t i = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    for (i = 0; i < nr_args; i++) {
        if ((argv[i] != PURC_VARIANT_INVALID) &&
                (!purc_variant_booleanize (argv[i]))) {
            judge = false;
            break;
        }
    }

    if (judge)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
or_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    bool judge = false;
    size_t i = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    for (i = 0; i < nr_args; i++) {
        if ((argv[i] != PURC_VARIANT_INVALID) &&
                (purc_variant_booleanize (argv[i]))) {
            judge = true;
            break;
        }
    }

    if (judge)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
xor_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    unsigned judge1 = 0x00;
    unsigned judge2 = 0x00;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (argv[1] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (purc_variant_booleanize (argv[0]))
        judge1 = 0x01;

    if (purc_variant_booleanize (argv[1]))
        judge2 = 0x01;

    judge1 ^= judge2;

    if (judge1)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
eq_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double value1 = 0.0L;
    double value2 = 0.0L;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    value1 = purc_variant_numberify (argv[0]);
    value2 = purc_variant_numberify (argv[1]);

    if (fabs (value1 - value2) < 1.0E-10)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
ne_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double value1 = 0.0L;
    double value2 = 0.0L;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    value1 = purc_variant_numberify (argv[0]);
    value2 = purc_variant_numberify (argv[1]);

    if (fabs (value1 - value2) >= 1.0E-10)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
gt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double value1 = 0.0L;
    double value2 = 0.0L;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    value1 = purc_variant_numberify (argv[0]);
    value2 = purc_variant_numberify (argv[1]);

    if (value1 > value2)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
ge_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double value1 = 0.0L;
    double value2 = 0.0L;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    value1 = purc_variant_numberify (argv[0]);
    value2 = purc_variant_numberify (argv[1]);

    if (value1 >= value2)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
lt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double value1 = 0.0L;
    double value2 = 0.0L;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    value1 = purc_variant_numberify (argv[0]);
    value2 = purc_variant_numberify (argv[1]);

    if (value1 < value2)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
le_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    double value1 = 0.0L;
    double value2 = 0.0L;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    value1 = purc_variant_numberify (argv[0]);
    value2 = purc_variant_numberify (argv[1]);

    if (value1 <= value2)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
streq_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;

    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0) {
        if (strcasecmp (buf1, buf2) == 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0) {
        if (strcmp (buf1, buf2) == 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_WILDCARD) == 0) {
        if (pcdvobjs_wildcard_cmp (buf2, buf1))
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_REG) == 0) {
        if (reg_cmp (buf1, buf2))
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
strne_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;

    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0) {
        if (strcasecmp (buf1, buf2) == 0)
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0) {
        if (strcmp (buf1, buf2) == 0)
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_WILDCARD) == 0) {
        if (pcdvobjs_wildcard_cmp (buf2, buf1))
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_REG) == 0) {
        if (reg_cmp (buf1, buf2))
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
strgt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;

    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0) {
        if (strcasecmp (buf1, buf2) > 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0) {
        if (strcmp (buf1, buf2) > 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
strge_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;


    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0) {
        if (strcasecmp (buf1, buf2) >= 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0) {
        if (strcmp (buf1, buf2) >= 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
strlt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;


    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0) {
        if (strcasecmp (buf1, buf2) < 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0) {
        if (strcmp (buf1, buf2) < 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
strle_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 3)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (argv[2] == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    purc_rwstream_t stream1 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    purc_rwstream_t stream2 = purc_rwstream_new_buffer (32, STREAM_SIZE);
    size_t sz_stream1 = 0;
    size_t sz_stream2 = 0;

    purc_variant_serialize (argv[1], stream1, 3, 0, &sz_stream1);
    purc_variant_serialize (argv[2], stream2, 3, 0, &sz_stream2);

    char *buf1 = purc_rwstream_get_mem_buffer (stream1, &sz_stream1);
    char *buf2 = purc_rwstream_get_mem_buffer (stream2, &sz_stream2);

    if (strcasecmp (option, STRING_COMP_MODE_CASELESS) == 0) {
        if (strcasecmp (buf1, buf2) <= 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else if (strcasecmp (option, STRING_COMP_MODE_CASE) == 0) {
        if (strcmp (buf1, buf2) <= 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }

    purc_rwstream_destroy (stream1);
    purc_rwstream_destroy (stream2);

    return ret_var;
}

static purc_variant_t
eval_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_object (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

#if 0
    size_t length = purc_variant_string_length (argv[0]);
    struct pcdvobjs_logical_param myparam = {0, argv[1]}; /* my instance data */
    yyscan_t lexer;                 /* flex instance data */

    if(logicallex_init_extra(&myparam, &lexer)) {
        return PURC_VARIANT_INVALID;
    }

    YY_BUFFER_STATE buffer = logical_scan_bytes (
                purc_variant_get_string_const (argv[0]), length, lexer);
    logical_switch_to_buffer (buffer, lexer);
    logicalparse(&myparam, lexer);
    logical_delete_buffer(buffer, lexer);
    logicallex_destroy (lexer);
#else // ! 0
    struct pcdvobjs_logical_param myparam = {
        0,
        argv[1],
        PURC_VARIANT_INVALID
    };
    pcdvobjs_logical_parse(purc_variant_get_string_const(argv[0]), &myparam);
#endif // 0

    if (myparam.result)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

// only for test now.
purc_variant_t pcdvobjs_get_logical (void)
{
    static struct pcdvobjs_dvobjs method [] = {
        {"not",   not_getter,   NULL},
        {"and",   and_getter,   NULL},
        {"or",    or_getter,    NULL},
        {"xor",   xor_getter,   NULL},
        {"eq",    eq_getter,    NULL},
        {"ne",    ne_getter,    NULL},
        {"gt",    gt_getter,    NULL},
        {"ge",    ge_getter,    NULL},
        {"lt",    lt_getter,    NULL},
        {"le",    le_getter,    NULL},
        {"streq", streq_getter, NULL},
        {"strne", strne_getter, NULL},
        {"strgt", strgt_getter, NULL},
        {"strge", strge_getter, NULL},
        {"strlt", strlt_getter, NULL},
        {"strle", strle_getter, NULL},
        {"eval",  eval_getter,  NULL}
    };

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}
