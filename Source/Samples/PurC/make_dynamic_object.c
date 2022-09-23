/*
 * @file make_dynamic_object.c
 * @author Vincent Wei
 * @date 2021/09/16
 * @brief A sample demonstrating how to make a dynmaic object and
 *      manage the anonymous variants correctly.
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

/* XXX: overflow uint64 since fibonacci[93] */
#define NR_MEMBERS      93

#define APPEND_ANONY_VAR(array, v)                                      \
    do {                                                                \
        if (v == PURC_VARIANT_INVALID ||                                \
                !purc_variant_array_append(array, v))                   \
            goto error;                                                 \
        purc_variant_unref(v);                                          \
    } while (0)

static purc_variant_t
qux_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    int sz_array = NR_MEMBERS;

    (void)root;
    (void)call_flags;

    if (nr_args > 0) {
        uint64_t sz;
        if (purc_variant_cast_to_ulongint(argv[0], &sz, false)) {
            if (sz > NR_MEMBERS)
                sz_array = NR_MEMBERS;
            else
                sz_array = (int)sz;
        }
    }

    purc_variant_t fibonacci;
    fibonacci = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (fibonacci != PURC_VARIANT_INVALID) {
        int i;
        uint64_t a1 = 1, a2 = 1, a3;
        purc_variant_t v;

        v = purc_variant_make_ulongint(a1);
        APPEND_ANONY_VAR(fibonacci, v);

        v = purc_variant_make_ulongint(a2);
        APPEND_ANONY_VAR(fibonacci, v);

        for (i = 2; i < sz_array; i++) {
            a3 = a1 + a2;
            v = purc_variant_make_ulongint(a3);
            APPEND_ANONY_VAR(fibonacci, v);

            a1 = a2;
            a2 = a3;
        }
    }

    return fibonacci;

error:
    purc_variant_unref(fibonacci);
    return PURC_VARIANT_INVALID;
}

static const struct method_info {
    const char          *name;
    purc_dvariant_method getter;
    purc_dvariant_method setter;

} methods [] = {
    { "foo", foo_getter, NULL },
    { "bar", bar_getter, NULL },
    { "qux", qux_getter, NULL },
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

#define DEF_ANONY_VARS(postfix, nr)                     \
    purc_variant_t vars_##postfix[nr] = { NULL };       \
    size_t nr_##postfix = 0;

#define MAKE_ANONY_VAR(postfix, v)                                      \
    do {                                                                \
        if (v == PURC_VARIANT_INVALID) {                                \
            goto error_##postfix;                                       \
        }                                                               \
                                                                        \
        vars_##postfix[nr_##postfix] = v;                               \
        nr_##postfix++;                                                 \
    } while (0)

#define UNREF_ANONY_VARS(postfix)                                       \
error_##postfix:                                                        \
    do {                                                                \
        for (size_t i = 0; i < nr_##postfix; i++) {                     \
            assert(vars_##postfix[i] != NULL);                          \
            purc_variant_unref(vars_##postfix[i]);                      \
            vars_##postfix[i] = NULL;                                   \
        }                                                               \
    } while (0)

int main(void)
{
    purc_instance_extra_info info = {};
    purc_init_ex(PURC_MODULE_VARIANT,
            "cn.fmsoft.hybridos.sample", "make_dynamic_object", &info);

    purc_variant_t foobar = make_dvobj_foobar();
    if (foobar == PURC_VARIANT_INVALID)
        quit_on_error(1);

    purc_variant_t dynamic, retv, v;
    DEF_ANONY_VARS(args, 10);
    purc_dvariant_method func;

    dynamic = purc_variant_object_get_by_ckey(foobar, "foo");
    if (dynamic == PURC_VARIANT_INVALID)
        quit_on_error(2);

    func = purc_variant_dynamic_get_getter(dynamic);
    retv = func(foobar, 0, NULL, false);
    printf ("getter returned %s for foo\n",
            purc_variant_get_string_const(retv));
    purc_variant_unref(retv);

    dynamic = purc_variant_object_get_by_ckey(foobar, "bar");
    if (dynamic == PURC_VARIANT_INVALID)
        quit_on_error(2);

    func = purc_variant_dynamic_get_getter(dynamic);
    retv = func(foobar, 0, NULL, false);
    printf ("getter returned %s for bar\n",
            purc_variant_get_string_const(retv));
    purc_variant_unref(retv);

    dynamic = purc_variant_object_get_by_ckey(foobar, "qux");
    if (dynamic == PURC_VARIANT_INVALID)
        quit_on_error(2);

    func = purc_variant_dynamic_get_getter(dynamic);
    retv = func(foobar, 0, NULL, false);
    printf ("getter returned a %d-long array for qux\n",
            (int)purc_variant_array_get_size(retv));
    purc_variant_unref(retv);

    v = purc_variant_make_number(10);
    MAKE_ANONY_VAR(args, v);
    retv = func(foobar, 1, vars_args + nr_args - 1, false);
    printf ("getter returned a %d-long array for qux\n",
            (int)purc_variant_array_get_size(retv));
    purc_variant_unref(retv);

    v = purc_variant_make_number(50);
    MAKE_ANONY_VAR(args, v);
    retv = func(foobar, 1, vars_args + nr_args - 1, false);
    printf ("getter returned a %d-long array for qux\n",
            (int)purc_variant_array_get_size(retv));
    purc_variant_unref(retv);

    UNREF_ANONY_VARS(args);

    purc_variant_unref(foobar);

    const struct purc_variant_stat *stat = NULL;
    stat = purc_variant_usage_stat();

    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_DYNAMIC]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_STRING]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_NUMBER]);

    purc_cleanup ();

    return 0;
}

