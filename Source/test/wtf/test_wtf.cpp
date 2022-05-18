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

#include <vector>

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

TEST(thread, basic)
{
    int volatile v = 0;
    pthread_t volatile thread_foo = pthread_self();
    PC_DEBUGX("pthread: 0x%zx", pthread_self());
    Thread::create("foo", [&] {
        v = 1;
        thread_foo = pthread_self();
        PC_DEBUGX("pthread: 0x%zx", pthread_self());
    })->waitForCompletion();
    ASSERT_EQ(v, 1);
    ASSERT_FALSE(pthread_equal(pthread_self(), thread_foo));
}

TEST(thread, named_thread)
{
    int volatile v = 0;
    pthread_t volatile thread_foo = pthread_self();
    PC_DEBUGX("pthread: 0x%zx", pthread_self());
    Thread::create("foo", [&] {
        v = 1;
        PC_DEBUGX("pthread: 0x%zx", pthread_self());
        if (pthread_equal(pthread_self(), thread_foo))
            abort();
        thread_foo = pthread_self();
    })->waitForCompletion();
    ASSERT_EQ(v, 1);
    ASSERT_FALSE(pthread_equal(pthread_self(), thread_foo));

    Thread::create("foo", [&] {
        v = 2;
        PC_DEBUGX("pthread: 0x%zx", pthread_self());
        if (!pthread_equal(pthread_self(), thread_foo))
        thread_foo = pthread_self();
    })->waitForCompletion();
    ASSERT_EQ(v, 2);
    ASSERT_FALSE(pthread_equal(pthread_self(), thread_foo));
}

TEST(thread, threads)
{
    PC_DEBUGX("pthread: 0x%zx/%zd", pthread_self(), pthread_self());
    std::vector<Ref<Thread> > threads;
    const size_t n = 3;
    for (size_t i=0; i<n; ++i) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "foo%zd", i);
        Ref<Thread> thread = Thread::create(buf, [&, i] {
                });
        threads.push_back(thread);
    }

    for (auto thread : threads) {
        thread->waitForCompletion();
    }
}

TEST(workqueue, basic)
{
    char name[1024];
    snprintf(name, sizeof(name), "wq_%s[%d]", __FILE__, __LINE__);

    BinarySemaphore semaphore;
    do {
        RefPtr<WorkQueue> workqueue = WorkQueue::create(name);
        workqueue->dispatch([&] {
            semaphore.signal();
        });
        semaphore.wait();
    } while (0);
}

