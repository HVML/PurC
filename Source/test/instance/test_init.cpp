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
#include "private/utils.h"
#include "../helpers.h"

#include <gtest/gtest.h>


TEST(interpreter, purc_init)
{
    unsigned int modules[] = {
        PURC_MODULE_UTILS,
        PURC_MODULE_DOM,
        PURC_MODULE_HTML,
        PURC_MODULE_XML,
        PURC_MODULE_VARIANT,
        PURC_MODULE_EJSON,
        PURC_MODULE_XGML,
        PURC_MODULE_HVML,
        // PURC_MODULE_PCRDR,
        // PURC_MODULE_ALL,
    };
    const char *app_name = "foo";
    const char *runner_name = "bar";


    for (size_t i=0; i<sizeof(modules)/sizeof(modules[0]); ++i) {
        if (modules[i] != PURC_MODULE_EJSON)
            continue;

        const purc_instance_extra_info extra_info = {};

        int r;
        r = purc_init_ex(modules[i], app_name, runner_name, &extra_info);

        ASSERT_EQ(r, 0);

        purc_cleanup();
    }
}


