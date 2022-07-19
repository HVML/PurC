/*
 * @file test_comprehensive_programs.cpp
 * @author Vincent Wei
 * @date 2022/07/19
 * @brief The program to test comprehensive programs including exit result,
 *      coroutines, and concurrently calls.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#undef NDEBUG

#include "purc.h"
#include "private/utils.h"
#include "private/interpreter.h"

#include "../helpers.h"
#include "tools.h"

#include <glob.h>
#include <gtest/gtest.h>

struct sample_data {
    char            *input_hvml;
    purc_variant_t   expected_result;
};

static void
sample_destroy(struct sample_data *sample)
{
    if (sample->input_hvml) {
        free(sample->input_hvml);
    }

    if (sample->expected_result) {
        purc_variant_unref(sample->expected_result);
    }

    free(sample);
}

static int my_cond_handler(purc_cond_t event, purc_coroutine_t cor,
        void *data)
{
    void *user_data = purc_coroutine_get_user_data(cor);
    if (!user_data) {
        return -1;
    }

    struct sample_data *sd = (struct sample_data*)user_data;

    if (event == PURC_COND_COR_EXITED) {
        /* TODO: check result here */
        (void)data;
    }
    else if (event == PURC_COND_COR_DESTROYED) {
        sample_destroy(sd);
    }

    return 0;
}

static int
add_sample(struct sample_data *sample)
{
    purc_vdom_t vdom;
    vdom = purc_load_hvml_from_string(sample->input_hvml);

    if (vdom == NULL) {
        ADD_FAILURE()
            << "failed to loading hvml:" << std::endl
            << sample->input_hvml << std::endl;
        sample_destroy(sample);
        return -1;
    }
    else {
        purc_coroutine_t cor = purc_schedule_vdom_null(vdom);
        purc_coroutine_set_user_data(cor, sample);
    }

    return 0;
}

static int
read_file(char *buf, size_t nr, const char *file)
{
    FILE *f = fopen(file, "r");
    if (!f) {
        ADD_FAILURE()
            << "Failed to open file [" << file << "]" << std::endl;
        return -1;
    }

    size_t n = fread(buf, 1, nr, f);
    if (ferror(f)) {
        ADD_FAILURE()
            << "Failed read file [" << file << "]" << std::endl;
        fclose(f);
        return -1;
    }

    fclose(f);

    if (n == sizeof(buf)) {
        ADD_FAILURE()
            << "Too small buffer to read file [" << file << "]" << std::endl;
        return -1;
    }

    buf[n] = '\0';

    return n;
}

static int
process_file(const char *file)
{
    std::cout << file << std::endl;
    char buf[8192];
    int n = read_file(buf, sizeof(buf), file);
    if (n == -1)
        return -1;

    struct sample_data *sample =
        (struct sample_data *)calloc(1, sizeof(*sample));
    sample->input_hvml = strdup(buf);

    return add_sample(sample);
}

static void go_test(const char *files)
{
    PurCInstance purc(false);
    purc_bind_runner_variables();

    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    char path[PATH_MAX+1];
    const char *env = "SOURCE_FILES";
    const char *rel = files;
    test_getpath_from_env_or_rel(path, sizeof(path),
        env, rel);

    if (!path[0]) {
        ADD_FAILURE()
            << "internal logic error" << std::endl;
    }
    else {
        globbuf.gl_offs = 0;
        r = glob(path, GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
        do {
            if (r) {
                ADD_FAILURE()
                    << "Failed to globbing @["
                    << path << "]: [" << errno << "]" << strerror(errno)
                    << std::endl;
                break;
            }
            if (globbuf.gl_pathc == 0)
                break;

            for (size_t i=0; i<globbuf.gl_pathc; ++i) {
                r = process_file(globbuf.gl_pathv[i]);
                if (r)
                    break;
            }
            if (r)
                break;
            purc_run((purc_cond_handler)my_cond_handler);
        } while (0);
        globfree(&globbuf);
    }

    std::cerr << "env: " << env << "=" << path << std::endl;
}

TEST(comp_hvml, basic)
{
    go_test("comp/0*.hvml");
}

TEST(comp_hvml, load)
{
    go_test("comp/1*.hvml");
}


TEST(comp_hvml, call)
{
    go_test("comp/2*.hvml");
}


