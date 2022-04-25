#include "purc.h"

#include "purc-rwstream.h"
#include "purc-utils.h"
#include "private/regex.h"
#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


TEST(regex, is_match)
{
    bool match = pcregex_is_match(".*", "abc");
    ASSERT_EQ(match, true);

    match = pcregex_is_match(".", "abc");
    ASSERT_EQ(match, true);
}

