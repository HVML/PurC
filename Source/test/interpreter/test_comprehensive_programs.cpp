/*
 * @file test_comprehensive_programs.cpp
 * @author Vincent Wei
 * @date 2022/07/19
 * @brief The program to test comprehensive programs including exit result,
 *      coroutines, and concurrently calls.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#undef NDEBUG

#include "purc/purc.h"
#include "private/utils.h"
#include "private/interpreter.h"

#include "../helpers.h"
#include "../tools.h"

#include <glob.h>
#include <gtest/gtest.h>

TEST(comp_hvml, bad_impl)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/9*.hvml");
}

TEST(comp_hvml, basic)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/0*.hvml");
}

TEST(comp_hvml, load)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/1*.hvml");
}

TEST(comp_hvml, call)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/2*.hvml");
}

TEST(comp_hvml, again)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/3*.hvml");
}

TEST(comp_hvml, tag)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/4*.hvml");
}

TEST(comp_hvml, var)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/5*.hvml");
}

TEST(comp_hvml, purcmc)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/6*.hvml");
}

TEST(comp_hvml, container_event)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/7*.hvml");
}

TEST(comp_hvml, operator_expression)
{
    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    go_comp_test("interpreter/comp/8*.hvml");
}

