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

#include "purc/purc-variant.h"
#include "purc/purc.h"
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

    const uint8_t buf[5] = {'1', '2', '\0', '3', '4'};
    vcm = pcvcm_node_new_byte_sequence(buf, 5);
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
    const uint8_t buf[5] = {'1', '2', '\0', '3', '4'};
    struct pcvcm_node *v7 = pcvcm_node_new_byte_sequence(buf, 5);

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
    const uint8_t buf[5] = {'1', '2', '\0', '3', '4'};
    struct pcvcm_node *v7 = pcvcm_node_new_byte_sequence(buf, 5);

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

/* Test cases for pcvcm_node_new_bigint function */
TEST(vcm, bigint_basic)
{
    struct pcvcm_node *vcm;

    /* Test decimal bigint */
    vcm = pcvcm_node_new_bigint("123456789012345678901234567890", 10);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(pcvcm_node_get_type(vcm), PCVCM_NODE_TYPE_BIG_INT);
    ASSERT_EQ(vcm->int_base, 10);
    ASSERT_EQ(vcm->quoted_type, PCVCM_NODE_QUOTED_TYPE_NONE);
    ASSERT_TRUE(pcvcm_node_is_closed(vcm));
    pcvcm_node_destroy(vcm);

    /* Test hexadecimal bigint */
    vcm = pcvcm_node_new_bigint("ABCDEF123456789", 16);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(pcvcm_node_get_type(vcm), PCVCM_NODE_TYPE_BIG_INT);
    ASSERT_EQ(vcm->int_base, 16);
    pcvcm_node_destroy(vcm);

    /* Test octal bigint */
    vcm = pcvcm_node_new_bigint("777123456701234567", 8);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(pcvcm_node_get_type(vcm), PCVCM_NODE_TYPE_BIG_INT);
    ASSERT_EQ(vcm->int_base, 8);
    pcvcm_node_destroy(vcm);
}

TEST(vcm, bigint_edge_cases)
{
    struct pcvcm_node *vcm;

    /* Test single digit */
    vcm = pcvcm_node_new_bigint("0", 10);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(vcm->sz_ptr[0], 1); /* string length */
    ASSERT_STREQ((char*)vcm->sz_ptr[1], "0");
    pcvcm_node_destroy(vcm);

    /* Test maximum decimal value */
    vcm = pcvcm_node_new_bigint("999999999999999999999999999999999999999999", 10);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(vcm->sz_ptr[0], 42); /* string length */
    pcvcm_node_destroy(vcm);

    /* Test hex with lowercase */
    vcm = pcvcm_node_new_bigint("abcdef0123456789", 16);
    ASSERT_NE(vcm, nullptr);
    ASSERT_STREQ((char*)vcm->sz_ptr[1], "abcdef0123456789");
    pcvcm_node_destroy(vcm);
}

TEST(vcm, bigint_memory_management)
{
    struct pcvcm_node *vcm;
    const char *test_str = "12345678901234567890123456789012345678901234567890";

    /* Test memory allocation and deallocation */
    vcm = pcvcm_node_new_bigint(test_str, 10);
    ASSERT_NE(vcm, nullptr);

    /* Verify string is properly copied */
    ASSERT_EQ(vcm->sz_ptr[0], strlen(test_str));
    ASSERT_STREQ((char*)vcm->sz_ptr[1], test_str);

    /* Verify memory is different from original */
    ASSERT_NE((void*)vcm->sz_ptr[1], (void*)test_str);

    /* Test proper cleanup */
    pcvcm_node_destroy(vcm);

    /* Test multiple allocations and deallocations */
    for (int i = 0; i < 100; i++) {
        vcm = pcvcm_node_new_bigint("987654321098765432109876543210", 10);
        ASSERT_NE(vcm, nullptr);
        pcvcm_node_destroy(vcm);
    }
}

TEST(vcm, bigint_string_content)
{
    struct pcvcm_node *vcm;

    /* Test empty string */
    vcm = pcvcm_node_new_bigint("", 10);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(vcm->sz_ptr[0], 0);
    ASSERT_STREQ((char*)vcm->sz_ptr[1], "");
    pcvcm_node_destroy(vcm);

    /* Test string with special characters for hex */
    vcm = pcvcm_node_new_bigint("DEADBEEF", 16);
    ASSERT_NE(vcm, nullptr);
    ASSERT_STREQ((char*)vcm->sz_ptr[1], "DEADBEEF");
    pcvcm_node_destroy(vcm);

    /* Test very long string */
    const char *long_str = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
    vcm = pcvcm_node_new_bigint(long_str, 10);
    ASSERT_NE(vcm, nullptr);
    ASSERT_EQ(vcm->sz_ptr[0], strlen(long_str));
    ASSERT_STREQ((char*)vcm->sz_ptr[1], long_str);
    pcvcm_node_destroy(vcm);
}

TEST(vcm, bigint_base_validation)
{
    struct pcvcm_node *vcm;

    /* Test all supported bases */
    const int supported_bases[] = {8, 10, 16};
    const char *test_values[] = {"12345670", "1234567890", "123456789ABCDEF"};

    for (size_t i = 0; i < sizeof(supported_bases)/sizeof(supported_bases[0]); i++) {
        vcm = pcvcm_node_new_bigint(test_values[i], supported_bases[i]);
        ASSERT_NE(vcm, nullptr);
        ASSERT_EQ(vcm->int_base, supported_bases[i]);
        ASSERT_EQ(pcvcm_node_get_type(vcm), PCVCM_NODE_TYPE_BIG_INT);
        pcvcm_node_destroy(vcm);
    }
}

TEST(vcm, bigint_null_termination)
{
    struct pcvcm_node *vcm;
    const char *test_str = "123456789";

    vcm = pcvcm_node_new_bigint(test_str, 10);
    ASSERT_NE(vcm, nullptr);

    /* Verify null termination */
    char *stored_str = (char*)vcm->sz_ptr[1];
    ASSERT_EQ(stored_str[vcm->sz_ptr[0]], '\0');

    /* Verify content integrity */
    for (size_t i = 0; i < vcm->sz_ptr[0]; i++) {
        ASSERT_EQ(stored_str[i], test_str[i]);
    }

    pcvcm_node_destroy(vcm);
}

struct vcm_again {

};

#define VCM_AGAIN_NAME          "VCM_AGAIN"

static purc_variant_t
name_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (call_flags & PCVRT_CALL_FLAG_AGAIN) {
        return purc_variant_make_string(VCM_AGAIN_NAME, false);
    }

    purc_set_error(PURC_ERROR_AGAIN);
    return PURC_VARIANT_INVALID;
}

static purc_nvariant_method
property_getter(void *entity, const char *key_name)
{
    UNUSED_PARAM(key_name);
    UNUSED_PARAM(entity);
    return name_getter;
}

static purc_nvariant_method
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

        .did_matched            = NULL,
        .on_observe             = NULL,
        .on_forget              = NULL,
        .on_release             = on_release,

        .priv_ops               = NULL,
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

    struct purc_ejson_parsing_tree *tree = purc_variant_ejson_parse_stream(rws);
    ASSERT_NE(tree, nullptr);

    purc_variant_t nv = vcm_again_variant_create();
    ASSERT_NE(nv, nullptr);

    struct pcvcm_eval_ctxt *ctxt = NULL;
    purc_variant_t v = pcvcm_eval_ex((struct pcvcm_node *)tree, &ctxt, find_var,
            nv, NULL, NULL, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);
    ASSERT_NE(ctxt, nullptr);

    int err = purc_get_last_error();
    ASSERT_EQ(err, PURC_ERROR_AGAIN);

    v = pcvcm_eval_again_ex((struct pcvcm_node *)tree,
            ctxt, find_var, nv, NULL, NULL, false, false);
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
    purc_ejson_parsing_tree_destroy(tree);
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

    struct purc_ejson_parsing_tree *tree = purc_variant_ejson_parse_stream(rws);
    ASSERT_NE(tree, nullptr);

    purc_variant_t nv = vcm_again_variant_create();
    ASSERT_NE(nv, nullptr);

    struct pcvcm_eval_ctxt *ctxt = NULL;
    purc_variant_t v = pcvcm_eval_ex((struct pcvcm_node*)tree, &ctxt,
            find_var, nv, NULL, NULL, false);
    ASSERT_EQ(v, PURC_VARIANT_INVALID);
    ASSERT_NE(ctxt, nullptr);

    int err = purc_get_last_error();
    ASSERT_EQ(err, PURC_ERROR_AGAIN);

    v = pcvcm_eval_again_ex((struct pcvcm_node *)tree,
            ctxt, find_var, nv, NULL, NULL, false, false);
    ASSERT_NE(v, PURC_VARIANT_INVALID);

    enum purc_variant_type type = purc_variant_get_type(v);
    ASSERT_EQ(type, PURC_VARIANT_TYPE_OBJECT);

    purc_variant_t name = purc_variant_object_get_by_ckey_ex(v, "name", true);
    ASSERT_NE(name, PURC_VARIANT_INVALID);

    const char *value = purc_variant_get_string_const(name);
    ASSERT_STREQ(value, VCM_AGAIN_NAME);

    err = purc_get_last_error();
    ASSERT_NE(err, PURC_ERROR_AGAIN);

    pcvcm_eval_ctxt_destroy(ctxt);
    purc_variant_unref(v);
    purc_variant_unref(nv);
    purc_ejson_parsing_tree_destroy(tree);
    purc_rwstream_destroy(rws);

    purc_cleanup();
}
