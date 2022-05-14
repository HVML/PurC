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

TEST(fetcher, runloop)
{
    ASSERT_FALSE(RunLoop::isMainInitizlized());
    RunLoop::current().dispatch([] {
        ASSERT_FALSE(RunLoop::isMainInitizlized());
        RunLoop::current().stop();
    });
    RunLoop::current().run();
    ASSERT_FALSE(RunLoop::isMainInitizlized());
}

