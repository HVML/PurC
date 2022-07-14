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

#include "private/debug.h"
#include "purc-runloop.h"

#include "../helpers.h"

#include <gtest/gtest.h>
#include <wtf/RunLoop.h>
#include <wtf/FileSystem.h>
#include <wtf/StdLibExtras.h>
#include <wtf/UniStdExtras.h>
#include <wtf/SystemTracing.h>
#include <wtf/WorkQueue.h>
#include <wtf/glib/GLibUtilities.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/URL.h>

#include <gio/gio.h>

TEST(runloop, basic)
{
    int v = 0;
    RunLoop::current().dispatch([&] {
        ++v;
        RunLoop::current().stop();
    });
    RunLoop::current().run();
    ASSERT_EQ(v, 1);
}

TEST(runloop, stop)
{
    RunLoop::initializeMain();
    int v = 0;
    Ref<Thread> thread = Thread::create("foo", [&] {
        for (int i=0; i<10; ++i) {
            RunLoop::current().dispatch([&] {
                v += 1;
            });
        }
        RunLoop::current().dispatch([&] {
            RunLoop::current().stop();
        });
        RunLoop::current().run();
    });

    thread->waitForCompletion();
    ASSERT_EQ(v, 10);
}

