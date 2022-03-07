#include "purc.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#define ATOM_BITS_NR        (sizeof(purc_atom_t) << 3)
#define BUCKET_BITS(bucket)       \
    ((purc_atom_t)bucket << (ATOM_BITS_NR - PURC_ATOM_BUCKET_BITS))

TEST(instance, def)
{
    int ret = purc_init(NULL, NULL, NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_atom_t endpoint_atom = BUCKET_BITS(PURC_ATOM_BUCKET_USER) | 1;
    const char *endpoint = purc_atom_to_string(endpoint_atom);

    char host_name[PURC_LEN_HOST_NAME + 1];
    purc_extract_host_name(endpoint, host_name);

    char runner_name[PURC_LEN_RUNNER_NAME + 1];
    purc_extract_runner_name(endpoint, runner_name);

    printf("Endpoint: %s\n", endpoint);

    ASSERT_STREQ(host_name, "localhost");
    ASSERT_STREQ(runner_name, "unknown");

    purc_log_debug("You will not see this message\n");

    purc_cleanup();
}

