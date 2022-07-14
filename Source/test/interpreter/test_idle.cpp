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

#include "purc-runloop.h"
#include "private/interpreter.h"

#include <gtest/gtest.h>


static void  on_idle_callback(void* ctxt)
{
    UNUSED_PARAM(ctxt);
    static int i = 0;
    if (i > 1000) {
        purc_runloop_stop(purc_runloop_get_current());
    }
    i++;
}

TEST(idle, idle)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);
    purc_bind_session_variables();

    purc_runloop_set_idle_func(purc_runloop_get_current(), on_idle_callback, NULL);

    purc_run(NULL);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}


