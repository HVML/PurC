/*
 * @file purc.c
 * @author XueShuming, Vincent Wei
 * @date 2022/03/07
 * @brief A standalone HVML interpreter/debugger based-on PurC.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "purc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <assert.h>

#define KEY_APP_NAME            "app"
#define DEF_APP_NAME            "cn.fmsoft.html.purc"

#define KEY_RUN_NAME            "run"
#define DEF_RUN_NAME            "main"

#define KEY_DATA_FETCHER        "data-fetcher"
#define DEF_DATA_FETCHER        "local"

#define KEY_RDR_PROTOCOL        "rdr-prot"
#define DEF_RDR_PROTOCOL        "headless"

#define KEY_RDR_URI             "rdr-uri"
#define DEF_RDR_URI_HEADLESS    "file:///dev/null"
#define DEF_RDR_URI_PURCMC      ("unix://" PCRDR_PURCMC_US_PATH)

#define KEY_URLS               "urls"

#define KEY_FLAG_QUIET          "quiet"

struct purc_run_info {
    purc_variant_t opts;
    purc_variant_t app_info;
};

static struct purc_run_info run_info;

static void print_version(FILE *fp)
{
    fputs("purc " PURC_VERSION_STRING "\n", fp);
}

static void print_short_copying(FILE *fp)
{
    fputs(
        "Copyright (C) 2022 FMSoft Technologies.\n"
        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n",
        fp);
}

static void print_long_copying(FILE *fp)
{
    fputs(
        "Copyright (C) 2022 FMSoft Technologies.\n"
        "\n"
        "This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as\n"
        "published by the Free Software Foundation, either version 3 of the\n"
        "License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public\n"
        "License along with this program. If not, see "
        "<https://www.gnu.org/licenses/>.\n",
        fp);
}

/* Command line help. */
static void print_usage(FILE *fp)
{
    fputs(
        "purc (" PURC_VERSION_STRING ") - "
        "a standalone HVML interpreter/debugger based-on PurC.\n",
        fp);

    print_short_copying(fp);

    fputs("\n", fp);

    fputs(
        "Usage: purc [ options ... ] [ file | url ] ... | [ app_desc_json | app_desc_ejson ]\n"
        "\n"
        "The following options can be supplied to the command:\n"
        "\n"
        "  -a --app=<app_name>\n"
        "        Run with the specified app name (default value is `cn.fmsoft.html.purc`).\n"
        "\n"
        "  -r --runner=<runner_name>\n"
        "        Run with the specified runner name (default value is `main`).\n"
        "\n"
        "  -d --data-fetcher=< local | remote >\n"
        "        The data fetcher; use `local` or `remote`.\n"
        "            - `local`: use the built-in data fetcher, and only `file://` URIs\n"
        "               supported.\n"
        "            - `remote`: use the remote data fetcher to support more URI schemas,\n"
        "               such as `http`, `https`, `ftp` and so on.\n"
        "\n"
        "  -p --rdr-prot=< headless | purcmc >\n"
        "        The renderer protocol; use `headless` (default) or `purcmc`.\n"
        "            - `headless`: use the built-in HEADLESS renderer.\n"
        "            - `purcmc`: use the remote PURCMC renderer;\n"
        "              `purc` connects to the renderer via Unix Socket or WebSocket.\n"

        "  -u --rdr-uri=<renderer_uri>\n"
        "        The renderer uri:\n"
        "            - For the renderer protocol `headleass`,\n"
        "              default value is not specified (nil).\n"
        "            - For the renderer protocol `purcmc`,\n"
        "              default value is `unix:///var/tmp/purcmc.sock`.\n"
        "\n"
        "  -q --quiet\n"
        "        Execute the program quietly (without redundant output).\n"
        "\n"
        "  -c --copying\n"
        "        Display detailed copying information and exit.\n"
        "\n"
        "  -v --version\n"
        "        Display version information and exit.\n"
        "\n"
        "  -h --help\n"
        "        This help.\n",
        fp);
}

struct my_opts {
    char *app;
    char *run;
    const char *data_fetcher;
    const char *rdr_prot;
    char *rdr_uri;
    pcutils_array_t *urls;
    char *app_info;

    bool quiet;
};

static struct my_opts *my_opts_new(void)
{
    struct my_opts *opts = calloc(1, sizeof(*opts));
    opts->urls = pcutils_array_create();
    pcutils_array_init(opts->urls, 1);
    return opts;
}

static void my_opts_delete(struct my_opts *opts, bool deep)
{
    if (deep) {
        if (opts->app)
            free(opts->app);
        if (opts->run)
            free(opts->run);

        for (size_t i = 0; i < opts->urls->length; i++) {
            free(opts->urls->list[i]);
        }
    }

    if (opts->app_info)
        free(opts->app_info);

    pcutils_array_destroy(opts->urls, true);

    free(opts);
}

static bool is_json_or_ejson_file(const char *file)
{
    const char *suffix;

    if ((suffix = strrchr(file, '.'))) {
        if (strcmp(suffix, ".json") == 0 ||
                strcmp(suffix, ".ejson") == 0) {

            FILE *fp = fopen(file, "r");
            if (fp == NULL) {
                goto done;
            }

            fclose(fp);

            return true;
        }
    }

done:
    return false;
}

static bool validate_url(struct my_opts *opts, const char *url)
{
    if (pcutils_url_is_valid(url)) {
        char *my_url = strdup(url);
        pcutils_array_push(opts->urls, my_url);
    }
    else {
        FILE *fp = fopen(url, "r");
        if (fp == NULL) {
            return false;
        }

        fclose(fp);

        char *my_url = calloc(1, strlen("file://") + strlen(url) + 1);
        strcpy(my_url, "file://");
        strcat(my_url, url);
        pcutils_array_push(opts->urls, my_url);
    }

    return true;
}

static int read_option_args(struct my_opts *opts, int argc, char **argv)
{
    static const char short_options[] = "a:r:d:p:u:qcvh";
    static const struct option long_opts[] = {
        { "app"            , required_argument , NULL , 'a' },
        { "runner"         , required_argument , NULL , 'r' },
        { "data-fetcher"   , required_argument , NULL , 'd' },
        { "rdr-prot"       , required_argument , NULL , 'p' },
        { "rdr-uri"        , required_argument , NULL , 'u' },
        { "quiet"          , no_argument       , NULL , 'q' },
        { "copying"        , no_argument       , NULL , 'c' },
        { "version"        , no_argument       , NULL , 'v' },
        { "help"           , no_argument       , NULL , 'h' },
        { 0, 0, 0, 0 }
    };

    int o, idx = 0;
    if (argc == 1) {
        print_usage(stdout);
        return -1;
    }

    while ((o = getopt_long(argc, argv, short_options, long_opts, &idx)) >= 0) {
        if (-1 == o || EOF == o)
            break;
        switch (o) {
        case 'h':
            print_usage(stdout);
            return -1;

        case 'v':
            print_version(stdout);
            return -1;

        case 'c':
            print_version(stdout);
            print_long_copying(stdout);
            return -1;

        case 'a':
            if (purc_is_valid_app_name(optarg)) {
                opts->app = strdup(optarg);
            }
            else
                goto bad_arg;
            break;

        case 'r':
            if (purc_is_valid_runner_name(optarg)) {
                opts->run = strdup(optarg);
            }
            else
                goto bad_arg;
            break;

        case 'd':
            if (strcmp(optarg, "none") == 0) {
                opts->data_fetcher = "none";
            }
            else if (strcmp(optarg, "local") == 0) {
                opts->data_fetcher = "local";
            }
            else if (strcmp(optarg, "remote") == 0) {
                opts->data_fetcher = "remote";
            }
            else {
                goto bad_arg;
            }

            break;

        case 'p':
            if (strcmp(optarg, "headless") == 0) {
                opts->rdr_prot = "headless";
            }
            else if (strcmp(optarg, "purcmc") == 0) {
                opts->rdr_prot = "purcmc";
            }
            else {
                goto bad_arg;
            }

            break;

        case 'u':
            if (pcutils_url_is_valid(optarg)) {
                opts->rdr_uri = strdup(optarg);
            }
            else {
                goto bad_arg;
            }

            break;

        case 'q':
            opts->quiet = true;
            break;

        case '?':
            break;

        default:
            return -1;
        }
    }

    if (optind < argc) {
        if (is_json_or_ejson_file(argv[optind])) {
            opts->app_info = strdup(argv[optind]);
        }
        else {
            for (int i = optind; i < argc; i++) {
                if (!validate_url(opts, argv[i])) {
                    if (!opts->quiet)
                        fprintf(stdout, "Got a bad file or URL: %s\n", argv[i]);
                    return -1;
                }
            }
        }
    }

    return 0;

bad_arg:
    if (!opts->quiet)
        fprintf(stdout, "Got an unknown argument: %s (%c)\n", optarg, o);
    return -1;
}

static void transfer_opts_to_variant(struct my_opts *opts)
{
    run_info.opts = purc_variant_make_object_0();
    purc_variant_t tmp;

    // set default options
    if (opts->app) {
        tmp = purc_variant_make_string_reuse_buff(opts->app,
                strlen(opts->app) + 1, false);
    }
    else {
        tmp = purc_variant_make_string_static(DEF_APP_NAME, false);
    }
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_APP_NAME, tmp);
    purc_variant_unref(tmp);

    if (opts->run) {
        tmp = purc_variant_make_string_reuse_buff(opts->run,
                strlen(opts->run) + 1, false);
    }
    else {
        tmp = purc_variant_make_string_static(DEF_RUN_NAME, false);
    }
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_RUN_NAME, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_string_static(opts->data_fetcher, false);
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_DATA_FETCHER, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_string_static(opts->rdr_prot, false);
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_RDR_PROTOCOL, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_string_reuse_buff(opts->rdr_uri,
            strlen(opts->rdr_uri) + 1, false);
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_RDR_URI, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_boolean(opts->quiet);
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_FLAG_QUIET, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_array_0();
    for (size_t i = 0; i < opts->urls->length; i++) {
        char *url = opts->urls->list[i];
        purc_variant_t url_vrt = purc_variant_make_string_reuse_buff(url,
                strlen(url) + 1, false);
        purc_variant_array_append(tmp, url_vrt);
        purc_variant_unref(url_vrt);
    }
    purc_variant_object_set_by_static_ckey(run_info.opts, KEY_URLS, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_boolean(opts->quiet);
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_FLAG_QUIET, tmp);
    purc_variant_unref(tmp);
}

static ssize_t cb_stdio_write(void *ctxt, const void *buf, size_t count)
{
    FILE *fp = ctxt;
    return fwrite(buf, 1, count, fp);
}

static bool evalute_app_info(const char *app_info)
{
    (void)app_info;
    return false;
}

int main(int argc, char** argv)
{
    int ret;

    struct my_opts *opts = my_opts_new();
    if (read_option_args(opts, argc, argv)) {
        my_opts_delete(opts, true);
        return EXIT_FAILURE;
    }

    if (opts->app_info == NULL &&
            (opts->urls == NULL || opts->urls->length == 0)) {
        if (!opts->quiet) {
            fprintf(stdout, "No valid HVML program specified\n");
            print_usage(stdout);
        }

        my_opts_delete(opts, true);
        return EXIT_FAILURE;
    }

    if (!opts->quiet) {
        print_version(stdout);
        print_short_copying(stdout);
    }

    unsigned int modules = 0;
    if (opts->data_fetcher == NULL ||
            strcmp(opts->data_fetcher, "local") == 0) {
        opts->data_fetcher = "local";
        modules = (PURC_MODULE_HVML | PURC_MODULE_PCRDR);
    }
    else if (strcmp(opts->data_fetcher, "remote") == 0) {
        modules = (PURC_MODULE_HVML | PURC_MODULE_PCRDR) | PURC_HAVE_FETCHER_R;
    }
    else if (strcmp(opts->data_fetcher, "none") == 0) {
        modules = (PURC_MODULE_HVML | PURC_MODULE_PCRDR) & ~PURC_HAVE_FETCHER;
    }

    purc_instance_extra_info extra_info = {};

    if (opts->rdr_prot == NULL || strcmp(opts->rdr_prot, "headless") == 0) {
        opts->rdr_prot = "headless";
        extra_info.renderer_prot = PURC_RDRPROT_HEADLESS;

        if (opts->rdr_uri == NULL) {
            opts->rdr_uri = strdup(DEF_RDR_URI_HEADLESS);
        }
    }
    else {
        assert(strcmp(opts->rdr_prot, "purcmc") == 0);

        extra_info.renderer_prot = PURC_RDRPROT_PURCMC;
        if (opts->rdr_uri == NULL) {
            opts->rdr_uri = strdup(DEF_RDR_URI_PURCMC);
        }
    }

    ret = purc_init_ex(modules, opts->app ? opts->app : DEF_APP_NAME,
            opts->run ? opts->run : DEF_RUN_NAME, &extra_info);
    if (ret != PURC_ERROR_OK) {
        if (!opts->quiet)
            fprintf(stderr, "Failed to initialize the PurC instance: %s\n",
                purc_get_error_message(ret));
        return EXIT_FAILURE;
    }

    transfer_opts_to_variant(opts);
    if (opts->app_info) {
        if (!evalute_app_info(opts->app_info)) {
            if (!opts->quiet)
                fprintf(stderr, "Failed to evalute the app info from %s\n",
                        opts->app_info);
            return EXIT_FAILURE;
        }
    }

    my_opts_delete(opts, false);

    purc_rwstream_t rws = purc_rwstream_new_for_dump(stdout, cb_stdio_write);
    purc_variant_serialize(run_info.opts, rws, 0,
            PCVARIANT_SERIALIZE_OPT_PRETTY |
            PCVARIANT_SERIALIZE_OPT_NOSLASHESCAPE, NULL);
    purc_rwstream_destroy(rws);

#if 0
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

    pcrdr_page_type type = PCRDR_PAGE_TYPE_PLAINWIN;
    purc_renderer_extra_info rdr_extra_info = {
        .title = DEF_PAGE_TITLE
    };
    ret = purc_attach_vdom_to_renderer(vdom,
            type, DEF_WORKSPACE_ID,
            DEF_PAGE_GROUP, DEF_PAGE_NAME, &rdr_extra_info);
    if (!ret) {
        fprintf(stderr, "Failed to attach renderer : %s\n",
                purc_get_error_message(purc_get_last_error()));
        goto failed;
    }

    purc_run(NULL);

failed:
    if (run_info.doc_content)
        free (run_info.doc_content);
#endif

    purc_variant_unref(run_info.opts);
    purc_cleanup();

    return EXIT_SUCCESS;
}
