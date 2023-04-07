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
#include "tools.h"

#include <glob.h>
#include <gtest/gtest.h>

TEST(comp_hvml, basic)
{
    go_comp_test("comp/0*.hvml");
}

TEST(comp_hvml, load)
{
    go_comp_test("comp/1*.hvml");
}

TEST(comp_hvml, call)
{
    go_comp_test("comp/2*.hvml");
}

TEST(comp_hvml, again)
{
    go_comp_test("comp/3*.hvml");
}

TEST(comp_hvml, tag)
{
    go_comp_test("comp/4*.hvml");
}

TEST(comp_hvml, var)
{
    go_comp_test("comp/5*.hvml");
}
