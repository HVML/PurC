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

    match = pcregex_is_match("\\d", "abc");
    ASSERT_EQ(match, false);

    match = pcregex_is_match("\\d", "1");
    ASSERT_EQ(match, true);

    match = pcregex_is_match("\\d", "a1b");
    ASSERT_EQ(match, true);
}

TEST(regex, match)
{
    const char *str = "abc";
    const char *pattern = "\\d";

    struct pcregex *regex = pcregex_new(pattern);
    ASSERT_NE(regex, nullptr);

    bool ret = pcregex_match(regex, str, NULL);
    ASSERT_EQ(ret, false);

    str = "a123";
    ret = pcregex_match(regex, str, NULL);
    ASSERT_EQ(ret, true);

    str = "1ab";
    ret = pcregex_match(regex, str, NULL);
    ASSERT_EQ(ret, true);

    pcregex_destroy(regex);
}
