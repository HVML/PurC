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

#include "private/hvml.h"
#include "private/utils.h"
#include "purc-rwstream.h"
#include "purc-variant.h"
#include "hvml/hvml-token.h"
#include "private/dvobjs.h"

#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gtest/gtest.h>

using namespace std;

char *trim(char *str)
{
    if (!str) {
        return NULL;
    }
    char *end;

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if(*str == 0) {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';
    return str;
}

TEST(runner, simple)
{
    const char *env = "SIMPLE_RUNNER_TEST_PATH";
    char data_path[PATH_MAX + 1] =  {0};
    char file_path[PATH_MAX + 1] = {0};
    char file[PATH_MAX+16] = {0};
    char *line = NULL;
    size_t sz = 0;
    ssize_t read = 0;

    PurCInstance purc;

    test_getpath_from_env_or_rel(data_path, sizeof(data_path), env,
            "simple_runner");

    strcpy(file_path, data_path);
    strcat(file_path, "/");
    strcat(file_path, "simple_runner.hvmls");

    FILE *fp = fopen(file_path, "r");
    ASSERT_NE(fp, nullptr);

    int nr_loaded = 0;
    while ((read = getline(&line, &sz, fp)) != -1) {
        if (line && line[0] != '#') {
            char *name = strtok(trim(line), " ");
            if (!name)
                continue;

            int n;
            n = snprintf(file, sizeof(file), "%s/%s.hvml", data_path, name);
            EXPECT_TRUE(n >= 0 && (size_t)n < sizeof(file));
            if (!(n >= 0 && (size_t)n < sizeof(file)))
                break;

            purc_vdom_t vdom = purc_load_hvml_from_file (file);
            purc_schedule_vdom_null(vdom);
            EXPECT_NE(vdom, nullptr) << file << std::endl;
            if (!vdom)
                break;

            ++nr_loaded;
        }
    }
    free (line);
    fclose (fp);

    if (nr_loaded)
        purc_run(NULL);
}

