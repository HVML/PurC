/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc/purc.h"
#include "private/debug.h"
#include "private/errors.h"
#include "private/variant.h"

#include "../helpers.h"

#include <gtest/gtest.h>

#if 0
static void*
ref(const void *v)
{
    return purc_variant_ref((purc_variant_t)v);
}
#endif

static void
unref(void *v)
{
    purc_variant_unref((purc_variant_t)v);
}

TEST(variant, tuple)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t v = purc_variant_make_tuple(3, NULL);
    ASSERT_NE(v, nullptr);

    size_t sz = purc_variant_tuple_get_size(v);
    ASSERT_EQ(sz, 3);

    bool b = purc_variant_is_type(v, PURC_VARIANT_TYPE_TUPLE);
    ASSERT_EQ(b, true);

    b = purc_variant_is_tuple(v);
    ASSERT_EQ(b, true);

    purc_variant_t m = purc_variant_tuple_get(v, 0);
    ASSERT_NE(m, nullptr);

    b = purc_variant_is_null(m);
    ASSERT_EQ(b, true);

    b = purc_variant_is_undefined(m);
    ASSERT_EQ(b, false);

    unref(v);
}


TEST(variant, tuple_member)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(3, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    ASSERT_NE(object, nullptr);

    bool r = purc_variant_tuple_set(tuple, 0, object);
    ASSERT_EQ(r, true);

    purc_variant_t v = purc_variant_tuple_get(tuple, 0);
    ASSERT_EQ(v, object);

    purc_variant_t array = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    ASSERT_NE(array, nullptr);

    r = purc_variant_tuple_set(tuple, 1, array);
    ASSERT_EQ(r, true);

    v = purc_variant_tuple_get(tuple, 1);
    ASSERT_EQ(v, array);


    purc_variant_t st = purc_variant_make_set(0, NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(st, nullptr);

    r = purc_variant_tuple_set(tuple, 2, st);
    ASSERT_EQ(r, true);

    v = purc_variant_tuple_get(tuple, 2);
    ASSERT_EQ(v, st);


    purc_variant_t s = purc_variant_make_string("test", false);
    ASSERT_NE(s, nullptr);

    r = purc_variant_tuple_set(tuple, 0, s);
    ASSERT_EQ(r, true);

    v = purc_variant_tuple_get(tuple, 0);
    ASSERT_EQ(v, s);
    v = purc_variant_tuple_get(tuple, 1);
    ASSERT_EQ(v, array);
    v = purc_variant_tuple_get(tuple, 2);
    ASSERT_EQ(v, st);

    r = purc_variant_tuple_set(tuple, 5, s);
    ASSERT_EQ(r, false);

    v = purc_variant_tuple_get(tuple, 0);
    ASSERT_EQ(v, s);
    v = purc_variant_tuple_get(tuple, 1);
    ASSERT_EQ(v, array);
    v = purc_variant_tuple_get(tuple, 2);
    ASSERT_EQ(v, st);

    r = purc_variant_tuple_set(tuple, 1, s);
    ASSERT_EQ(r, true);

    v = purc_variant_tuple_get(tuple, 0);
    ASSERT_EQ(v, s);
    v = purc_variant_tuple_get(tuple, 1);
    ASSERT_EQ(v, s);
    v = purc_variant_tuple_get(tuple, 2);
    ASSERT_EQ(v, st);

    unref(s);
    unref(st);
    unref(array);
    unref(object);
    unref(tuple);
}

TEST(variant, tuple_as_member)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(3, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    ASSERT_NE(object, nullptr);
    bool r = purc_variant_object_set_by_static_ckey(object, "tuple", tuple);
    ASSERT_EQ(r, true);
    purc_variant_t v = purc_variant_object_get_by_ckey(object, "tuple");
    ASSERT_EQ(v, tuple);


    purc_variant_t array = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    ASSERT_NE(array, nullptr);
    r = purc_variant_array_append(array, tuple);
    ASSERT_EQ(r, true);
    v = purc_variant_array_get(array, 0);
    ASSERT_EQ(v, tuple);

    purc_variant_t st = purc_variant_make_set(0, NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(st, nullptr);
    r = purc_variant_set_add(st, tuple, true);
    ASSERT_EQ(r, true);
    v = purc_variant_set_get_by_index(st, 0);
    ASSERT_EQ(v, tuple);


    unref(st);
    unref(array);
    unref(object);
    unref(tuple);
}

TEST(variant, tuple_stringify)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    char buf[8192];

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t s = purc_variant_make_string_static("abc", false);
    ASSERT_NE(s, nullptr);

    purc_variant_tuple_set(tuple, 0, s);

    int r = purc_variant_stringify_buff(buf, sizeof(buf), tuple);
    ASSERT_NE(r, -1);
    ASSERT_STREQ("abc\n", buf);

    unref(s);
    unref(tuple);
}

TEST(variant, tuple_serialize)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    char buf[8192];

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t s = purc_variant_make_string_static("abc", false);
    ASSERT_NE(s, nullptr);

    purc_variant_tuple_set(tuple, 0, s);

    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    size_t len_expected = 0;
    purc_variant_serialize(tuple, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    buf[len_expected] = 0;
    ASSERT_STREQ("[\"abc\"]", buf);

    purc_rwstream_destroy(my_rws);
    unref(s);
    unref(tuple);
}

TEST(variant, tuple_serialize_ejson)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    char buf[8192];

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t s = purc_variant_make_string_static("abc", false);
    ASSERT_NE(s, nullptr);

    purc_variant_tuple_set(tuple, 0, s);

    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    size_t len_expected = 0;
    purc_variant_serialize(tuple, my_rws,
            0,
            PCVRNT_SERIALIZE_OPT_PLAIN | PCVRNT_SERIALIZE_OPT_TUPLE_EJSON,
            &len_expected);
    buf[len_expected] = 0;
    ASSERT_STREQ("[!\"abc\"]", buf);

    purc_rwstream_destroy(my_rws);
    unref(s);
    unref(tuple);
}

static bool tuple_change_handler (
        purc_variant_t src,
        pcvar_op_t op,
        void *ctxt,
        size_t nr_args,
        purc_variant_t *argv
        )
{
    UNUSED_PARAM(src);
    UNUSED_PARAM(op);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    uint64_t pos;
    purc_variant_cast_to_ulongint(argv[0], &pos, false);
    fprintf(stderr, "change listener\n");
    fprintf(stderr, "nr_args=%ld\n", nr_args);
    fprintf(stderr, "pos=%ld\n", pos);
    fprintf(stderr, "o=%s\n", pcvariant_typename(argv[1]));
    fprintf(stderr, "n=%s\n", pcvariant_typename(argv[2]));
    fprintf(stderr, "n=%s\n", purc_variant_get_string_const(argv[2]));
    return true;
}

static bool tuple_changed_handler (
        purc_variant_t src,
        pcvar_op_t op,
        void *ctxt,
        size_t nr_args,
        purc_variant_t *argv
        )
{
    UNUSED_PARAM(src);
    UNUSED_PARAM(op);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    uint64_t pos;
    purc_variant_cast_to_ulongint(argv[0], &pos, false);
    fprintf(stderr, "changed listener\n");
    fprintf(stderr, "nr_args=%ld\n", nr_args);
    fprintf(stderr, "pos=%ld\n", pos);
    fprintf(stderr, "o=%s\n", pcvariant_typename(argv[1]));
    fprintf(stderr, "n=%s\n", pcvariant_typename(argv[2]));
    fprintf(stderr, "n=%s\n", purc_variant_get_string_const(argv[2]));
    return true;
}

TEST(variant, tuple_listener)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);


    purc_variant_t s = purc_variant_make_string_static("abc", false);
    ASSERT_NE(s, nullptr);

    int op = PCVAR_OPERATION_CHANGE;
    struct pcvar_listener *prev;
    prev = purc_variant_register_pre_listener(tuple, (pcvar_op_t)op,
            tuple_change_handler, tuple);
    ASSERT_NE(prev, nullptr);

    struct pcvar_listener *listener;
    listener = purc_variant_register_post_listener(tuple, (pcvar_op_t)op,
            tuple_changed_handler, tuple);

    ASSERT_NE(listener, nullptr);

    purc_variant_tuple_set(tuple, 0, s);

    purc_variant_revoke_listener(tuple, prev);
    purc_variant_revoke_listener(tuple, listener);
    unref(s);
    unref(tuple);
}

static bool dump_handle (
        purc_variant_t src,
        pcvar_op_t op,
        void *ctxt,
        size_t nr_args,
        purc_variant_t *argv
        )
{
    UNUSED_PARAM(src);
    UNUSED_PARAM(op);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    char buf[2048];
    fprintf(stderr, "#####> begin dump handle\n");
    switch (op) {
    case PCVAR_OPERATION_GROW:
        fprintf(stderr, "op=PCVAR_OPERATION_GROW\n");
        break;
    case PCVAR_OPERATION_SHRINK:
        fprintf(stderr, "op=PCVAR_OPERATION_SHRINK\n");
        break;
    case PCVAR_OPERATION_CHANGE:
        fprintf(stderr, "op=PCVAR_OPERATION_CHANGE\n");
        break;
    case PCVAR_OPERATION_REFASCHILD:
        fprintf(stderr, "op=PCVAR_OPERATION_REFASCHILD\n");
        break;
    case PCVAR_OPERATION_ALL:
        fprintf(stderr, "op=PCVAR_OPERATION_ALL\n");
        break;
    }
    fprintf(stderr, "nr_args=%ld\n", nr_args);
    for (size_t i = 0; i < nr_args; i++) {
        purc_variant_stringify_buff(buf, sizeof(buf), argv[i]);
        fprintf(stderr, "argv[%ld].type=%s|stringify=%s\n", i,
                pcvariant_typename(argv[i]), buf);
    }
    fprintf(stderr, "#####> end dump handle\n");
    return true;
}

TEST(variant, tuple_as_object_value)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t s = purc_variant_make_string_static("abc", false);
    ASSERT_NE(s, nullptr);
    purc_variant_tuple_set(tuple, 0, s);

    purc_variant_t object = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    ASSERT_NE(object, nullptr);

    int op = PCVAR_OPERATION_GROW | PCVAR_OPERATION_CHANGE;
    struct pcvar_listener *listener;
    listener = purc_variant_register_pre_listener(object, (pcvar_op_t)op,
            dump_handle, object);

    purc_variant_object_set_by_static_ckey(object, "key", tuple);

    purc_variant_revoke_listener(object, listener);

    unref(object);
    unref(s);
    unref(tuple);
}

TEST(variant, tuple_as_array_member)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t s = purc_variant_make_string_static("tuple_member", false);
    ASSERT_NE(s, nullptr);
    purc_variant_tuple_set(tuple, 0, s);

    purc_variant_t array = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    ASSERT_NE(array, nullptr);

    int op = PCVAR_OPERATION_GROW | PCVAR_OPERATION_CHANGE | PCVAR_OPERATION_SHRINK;
    struct pcvar_listener *listener;
    listener = purc_variant_register_pre_listener(array, (pcvar_op_t)op,
            dump_handle, array);

    purc_variant_array_append(array, tuple);
    purc_variant_array_remove(array, 0);

    purc_variant_revoke_listener(array, listener);

    unref(array);
    unref(s);
    unref(tuple);
}

TEST(variant, tuple_as_set_member)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t s = purc_variant_make_string_static("tuple_member", false);
    ASSERT_NE(s, nullptr);
    purc_variant_tuple_set(tuple, 0, s);

    purc_variant_t st = purc_variant_make_set(0, NULL, PURC_VARIANT_INVALID);
    ASSERT_NE(st, nullptr);

    int op = PCVAR_OPERATION_GROW | PCVAR_OPERATION_CHANGE | PCVAR_OPERATION_SHRINK;
    struct pcvar_listener *listener;
    listener = purc_variant_register_pre_listener(st, (pcvar_op_t)op,
            dump_handle, st);

    purc_variant_set_add(st, tuple, true);
    purc_variant_set_remove_by_index(st, 0);

    purc_variant_revoke_listener(st, listener);

    unref(st);
    unref(s);
    unref(tuple);
}

TEST(variant, tuple_as_set_member_constraint)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t s = purc_variant_make_string_static("tuple_member", false);
    ASSERT_NE(s, nullptr);
    purc_variant_tuple_set(tuple, 0, s);

    purc_variant_t tp = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tp, nullptr);
    purc_variant_tuple_set(tp, 0, s);

    purc_variant_t st = purc_variant_make_set_by_ckey_ex(0, "id", false,
            PURC_VARIANT_INVALID);
    PRINT_VARIANT(st);
    ASSERT_NE(st, nullptr);

    int op = PCVAR_OPERATION_GROW | PCVAR_OPERATION_CHANGE | PCVAR_OPERATION_SHRINK;
    struct pcvar_listener *listener;
    listener = purc_variant_register_pre_listener(st, (pcvar_op_t)op,
            dump_handle, st);

    bool ret = purc_variant_set_add(st, tuple, true);
    ASSERT_EQ(ret, true);

    ret = purc_variant_set_add(tp, tuple, true);
    ASSERT_EQ(ret, false);

    purc_variant_revoke_listener(st, listener);

    unref(st);
    unref(tp);
    unref(s);
    unref(tuple);
}

TEST(variant, tuple_as_set_member_constraint_with_key)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant_tuple", false);

    purc_variant_t tuple = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tuple, nullptr);

    purc_variant_t s = purc_variant_make_string_static("tuple_member", false);
    ASSERT_NE(s, nullptr);
    purc_variant_tuple_set(tuple, 0, s);

    purc_variant_t tp = purc_variant_make_tuple(1, NULL);
    ASSERT_NE(tp, nullptr);
    purc_variant_tuple_set(tp, 0, s);


    purc_variant_t ob_1 = purc_variant_make_object_by_static_ckey(1, "id", tuple);
    ASSERT_NE(ob_1, nullptr);

    purc_variant_t ob_2 = purc_variant_make_object_by_static_ckey(1, "id", tp);
    ASSERT_NE(ob_2, nullptr);

    purc_variant_t st = purc_variant_make_set_by_ckey_ex(0, "id", false,
            PURC_VARIANT_INVALID);
    PRINT_VARIANT(st);
    ASSERT_NE(st, nullptr);

    int op = PCVAR_OPERATION_GROW | PCVAR_OPERATION_CHANGE | PCVAR_OPERATION_SHRINK;
    struct pcvar_listener *listener;
    listener = purc_variant_register_pre_listener(st, (pcvar_op_t)op,
            dump_handle, st);

    bool ret = purc_variant_set_add(st, ob_1, true);
    ASSERT_EQ(ret, true);

    ret = purc_variant_set_add(tp, ob_2, true);
    ASSERT_EQ(ret, false);

    purc_variant_revoke_listener(st, listener);

    unref(st);
    unref(ob_2);
    unref(ob_1);
    unref(tp);
    unref(s);
    unref(tuple);
}
