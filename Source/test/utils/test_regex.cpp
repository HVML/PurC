/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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

    match = pcregex_is_match(NULL, "a1b");
    ASSERT_EQ(match, false);

    match = pcregex_is_match("\\d", NULL);
    ASSERT_EQ(match, false);

    match = pcregex_is_match("abc", "abc");
    ASSERT_EQ(match, true);

    match = pcregex_is_match("abc", "abc");
    ASSERT_EQ(match, true);

    match = pcregex_is_match("...", "abc");
    ASSERT_EQ(match, true);

    match = pcregex_is_match("^[A-Za-z_][A-Za-z0-9_]*$", "123a");
    ASSERT_EQ(match, false);

    match = pcregex_is_match("^[A-Za-z_][A-Za-z0-9_]*$", "a123a");
    ASSERT_EQ(match, true);

    match = pcregex_is_match("^[A-Za-z_][A-Za-z0-9_]*$", "A123a");
    ASSERT_EQ(match, true);

    match = pcregex_is_match("^[A-Za-z_][A-Za-z0-9_]*$", "_A123a");
    ASSERT_EQ(match, true);

    match = pcregex_is_match("^[A-Za-z_][A-Za-z0-9_]*$", "a123a-");
    ASSERT_EQ(match, false);

    match = pcregex_is_match("^[A-Za-z_][A-Za-z0-9_]*$", "a123a-%");
    ASSERT_EQ(match, false);

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

TEST(regex, match_info)
{
    const char *str = "abc def xyz";
    const char *pattern = "[a-z]+";
    struct pcregex_match_info *infos = NULL;

    struct pcregex *regex = pcregex_new(pattern);
    ASSERT_NE(regex, nullptr);

    bool ret = pcregex_match(regex, str, &infos);
    ASSERT_EQ(ret, true);

    ret = pcregex_match_info_matches(infos);
    ASSERT_EQ(ret, true);

    char *word = pcregex_match_info_fetch(infos, 0);
    ASSERT_STREQ(word, "abc");
    free(word);
    word = NULL;

    ret = pcregex_match_info_next(infos);
    ASSERT_EQ(ret, true);

    word = pcregex_match_info_fetch(infos, 0);
    ASSERT_STREQ(word, "def");
    free(word);
    word = NULL;

    ret = pcregex_match_info_next(infos);
    ASSERT_EQ(ret, true);

    word = pcregex_match_info_fetch(infos, 0);
    ASSERT_STREQ(word, "xyz");
    free(word);
    word = NULL;

    pcregex_match_info_destroy(infos);
    pcregex_destroy(regex);
}

