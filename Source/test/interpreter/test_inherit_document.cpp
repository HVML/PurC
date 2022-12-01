/*
 * @file test_inherit_document.cpp
 * @author Vincent Wei
 * @date 2022/07/22
 * @brief The program to test inherit document of the curator.
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

#include "purc/purc.h"
#include "private/utils.h"
#include "private/interpreter.h"

#include "../helpers.h"
#include "tools.h"

#include <glob.h>
#include <gtest/gtest.h>

#define SAMPLE_DATA_NAME    "sample-data"

struct sample_ctxt {
    unsigned        nr_crtns;
    purc_variant_t  args;
    purc_variant_t  expected;
};

static void
sample_destroy(struct sample_ctxt *cd)
{
    if (cd->args)
        purc_variant_unref(cd->args);
    if (cd->expected)
        purc_variant_unref(cd->expected);

    free(cd);
}

static purc_variant_t
get_dvobj(void* ctxt, const char* name)
{
    struct sample_ctxt *ud = (struct sample_ctxt *)ctxt;
    if (strcmp(name, "ARGS") == 0)
        return ud->args;

    return purc_get_runner_variable(name);
}

static bool
generate_expected_document(const char *html, struct sample_ctxt *ud)
{
    struct purc_ejson_parsing_tree *ptree;

    ptree = purc_variant_ejson_parse_string(html, strlen(html));
    if (ptree) {
        ud->expected = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj, ud, true);
        purc_ejson_parsing_tree_destroy(ptree);

#if 0
        if (purc_variant_is_string(ud->expected)) {
            purc_log_info("The exepected HTML template: %s\n", html);
            purc_log_info("The exepected HTML: %s\n",
                    purc_variant_get_string_const(ud->expected));
        }
#endif

        return true;
    }
    else {
        purc_log_error("failed purc_variant_ejson_parse_string(): %s\n",
                purc_get_error_message(purc_get_last_error()));
    }

    return false;
}

static int
add_sample(const char *hvml, const char *html_template)
{
    struct sample_ctxt *cd;

    cd = (struct sample_ctxt*)calloc(1, sizeof(*cd));
    assert(cd);

    purc_vdom_t vdom;
    if ((vdom = purc_load_hvml_from_string(hvml)) == NULL) {
        ADD_FAILURE()
            << "failed purc_load_hvml_from_string()" << std::endl;
        sample_destroy(cd);
        return -1;
    }

    purc_coroutine_t parent = purc_schedule_vdom_null(vdom);
    purc_coroutine_set_user_data(parent, cd);
    purc_atom_t pcid = purc_coroutine_identifier(parent);

    purc_coroutine_t child = purc_schedule_vdom(vdom,
            pcid, PURC_VARIANT_INVALID, PCRDR_PAGE_TYPE_INHERIT,
            NULL, NULL, NULL, NULL, NULL, NULL);
    purc_coroutine_set_user_data(child, cd);

    purc_atom_t ccid = purc_coroutine_identifier(child);

    cd->args = purc_variant_make_object_0();
    purc_variant_t tmp;
    tmp = purc_variant_make_ulongint(pcid);
    purc_variant_object_set_by_static_ckey(cd->args, "pcid", tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_ulongint(ccid);
    purc_variant_object_set_by_static_ckey(cd->args, "ccid", tmp);
    purc_variant_unref(tmp);

    if (!generate_expected_document(html_template, cd)) {
        ADD_FAILURE()
            << "failed generate_expected_document()" << std::endl;
        sample_destroy(cd);
        return -1;
    }

    cd->nr_crtns = 2;
    purc_set_local_data(SAMPLE_DATA_NAME, (uintptr_t)cd, NULL);
    return 0;
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

static int
my_cond_handler(purc_cond_t event, void *arg, void *data)
{
    (void)arg;
    (void)data;

    purc_log_info("condition: %s\n", cond_names[event]);

    struct sample_ctxt *cd;
    purc_get_local_data(SAMPLE_DATA_NAME, (uintptr_t *)(void *)&cd, NULL);
    if (cd == NULL) {
        purc_log_error("failed purc_get_local_data");
        return 0;
    }

    if (event == PURC_COND_COR_CREATED) {
        purc_log_info("New coroutine created\n");
        cd->nr_crtns++;
    }
    else if (event == PURC_COND_COR_EXITED) {
        cd->nr_crtns--;

        if (cd->nr_crtns == 0) {
            purc_log_info("All coroutines exited\n");

            struct purc_cor_exit_info *info =
                (struct purc_cor_exit_info *)data;
            purc_document_t doc = info->doc;

            if (cd->expected) {
                const char *html;
                size_t sz;

                html = purc_variant_get_string_const_ex(cd->expected, &sz);
                purc_document_t doc_expected;
                doc_expected = purc_document_load(PCDOC_K_TYPE_HTML, html, sz);

                int diff = 0;
                char *ctnt = intr_util_comp_docs(doc, doc_expected, &diff);
                purc_document_delete(doc_expected);
                if (ctnt != NULL && diff == 0) {
                    free(ctnt);
                    return 0;
                }

                ADD_FAILURE()
                    << "The generated document does not match to the expected document:" << std::endl
                    << std::endl
                    << "generated:" << std::endl
                    << TCS_YELLOW << ctnt << TCS_NONE
                    << std::endl
                    << "expected:" << std::endl
                    << TCS_YELLOW << html << TCS_NONE
                    << std::endl;

                free(ctnt);
            }
        }
    }
    else if (event == PURC_COND_COR_DESTROYED) {
        if (cd->nr_crtns == 0) {
            purc_remove_local_data(SAMPLE_DATA_NAME);
            sample_destroy(cd);
        }
    }

    return 0;
}

static const char *hello_hvml = ""
"<!DOCTYPE hvml>"
"<hvml target='html'>"
    "<head>"
        "<title>Hello, world!</title>"
    "</head>"
    "<body>"
        "<ul>"
            "<iterate on 0 onlyif $L.lt($0<, 10) with $DATA.arith('+', $0<, 1) nosetotail >"
                "<li>$? Hello, world! -- from COROUTINE-$CRTN.cid</li>"
            "</iterate>"
        "</ul>"
    "</body>"
"</hvml>";

static const char *html_template = ""
"\"<html>"
    "<head>"
        "<title>Hello, world!</title>"
        "<title>Hello, world!</title>"
    "</head>"
    "<body>"
        "<ul>"
            "<li>0 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>1 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>2 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>3 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>4 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>5 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>6 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>7 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>8 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
            "<li>9 Hello, world! -- from COROUTINE-$ARGS.pcid</li>"
        "</ul>"
        "<ul>"
            "<li>0 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>1 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>2 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>3 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>4 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>5 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>6 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>7 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>8 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
            "<li>9 Hello, world! -- from COROUTINE-$ARGS.ccid</li>"
        "</ul>"
    "</body>"
"</html>\"";

TEST(inhert_doc, hello)
{
    PurCInstance purc(false);

    add_sample(hello_hvml, html_template);
    purc_run(my_cond_handler);
}

