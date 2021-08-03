#include "purc.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

static inline int my_puts(const char* str)
{
    return fputs(str, stderr);
}

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

