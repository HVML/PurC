#include "purc.h"

#include "private/runloop.h"
#include "private/interpreter.h"

#include <gtest/gtest.h>


int on_idle_callback(void* ctxt)
{
    UNUSED_PARAM(ctxt);
    static int i = 0;
    if (i > 100000) {
        pcrunloop_stop(pcrunloop_get_current());
    }
    i++;
    return 0;
}

TEST(idle, idle)
{
    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    pcrunloop_set_idle_func(pcrunloop_get_current(), on_idle_callback, NULL);

    purc_run(PURC_VARIANT_INVALID, NULL);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}


