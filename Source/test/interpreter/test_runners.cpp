/*
 * @file test-runners.c
 * @author Vincent Wei
 * @date 2022/07/07
 * @brief The program to test the following APIs:
 *      - purc_inst_create_or_get()
 *      - purc_inst_schedule_vdom()
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
#include "../helpers.h"

#include <gtest/gtest.h>

static struct purc_instance_extra_info worker_info = {
    PURC_RDRPROT_HEADLESS,
    "file:///tmp/" APP_NAME ".log",
    "sslCert",
    "sslKey",
    "workspaceName",
    "workspaceTitle",
    "<html></html>",            // workspaceLayout
};

static int work_cond_handler(purc_cond_t event, void *arg, void *data)
{
    purc_log_info("called: %d\n", event);

    if (event == PURC_COND_STARTED) {
        purc_atom_t sid = (purc_atom_t)(uintptr_t)arg;
        purc_instance_extra_info *info = (purc_instance_extra_info *)data;

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
        assert(strcmp(run_name, "worker") == 0);

        assert(info->renderer_prot == worker_info.renderer_prot);
        assert(strcmp(info->renderer_uri, worker_info.renderer_uri) == 0);
        assert(strcmp(info->ssl_cert, worker_info.ssl_cert) == 0);
        assert(strcmp(info->ssl_key, worker_info.ssl_key) == 0);
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
        assert(strcmp(run_name, "worker") == 0);
    }
    else if (event == PURC_COND_SHUTDOWN_ASKED) {
        return 0;
    }

    return 0;
}

static const char *hvml = "<hvml><body><sleep for 2s /></body></hvml>";
static const char *request_json = "{ names: 'PurC', OS: ['Linux', 'macOS', 'HybridOS', 'Windows'] }";
static const char *toolkit_style_json = "{ 'darkMode': true, 'backgroudColor': { 'r': 0, 'g': 0, 'b': 0 } }";

TEST(interpreter, runners)
{
    struct purc_instance_extra_info inst_info = { };
    inst_info.renderer_prot = PURC_RDRPROT_HEADLESS;
    inst_info.workspace_name = "main";

    PurCInstance purc(PURC_MODULE_HVML, APP_NAME, "main", &inst_info);
    ASSERT_TRUE(purc);

    purc_variant_t request =
        purc_variant_make_from_json_string(request_json,
                strlen(request_json));
    ASSERT_NE(request, nullptr);

    purc_variant_t toolkit_style =
        purc_variant_make_from_json_string(toolkit_style_json,
                strlen(toolkit_style_json));
    ASSERT_NE(toolkit_style, nullptr);

    purc_atom_t work_inst = purc_inst_create_or_get(APP_NAME,
            "worker", work_cond_handler, &worker_info);
    ASSERT_NE(work_inst, 0);

    purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
    ASSERT_NE(vdom, nullptr);

    purc_renderer_extra_info rdr_info = {};
    rdr_info.title = "def_page_title";
    purc_coroutine_t co = purc_schedule_vdom(vdom,
            0, request, PCRDR_PAGE_TYPE_NULL,
            "main",   /* target_workspace */
            NULL,     /* target_group */
            NULL,     /* page_name */
            &rdr_info, NULL, NULL);
    ASSERT_NE(co, nullptr);

    struct purc_renderer_extra_info worker_rdr_info =
    {
        "my class",
        "my title",
        "my layoutStyle",
        toolkit_style,
        "<section></section>",
    };

    purc_atom_t worker_cor = purc_inst_schedule_vdom(work_inst, vdom,
            purc_coroutine_identifier(co), request, PCRDR_PAGE_TYPE_NULL,
            "main",
            NULL,
            "my page name",
            &worker_rdr_info, NULL);
    ASSERT_NE(worker_cor, 0);

    purc_run(NULL);

    purc_inst_ask_to_shutdown(work_inst);

    unsigned int seconds = 0;
    while (purc_atom_to_string(work_inst) && seconds < 10) {
        purc_log_info("Wait for the termination of worker instance...\n");
        sleep(1);
        seconds++;
    }

    purc_variant_unref(request);
    purc_variant_unref(toolkit_style);
}

