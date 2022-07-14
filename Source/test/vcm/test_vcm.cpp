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



