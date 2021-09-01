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

    for (size_t i=0; i<sizeof(nodes)/sizeof(nodes[0]); ++i) {
        ASSERT_NE(nodes[i], nullptr);
    }

    struct pcvcm_node *root;
    root = pcvcm_node_new_object(sizeof(nodes)/sizeof(nodes[0]), nodes);
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

    for (size_t i=0; i<sizeof(nodes)/sizeof(nodes[0]); ++i) {
        ASSERT_NE(nodes[i], nullptr);
    }

    struct pcvcm_node *root;
    root = pcvcm_node_new_array(sizeof(nodes)/sizeof(nodes[0]), nodes);
    ASSERT_NE(root, nullptr);

    pcvcm_node_destroy(root);
}



