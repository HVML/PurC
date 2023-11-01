/*
** Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#include "purc/purc-helpers.h"
#include "../helpers.h"

#include <glib.h>

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

TEST(test_split_page_identifier, split_page_identifier)
{
    char type[PURC_LEN_IDENTIFIER + 1];
    char name[PURC_LEN_IDENTIFIER + 1];
    char workspace[PURC_LEN_IDENTIFIER + 1];
    char group[PURC_LEN_IDENTIFIER + 1];

    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    static struct positvie_case {
        const char *page_id;
        const char *type;
        const char *name;
        const char *workspace;
        const char *group;
    } positive_cases[] = {
        { "null:", "null", "", "", "" },
        { "inherit:", "inherit", "", "", "" },
        { "self:", "self", "", "", "" },
        { "widget:name@workspace/group", "widget", "name", "workspace", "group" },
        { "plainwin:name@workspace/group", "plainwin", "name", "workspace", "group" },
        { "plainwin:name@group", "plainwin", "name", "", "group" },
        { "widget:name@group", "widget", "name", "", "group" },
        { "plainwin:name", "plainwin", "name", "", "" },
        { "widget:name", "widget", "name", "", "" },
    };

    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        int ret = purc_split_page_identifier(positive_cases[i].page_id,
                type, name, workspace, group);

        std::cout << "testing: " << positive_cases[i].page_id << std::endl;

        ASSERT_GE(ret, 0);
        ASSERT_STREQ(type, positive_cases[i].type);
        ASSERT_STREQ(name, positive_cases[i].name);
        ASSERT_STREQ(workspace, positive_cases[i].workspace);
        ASSERT_STREQ(group, positive_cases[i].group);
    }

    static const char *negative_cases[] = {
        "null",
        "345",
        "plainwin:",
        "widget:",
        "plainwin@group",
        "widget:name/group",
    };

    for (size_t i = 0; i < sizeof(negative_cases)/sizeof(negative_cases[0]); i++) {
        int ret = purc_split_page_identifier(negative_cases[i],
                type, name, workspace, group);
        ASSERT_LT(ret, 0);
    }
}

