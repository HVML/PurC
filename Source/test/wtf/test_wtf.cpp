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

