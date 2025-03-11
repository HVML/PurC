/*
 * @file test-spawn.c
 * @author Vincent Wei
 * @date 2025/03/11
 * @brief The program tests $SYS.spawn(), $SYS.pipe(), $SYS.close(),
 *      and $STREAM.from()
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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
#include "../helpers.h"
#include "../tools.h"

#include <gtest/gtest.h>

TEST(spawn, bc)
{
    PurCInstance purc(false);

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDERR);

    run_one_comp_test("dvobjs/spawn/spawn-bc.hvml");
}

