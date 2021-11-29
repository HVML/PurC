#include "purc.h"
#include "private/sorted-array.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

// to test basic functions of atom
TEST(utils, atom_basic)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "variant", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);
    char buff [] = "HVML";
    const char *str;

    purc_atom_t atom;

    atom = purc_atom_from_static_string(NULL);
    ASSERT_EQ(atom, 0);

    atom = purc_atom_from_string(NULL);
    ASSERT_EQ(atom, 0);

    atom = purc_atom_from_static_string("HVML");
    ASSERT_EQ(atom, 1);

    str = purc_atom_to_string(1);
    ASSERT_STREQ(str, "HVML");

    atom = purc_atom_try_string("HVML");
    ASSERT_EQ(atom, 1);

    atom = purc_atom_try_string("PurC");
    ASSERT_EQ(atom, 0);

    atom = purc_atom_try_string(NULL);
    ASSERT_EQ(atom, 0);

    atom = purc_atom_from_string(buff);
    ASSERT_EQ(atom, 1);

    atom = purc_atom_from_string("PurC");
    ASSERT_EQ(atom, 2);

    atom = purc_atom_try_string("HVML");
    ASSERT_EQ(atom, 1);

    atom = purc_atom_try_string("PurC");
    ASSERT_EQ(atom, 2);

    str = purc_atom_to_string(1);
    ASSERT_STREQ(str, "HVML");

    str = purc_atom_to_string(2);
    ASSERT_STREQ(str, "PurC");

    str = purc_atom_to_string(3);
    ASSERT_EQ(str, nullptr);

    atom = purc_atom_try_string(NULL);
    ASSERT_EQ(atom, 0);

    purc_cleanup ();
}

// to test sorted array
static int sortv[10] = { 1, 8, 7, 5, 4, 6, 9, 0, 2, 3 };

static int
intcmp(const void *sortv1, const void *sortv2)
{
    int i = (int)(intptr_t)sortv1;
    int j = (int)(intptr_t)sortv2;

    return i - j;
}

TEST(utils, sorted_array_asc)
{
    struct sorted_array *sa;
    int n;

    sa = sorted_array_create(SAFLAG_DEFAULT, 4, NULL,
            intcmp);

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 0);

    for (int i = 0; i < 10; i++) {
        int ret = sorted_array_add (sa, (void *)(intptr_t)sortv[i],
                (void *)(intptr_t)(sortv[i] + 100));
        ASSERT_EQ(ret, 0);
    }

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 10);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        ASSERT_EQ(sortv, i);
    }

    sorted_array_remove (sa, (void *)(intptr_t)0);
    sorted_array_remove (sa, (void *)(intptr_t)9);
    sorted_array_delete (sa, 0);

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 7);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        ASSERT_EQ(sortv, i + 2);
    }

    sorted_array_destroy(sa);
}

TEST(utils, sorted_array_desc)
{
    struct sorted_array *sa;
    int n;

    sa = sorted_array_create(SAFLAG_ORDER_DESC, 4, NULL,
            intcmp);

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 0);

    for (int i = 0; i < 10; i++) {
        int ret = sorted_array_add (sa, (void *)(intptr_t)sortv[i],
                (void *)(intptr_t)(sortv[i] + 100));
        ASSERT_EQ(ret, 0);
    }

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 10);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        sortv = 9 - sortv;
        ASSERT_EQ(sortv, i);
    }

    sorted_array_remove (sa, (void *)(intptr_t)0);
    sorted_array_remove (sa, (void *)(intptr_t)9);

    n = (int)sorted_array_count (sa);
    ASSERT_EQ(n, 8);

    for (int i = 0; i < n; i++) {
        void *data;
        int sortv = (int)(intptr_t)sorted_array_get (sa, i, &data);

        ASSERT_EQ((int)(intptr_t)data, sortv + 100);
        sortv = 8 - sortv;
        ASSERT_EQ(sortv, i);
    }

    bool found;
    found = sorted_array_find (sa, (void *)(intptr_t)0, NULL);
    ASSERT_EQ(found, false);
    found = sorted_array_find (sa, (void *)(intptr_t)9, NULL);
    ASSERT_EQ(found, false);

    for (int i = 1; i < 9; i++) {
        void *data;
        found = sorted_array_find (sa, (void *)(intptr_t)i, &data);

        ASSERT_EQ(found, true);
        ASSERT_EQ((int)(intptr_t)data, i + 100);
    }

    sorted_array_destroy(sa);
}

