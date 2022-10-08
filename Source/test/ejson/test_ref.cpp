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

#include "private/ejson.h"
#include "purc/purc-rwstream.h"

#include <gtest/gtest.h>

TEST(ejson, ref_manual_good)
{
    // add child bottom-up
    purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test", "ejson", NULL);

    purc_variant_t root = purc_variant_make_array(0, NULL);
    purc_variant_t arr1 = purc_variant_make_array(0, NULL);

    purc_variant_t str = purc_variant_make_string("hello", true);
    purc_variant_array_append(arr1, str);

    purc_variant_array_append(root, arr1);

    purc_variant_unref(root);
    purc_variant_unref(arr1);
    purc_variant_unref(str);

    purc_cleanup();
}

TEST(ejson, ref_manual_bad)
{
    // add child top-down
    purc_init_ex (PURC_MODULE_EJSON, "cn.fmsoft.hybridos.test", "ejson", NULL);

    purc_variant_t root = purc_variant_make_array(0, NULL);
    purc_variant_t arr1 = purc_variant_make_array(0, NULL);
    purc_variant_array_append(root, arr1);

    purc_variant_t str = purc_variant_make_string("hello", true);
    purc_variant_array_append(arr1, str);

    purc_variant_unref(root);
    purc_variant_unref(arr1);
    purc_variant_unref(str);

    purc_cleanup();
}

