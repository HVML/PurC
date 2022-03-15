#include "purc.h"

#include "private/fetcher.h"
#include "private/debug.h"
#include "private/runloop.h"

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

TEST(fetcher, cleanup)
{
    do {
        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_utils_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_UTILS,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_dom_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_DOM,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_html_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_HTML,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_xml_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_XML,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_variant_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_VARIANT,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_ejson_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_EJSON,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_xgml_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_XGML,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_hvml_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_HVML,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, init_pcrdr_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        int r = purc_init_ex (PURC_MODULE_PCRDR,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        if (r)
            break;

        purc_cleanup();
    } while (0);
}

TEST(fetcher, runloop)
{
    do {
        pcrunloop_init_main();
    } while (0);
}

TEST(fetcher, runloop_stop)
{
    do {
        pcrunloop_init_main();
        pcrunloop_t main_loop = pcrunloop_get_main();
        pcrunloop_stop(main_loop);
    } while (0);
}

TEST(fetcher, g_slice)
{
    gpointer p = g_slice_alloc0(128);
    ASSERT_NE(p, nullptr);
    g_slice_free1(128, p);
}

TEST(fetcher, launcher)
{
    // process-wide-`leakage`, to be confirmed
    do {
        GSubprocessLauncher *launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_INHERIT_FDS);
        ASSERT_NE(launcher, nullptr);
        // enough?
        g_object_unref(launcher);
    } while (0);
}

#if 0                        /* { */
TEST(fetcher, init_cleanup)
{
    do {
        purc_instance_extra_info info = {};
        int r = purc_init("cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

        if (r)
            break;

        purc_cleanup();
    } while (0);
}

TEST(fetcher, basic)
{
    if (1)
        return;

    do {
        purc_instance_extra_info info = {};
        purc_init_ex (PURC_MODULE_HVML,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

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
        // purc_instance_extra_info info = {};
        // purc_init_ex (PURC_MODULE_UTILS,"cn.fmsoft.hybridos.sample",
        //     "pcfetcher", &info);

        purc_cleanup();
    } while (0);
}

TEST(fetcher, leak)
{
    if (1)
        return;

    do {
        GSubprocessLauncher *launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_INHERIT_FDS);
        ASSERT_NE(launcher, nullptr);
        g_object_unref(launcher);
    } while (0);
}

TEST(fetcher, leak1)
{
    if (1)
        return;

    do {
        GSubprocessLauncher *launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_INHERIT_FDS);
        ASSERT_NE(launcher, nullptr);
        GError *err = NULL;
        GSubprocess *process = g_subprocess_launcher_spawn(launcher, &err, "/bin/true", NULL);
        ASSERT_NE(process, nullptr);
        ASSERT_EQ(err, nullptr);
        g_object_unref(launcher);
    } while (0);
}

TEST(fetcher, leak2)
{
    if (1)
        return;

    do {
        GRefPtr<GSubprocessLauncher> launcher = adoptGRef(g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_INHERIT_FDS));
        GUniqueOutPtr<GError> error;
        GRefPtr<GSubprocess> process;
        process = adoptGRef(g_subprocess_launcher_spawn(launcher.get(), &error.outPtr(), "/bin/true", NULL));
    } while (0);
}


TEST(fetcher, leak3)
{
    if (1)
        return;

    do {
        String processPath = FileSystem::pathByAppendingComponent("/usr/bin", "processName");
    } while (0);
}

TEST(fetcher, leak4)
{
    if (1)
        return;

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

#endif                       /* } */

