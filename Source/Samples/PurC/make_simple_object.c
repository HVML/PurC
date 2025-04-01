/*
 * @file make_simple_object.c
 * @author Vincent Wei
 * @date 2021/09/20
 * @brief A sample demonstrating how to make a simple object and
 *      manage the anonymous properties correctly.
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

#include "purc/purc.h"

#undef NDEBUG
#include <assert.h>
#include <stdlib.h>

static purc_variant_t
foo_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    (void)root;
    (void)nr_args;
    (void)argv;
    (void)call_flags;

    return purc_variant_make_string_static("FOO", false);
}

static purc_variant_t
bar_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    (void)root;
    (void)nr_args;
    (void)argv;
    (void)call_flags;

    return purc_variant_make_string_static("BAR", false);
}

static const struct method_info {
    const char          *name;
    purc_dvariant_method getter;
    purc_dvariant_method setter;

} methods [] = {
    { "foo", foo_getter, NULL },
    { "bar", bar_getter, NULL },
};

static purc_variant_t make_dvobj_foobar(void)
{
    purc_variant_t dvobj;

    dvobj = purc_variant_make_object_by_static_ckey(0,
            NULL, PURC_VARIANT_INVALID);
    if (dvobj == PURC_VARIANT_INVALID)
        goto failed;

    for (size_t i = 0; i < PCA_TABLESIZE(methods); i++) {
        purc_variant_t v;
        v = purc_variant_make_dynamic(methods[i].getter,
                methods[i].setter);

        if (v == PURC_VARIANT_INVALID ||
                !purc_variant_object_set_by_static_ckey(dvobj,
                    methods[i].name, v)) {
            goto error;
        }

        purc_variant_unref(v);
    }

    return dvobj;

error:
    purc_variant_unref(dvobj);

failed:
    return PURC_VARIANT_INVALID;
}

static void quit_on_error(int errcode)
{
    fprintf(stderr, "Failed: %d\n", errcode);
    exit (errcode);
}

int main(void)
{
    purc_instance_extra_info info = {};
    purc_init_ex(PURC_MODULE_VARIANT,
            "cn.fmsoft.hybridos.sample", "make_dynamic_object", &info);

    purc_variant_t foobar = make_dvobj_foobar();
    if (foobar == PURC_VARIANT_INVALID)
        quit_on_error(1);

    purc_variant_t dynamic, retv;
    purc_dvariant_method func;

    dynamic = purc_variant_object_get_by_ckey(foobar, "foo", true);
    if (dynamic == PURC_VARIANT_INVALID)
        quit_on_error(2);

    func = purc_variant_dynamic_get_getter(dynamic);
    retv = func(foobar, 0, NULL, PCVRT_CALL_FLAG_NONE);
    printf ("getter returned %s for foo\n",
            purc_variant_get_string_const(retv));
    purc_variant_unref(retv);

    dynamic = purc_variant_object_get_by_ckey(foobar, "bar", true);
    if (dynamic == PURC_VARIANT_INVALID)
        quit_on_error(2);

    func = purc_variant_dynamic_get_getter(dynamic);
    retv = func(foobar, 0, NULL, PCVRT_CALL_FLAG_NONE);
    printf ("getter returned %s for bar\n",
            purc_variant_get_string_const(retv));
    purc_variant_unref(retv);

    purc_variant_unref(foobar);

    const struct purc_variant_stat *stat = NULL;
    stat = purc_variant_usage_stat();

    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_DYNAMIC]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_STRING]);

    purc_cleanup ();

    return 0;
}

