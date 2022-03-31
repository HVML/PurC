/*
 * @file url.c
 * @author Vincent Wei
 * @date 2022/03/31
 * @brief The implementation of URL dynamic variant object.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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
#include "private/dvobjs.h"

static purc_variant_t
encode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *url;
    size_t len;

    url = purc_variant_get_string_const_ex(argv[0], &len);
    if (url == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (len == 0) {
        return purc_variant_ref(argv[0]);
    }

    // TODO

failed:
    if (silently)
        return purc_variant_make_string_static("", false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
decode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *url;
    size_t len;

    url = purc_variant_get_string_const_ex(argv[0], &len);
    if (url == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (len == 0) {
        return purc_variant_ref(argv[0]);
    }

    // TODO

failed:
    if (silently)
        return purc_variant_make_string_static("", false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
rawencode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *url;
    size_t len;

    url = purc_variant_get_string_const_ex(argv[0], &len);
    if (url == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (len == 0) {
        return purc_variant_ref(argv[0]);
    }

    // TODO

failed:
    if (silently)
        return purc_variant_make_string_static("", false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
rawdecode_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *url;
    size_t len;

    url = purc_variant_get_string_const_ex(argv[0], &len);
    if (url == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (len == 0) {
        return purc_variant_ref(argv[0]);
    }

    // TODO

failed:
    if (silently)
        return purc_variant_make_string_static("", false);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_url_new(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;

    static struct purc_dvobj_method methods [] = {
        { "encode",     encode_getter,      NULL },
        { "decode",     decode_getter,      NULL },
        { "rawencode",  rawencode_getter,   NULL },
        { "rawdecode",  rawdecode_getter,   NULL },
    };

    retv = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    return retv;
}
