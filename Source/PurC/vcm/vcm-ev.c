/*
 * @file vcm-ev.c
 * @author XueShuming
 * @date 2021/09/02
 * @brief The impl of vcm expression variable
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "purc-rwstream.h"

#include "private/errors.h"
#include "private/stack.h"
#include "private/interpreter.h"
#include "private/utils.h"
#include "private/vcm.h"

#include "eval.h"

#define PCVCM_EV_WITHOUT_ARGS       "__pcvcm_ev_without_args"

// expression variable
struct pcvcm_ev {
    struct pcvcm_node *vcm;
    char *method_name;
    char *const_method_name;
    purc_variant_t values;                  // object: stringify(args) : value
    purc_variant_t last_value;
    bool release_vcm;
    bool constantly;
};

static purc_variant_t
eval_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    struct pcintr_stack *stack = pcintr_get_stack();
    if (!stack) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t args = PURC_VARIANT_INVALID;
    if (argv) {
        args = purc_variant_make_tuple(nr_args, argv);
        if (!args) {
            return PURC_VARIANT_INVALID;
        }
    }

    purc_variant_t result = pcvcm_eval_sub_expr(vcm_ev->vcm, stack, args,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));

    if (args) {
        purc_variant_unref(args);
    }
    return result;
}

static ssize_t
cb_calc_md5(void *ctxt, const void *buf, size_t count)
{
    pcutils_md5_ctxt *md5_ctxt = ctxt;
    pcutils_md5_hash(md5_ctxt, buf, count);
    return count;
}

static purc_variant_t
build_const_key(struct pcintr_stack *stack, purc_variant_t args)
{
    UNUSED_PARAM(stack);
    pcutils_md5_ctxt md5_ctxt;
    purc_variant_t ret = PURC_VARIANT_INVALID;
    purc_variant_t key = PURC_VARIANT_INVALID;
    purc_rwstream_t stream = purc_rwstream_new_for_dump(&md5_ctxt, cb_calc_md5);
    if (stream == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    key = args;
    ssize_t nr = purc_variant_tuple_get_size(args);
    if (nr == 1) {
        switch (args->type) {
            case PURC_VARIANT_TYPE_UNDEFINED:
            case PURC_VARIANT_TYPE_NULL:
            case PURC_VARIANT_TYPE_BOOLEAN:
            case PURC_VARIANT_TYPE_EXCEPTION:
            case PURC_VARIANT_TYPE_NUMBER:
            case PURC_VARIANT_TYPE_LONGINT:
            case PURC_VARIANT_TYPE_ULONGINT:
            case PURC_VARIANT_TYPE_LONGDOUBLE:
            case PURC_VARIANT_TYPE_ATOMSTRING:
                key = purc_variant_tuple_get(args, 0);
                break;

            case PURC_VARIANT_TYPE_STRING:
            case PURC_VARIANT_TYPE_BSEQUENCE:
            case PURC_VARIANT_TYPE_DYNAMIC:
            case PURC_VARIANT_TYPE_NATIVE:
            case PURC_VARIANT_TYPE_OBJECT:
            case PURC_VARIANT_TYPE_ARRAY:
            case PURC_VARIANT_TYPE_SET:
            case PURC_VARIANT_TYPE_TUPLE:
            default:
                key = args;
                break;
        }
    }

    pcutils_md5_begin(&md5_ctxt);
    if (purc_variant_stringify(stream, key,
            PCVRNT_STRINGIFY_OPT_BSEQUENCE_BAREBYTES, NULL) < 0) {
        goto out;
    }

    purc_rwstream_destroy(stream);

    unsigned char md5[PCUTILS_MD5_DIGEST_SIZE];
    pcutils_md5_end(&md5_ctxt, md5);


    /* add scope */
    struct pcintr_stack_frame *frame = pcintr_stack_get_bottom_frame(stack);
    if (frame && frame->pos) {
        uintptr_t p = (uintptr_t)frame->pos;
        pcutils_md5_hash(&md5_ctxt, (char*)p, sizeof(uintptr_t));
    }

    char hex[sizeof(md5) * 2 + 1] = {0};
    pcutils_bin2hex(md5, sizeof(md5), hex, true);
    ret  = purc_variant_make_string(hex, false);
out:
    return ret;
}

static purc_variant_t
eval_const_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    purc_variant_t ret = PURC_VARIANT_INVALID;
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    struct pcintr_stack *stack = pcintr_get_stack();
    if (!stack) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t key = PURC_VARIANT_INVALID;
    purc_variant_t args = PURC_VARIANT_INVALID;
    if (argv) {
        args = purc_variant_make_tuple(nr_args, argv);
        if (!args) {
            goto out;
        }
        key = build_const_key(stack, args);
    }
    else {
        key = purc_variant_make_string_static(PCVCM_EV_WITHOUT_ARGS, false);
    }

    if (!key) {
        goto out;
    }

    ret = purc_variant_object_get(vcm_ev->values, key);
    if (ret) {
        purc_variant_ref(ret);
        goto out;
    }

    /* clear not found */
    purc_clr_error();

    ret = pcvcm_eval_sub_expr(vcm_ev->vcm, stack, args,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));
    if (ret) {
        purc_variant_object_set(vcm_ev->values, key, ret);
    }

out:
    if (key) {
        purc_variant_unref(key);
    }
    if (args) {
        purc_variant_unref(args);
    }
    return ret;
}

static purc_variant_t
vcm_ev_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return purc_variant_make_boolean(true);
}


static purc_variant_t
last_value_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    return vcm_ev->last_value;
}

static purc_variant_t
last_value_setter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    if (nr_args == 0) {
        return PURC_VARIANT_INVALID;
    }

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    if (vcm_ev->last_value) {
        purc_variant_unref(vcm_ev->last_value);
    }
    vcm_ev->last_value = argv[0];
    if (vcm_ev->last_value) {
        purc_variant_ref(vcm_ev->last_value);
    }
    return vcm_ev->last_value;
}

static purc_variant_t
method_name_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    return purc_variant_make_string(vcm_ev->method_name, false);
}

static purc_variant_t
const_method_name_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    return purc_variant_make_string(vcm_ev->const_method_name, false);
}

static purc_variant_t
constantly_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    return purc_variant_make_boolean(vcm_ev->constantly);
}

static inline purc_nvariant_method
property_getter(void *native_entity, const char *key_name)
{
    UNUSED_PARAM(native_entity);
    if (key_name == NULL)
        goto failed;

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    if (strcmp(vcm_ev->method_name, key_name) == 0) {
        return eval_getter;
    }
    else if (strcmp(vcm_ev->const_method_name, key_name) == 0) {
        return vcm_ev->constantly ? eval_const_getter : NULL;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_VCM_EV) == 0) {
        return vcm_ev_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_LAST_VALUE) == 0) {
        return last_value_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_METHOD_NAME) == 0) {
        return method_name_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_CONST_METHOD_NAME) == 0) {
        return const_method_name_getter;
    }
    else if (strcmp(key_name, PCVCM_EV_PROPERTY_CONSTANTLY) == 0) {
        return constantly_getter;
    }

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static inline purc_nvariant_method
property_setter(void *native_entity, const char *key_name)
{
    UNUSED_PARAM(native_entity);
    if (key_name && strcmp(key_name, PCVCM_EV_PROPERTY_LAST_VALUE) == 0) {
        return last_value_setter;
    }

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

bool
on_observe(void *native_entity, const char *event_name,
        const char *event_subname)
{
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)native_entity;
    struct pcintr_stack *stack = pcintr_get_stack();
    if (!stack) {
        return false;
    }
    vcm_ev->last_value = pcvcm_eval(vcm_ev->vcm, stack, false);
    return (vcm_ev->last_value) ? true : false;
}

static void
on_release(void *native_entity)
{
    struct pcvcm_ev *vcm_variant = (struct pcvcm_ev*)native_entity;
    if (vcm_variant->release_vcm) {
        free(vcm_variant->vcm);
    }
    if (vcm_variant->method_name) {
        free(vcm_variant->method_name);
    }
    if (vcm_variant->const_method_name) {
        free(vcm_variant->const_method_name);
    }
    purc_variant_unref(vcm_variant->values);
    if (vcm_variant->last_value) {
        purc_variant_unref(vcm_variant->last_value);
    }
    free(vcm_variant);
}

purc_variant_t
pcvcm_to_expression_variable(struct pcvcm_node *vcm, const char *method_name,
        bool constantly, bool release_vcm)
{
    UNUSED_PARAM(method_name);
    purc_variant_t v = PURC_VARIANT_INVALID;

    static struct purc_native_ops ops = {
        .property_getter        = property_getter,
        .property_setter        = property_setter,
        .property_cleaner       = NULL,
        .property_eraser        = NULL,

        .updater                = NULL,
        .cleaner                = NULL,
        .eraser                 = NULL,

        .on_observe            = on_observe,
        .on_release            = on_release,
    };

    struct pcvcm_ev *vcm_ev = (struct pcvcm_ev*)calloc(1,
            sizeof(struct pcvcm_ev));
    if (!vcm_ev) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    if (method_name) {
        vcm_ev->method_name = strdup(method_name);
    }
    else {
        vcm_ev->method_name = strdup(PCVCM_EV_DEFAULT_METHOD_NAME);
    }

    if (!vcm_ev->method_name) {
        goto out_free_ev;
    }

    vcm_ev->values = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (!vcm_ev->values) {
        goto out_free_method_name;
    }

    size_t nr = strlen(vcm_ev->method_name) + strlen(PCVCM_EV_CONST_SUFFIX);
    vcm_ev->const_method_name = malloc(nr + 1);
    if (!vcm_ev->method_name) {
        goto out_unref_values;
    }

    sprintf(vcm_ev->const_method_name, "%s%s", vcm_ev->method_name,
            PCVCM_EV_CONST_SUFFIX);

    v = purc_variant_make_native(vcm_ev, &ops);
    if (v == PURC_VARIANT_INVALID) {
        goto out_free_const_method_name;
    }

    vcm_ev->vcm = vcm;
    vcm_ev->release_vcm = release_vcm;
    vcm_ev->constantly = constantly;

    return v;

out_free_const_method_name:
    free(vcm_ev->const_method_name);

out_unref_values:
    purc_variant_unref(vcm_ev->values);

out_free_method_name:
    free(vcm_ev->method_name);

out_free_ev:
    free(vcm_ev);

out:
    return v;
}

