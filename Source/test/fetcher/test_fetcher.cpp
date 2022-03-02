#include "purc.h"

#include "private/fetcher.h"

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

TEST(fetcher, basic)
{
    if (1)
        return;

    do {
        purc_instance_extra_info info = {};
        info.enable_remote_fetcher = false;
        purc_init ("cn.fmsoft.hybridos.sample", "pcfetcher", &info);

        RunLoop::initializeMain();
        AtomString::init();

        // pcfetcher_init(10, 1024, true);
        // pcfetcher_term();
        purc_cleanup();
    } while (0);
}

TEST(fetcher, basic1)
{
    do {
        purc_instance_extra_info info = {};
        info.enable_remote_fetcher = false;
        purc_init ("cn.fmsoft.hybridos.sample", "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, leak)
{
    do {
        GSubprocessLauncher *launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_INHERIT_FDS);
        ASSERT_NE(launcher, nullptr);
        g_object_unref(launcher);
    } while (0);
}

TEST(fetcher, leak1)
{
    do {
        GSubprocessLauncher *launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_INHERIT_FDS);
        ASSERT_NE(launcher, nullptr);
        GError *err = NULL;
        GSubprocess *process = g_subprocess_launcher_spawn(launcher, &err, "/usr/bin/true", NULL);
        ASSERT_EQ(process, nullptr);
        ASSERT_NE(err, nullptr);
        g_object_unref(launcher);
    } while (0);
}

TEST(fetcher, leak2)
{
    do {
        GRefPtr<GSubprocessLauncher> launcher = adoptGRef(g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_INHERIT_FDS));
        GUniqueOutPtr<GError> error;
        GRefPtr<GSubprocess> process;
        process = adoptGRef(g_subprocess_launcher_spawn(launcher.get(), &error.outPtr(), "/usr/bin/true", NULL));
    } while (0);
}


TEST(fetcher, leak3)
{
    do {
        String processPath = FileSystem::pathByAppendingComponent("/usr/bin", "processName");
    } while (0);
}

TEST(fetcher, leak4)
{
    do {
        WTF::URL url(WTF::URL(), "hello");
        url.setPath("world");
    } while (0);
}

// TEST(fetcher, leak3)
// {
//     do {
//         pcfetcher_init(100, 10240, true);
//         pcfetcher_term();
//     } while (0);
// }

