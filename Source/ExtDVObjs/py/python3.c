/*
 * @file python3.c
 * @author Vincent Wei
 * @date 2023/03/03
 * @brief The implementation of dynamic variant object $PY.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#undef NDEBUG

#include "config.h"
#include "private/map.h"
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "purc-errors.h"

#include <Python.h>
#include <assert.h>

#define PY_DVOBJ_VERSION    0

static struct dvobj_py_info {
    purc_variant_t entity;
    pcutils_map *prop_map;
    char *error;
} py_info;

static purc_nvariant_method property_getter(void* native_entity,
            const char* propert_name)
{
    struct dvobj_py_info *info = native_entity;
    assert(&py_info == info);

    pcutils_map_entry *entry = pcutils_map_find(info->prop_map, propert_name);
    if (entry)
        return (purc_nvariant_method)entry->val;

    return NULL;
}

static void on_release(void* native_entity)
{
    struct dvobj_py_info *info = native_entity;

    assert(Py_IsInitialized());
    assert(&py_info == native_entity);

    Py_Finalize();

    free(info->error);
    pcutils_map_destroy(info->prop_map);

    info->error = NULL;
    info->prop_map = NULL;
    info->entity = PURC_VARIANT_INVALID;
}

static struct purc_native_ops py_ops = {
    .property_getter = property_getter,
    .on_release = on_release,
};

#define INFO_VERSION    "version"
#define INFO_PLATFORM   "platform"
#define INFO_COPYRIGHT  "copyright"
#define INFO_COMPILER   "compiler"
#define INFO_BUILD_INFO "build-info"

purc_variant_t info_getter(void* native_entity,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    assert(Py_IsInitialized());
    assert(&py_info == native_entity);
    bool silently = call_flags & PCVRT_CALL_FLAG_SILENTLY;
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    if (nr_args > 0) {
        const char *which_str;
        size_t which_len;
        which_str = purc_variant_get_string_const_ex(argv[0], &which_len);
        if (which_str == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        which_str = pcutils_trim_spaces(which_str, &which_len);
        const char *result = NULL;
        if (which_len > 0) {
            switch (which_str[0]) {
                case 'v':
                    if (strcasecmp(which_str, INFO_VERSION) == 0) {
                        result = Py_GetVersion();
                    }
                    break;
                case 'p':
                    if (strcasecmp(which_str, INFO_PLATFORM) == 0) {
                        result = Py_GetPlatform();
                    }
                    break;
                case 'c':
                    if (strcasecmp(which_str, INFO_COPYRIGHT) == 0) {
                        result = Py_GetCopyright();
                    }
                    else if (strcasecmp(which_str, INFO_COMPILER) == 0) {
                        result = Py_GetCompiler();
                    }
                    break;
                case 'b':
                    if (strcasecmp(which_str, INFO_BUILD_INFO) == 0) {
                        result = Py_GetBuildInfo();
                    }
                    break;
                default:
                    break;
            }

            retv = purc_variant_make_string_static(result, false);
        }
    }

    if (retv == PURC_VARIANT_INVALID) {
        retv = purc_variant_make_object_0();
        if (retv == PURC_VARIANT_INVALID) {
            goto fatal;
        }

        val = purc_variant_make_string_static(Py_GetVersion(), false);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv, INFO_VERSION, val))
            goto fatal;
        purc_variant_unref(val);

        val = purc_variant_make_string_static(Py_GetPlatform(), false);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv, INFO_PLATFORM, val))
            goto fatal;
        purc_variant_unref(val);

        val = purc_variant_make_string_static(Py_GetCopyright(), false);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv, INFO_COPYRIGHT, val))
            goto fatal;
        purc_variant_unref(val);

        val = purc_variant_make_string_static(Py_GetCompiler(), false);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv, INFO_COMPILER, val))
            goto fatal;
        purc_variant_unref(val);

        val = purc_variant_make_string_static(Py_GetBuildInfo(), false);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv, INFO_BUILD_INFO, val))
            goto fatal;
        purc_variant_unref(val);
    }

    return retv;

fatal:
    silently = false;

failed:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    if (silently)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

purc_variant_t error_getter(void* native_entity,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    struct dvobj_py_info *info = native_entity;
    return purc_variant_make_string(info->error, false);
}

static purc_variant_t create_py(void)
{
    if (py_info.entity) {
        assert(Py_IsInitialized());
        assert(py_info.error != NULL);
        assert(py_info.prop_map != NULL);
    }
    else {
        Py_Initialize();

        py_info.prop_map = pcutils_map_create(
                copy_key_string, free_key_string,
                NULL, NULL, comp_key_string, true);
        pcutils_map_insert(py_info.prop_map, "info", info_getter);
        pcutils_map_insert(py_info.prop_map, "error", error_getter);
        py_info.error = strdup("Ok");
        py_info.entity = purc_variant_make_native(&py_info, &py_ops);
    }

    return py_info.entity;
}

static struct dvobj_info {
    const char *name;
    const char *desc;
    purc_variant_t (*create_func)(void);
} dvobjs[] = {
    {
        "PY",                                       // name
        "Implementaion of $PY based on CPython",    // description
        create_py                                   // create function
    },
};

purc_variant_t __purcex_load_dynamic_variant(const char *name, int *ver_code)
{
    size_t i = 0;
    for (i = 0; i < PCA_TABLESIZE(dvobjs); i++) {
        if (strcasecmp(name, dvobjs[i].name) == 0)
            break;
    }

    if (i == PCA_TABLESIZE(dvobjs))
        return PURC_VARIANT_INVALID;
    else {
        *ver_code = PY_DVOBJ_VERSION;
        return dvobjs[i].create_func();
    }
}

size_t __purcex_get_number_of_dynamic_variants(void)
{
    return PCA_TABLESIZE(dvobjs);
}

const char *__purcex_get_dynamic_variant_name(size_t idx)
{
    if (idx >= PCA_TABLESIZE(dvobjs))
        return NULL;
    else
        return dvobjs[idx].name;
}

const char *__purcex_get_dynamic_variant_desc(size_t idx)
{
    if (idx >= PCA_TABLESIZE(dvobjs))
        return NULL;
    else
        return dvobjs[idx].desc;
}

