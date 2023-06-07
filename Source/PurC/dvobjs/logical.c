/*
 * @file logical.c
 * @author Geng Yue, Vincent Wei
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
#include "private/utils.h"
#include "purc-variant.h"
#include "helper.h"

#include <math.h>
#include <regex.h>

static bool reg_cmp(const char *buf1, const char *buf2)
{
    regex_t reg;

    assert(buf1);
    assert(buf2);

    if (regcomp(&reg, buf1, REG_EXTENDED | REG_NOSUB) < 0) {
        goto error;
    }

    if (regexec(&reg, buf2, 0, NULL, 0) == REG_NOMATCH) {
        goto error_free;
    }

    regfree(&reg);
    return true;

error_free:
    regfree(&reg);

error:
    return false;
}

static purc_variant_t
not_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    bool result;

    if (nr_args < 1) {
        // treat as undefined
        result = true;
    }
    else {
        result = !purc_variant_booleanize(argv[0]);
    }

    return purc_variant_make_boolean(result);
}

static purc_variant_t
and_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    bool result;
    if (nr_args < 2) {
        // treat as undefined
        result = false;
    }
    else {
        result = true;
        for (size_t i = 0; i < nr_args; i++) {
            if (!purc_variant_booleanize(argv[i])) {
                result = false;
                break;
            }
        }
    }

    return purc_variant_make_boolean(result);
}

static purc_variant_t
or_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    bool result;
    if (nr_args == 0) {
        // treat as undefined
        result = false;
    }
    else if (nr_args == 1) {
        result = purc_variant_booleanize(argv[0]);
    }
    else {
        result = false;
        for (size_t i = 0; i < nr_args; i++) {
            if (purc_variant_booleanize(argv[i])) {
                result = true;
                break;
            }
        }
    }

    return purc_variant_make_boolean(result);
}

static purc_variant_t
xor_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    uint8_t judge1;
    uint8_t judge2;

    if (nr_args == 0) {
        judge1 = 0;
        judge2 = 0;
    }
    else if (nr_args == 1) {
        if (purc_variant_booleanize(argv[0]))
            judge1 = 1;
        else
            judge1 = 0;
        judge2 = 0;
    }
    else {
        if (purc_variant_booleanize (argv[0]))
            judge1 = 1;
        else
            judge1 = 0;

        if (purc_variant_booleanize (argv[1]))
            judge2 = 1;
        else
            judge2 = 0;
    }

    judge1 ^= judge2;

    return purc_variant_make_boolean(judge1 != 0);
}

static purc_variant_t
eq_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    double v1, v2;

    if (nr_args == 0) {
        v1 = 0;
        v2 = 0;
    }
    else if (nr_args == 1) {
        v1 = purc_variant_numerify(argv[0]);
        v2 = 0;
    }
    else {
        v1 = purc_variant_numerify(argv[0]);
        v2 = purc_variant_numerify(argv[1]);
    }

    return purc_variant_make_boolean(pcutils_equal_doubles(v1, v2));
}

static purc_variant_t
ne_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    double v1, v2;

    if (nr_args == 0) {
        v1 = 0;
        v2 = 0;
    }
    else if (nr_args == 1) {
        v1 = purc_variant_numerify(argv[0]);
        v2 = 0;
    }
    else {
        v1 = purc_variant_numerify(argv[0]);
        v2 = purc_variant_numerify(argv[1]);
    }

    return purc_variant_make_boolean(!pcutils_equal_doubles(v1, v2));
}

static purc_variant_t
gt_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    double v1, v2;

    if (nr_args == 0) {
        v1 = 0;
        v2 = 0;
    }
    else if (nr_args == 1) {
        v1 = purc_variant_numerify(argv[0]);
        v2 = 0;
    }
    else {
        v1 = purc_variant_numerify(argv[0]);
        v2 = purc_variant_numerify(argv[1]);
    }

    return purc_variant_make_boolean((v1 > v2));
}

static purc_variant_t
ge_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    double v1, v2;

    if (nr_args == 0) {
        v1 = 0;
        v2 = 0;
    }
    else if (nr_args == 1) {
        v1 = purc_variant_numerify(argv[0]);
        v2 = 0;
    }
    else {
        v1 = purc_variant_numerify(argv[0]);
        v2 = purc_variant_numerify(argv[1]);
    }

    return purc_variant_make_boolean((v1 >= v2));
}

static purc_variant_t
lt_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    double v1, v2;

    if (nr_args == 0) {
        v1 = 0;
        v2 = 0;
    }
    else if (nr_args == 1) {
        v1 = purc_variant_numerify(argv[0]);
        v2 = 0;
    }
    else {
        v1 = purc_variant_numerify(argv[0]);
        v2 = purc_variant_numerify(argv[1]);
    }

    return purc_variant_make_boolean((v1 < v2));
}

static purc_variant_t
le_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    double v1, v2;

    if (nr_args == 0) {
        v1 = 0;
        v2 = 0;
    }
    else if (nr_args == 1) {
        v1 = purc_variant_numerify(argv[0]);
        v2 = 0;
    }
    else {
        v1 = purc_variant_numerify(argv[0]);
        v2 = purc_variant_numerify(argv[1]);
    }

    return purc_variant_make_boolean((v1 <= v2));
}

static int strcmp_method(purc_variant_t arg)
{
    const char *option;
    size_t option_len;

    option = purc_variant_get_string_const_ex(arg, &option_len);
    if (option == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return -1;
    }

    option = pcutils_trim_spaces(option, &option_len);
    if (option_len == 0) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    int id = pcdvobjs_global_keyword_id(option, option_len);
    if (id == PURC_K_KW_caseless ||
            id == PURC_K_KW_case ||
            id == PURC_K_KW_wildcard ||
            id == PURC_K_KW_regexp)
        return id;

    pcinst_set_error(PURC_ERROR_INVALID_VALUE);
    return -1;
}

static int strcmp_case(purc_variant_t arg)
{
    const char *option;
    size_t option_len;

    option = purc_variant_get_string_const_ex(arg, &option_len);
    if (option == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return -1;
    }

    option = pcutils_trim_spaces(option, &option_len);
    if (option_len == 0) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    int id = pcdvobjs_global_keyword_id(option, option_len);
    if (id == PURC_K_KW_caseless || id == PURC_K_KW_case)
        return id;

    pcinst_set_error(PURC_ERROR_INVALID_VALUE);
    return -1;
}

static purc_variant_t
streq_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int method;
    if ((method = strcmp_method(argv[0])) < 0) {
        goto failed;
    }

    const char *str1, *str2;
    char *buff1 = NULL, *buff2 = NULL;

    str1 = purc_variant_get_string_const(argv[1]);
    if (str1 == NULL) {
        if (purc_variant_stringify_alloc(&buff1, argv[1]) < 0)
            goto fatal;
        str1 = buff1;
    }

    str2 = purc_variant_get_string_const(argv[2]);
    if (str2 == NULL) {
        if (purc_variant_stringify_alloc(&buff2, argv[2]) < 0) {
            if (buff1)
                free(buff1);
            goto fatal;
        }
        str2 = buff2;
    }

    bool result;
    switch (method) {
    case PURC_K_KW_case:
        result = (strcmp(str1, str2) == 0);
        break;
    case PURC_K_KW_caseless:
        result = (pcutils_strcasecmp(str1, str2) == 0);
        break;
    case PURC_K_KW_wildcard:
        result = pcdvobjs_wildcard_cmp(str2, str1);
        break;
    case PURC_K_KW_regexp:
        result = reg_cmp(str1, str2);
        break;
    default:
        assert(0);
        result = false;
        break;
    }

    if (buff1)
        free(buff1);
    if (buff2)
        free(buff2);

    return purc_variant_make_boolean(result);

failed:
    if ((call_flags & PCVRT_CALL_FLAG_SILENTLY))
        return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
strne_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int method;
    if ((method = strcmp_method(argv[0])) < 0) {
        goto failed;
    }

    const char *str1, *str2;
    char *buff1 = NULL, *buff2 = NULL;

    str1 = purc_variant_get_string_const(argv[1]);
    if (str1 == NULL) {
        if (purc_variant_stringify_alloc(&buff1, argv[1]) < 0)
            goto fatal;
        str1 = buff1;
    }

    str2 = purc_variant_get_string_const(argv[2]);
    if (str2 == NULL) {
        if (purc_variant_stringify_alloc(&buff2, argv[2]) < 0) {
            if (buff1)
                free(buff1);
            goto fatal;
        }
        str2 = buff2;
    }

    bool result;
    switch (method) {
    case PURC_K_KW_case:
        result = (strcmp(str1, str2) != 0);
        break;
    case PURC_K_KW_caseless:
        result = (pcutils_strcasecmp(str1, str2) != 0);
        break;
    case PURC_K_KW_wildcard:
        result = !pcdvobjs_wildcard_cmp(str2, str1);
        break;
    case PURC_K_KW_regexp:
        result = !reg_cmp(str1, str2);
        break;
    default:
        assert(0);
        result = false;
        break;
    }

    if (buff1)
        free(buff1);
    if (buff2)
        free(buff2);

    return purc_variant_make_boolean(result);

failed:
    if ((call_flags & PCVRT_CALL_FLAG_SILENTLY))
        return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
strgt_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int method;
    if ((method = strcmp_case(argv[0])) < 0) {
        goto failed;
    }

    const char *str1, *str2;
    char *buff1 = NULL, *buff2 = NULL;

    str1 = purc_variant_get_string_const(argv[1]);
    if (str1 == NULL) {
        if (purc_variant_stringify_alloc(&buff1, argv[1]) < 0)
            goto fatal;
        str1 = buff1;
    }

    str2 = purc_variant_get_string_const(argv[2]);
    if (str2 == NULL) {
        if (purc_variant_stringify_alloc(&buff2, argv[2]) < 0) {
            if (buff1)
                free(buff1);
            goto fatal;
        }
        str2 = buff2;
    }

    bool result;
    switch (method) {
    case PURC_K_KW_case:
        result = (strcmp(str1, str2) > 0);
        break;
    case PURC_K_KW_caseless:
        result = (pcutils_strcasecmp(str1, str2) > 0);
        break;
    default:
        assert(0);
        result = false;
        break;
    }

    if (buff1)
        free(buff1);
    if (buff2)
        free(buff2);

    return purc_variant_make_boolean(result);

failed:
    if ((call_flags & PCVRT_CALL_FLAG_SILENTLY))
        return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
strge_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int method;
    if ((method = strcmp_case(argv[0])) < 0) {
        goto failed;
    }

    const char *str1, *str2;
    char *buff1 = NULL, *buff2 = NULL;

    str1 = purc_variant_get_string_const(argv[1]);
    if (str1 == NULL) {
        if (purc_variant_stringify_alloc(&buff1, argv[1]) < 0)
            goto fatal;
        str1 = buff1;
    }

    str2 = purc_variant_get_string_const(argv[2]);
    if (str2 == NULL) {
        if (purc_variant_stringify_alloc(&buff2, argv[2]) < 0) {
            if (buff1)
                free(buff1);
            goto fatal;
        }
        str2 = buff2;
    }

    bool result;
    switch (method) {
    case PURC_K_KW_case:
        result = (strcmp(str1, str2) >= 0);
        break;
    case PURC_K_KW_caseless:
        result = (pcutils_strcasecmp(str1, str2) >= 0);
        break;
    default:
        assert(0);
        result = false;
        break;
    }

    if (buff1)
        free(buff1);
    if (buff2)
        free(buff2);

    return purc_variant_make_boolean(result);

failed:
    if ((call_flags & PCVRT_CALL_FLAG_SILENTLY))
        return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
strlt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int method;
    if ((method = strcmp_case(argv[0])) < 0) {
        goto failed;
    }

    const char *str1, *str2;
    char *buff1 = NULL, *buff2 = NULL;

    str1 = purc_variant_get_string_const(argv[1]);
    if (str1 == NULL) {
        if (purc_variant_stringify_alloc(&buff1, argv[1]) < 0)
            goto fatal;
        str1 = buff1;
    }

    str2 = purc_variant_get_string_const(argv[2]);
    if (str2 == NULL) {
        if (purc_variant_stringify_alloc(&buff2, argv[2]) < 0) {
            if (buff1)
                free(buff1);
            goto fatal;
        }
        str2 = buff2;
    }

    bool result;
    switch (method) {
    case PURC_K_KW_case:
        result = (strcmp(str1, str2) < 0);
        break;
    case PURC_K_KW_caseless:
        result = (pcutils_strcasecmp(str1, str2) < 0);
        break;
    default:
        assert(0);
        result = false;
        break;
    }

    if (buff1)
        free(buff1);
    if (buff2)
        free(buff2);

    return purc_variant_make_boolean(result);

failed:
    if ((call_flags & PCVRT_CALL_FLAG_SILENTLY))
        return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
strle_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int method;
    if ((method = strcmp_case(argv[0])) < 0) {
        goto failed;
    }

    const char *str1, *str2;
    char *buff1 = NULL, *buff2 = NULL;

    str1 = purc_variant_get_string_const(argv[1]);
    if (str1 == NULL) {
        if (purc_variant_stringify_alloc(&buff1, argv[1]) < 0)
            goto fatal;
        str1 = buff1;
    }

    str2 = purc_variant_get_string_const(argv[2]);
    if (str2 == NULL) {
        if (purc_variant_stringify_alloc(&buff2, argv[2]) < 0) {
            if (buff1)
                free(buff1);
            goto fatal;
        }
        str2 = buff2;
    }

    bool result;
    switch (method) {
    case PURC_K_KW_case:
        result = (strcmp(str1, str2) <= 0);
        break;
    case PURC_K_KW_caseless:
        result = (pcutils_strcasecmp(str1, str2) <= 0);
        break;
    default:
        assert(0);
        result = false;
        break;
    }

    if (buff1)
        free(buff1);
    if (buff2)
        free(buff2);

    return purc_variant_make_boolean(result);

failed:
    if ((call_flags & PCVRT_CALL_FLAG_SILENTLY))
        return purc_variant_make_undefined();

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
eval_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        pcinst_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *exp = purc_variant_get_string_const(argv[0]);
    if (exp == NULL) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args > 1 && !purc_variant_is_object(argv[1])) {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    struct pcdvobjs_logical_param myparam = {
        0,
        (nr_args > 1) ? argv[1] : PURC_VARIANT_INVALID,
        PURC_VARIANT_INVALID
    };
    pcdvobjs_logical_parse(exp, &myparam);

    return purc_variant_make_boolean(myparam.result);

failed:
    if ((call_flags & PCVRT_CALL_FLAG_SILENTLY))
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_logical_new(void)
{
    static struct purc_dvobj_method method [] = {
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

    return purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
}
