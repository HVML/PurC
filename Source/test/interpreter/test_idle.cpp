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


