#include "purc.h"

#include "private/instance.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#define ATOM_BITS_NR        (sizeof(purc_atom_t) << 3)
#define BUCKET_BITS(bucket)       \
    ((purc_atom_t)bucket << (ATOM_BITS_NR - PURC_ATOM_BUCKET_BITS))

TEST(instance, mylog)
{
    // initial purc
    int ret = purc_init("cn.fmsoft.hvml.purc", "test", NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_atom_t endpoint_atom = BUCKET_BITS(PURC_ATOM_BUCKET_USER) | 1;
    const char *endpoint = purc_atom_to_string(endpoint_atom);

    char host_name[PURC_LEN_HOST_NAME + 1];
    purc_extract_host_name(endpoint, host_name);

    char app_name[PURC_LEN_APP_NAME + 1];
    purc_extract_app_name(endpoint, app_name);

    char runner_name[PURC_LEN_RUNNER_NAME + 1];
    purc_extract_runner_name(endpoint, runner_name);

    ASSERT_STREQ(host_name, "localhost");
    ASSERT_STREQ(app_name, "cn.fmsoft.hvml.purc");
    ASSERT_STREQ(runner_name, "test");

    purc_enable_log(true, false);

    purc_log_info("You will see this message in /var/tmp/purc-cn.fmsoft.hvml.purc-test.log marked INFO\n");
    purc_log_debug("You will see this message in /var/tmp/purc-cn.fmsoft.hvml.purc-test.log marked DEBUG\n");
    purc_log_warn("You will see this message in /var/tmp/purc-cn.fmsoft.hvml.purc-test.log marked WARN\n");
    purc_log_error("You will see this message in /var/tmp/purc-cn.fmsoft.hvml.purc-test.log marked ERROR\n");

    purc_enable_log(false, false);
    purc_log_debug("You will not see this message\n");

    purc_cleanup();
}

