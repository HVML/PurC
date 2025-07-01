/*
 * @file coroutine.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of CRTN dynamic variant object.
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
#include "private/vdom.h"
#include "private/dvobjs.h"
#include "private/url.h"
#include "private/regex.h"
#include "purc-variant.h"
#include "helper.h"

#include <limits.h>

#define DEFAULT_HVML_BASE           "file://"
#define DEFAULT_HVML_TARGET         "void"
#define DEFAULT_HVML_TIMEOUT        10.0
#define DVOBJ_HVML_DATA_NAME        "__handle_ctrl_props"

#define DEFAULT_HVML_TIMEOUT_SEC    (time_t)DEFAULT_HVML_TIMEOUT
#define DEFAULT_HVML_TIMEOUT_NSEC   (long)((DEFAULT_HVML_TIMEOUT -  \
            DEFAULT_HVML_TIMEOUT_SEC) * 1000000000)

static inline pcintr_coroutine_t
hvml_ctrl_coroutine(purc_variant_t root)
{
    purc_variant_t var;

    var = purc_variant_object_get_by_ckey_ex(root, DVOBJ_HVML_DATA_NAME, true);
    assert(var && purc_variant_is_native(var));

    return (pcintr_coroutine_t)purc_variant_native_get_entity(var);
}

static purc_variant_t
target_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_string_static(cor->target, false);
}

static purc_variant_t
base_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

#if 0 // VWNOTE: redundant check
    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_string_static(cor->base_url_string, false);
}

static purc_variant_t
base_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
#if 0 // VWNOTE: redundant check
    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    const char *url;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((url = purc_variant_get_string_const(argv[0])) == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    /*
     * If the url is invalid, cor->base_url_broken_down should not
     * be changed. If the url is valid, perhaps the string returned
     * by pcutils_url_assembly() is not identical to the input string.
     *
     * For example:
     *
     * The input string is `http://www.minigui.org`, but the the output string
     * of pcutils_url_assembly() is `http://www.minigui.org/`
     */
    if (pcutils_url_break_down(&(cor->base_url_broken_down), url)) {
        char *url = pcutils_url_assembly(&cor->base_url_broken_down, true);
        if (url) {
            free(cor->base_url_string);
            cor->base_url_string = url;
            return purc_variant_make_string_static(url, false);
        }
        else {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
max_iteration_count_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

#if 0 // VWNOTE: redundant check
    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_ulongint(cor->max_iteration_count);
}

static purc_variant_t
max_iteration_count_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(call_flags);

#if 0 // VWNOTE: redundant check
    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_ulongint (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    uint64_t u64;
    if (purc_variant_cast_to_ulongint(argv[0], &u64, false) && u64 > 0) {
        pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
        assert(cor);

        cor->max_iteration_count = u64;
        return purc_variant_make_ulongint(u64);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
max_recursion_depth_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

#if 0 // VWNOTE: redundant check
    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_ulongint(cor->max_recursion_depth);
}

static purc_variant_t
max_recursion_depth_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
#if 0 // VWNOTE: redundant check
    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_ulongint (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    uint64_t u64;
    if (purc_variant_cast_to_ulongint(argv[0], &u64, false) &&
            u64 > 0 && u64 <= USHRT_MAX) {
        pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
        assert(cor);

        cor->max_recursion_depth = u64;
        return purc_variant_make_ulongint(u64);
    }

    purc_set_error(PURC_ERROR_INVALID_VALUE);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
max_embedded_levels_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    return purc_variant_make_ulongint(cor->max_embedded_levels);
}

static purc_variant_t
max_embedded_levels_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    uint64_t u64;
    if (purc_variant_cast_to_ulongint(argv[0], &u64, false) &&
            u64 > 0 && u64 <= MAX_EMBEDDED_LEVELS) {
        pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
        assert(cor);

        cor->max_embedded_levels = u64;
        return purc_variant_make_ulongint(u64);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
timeout_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

#if 0 // VWNOTE: redundant check
    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    double number = (double)cor->timeout.tv_sec +
                (double)cor->timeout.tv_nsec / 1000000000.0;
    return purc_variant_make_number(number);
}

static purc_variant_t
timeout_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(call_flags);

#if 0 // VWNOTE: redundant check
    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_number (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
#endif

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    double number = 0.0;
    if (purc_variant_cast_to_number(argv[0], &number, false)) {
        pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
        assert(cor);

        if (number > 0.0) {
            cor->timeout.tv_sec = (long)number;
            cor->timeout.tv_nsec = (long)
                ((number - cor->timeout.tv_sec) * 1000000000);

            return purc_variant_make_number(number);
        }
    }

    purc_set_error(PURC_ERROR_INVALID_VALUE);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

#if 0 // Removed since 0.9.22
static purc_variant_t
sending_document_by_url_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);
    return purc_variant_make_boolean(cor->sending_document_by_url);
}

static purc_variant_t
sending_document_by_url_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    assert(cor);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (purc_variant_is_boolean(argv[0])) {
        if (purc_variant_is_true(argv[0])) {
            cor->sending_document_by_url = 1;
        }
        else {
            cor->sending_document_by_url = 0;
        }
    }
    else {
        pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    return purc_variant_make_boolean(cor->sending_document_by_url);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(cor->sending_document_by_url);

    return PURC_VARIANT_INVALID;
}
#endif

static purc_variant_t
cid_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    return purc_variant_make_ulongint(cor->cid);
}

static purc_variant_t
uri_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    const char *uri = pcintr_coroutine_get_uri(cor);
    return purc_variant_make_string(uri, false);
}

static purc_variant_t
token_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    return purc_variant_make_string(cor->token, false);
}

static purc_variant_t
token_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    const char *token;
    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((token = purc_variant_get_string_const(argv[0])) == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    if (0 != pcintr_coroutine_set_token(cor, token)) {
        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
curator_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    pcintr_coroutine_t cor = hvml_ctrl_coroutine(root);
    return purc_variant_make_ulongint(cor->curator);
}

static purc_variant_t static_variable_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    purc_variant_t at = PURC_VARIANT_INVALID;

    if (nr_args > 0) {
        at = argv[0];
        if (!purc_variant_is_string(at) && !purc_variant_is_ulongint(at)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    if (!pcintr_is_variable_token(property_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    pcintr_coroutine_t cor = (pcintr_coroutine_t)native_entity;
    pcintr_stack_t stack = &cor->stack;
    struct pcintr_stack_frame *frame = pcintr_stack_get_bottom_frame(stack);
    purc_variant_t ret = pcintr_get_named_variable(stack,
            frame, property_name, at, false, false);
    return ret ? purc_variant_ref(ret) : PURC_VARIANT_INVALID;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t static_variable_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    purc_variant_t at = PURC_VARIANT_INVALID;
    purc_variant_t val;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!pcintr_is_variable_token(property_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    val = argv[0];

    if (nr_args > 1) {
        at = argv[1];
        if (!purc_variant_is_string(at) && !purc_variant_is_ulongint(at)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }


    pcintr_coroutine_t cor = (pcintr_coroutine_t)native_entity;
    pcintr_stack_t stack = &cor->stack;
    struct pcintr_stack_frame *frame = pcintr_stack_get_bottom_frame(stack);
    int ret = pcintr_bind_named_variable(stack,
            frame, property_name, at, false, false, val);

    return purc_variant_make_boolean(ret == 0);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t static_self_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    if (property_name) {
        return static_variable_getter(native_entity, property_name,
                nr_args, argv, call_flags);
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t static_self_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    if (property_name) {
        return static_variable_setter(native_entity, property_name,
                nr_args, argv, call_flags);
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }
    return PURC_VARIANT_INVALID;
}


static purc_nvariant_method
static_property_getter(void* native_entity, const char* property_name)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);

    if (property_name) {
        return static_variable_getter;
    }
    return static_self_getter;
}

static purc_nvariant_method
static_property_setter(void* native_entity, const char* property_name)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);

    if (property_name) {
        return static_variable_setter;
    }
    return static_self_setter;
}

static struct purc_native_ops native_static_var_ops = {
    .property_getter = static_property_getter,
    .property_setter = static_property_setter,
};

static purc_variant_t temp_self_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t temp_self_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t temp_variable_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    purc_variant_t at = PURC_VARIANT_INVALID;

    if (nr_args > 0) {
        at = argv[0];
        if (!purc_variant_is_string(at) && !purc_variant_is_ulongint(at)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    if (!pcintr_is_variable_token(property_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    pcintr_coroutine_t cor = (pcintr_coroutine_t)native_entity;
    pcintr_stack_t stack = &cor->stack;
    struct pcintr_stack_frame *frame = pcintr_stack_get_bottom_frame(stack);
    purc_variant_t ret = pcintr_get_named_variable(stack,
            frame, property_name, at, true, false);
    return ret ? purc_variant_ref(ret) : PURC_VARIANT_INVALID;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t temp_variable_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    purc_variant_t at = PURC_VARIANT_INVALID;
    purc_variant_t val;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!pcintr_is_variable_token(property_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    val = argv[0];

    if (nr_args > 1) {
        at = argv[1];
        if (!purc_variant_is_string(at) && !purc_variant_is_ulongint(at)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    pcintr_coroutine_t cor = (pcintr_coroutine_t)native_entity;
    pcintr_stack_t stack = &cor->stack;
    struct pcintr_stack_frame *frame = pcintr_stack_get_bottom_frame(stack);
    int ret = pcintr_bind_named_variable(stack,
            frame, property_name, at, true, false, val);

    return purc_variant_make_boolean(ret == 0);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }
    return PURC_VARIANT_INVALID;
}

static purc_nvariant_method
temp_property_getter(void* native_entity, const char* property_name)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);

    if (property_name) {
        return temp_variable_getter;
    }
    return temp_self_getter;
}

static purc_nvariant_method
temp_property_setter(void* native_entity, const char* property_name)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);

    if (property_name) {
        return temp_variable_setter;
    }
    return temp_self_setter;
}

static struct purc_native_ops native_temp_var_ops = {
    .property_getter = temp_property_getter,
    .property_setter = temp_property_setter,
};

purc_variant_t
purc_dvobj_coroutine_new(pcintr_coroutine_t cor)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t static_val = PURC_VARIANT_INVALID;
    purc_variant_t temp_val = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    static const struct purc_dvobj_method method [] = {
        { "target", target_getter, NULL },
        { "base", base_getter, base_setter },
        { "max_iteration_count",    // TODO: remove in 0.9.24
            max_iteration_count_getter, max_iteration_count_setter },
        { "max_recursion_depth",    // TODO: remove in 0.9.24
            max_recursion_depth_getter, max_recursion_depth_setter },
        { "max_embedded_levels",    // TODO: remove in 0.9.24
            max_embedded_levels_getter, max_embedded_levels_setter },
        { "maxIterationCount",
            max_iteration_count_getter, max_iteration_count_setter },
        { "maxRecursionDepth",
            max_recursion_depth_getter, max_recursion_depth_setter },
        { "maxEmbeddedLevels",
            max_embedded_levels_getter, max_embedded_levels_setter },
        /* Removed since 0.9.22
        { "sendingDocumentByURL",   // 
            sending_document_by_url_getter, sending_document_by_url_setter }, */
        { "timeout", timeout_getter, timeout_setter },
        { "cid",     cid_getter,     NULL },
        { "uri",     uri_getter,     NULL },
        { "token",   token_getter,   token_setter },
        { "curator", curator_getter, NULL },
    };

    retv = purc_dvobj_make_from_methods(method, PCA_TABLESIZE(method));
    if (retv == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    static_val = purc_variant_make_native(cor, &native_static_var_ops);
    if (static_val == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    if (!purc_variant_object_set_by_static_ckey(retv, "static", static_val)) {
        goto failed;
    }

    temp_val = purc_variant_make_native(cor, &native_temp_var_ops);
    if (temp_val == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    if (!purc_variant_object_set_by_static_ckey(retv, "temp", temp_val)) {
        goto failed;
    }

    cor->target = strdup(DEFAULT_HVML_TARGET);
    if (cor->target == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    cor->base_url_string = strdup(DEFAULT_HVML_BASE);
    if (cor->base_url_string == NULL ||
            !pcutils_url_break_down(&cor->base_url_broken_down,
                DEFAULT_HVML_BASE)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    cor->max_iteration_count = UINT64_MAX;
    cor->max_recursion_depth = UINT16_MAX;
    cor->max_embedded_levels = DEF_EMBEDDED_LEVELS;
    cor->timeout.tv_sec = DEFAULT_HVML_TIMEOUT_SEC;
    cor->timeout.tv_nsec = DEFAULT_HVML_TIMEOUT_NSEC;

    val = purc_variant_make_native((void *)cor, NULL);
    if (val == PURC_VARIANT_INVALID) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!purc_variant_object_set_by_static_ckey(retv,
                DVOBJ_HVML_DATA_NAME, val)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    purc_variant_unref(static_val);
    purc_variant_unref(temp_val);
    purc_variant_unref(val);

    return retv;

failed:
    if (val)
        purc_variant_unref(val);
    if (temp_val)
        purc_variant_unref(temp_val);
    if (static_val)
        purc_variant_unref(static_val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}
