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
    char            *file;
    char            *input_hvml;
    purc_variant_t   expected_result;
};

static void
sample_destroy(struct sample_data *sample)
{
    if (sample->file) {
        free(sample->file);
    }

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
    if (event == PURC_COND_COR_EXITED) {
        void *user_data = purc_coroutine_get_user_data(cor);
        if (!user_data) {
            return -1;
        }

        struct sample_data *sample = (struct sample_data*)user_data;

        struct purc_cor_exit_info *info = (struct purc_cor_exit_info *)data;
        if (!purc_variant_is_equal_to(sample->expected_result, info->result)) {
            char exe_result[1024];
            char exp_result[1024];
            purc_rwstream_t my_rws;

            my_rws = purc_rwstream_new_from_mem(exp_result,
                    sizeof(exp_result) - 1);
            size_t len_expected = 0;
            ssize_t n = purc_variant_serialize(sample->expected_result, my_rws,
                    0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
            exp_result[n] = 0;
            purc_rwstream_destroy(my_rws);

            my_rws = purc_rwstream_new_from_mem(exe_result,
                    sizeof(exe_result) - 1);
            if (info->result) {
                size_t len_expected = 0;
                ssize_t n = purc_variant_serialize(info->result, my_rws,
                        0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
                exe_result[n] = 0;

            }
            else {
                strcpy(exe_result, "INVALID VALUE");
            }
            purc_rwstream_destroy(my_rws);

            ADD_FAILURE()
                << sample->file << std::endl
                << "The execute result does not match to the expected result: "
                << std::endl
                << TCS_YELLOW
                << exe_result
                << TCS_NONE
                << " vs. "
                << TCS_YELLOW
                << exp_result
                << TCS_NONE
                << std::endl;

        }
        else {
            std::cout
                << TCS_GREEN
                << "Passed"
                << TCS_NONE
                << std::endl;
        }
    }
    else if (event == PURC_COND_COR_DESTROYED) {
        void *user_data = purc_coroutine_get_user_data(cor);
        if (!user_data) {
            return -1;
        }

        struct sample_data *sample = (struct sample_data*)user_data;

        sample_destroy(sample);
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
            << sample->file << std::endl;
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

static purc_variant_t eval_expected_result(const char *code)
{
    char *ejson = NULL;
    size_t ejson_len = 0;
    const char *line = code;

    while (*line == '#') {

        line++;

        // skip blank character: space or tab
        while (isblank(*line)) {
            line++;
        }

        if (strncmp(line, "RESULT:", strlen("RESULT:")) == 0) {
            line += strlen("RESULT:");

            const char *eol = line;
            while (*eol != '\n') {
                eol++;
            }

            ejson_len = eol - line;
            if (ejson_len > 0) {
                ejson = strndup(line, ejson_len);
            }

            break;
        }
        else {
            // skip left characters in the line
            while (*line != '\n') {
                line++;
            }
            line++;

            // skip blank character: space or tab
            while (isblank(*line) || *line == '\n') {
                line++;
            }
        }
    }

    purc_variant_t result;
    if (ejson) {
        result = purc_variant_make_from_json_string(ejson, ejson_len);
    }
    else {
        result = purc_variant_make_undefined();
    }

    if (ejson)
        free(ejson);

    /* purc_log_debug("result type: %s\n",
            purc_variant_typename(purc_variant_get_type(result))); */

    return result;
}

static int
process_file(const char *file)
{
    std::cout << std::endl << "Running " << file << std::endl;

    char buf[8192];
    int n = read_file(buf, sizeof(buf), file);
    if (n == -1)
        return -1;

    struct sample_data *sample =
        (struct sample_data *)calloc(1, sizeof(*sample));

    sample->file = strdup(file);
    sample->input_hvml = strdup(buf);
    sample->expected_result = eval_expected_result(buf);

    return add_sample(sample);
}

static void go_test(const char *files)
{
    PurCInstance purc(false);

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
                purc_run((purc_cond_handler)my_cond_handler);
            }
            if (r)
                break;
        } while (0);
        globfree(&globbuf);
    }

    // std::cerr << "env: " << env << "=" << path << std::endl;
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


