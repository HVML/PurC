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


#include "purc/purc.h"

#include "helpers.h"
#include "tools.h"

#include <glob.h>
#include <gtest/gtest.h>
#include <string.h>

struct comp_sample_data {
    char            *file;
    char            *input_hvml;
    purc_variant_t   expected_result;
};


char *
intr_util_dump_doc(purc_document_t doc, size_t *len)
{
    char *retp = NULL;
    unsigned opt = 0;

    opt |= PCDOC_SERIALIZE_OPT_UNDEF;
    opt |= PCDOC_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCDOC_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCDOC_SERIALIZE_OPT_FULL_DOCTYPE;

    purc_rwstream_t stm = NULL;
    stm = purc_rwstream_new_buffer(0, 8192);
    if (stm == NULL)
        goto failed;

    if (purc_document_serialize_contents_to_stream(doc, opt, stm))
        goto failed;

    retp = (char *)purc_rwstream_get_mem_buffer_ex(stm, len, NULL, true);

failed:
    if (stm)
        purc_rwstream_destroy(stm);

    return retp;
}

char *
intr_util_comp_docs(purc_document_t doc_l, purc_document_t doc_r, int *diff)
{
    char *retp = NULL;
    unsigned opt = 0;
    size_t len_l, len_r;
    char *pl;
    char *pr;

    opt |= PCDOC_SERIALIZE_OPT_UNDEF;
    opt |= PCDOC_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCDOC_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCDOC_SERIALIZE_OPT_FULL_DOCTYPE;

    purc_rwstream_t stm_l = NULL, stm_r = NULL;
    stm_l = purc_rwstream_new_buffer(0, 8192);
    stm_r = purc_rwstream_new_buffer(0, 8192);
    if (stm_l == NULL || stm_r == NULL)
        goto failed;

    if (purc_document_serialize_contents_to_stream(doc_l, opt, stm_l) ||
            purc_document_serialize_contents_to_stream(doc_r, opt, stm_r))
        goto failed;

    pl = (char *)purc_rwstream_get_mem_buffer_ex(stm_l, &len_l, NULL, true);
    pr = (char *)purc_rwstream_get_mem_buffer_ex(stm_r, &len_r, NULL, true);

    if (pl && pr) {
        *diff = strcmp(pl, pr);
        if (*diff) {
            purc_log_debug("diff:\n%s\n%s", pl, pr);
        }
        retp = pl;
    }
    free(pr);
    // free(pl);

failed:
    if (stm_l)
        purc_rwstream_destroy(stm_l);
    if (stm_r)
        purc_rwstream_destroy(stm_r);

    return retp;
}

static void
comp_sample_destroy(struct comp_sample_data *sample)
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

static ssize_t
cb_stdio_write(void *ctxt, const void *buf, size_t count)
{
    FILE *fp = (FILE *)ctxt;
    return fwrite(buf, 1, count, fp);
}

static const char *cond_names[] = {
    "PURC_COND_STARTED",
    "PURC_COND_STOPPED",
    "PURC_COND_NOCOR",
    "PURC_COND_IDLE",
    "PURC_COND_COR_CREATED",
    "PURC_COND_COR_ONE_RUN",
    "PURC_COND_COR_EXITED",
    "PURC_COND_COR_TERMINATED",
    "PURC_COND_COR_DESTROYED",
    "PURC_COND_UNK_REQUEST",
    "PURC_COND_UNK_EVENT",
    "PURC_COND_SHUTDOWN_ASKED",
};

int client_cond_handler(purc_cond_k event, void *arg, void *data)
{
    (void)data;

    if (event == PURC_COND_STARTED) {
        purc_atom_t sid = (purc_atom_t)(uintptr_t)arg;
        // purc_instance_extra_info *info = (purc_instance_extra_info *)data;

        const char *endpoint = purc_atom_to_string(sid);
        assert(endpoint);

        char host_name[PURC_LEN_HOST_NAME + 1];
        purc_extract_host_name(endpoint, host_name);
        assert(strcmp(host_name, PCRDR_LOCALHOST) == 0);

        char app_name[PURC_LEN_APP_NAME + 1];
        purc_extract_app_name(endpoint, app_name);
        assert(strcmp(app_name, APP_NAME) == 0);

        char run_name[PURC_LEN_RUNNER_NAME + 1];
        purc_extract_runner_name(endpoint, run_name);
        assert(strncmp(run_name, "client", 6) == 0);
    }
    else if (event == PURC_COND_STOPPED) {
        purc_atom_t sid = (purc_atom_t)(uintptr_t)arg;
        assert(sid != 0);

        const char *endpoint = purc_atom_to_string(sid);
        assert(endpoint);

        char host_name[PURC_LEN_HOST_NAME + 1];
        purc_extract_host_name(endpoint, host_name);
        assert(strcmp(host_name, PCRDR_LOCALHOST) == 0);

        char app_name[PURC_LEN_APP_NAME + 1];
        purc_extract_app_name(endpoint, app_name);
        assert(strcmp(app_name, APP_NAME) == 0);

        char run_name[PURC_LEN_RUNNER_NAME + 1];
        purc_extract_runner_name(endpoint, run_name);
        assert(strncmp(run_name, "client", 6) == 0);
    }
    else if (event == PURC_COND_SHUTDOWN_ASKED) {
        purc_log_info("condition: %s\n", cond_names[event]);
    }
    else if (event == PURC_COND_COR_TERMINATED) {

        purc_coroutine_t cor = (purc_coroutine_t)arg;
        struct purc_cor_term_info *info = (struct purc_cor_term_info *)data;
        purc_atom_t cid = purc_coroutine_identifier(cor);

        fprintf(stdout,
                "A coroutine (%s) in client instance terminated due to `%s`.\n",
                purc_atom_to_string(cid), purc_atom_to_string(info->except));

        purc_rwstream_t dump_stm;
        dump_stm = purc_rwstream_new_for_dump(stdout, cb_stdio_write);
        fprintf(stdout, ">> The executing stack frame(s):\n");
        purc_coroutine_dump_stack(cor, dump_stm);
        fprintf(stdout, "\n");
        purc_rwstream_destroy(dump_stm);
    }

    return 0;
}

static int
comp_cond_handler(purc_cond_k event, purc_coroutine_t cor,
        void *data)
{
    if (event == PURC_COND_COR_ONE_RUN) {
        struct purc_cor_run_info *info = (struct purc_cor_run_info *)data;
        if (info->run_idx == 0) {
            uintptr_t p = 0;

            purc_get_local_data(FN_AFTER_FIRST_RUN, &p, NULL);
            after_first_run_fn after_first_run = (after_first_run_fn)p;

            if (after_first_run) {
                purc_log_info("Going to call after_first_run()\n");
                after_first_run(cor, info);
            }
        }
    }
    else if (event == PURC_COND_COR_EXITED) {
        void *user_data = purc_coroutine_get_user_data(cor);
        if (!user_data) {
            return -1;
        }

        struct comp_sample_data *sample = (struct comp_sample_data*)user_data;

        struct purc_cor_exit_info *info = (struct purc_cor_exit_info *)data;
        if (!purc_variant_is_equal_to(sample->expected_result, info->result)) {
            char exe_result[1024];
            char exp_result[1024];
            purc_rwstream_t my_rws;

            my_rws = purc_rwstream_new_from_mem(exp_result,
                    sizeof(exp_result) - 1);
            size_t len_expected = 0;
            ssize_t n = purc_variant_serialize(sample->expected_result, my_rws,
                    0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
            exp_result[n] = 0;
            purc_rwstream_destroy(my_rws);

            my_rws = purc_rwstream_new_from_mem(exe_result,
                    sizeof(exe_result) - 1);
            if (info->result) {
                size_t len_expected = 0;
                ssize_t n = purc_variant_serialize(info->result, my_rws,
                        0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
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
    else if (event == PURC_COND_COR_TERMINATED) {
        void *user_data = purc_coroutine_get_user_data(cor);
        if (!user_data) {
            return -1;
        }

        struct comp_sample_data *sample = (struct comp_sample_data*)user_data;

        struct purc_cor_term_info *info = (struct purc_cor_term_info *)data;
        ADD_FAILURE()
            << sample->file << std::endl
            << "The coroutine terminated due to an exception: "
            << TCS_YELLOW
            << purc_atom_to_string(info->except)
            << TCS_NONE
            << std::endl;

            purc_rwstream_t dump_stm;
            dump_stm = purc_rwstream_new_for_dump(stdout, cb_stdio_write);
            fprintf(stdout, ">> The executing stack frame(s):\n");
            purc_coroutine_dump_stack(cor, dump_stm);
            fprintf(stdout, "\n");
            purc_rwstream_destroy(dump_stm);
    }
    else if (event == PURC_COND_COR_DESTROYED) {
        void *user_data = purc_coroutine_get_user_data(cor);
        if (!user_data) {
            return -1;
        }

        struct comp_sample_data *sample = (struct comp_sample_data*)user_data;

        comp_sample_destroy(sample);
    }

    return 0;
}

static int
comp_add_sample(struct comp_sample_data *sample, purc_variant_t request)
{
    purc_vdom_t vdom;
    vdom = purc_load_hvml_from_string(sample->input_hvml);

    if (vdom == NULL) {
        ADD_FAILURE()
            << TCS_RED
            << "Errors when loading HVML program: "
            << sample->file
            << TCS_NONE
            << std::endl;
        comp_sample_destroy(sample);
        return -1;
    }
    else {
        purc_coroutine_t cor = purc_schedule_vdom(vdom, 0, request,
            PCRDR_PAGE_TYPE_NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        purc_coroutine_set_user_data(cor, sample);
    }

    return 0;
}

static purc_variant_t
comp_eval_expected_result(const char *code)
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
comp_process_file(const char *file, purc_variant_t request)
{
    std::cout << std::endl << "Running " << file << std::endl;

    size_t n;
    char *buf = purc_load_file_contents(file, &n);
    if (buf == NULL)
        return -1;

    struct comp_sample_data *sample =
        (struct comp_sample_data *)calloc(1, sizeof(*sample));

    sample->file = strdup(file);
    sample->input_hvml = buf;
    sample->expected_result = comp_eval_expected_result(buf);

    return comp_add_sample(sample, request);
}

#ifndef PATH_MAX
#   define PATH_MAX 4096
#endif

void
go_comp_test(const char *files)
{
    PurCInstance purc(false);

    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    char path[PATH_MAX];
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
                int ret = comp_process_file(globbuf.gl_pathv[i], NULL);
                if (ret == 0)
                    purc_run((purc_cond_handler)comp_cond_handler);
            }
        } while (0);
        globfree(&globbuf);
    }

    // std::cerr << "env: " << env << "=" << path << std::endl;
}

void run_one_comp_test(const char *file, const char *query)
{
    char path[PATH_MAX];
    const char *env = "SOURCE_FILES";
    const char *rel = file;
    test_getpath_from_env_or_rel(path, sizeof(path),
        env, rel);

    purc_variant_t request = PURC_VARIANT_INVALID;
    if (query)
        request = purc_make_object_from_query_string(query, false);

    int ret = comp_process_file(path, request);
    if (ret == 0)
        purc_run((purc_cond_handler)comp_cond_handler);

    if (request)
        purc_variant_unref(request);
}


char *make_query_with_base(const char *format)
{
    char path[PATH_MAX];
    const char *env = "SOURCE_FILES";
    const char *rel = "renderer/hvml/";
    test_getpath_from_env_or_rel(path, sizeof(path), env, rel);
    char *base = purc_url_encode_alloc(path, true);
    assert(base);

    char *query;
    asprintf(&query, format, base);
    free(base);

    return query;
}

