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

#include "purc-variant.h"
#include "purc.h"
#include "private/vcm.h"

#include <gtest/gtest.h>

TEST(vcm, basic)
{
    struct pcvcm_node *vcm;
    vcm = pcvcm_node_new_string("hello");
    ASSERT_NE(vcm, nullptr);
    pcvcm_node_destroy(vcm);

    vcm = pcvcm_node_new_null();
    ASSERT_NE(vcm, nullptr);
    pcvcm_node_destroy(vcm);

    vcm = pcvcm_node_new_boolean(true);
    ASSERT_NE(vcm, nullptr);
    pcvcm_node_destroy(vcm);

    vcm = pcvcm_node_new_number(1.23);
    ASSERT_NE(vcm, nullptr);
    pcvcm_node_destroy(vcm);

    vcm = pcvcm_node_new_longint(3344);
    ASSERT_NE(vcm, nullptr);
    pcvcm_node_destroy(vcm);

    vcm = pcvcm_node_new_ulongint(445566L);
    ASSERT_NE(vcm, nullptr);
    pcvcm_node_destroy(vcm);

    vcm = pcvcm_node_new_longdouble(1.23e23);
    ASSERT_NE(vcm, nullptr);
    pcvcm_node_destroy(vcm);

    vcm = pcvcm_node_new_byte_sequence("12\034", 5);
    ASSERT_NE(vcm, nullptr);
    pcvcm_node_destroy(vcm);
}

TEST(vcm, object)
{
    struct pcvcm_node *k1 = pcvcm_node_new_string("hello");
    struct pcvcm_node *v1 = pcvcm_node_new_null();

    struct pcvcm_node *k2 = pcvcm_node_new_string("world");
    struct pcvcm_node *v2 = pcvcm_node_new_boolean(true);

    struct pcvcm_node *k3 = pcvcm_node_new_string("k3");
    struct pcvcm_node *v3 = pcvcm_node_new_number(1.23);

    struct pcvcm_node *k4 = pcvcm_node_new_string("k4");
    struct pcvcm_node *v4 = pcvcm_node_new_longint(3344);

    struct pcvcm_node *k5 = pcvcm_node_new_string("k5");
    struct pcvcm_node *v5 = pcvcm_node_new_ulongint(445566L);

    struct pcvcm_node *k6 = pcvcm_node_new_string("k6");
    struct pcvcm_node *v6 = pcvcm_node_new_longdouble(1.23e23);

    struct pcvcm_node *k7 = pcvcm_node_new_string("k7");
    struct pcvcm_node *v7 = pcvcm_node_new_byte_sequence("12\034", 5);

    struct pcvcm_node* nodes[] = {
        k1, v1,
        k2, v2,
        k3, v3,
        k4, v4,
        k5, v5,
        k6, v6,
        k7, v7,
    };

    for (size_t i=0; i<PCA_TABLESIZE(nodes); ++i) {
        ASSERT_NE(nodes[i], nullptr);
    }

    struct pcvcm_node *root;
    root = pcvcm_node_new_object(PCA_TABLESIZE(nodes), nodes);
    ASSERT_NE(root, nullptr);

    pcvcm_node_destroy(root);
}

TEST(vcm, array)
{
    struct pcvcm_node *k1 = pcvcm_node_new_string("hello");
    struct pcvcm_node *v1 = pcvcm_node_new_null();

    struct pcvcm_node *k2 = pcvcm_node_new_string("world");
    struct pcvcm_node *v2 = pcvcm_node_new_boolean(true);

    struct pcvcm_node *k3 = pcvcm_node_new_string("k3");
    struct pcvcm_node *v3 = pcvcm_node_new_number(1.23);

    struct pcvcm_node *k4 = pcvcm_node_new_string("k4");
    struct pcvcm_node *v4 = pcvcm_node_new_longint(3344);

    struct pcvcm_node *k5 = pcvcm_node_new_string("k5");
    struct pcvcm_node *v5 = pcvcm_node_new_ulongint(445566L);

    struct pcvcm_node *k6 = pcvcm_node_new_string("k6");
    struct pcvcm_node *v6 = pcvcm_node_new_longdouble(1.23e23);

    struct pcvcm_node *k7 = pcvcm_node_new_string("k7");
    struct pcvcm_node *v7 = pcvcm_node_new_byte_sequence("12\034", 5);

    struct pcvcm_node *vo = pcvcm_node_new_object(0, NULL);

    struct pcvcm_node* nodes[] = {
        k1, v1,
        k2, v2,
        k3, v3,
        k4, v4,
        k5, v5,
        k6, v6,
        k7, v7,
        vo,
    };

    for (size_t i=0; i<PCA_TABLESIZE(nodes); ++i) {
        ASSERT_NE(nodes[i], nullptr);
    }

    struct pcvcm_node *root;
    root = pcvcm_node_new_array(PCA_TABLESIZE(nodes), nodes);
    ASSERT_NE(root, nullptr);

    pcvcm_node_destroy(root);
}

struct vcm_again {

};

#define VCM_AGAIN_NAME          "VCM_AGAIN"

static purc_variant_t
name_getter(void *native_entity, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (call_flags & PCVRT_CALL_FLAG_AGAIN) {
        return purc_variant_make_string(VCM_AGAIN_NAME, false);
    }

    purc_set_error(PURC_ERROR_AGAIN);
    return PURC_VARIANT_INVALID;
}

static inline purc_nvariant_method
property_getter(void *entity, const char *key_name)
{
    UNUSED_PARAM(key_name);
    UNUSED_PARAM(entity);
    return name_getter;
}

static inline purc_nvariant_method
property_setter(void *entity, const char *key_name)
{
    UNUSED_PARAM(key_name);
    UNUSED_PARAM(entity);
    return NULL;
}

static void
on_release(void *native_entity)
{
    struct vcm_again *v = (struct vcm_again*)native_entity;
    free(v);
}

purc_variant_t
vcm_again_variant_create()
{
    static struct purc_native_ops ops = {
        .property_getter        = property_getter,
        .property_setter        = property_setter,
        .property_cleaner       = NULL,
        .property_eraser        = NULL,

        .updater                = NULL,
        .cleaner                = NULL,
        .eraser                 = NULL,

        .match_observe          = NULL,
        .on_observe             = NULL,
        .on_forget              = NULL,
        .on_release             = on_release,
    };

    struct vcm_again *va = (struct vcm_again*)calloc(1,
            sizeof(struct vcm_again));
    if (!va) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = purc_variant_make_native(va, &ops);
    if (v == PURC_VARIANT_INVALID) {
        free(va);
        return PURC_VARIANT_INVALID;
    }
    return v;
}

purc_variant_t find_var(void* ctxt, const char* name)
{
    UNUSED_PARAM(name);
    return purc_variant_t(ctxt);
}

TEST(vcm, again)
{
    const char *ejson = "$AGAIN.name";
    size_t sz = strlen(ejson);


    purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "vcm_eval", NULL);


    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)ejson, sz);
    ASSERT_NE(rws, nullptr);

    struct purc_ejson_parse_tree *tree = purc_variant_ejson_parse_stream(rws);
    ASSERT_NE(tree, nullptr);

    purc_variant_t nv = vcm_again_variant_create();
    ASSERT_NE(nv, nullptr);

    struct pcvcm_eval_ctxt *ctxt = NULL;
    purc_variant_t v = pcvcm_eval_ex((struct pcvcm_node*)tree, &ctxt,
            find_var, nv, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);
    ASSERT_NE(ctxt, nullptr);

    int err = purc_get_last_error();
    ASSERT_EQ(err, PURC_ERROR_AGAIN);

    v =  pcvcm_eval_again_ex((struct pcvcm_node *)tree,
        ctxt, find_var, nv, false, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    enum purc_variant_type type = purc_variant_get_type(v);
    ASSERT_EQ(type, PURC_VARIANT_TYPE_STRING);

    const char *value = purc_variant_get_string_const(v);
    ASSERT_STREQ(value, VCM_AGAIN_NAME);

    err = purc_get_last_error();
    ASSERT_NE(err, PURC_ERROR_AGAIN);

    pcvcm_eval_ctxt_destroy(ctxt);

    purc_variant_unref(v);
    purc_variant_unref(nv);
    purc_variant_ejson_parse_tree_destroy(tree);
    purc_rwstream_destroy(rws);

    purc_cleanup();
}


TEST(vcm, again_ex)
{
    const char *ejson = "{name:$AGAIN.name}";
    size_t sz = strlen(ejson);


    purc_init_ex(PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test",
            "vcm_eval", NULL);


    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)ejson, sz);
    ASSERT_NE(rws, nullptr);

    struct purc_ejson_parse_tree *tree = purc_variant_ejson_parse_stream(rws);
    ASSERT_NE(tree, nullptr);

    purc_variant_t nv = vcm_again_variant_create();
    ASSERT_NE(nv, nullptr);

    struct pcvcm_eval_ctxt *ctxt = NULL;
    purc_variant_t v = pcvcm_eval_ex((struct pcvcm_node*)tree, &ctxt,
            find_var, nv, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);
    ASSERT_NE(ctxt, nullptr);

    int err = purc_get_last_error();
    ASSERT_EQ(err, PURC_ERROR_AGAIN);

    v =  pcvcm_eval_again_ex((struct pcvcm_node *)tree,
        ctxt, find_var, nv, false, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    enum purc_variant_type type = purc_variant_get_type(v);
    ASSERT_EQ(type, PURC_VARIANT_TYPE_OBJECT);

    purc_variant_t name = purc_variant_object_get_by_ckey(v, "name");
    ASSERT_NE(name, PURC_VARIANT_INVALID);

    const char *value = purc_variant_get_string_const(name);
    ASSERT_STREQ(value, VCM_AGAIN_NAME);

    err = purc_get_last_error();
    ASSERT_NE(err, PURC_ERROR_AGAIN);

    pcvcm_eval_ctxt_destroy(ctxt);
    purc_variant_unref(v);
    purc_variant_unref(nv);
    purc_variant_ejson_parse_tree_destroy(tree);
    purc_rwstream_destroy(rws);

    purc_cleanup();
}
