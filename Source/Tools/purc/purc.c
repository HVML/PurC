/*
 * @file purc.c
 * @author XueShuming
 * @date 2022/03/07
 * @brief A standalone HVML interpreter/debugger based-on PurC.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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


#include "purc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>

#define LEN_TARGET_NAME     10
#define LEN_DATA_FETCHER    10

#define DEF_WORKSPACE_ID       "purc_def_workspace"
#define DEF_WINDOW_ID          "purc_def_window"

struct purc_run_info {
    char app_name[PURC_LEN_APP_NAME + 1];
    char runner_name[PURC_LEN_RUNNER_NAME + 1];
    char target_name[LEN_TARGET_NAME + 1];          // rdr
    char data_fetcher[LEN_DATA_FETCHER + 1];          // data fetcher

    char *doc_content;
    char url[PATH_MAX + 1];
};

struct purc_run_info run_info;

static void print_copying(void)
{
    fprintf (stdout,
        "\n"
        "PurC - A standalone HVML interpreter/debugger based-on PurC.\n"
        "\n"
        "This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU Lesser General Public License as\n"
        "published by the Free Software Foundation, either version 3 of the\n"
        "License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU Lesser General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU Lesser General Public\n"
        "License along with this program. If not, see "
        "<https://www.gnu.org/licenses/>.\n"
        );
    fprintf (stdout, "\n");
}

/* Command line help. */
static void print_usage(void)
{
    printf ("PurC (%s) - "
            "a standalone HVML interpreter/debugger based-on PurC.\n\n",
            PURC_VERSION_STRING);

    printf (
            "Usage: "
            "purc [ options ... ]\n\n"
            ""
            "The following options can be supplied to the command:\n\n"
            ""
            "  -a --app=<app_name>          - Run with the specified app name.\n"
            "  -r --runner=<runner_name>    - Run with the specified runner name.\n"
            "  -f --file=<hvml_file>        - The initial HVML file to load.\n"
            "  -u --url=<hvml_url>          - The initial HVML file to load.\n"
            "  -t --target=<renderer_name>  - The renderer name.\n"
            "  -d --data-fetcher=<fetcher>  - The data fetcher(none, local, remote).\n"
            "  -v --version                 - Display version information and exit.\n"
            "  -h --help                    - This help.\n"
            "\n"
            );
}

static char *load_doc_content(const char *file)
{
    FILE *f = fopen(file, "r");
    char *buf = NULL;

    if (f) {
        if (fseek(f, 0, SEEK_END))
            goto failed;

        long len = ftell(f);
        if (len < 0)
            goto failed;

        buf = malloc(len + 1);
        if (buf == NULL)
            goto failed;

        fseek(f, 0, SEEK_SET);
        if (fread(buf, 1, len, f) < (size_t)len) {
            free(buf);
            buf = NULL;
        }
        buf[len] = '\0';

failed:
        fclose(f);
    }

    return buf;
}

static char short_options[] = "a:r:f:u:t:d:vh";
static struct option long_opts[] = {
    {"app"            , required_argument , NULL , 'a' } ,
    {"runner"         , required_argument , NULL , 'r' } ,
    {"file"           , required_argument , NULL , 'f' } ,
    {"url"            , required_argument , NULL , 'u' } ,
    {"target"         , required_argument , NULL , 't' } ,
    {"data-fetcher"   , required_argument , NULL , 'd' } ,
    {"version"        , no_argument       , NULL , 'v' } ,
    {"help"           , no_argument       , NULL , 'h' } ,
    {0, 0, 0, 0}
};

static int read_option_args (int argc, char **argv)
{
    int o, idx = 0;
    if (argc == 1) {
        print_usage ();
        return -1;
    }

    while ((o = getopt_long (argc, argv, short_options, long_opts, &idx)) >= 0) {
        if (-1 == o || EOF == o)
            break;
        switch (o) {
        case 'h':
            print_usage ();
            return -1;
        case 'v':
            fprintf (stdout, "PurC: %s\n", PURC_VERSION_STRING);
            return -1;
        case 'a':
            if (strlen (optarg) < PURC_LEN_APP_NAME)
                strcpy (run_info.app_name, optarg);
            break;
        case 'r':
            if (strlen (optarg) < PURC_LEN_RUNNER_NAME)
                strcpy (run_info.runner_name, optarg);
            break;
        case 'f':
            run_info.doc_content = load_doc_content(optarg);
            if (run_info.doc_content == NULL) {
                return -1;
            }
            break;
        case 'u':
            if (strlen (optarg) < PATH_MAX)
                strcpy (run_info.url, optarg);
            break;
        case 't':
            if (strlen (optarg) < LEN_TARGET_NAME)
                strcpy (run_info.target_name, optarg);
            break;
        case 'd':
            if (strlen (optarg) < LEN_DATA_FETCHER)
                strcpy (run_info.data_fetcher, optarg);
            break;
        case '?':
            print_usage ();
            return -1;
        default:
            return -1;
        }
    }

    if (optind < argc) {
        print_usage ();
        return -1;
    }

    return 0;
}

int main(int argc, char** argv)
{
    int ret;
    print_copying();
    if (read_option_args (argc, argv)) {
        return EXIT_FAILURE;
    }

    purc_instance_extra_info extra_info = {};

    unsigned int modules = 0;
    if (!run_info.data_fetcher[0] ||
            strcmp(run_info.data_fetcher, "local") == 0) {
        modules = (PURC_MODULE_HVML | PURC_MODULE_PCRDR);
    }
    else if (strcmp(run_info.data_fetcher, "remote") == 0) {
        modules = (PURC_MODULE_HVML | PURC_MODULE_PCRDR) | PURC_HAVE_FETCHER_R;
    }
    else if (strcmp(run_info.data_fetcher, "none") == 0) {
        modules = (PURC_MODULE_HVML | PURC_MODULE_PCRDR) & ~PURC_HAVE_FETCHER;
    }

    if (!run_info.app_name[0]) {
        strcpy(run_info.app_name, "cn.fmsoft.hybridos.purc");
    }

    if (!run_info.runner_name[0]) {
        strcpy(run_info.runner_name, "purc");
    }

    if (strcmp(run_info.target_name, "purcmc") == 0) {
        extra_info.renderer_prot = PURC_RDRPROT_PURCMC;
        extra_info.renderer_uri = "unix://" PCRDR_PURCMC_US_PATH;
    };

    ret = purc_init_ex(modules, run_info.app_name, run_info.runner_name,
            &extra_info);
    if (ret != PURC_ERROR_OK) {
        fprintf(stderr, "Failed to initialize the PurC instance: %s\n",
                purc_get_error_message (ret));
        return EXIT_FAILURE;
    }


    purc_vdom_t vdom = NULL;
    if (run_info.doc_content) {
        vdom = purc_load_hvml_from_string(run_info.doc_content);
    }
    else if (run_info.url[0]) {
        vdom = purc_load_hvml_from_url(run_info.url);
    }
    else {
        goto failed;
    }

    if (!vdom) {
        fprintf(stderr, "Failed to load hvml : %s\n",
                purc_get_error_message(purc_get_last_error()));
        goto failed;
    }

    purc_renderer_extra_info rdr_extra_info = {};
    ret = purc_attach_vdom_to_renderer(vdom,
            DEF_WORKSPACE_ID,
            DEF_WINDOW_ID,
            NULL,
            NULL,
            &rdr_extra_info);
    if (!ret) {
        fprintf(stderr, "Failed to attach renderer : %s\n",
                purc_get_error_message(purc_get_last_error()));
        goto failed;
    }

    purc_run(PURC_VARIANT_INVALID, NULL);

failed:
    if (run_info.doc_content)
        free (run_info.doc_content);

    purc_cleanup();

    return EXIT_SUCCESS;
}
