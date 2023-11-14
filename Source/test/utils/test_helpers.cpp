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
        std::cout << "testing: " << positive_cases[i].page_id << std::endl;

        int ret = purc_split_page_identifier(positive_cases[i].page_id,
                type, name, workspace, group);

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

TEST(test_window_styles, window_styles)
{
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    static const struct purc_screen_info screen = { 1920, 1280, 96, 1 };

    static struct positvie_case {
        const char *styles;
        struct purc_window_geometry geometry;
    } positive_cases[] = {
        { "", { 0, 0, 1920, 1280 } },   // default geometry: full-screen.
        { "window-size:screen", { 0, 0, 1920, 1280 } },
        { "window-size:square", { 0, 0, 1280, 1280 } },
        { "window-size:50\% auto", { 0, 0, 960, 1280 } },
        { "window-size:50\% 50%", { 0, 0, 960, 640 } },
        { "window-size:50\%", { 0, 0, 960, 1280 } },
        { "window-size:50\% 450px", { 0, 0, 960, 450 } },
        { "window-size:aspect-ratio 1 1", { 0, 0, 1280, 1280 } },
        { "window-size:aspect-ratio 4 3", { 0, 0, 1707, 1280 } },
        { "window-size:aspect-ratio 3 4", { 0, 0, 960, 1280 } },
        { "window-size:aspect-ratio 2 1", { 0, 0, 1920, 960 } },
        { "window-size:50% 50%; window-position:top", { 480, 0, 960, 640 } },
        { "window-size:50% 50%; window-position:left", { 0, 320, 960, 640 } },
        { "window-size:200% 200%; window-position:center", { -960, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position:right", { -1920, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position:bottom", { -960, -1280, 3840, 2560 } },
        { "window-size:200% 200%; window-position:50% 50%", { -960, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position:0 0;", { 0, 0, 3840, 2560 } },
        { "window-size:200% 200%; window-position:left 50%", { 0, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position:right 50%", { -1920, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position: top 50%", { -960, 0, 3840, 2560 } },
        { "window-size:200% 200%; window-position: 50\% bottom", { -960, -1280, 3840, 2560 } },
        { "window-size:200% 200%; window-position: left top 50px", { 50, 50, 3840, 2560 } },
        { "window-size:200% 200%; window-position: left 50px center", { 50, -640, 3840, 2560 } },
        { "window-size:200% 200%; window-position: left 10px top 20px", { 10, 20, 3840, 2560 } },
        { "window-size:200% 200%; window-position: left -10px top -20px", { -10, -20, 3840, 2560 } },
        { "window-size:200% 200%; window-position: center -10px center -20px", { -970, -660, 3840, 2560 } },
    };

    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        struct purc_window_geometry geometry;

        std::cout << "testing: " << positive_cases[i].styles << std::endl;

        int ret = purc_evaluate_standalone_window_geometry_from_styles(
                positive_cases[i].styles, &screen, &geometry);

        ASSERT_EQ(ret, 0);
        ASSERT_EQ(geometry.x, positive_cases[i].geometry.x);
        ASSERT_EQ(geometry.y, positive_cases[i].geometry.y);
        ASSERT_EQ(geometry.width, positive_cases[i].geometry.width);
        ASSERT_EQ(geometry.height, positive_cases[i].geometry.height);
    }
}

TEST(test_transition_styles, transition_style)
{
    purc_enable_log_ex(PURC_LOG_MASK_ALL, PURC_LOG_FACILITY_STDOUT);

    static struct positvie_case {
        int ret;
        const char *styles;
        purc_window_transition_function move_func;
        uint32_t move_duration;
    } positive_cases[] = {
        { 0, "", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: none 100", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 100 },
        { -1, "window-transition-move: linear -1", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: linear 100", PURC_WINDOW_TRANSITION_FUNCTION_LINEAR, 100 },
        { 0, "window-transition-move: linear 0", PURC_WINDOW_TRANSITION_FUNCTION_LINEAR, 0 },
        { 0, "window-transition-move: linear 99;", PURC_WINDOW_TRANSITION_FUNCTION_LINEAR, 99 },
        { 0, "window-transition-move: linear 99  aabb;", PURC_WINDOW_TRANSITION_FUNCTION_LINEAR, 99 },
        { -1, "window-transition-move: linear -1", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: InQuad 100", PURC_WINDOW_TRANSITION_FUNCTION_INQUAD, 100 },
        { 0, "window-transition-move: InQuad 0", PURC_WINDOW_TRANSITION_FUNCTION_INQUAD, 0 },
        { 0, "window-transition-move: InQuad 99;", PURC_WINDOW_TRANSITION_FUNCTION_INQUAD, 99 },
        { 0, "window-transition-move: InQuad 99  aabb;", PURC_WINDOW_TRANSITION_FUNCTION_INQUAD, 99 },
        { -1, "window-transition-move: OutQuad -1", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: OutQuad 100", PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD, 100 },
        { 0, "window-transition-move: OutQuad 0", PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD, 0 },
        { 0, "window-transition-move: OutQuad 99;", PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD, 99 },
        { 0, "window-transition-move: OutQuad 99  aabb;", PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD, 99 },
        { -1, "window-transition-move: InOutQuad -1", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
        { 0, "window-transition-move: InOutQuad 100", PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD, 100 },
        { 0, "window-transition-move: InOutQuad 0", PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD, 0 },
        { 0, "window-transition-move: InOutQuad 99;", PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD, 99 },
        { 0, "window-transition-move: InOutQuad 99  aabb;", PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD, 99 },
        { 0, "window-transition-move: ppp aabb;", PURC_WINDOW_TRANSITION_FUNCTION_NONE, 0 },
    };

    for (size_t i = 0; i < sizeof(positive_cases)/sizeof(positive_cases[0]); i++) {
        struct purc_window_transition transition;

        std::cout << "testing: " << positive_cases[i].styles << std::endl;

        int ret = purc_evaluate_standalone_window_transition_from_styles(
                positive_cases[i].styles, &transition);

        ASSERT_EQ(ret, positive_cases[i].ret);
        ASSERT_EQ(transition.move_func, positive_cases[i].move_func);
        ASSERT_EQ(transition.move_duration, positive_cases[i].move_duration);
    }
}
