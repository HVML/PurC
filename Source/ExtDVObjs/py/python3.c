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
#include "private/instance.h"
#include "purc-variant.h"
#include "purc-errors.h"

#include <Python.h>
#include <assert.h>

#define PY_DVOBJ_VERNAME        "0.1.0"
#define PY_DVOBJ_VERCODE        0

#define STR(x)                  #x
#define STR2(x)                 STR(x)
#define PY_DVOBJ_VERCODE_STR    STR2(PY_DVOBJ_VERCODE)

#define PY_INFO_VERSION     "version"
#define PY_INFO_PLATFORM    "platform"
#define PY_INFO_COPYRIGHT   "copyright"
#define PY_INFO_COMPILER    "compiler"
#define PY_INFO_BUILD_INFO  "build-info"

#define PY_HANDLE_NAME      "__handle_python__"
#define PY_ATTR_HVML        "__hvml__"

struct dvobj_pyinfo {
    pcutils_map *prop_map;
};

static inline struct dvobj_pyinfo *
get_pyinfo(purc_variant_t root)
{
    purc_variant_t v;

    v = purc_variant_object_get_by_ckey(root, PY_HANDLE_NAME);
    assert(v && purc_variant_is_native(v));

    return (struct dvobj_pyinfo *)purc_variant_native_get_entity(v);
}

#if 0
static purc_variant_t call_pyfunc(void* native_entity,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    return purc_variant_make_undefined();
}

static purc_nvariant_method property_getter(void* native_entity,
            const char* property_name)
{
    struct dvobj_pyinfo *pyinfo = native_entity;
    assert(&pyinfo == pyinfo);

    pcutils_map_entry *entry = pcutils_map_find(pyinfo->prop_map, property_name);
    if (entry) {
        PyObject *o = (PyObject *)entry->val;
        if (PyFunction_Check(o)) {
            return call_pyfunc;
        }
        else if (PyModule_Check(o)) {
            /* We use a Python Capsule to store the variant created for
               the module */
            PyObject *cap = PyObject_GetAttrString(o, PY_ATTR_HVML);
            if (PyCapsule_CheckExact(cap)) {
                purc_variant_t mod_entity;
                mod_entity = PyCapsule_GetPointer(p, NULL);
                assert(mod_entity);

                return 
            }
        }
    }

    return NULL;
}
#endif

static purc_variant_t run_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    return purc_variant_make_undefined();
}

static purc_variant_t import_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    return purc_variant_make_undefined();
}

static purc_variant_t make_impl_object(void)
{
    static const char *kvs[] = {
        "vendor",
        "HVML Community",
        "author",
        "Vincent Wei",
        "verName",
        PY_DVOBJ_VERNAME,
        "verCode",
        PY_DVOBJ_VERCODE_STR,
        "license",
        "LGPLv3+",
        "url",
        "https://hvml.fmsoft.cn",
        "repo",
        "https://github.com/HVML",
    };

    purc_variant_t retv, val = PURC_VARIANT_INVALID;
    retv = purc_variant_make_object_0();
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    assert(PCA_TABLESIZE(kvs) % 2 == 0);
    for (size_t i = 0; i < PCA_TABLESIZE(kvs); i += 2) {
        val = purc_variant_make_string_static(kvs[i+1], false);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv, kvs[i], val))
            goto fatal;
        purc_variant_unref(val);
    }

    return retv;

fatal:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t make_info_object(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val  = PURC_VARIANT_INVALID;

    retv = purc_variant_make_object_0();
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    val = purc_variant_make_string_static(Py_GetVersion(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_VERSION, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string_static(Py_GetPlatform(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_PLATFORM, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string_static(Py_GetCopyright(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_COPYRIGHT, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string_static(Py_GetCompiler(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_COMPILER, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string_static(Py_GetBuildInfo(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_BUILD_INFO, val))
        goto fatal;
    purc_variant_unref(val);

    return retv;

fatal:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}

static void on_release_pyinfo(void* native_entity)
{
    struct dvobj_pyinfo *pyinfo = native_entity;

    assert(Py_IsInitialized());

    Py_Finalize();

    pcutils_map_destroy(pyinfo->prop_map);
    free(pyinfo);
}

static purc_variant_t create_py(void)
{
    static struct purc_dvobj_method methods[] = {
        { "run",           run_getter,      NULL },
        { "import",        import_getter,   NULL },
    };

    static struct purc_native_ops pyinfo_ops = {
        .on_release = on_release_pyinfo,
    };

    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    purc_variant_t py;
    py = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));

    purc_variant_t val = PURC_VARIANT_INVALID;
    struct dvobj_pyinfo *pyinfo = NULL;
    if (py) {
        pyinfo = calloc(1, sizeof(*pyinfo));
        if (pyinfo == NULL)
            goto failed_info;
        pyinfo->prop_map = pcutils_map_create(
                copy_key_string, free_key_string,
                NULL, NULL, comp_key_string, true);
        if (pyinfo->prop_map == NULL)
            goto failed_info;

        if ((val = make_impl_object()) == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, "impl", val))
            goto fatal;
        purc_variant_unref(val);

        if ((val = make_info_object()) == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, "info", val))
            goto fatal;
        purc_variant_unref(val);

        if ((val = purc_variant_make_native((void *)pyinfo,
                        &pyinfo_ops)) == NULL)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py,
                    PY_HANDLE_NAME, val))
            goto fatal;
        purc_variant_unref(val);

        val = purc_variant_make_string_static("Ok", false);
        if (!purc_variant_object_set_by_static_ckey(py, "error", val))
            goto fatal;
        purc_variant_unref(val);
        return py;
    }

failed_info:
    if (pyinfo) {
        if (pyinfo->prop_map) {
            pcutils_map_destroy(pyinfo->prop_map);
        }
        free(pyinfo);
    }

fatal:
    if (val)
        purc_variant_unref(val);
    if (py)
        purc_variant_unref(py);

    pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
    return PURC_VARIANT_INVALID;
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
        *ver_code = PY_DVOBJ_VERCODE;
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

