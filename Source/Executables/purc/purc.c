/*
 * @file purc.c
 * @author XueShuming, Vincent Wei
 * @date 2022/03/07
 * @brief A standalone HVML interpreter/debugger based on PurC.
 *
 * Copyright (C) 2021 ~ 2023 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
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

// #undef NDEBUG

#include <purc/purc.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "config.h"
#include "foil.h"
#include "seeker.h"

#define KEY_APP_NAME            "app"
#define DEF_APP_NAME            "cn.fmsoft.hvml.purc"

#define KEY_RUN_NAME            "runner"

#define KEY_DATA_FETCHER        "dataFetcher"
#define DEF_DATA_FETCHER        "local"

#define KEY_RDR_PROTOCOL        "rdrProt"
#define DEF_RDR_PROTOCOL        "headless"

#define KEY_RDR_URI             "rdrUri"
#define DEF_RDR_URI_HEADLESS    "file:///dev/null"
#define DEF_RDR_URI_SOCKET      "unix://" PCRDR_PURCMC_US_PATH
#define DEF_RDR_URI_WEBSOCKET   "ws://localhost:" PCRDR_PURCMC_WS_PORT

#define KEY_FLAG_REQUEST        "request"

#define KEY_URLS                "urls"
#define KEY_BODYIDS             "bodyIds"

#define KEY_FLAG_PARALLEL       "parallel"
#define KEY_FLAG_VERBOSE        "verbose"

struct run_info {
    purc_variant_t opts;
    purc_variant_t app_info;
    purc_rwstream_t dump_stm;
};

static struct run_info run_info;

static void print_version(FILE *fp)
{
    fputs("purc " PURC_VERSION_STRING "\n", fp);
}

static void print_short_copying(FILE *fp)
{
    fputs(
        "Copyright (C) 2022, 2023 FMSoft Technologies.\n"
        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n",
        fp);
}

static void print_long_copying(FILE *fp)
{
    fputs(
        "Copyright (C) 2022, 2023 FMSoft Technologies.\n"
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
        "a standalone HVML interpreter/debugger based on PurC.\n",
        fp);

    print_short_copying(fp);

    fputs("\n", fp);

    fputs(
        "Usage: purc [ options ... ] [ file | url ] ... | [ app_desc_json | app_desc_ejson ]\n"
        "\n"
        "The following options can be supplied to the command:\n"
        "\n"
        "  -a --app=< app_name >\n"
        "        Run with the specified app name (default: `cn.fmsoft.hvml.purc`).\n"
        "\n"
        "  -r --runner=< runner_name >\n"
        "        Run with the specified runner name (default: the md5sum of the URL of first HVML program).\n"
        "\n"
        "  -d --data-fetcher=< local | remote >\n"
        "        The data fetcher; use `local` or `remote`.\n"
        "            - `local`: use the built-in data fetcher, and only `file://` URLs\n"
        "               supported.\n"
        "            - `remote`: use the remote data fetcher to support more URL schemas,\n"
        "               such as `http`, `https`, `ftp` and so on.\n"
        "\n"
        "  -c --rdr-comm=< headless | thread | socket | websocket>\n"
        "        The renderer commnunication method; use `headless` (default), `thread`, or `socket`.\n"
        "            - `headless`: use the built-in headless renderer.\n"
        "            - `thread`: use the built-in thread-based renderer.\n"
        "            - `socket`: use the remote UNIX domain socket-based renderer;\n"
        "            - `websocket`: use the remote websocket-based renderer;\n"
        "              `purc` will connect to the renderer via Unix Socket or WebSocket.\n"
        "\n"
        "  -u --rdr-uri=< renderer_uri >\n"
        "        The renderer uri or shortname:\n"
        "            - For the renderer comm method `headless`,\n"
        "              the default value is `file:///dev/null`.\n"
        "            - For the renderer comm method `thread`,\n"
        "              the default value is the first available one:\n"
        "              `foil` if Foil is enabled, otherwise `seeker`.\n"
        "            - For the renderer comm method `socket`,\n"
        "              the default value is `unix:///var/tmp/purcmc.sock`.\n"
        "            - For the renderer comm method `websocket`,\n"
        "              the default value is `ws://localhost:7702`.\n"
        "\n"
        "  -j --request=< json_file | - >\n"
        "        The JSON file contains the request data which will be passed to\n"
        "        the HVML programs; use `-` if the JSON data will be given through\n"
        "        STDIN stream. (Ctrl+D for end of input after you input the JSON data in a terminal.)\n"
        "\n"
        "  -q --query=< query_string >\n"
        "        Use a URL query string (in RFC 3986) for the request data which will be passed to \n"
        "        the HVML programs; e.g., --query='case=displayBlock&lang=zh'.\n"
        "\n"
        "  -P --pageid\n"
        "        The page identifier for the HVML programs which do not run in parallel.\n"
        "\n"
        "  -L --layout-style\n"
        "        The layout style for the HVML programs which do not run in parallel.\n"
        "        This option is only valid if the page type is `plainwin` or `widget`.\n"
        "\n"
        "  -T --toolkit-style\n"
        "        The toolkit style for the HVML programs which do not run in parallel.\n"
        "        This option is only valid if the page type is `plainwin` or `widget`.\n"
        "\n"
        "  -A --transition-style\n"
        "        The transition style for the HVML programs which do not run in parallel.\n"
        "        This option is only valid if the page type is `plainwin`.\n"
        "\n"
        "  -s --allow-switching-rdr=< true | false >\n"
        "        Allow switching renderer.\n"
        "\n"
        "  -l --parallel\n"
        "        Execute multiple programs in parallel.\n"
        "\n"
        "  -v --verbose\n"
        "        Execute the program(s) with verbose output.\n"
        "\n"
        "  -C --copying\n"
        "        Display detailed copying information and exit.\n"
        "\n"
        "  -V --version\n"
        "        Display version information and exit.\n"
        "\n"
        "  -h --help\n"
        "        This help.\n"
        "\n"
        "(root only options)\n"
        "  -R --chroot <directory>\n"
        "       Change root to the specified directory\n"
        "       (default is the `/app/<app_name>/`)\n"
        "\n"
        "  -U --setuser <user>\n"
        "      Set user identity to the user specified\n"
        "       (default is the user named <app_name> if it exists).\n"
        "\n"
        "  -G --setgroup <group>\n"
        "      Set group identity to the group specified\n"
        "       (default is the group named <app_name> if it exists>).\n"
        "\n",
        fp);
}

struct my_opts {
    char *app;
    char *run;
    const char *data_fetcher;
    const char *rdr_prot;
    const char *pageid;
    const char *layout_style;
    const char *toolkit_style;
    const char *transition_style;
    const char *allow_switching_rdr;
    const char *chroot;
    const char *setuser;
    const char *setgroup;

    char *rdr_uri;
    char *request;
    char *query;

    pcutils_array_t *urls;
    pcutils_array_t *body_ids;
    pcutils_array_t *contents;
    char *app_info;

    bool parallel;
    bool verbose;
};

static const char *archedata_header =
"{"
    "'app': $OPTS.app,"
    "'runners': ["
        "{"
            "'runner': $OPTS.runner,"
            "'renderer': { 'commMethod': $OPTS.rdrComm, 'uri': $OPTS.rdrUri, "
                "'workspaceName': 'default' },"
            "'coroutines': [";

static const char *archedata_coroutine =
                "{ 'url': $OPTS.urls[%u], 'bodyId': $OPTS.bodyIds[%u],"
                    "'request': $OPTS.request,"
                    "'renderer': { 'pageType': 'plainwin', 'pageName': 'win%u' }"
                "},";

static const char *archedata_footer =
            "]"
        "},"
    "]"
"}";

static bool construct_app_info(struct my_opts *opts)
{
    assert(opts->app_info == NULL);
    assert(opts->urls->length > 0);

    size_t length = strlen(archedata_header) + strlen(archedata_footer);
    length += strlen(archedata_coroutine) * opts->urls->length;
    length += 16 * opts->urls->length;
    length += 1;

    char *app_info = malloc(length);
    strcpy(app_info, archedata_header);

    for (size_t i = 0; i < opts->urls->length; i++) {
        char buff[256];
        int n = snprintf(buff, sizeof(buff), archedata_coroutine,
                (unsigned)i, (unsigned)i, (unsigned)i);
        if (n < 0 || (size_t)n > sizeof(buff)) {
            return false;
        }

        strcat(app_info, buff);
    }

    strcat(app_info, archedata_footer);
    opts->app_info = app_info;
    return true;
}

static struct my_opts *my_opts_new(void)
{
    struct my_opts *opts = calloc(1, sizeof(*opts));

    opts->urls = pcutils_array_create();
    pcutils_array_init(opts->urls, 1);

    opts->body_ids = pcutils_array_create();
    pcutils_array_init(opts->body_ids, 1);

    opts->contents = pcutils_array_create();
    pcutils_array_init(opts->contents, 1);
    return opts;
}

static void my_opts_delete(struct my_opts *opts)
{
    if (opts->app)
        free(opts->app);
    if (opts->run)
        free(opts->run);

    for (size_t i = 0; i < opts->urls->length; i++) {
        if (opts->urls->list[i])
            free(opts->urls->list[i]);
    }

    for (size_t i = 0; i < opts->body_ids->length; i++) {
        if (opts->body_ids->list[i])
            free(opts->body_ids->list[i]);
    }

    for (size_t i = 0; i < opts->contents->length; i++) {
        if (opts->contents->list[i])
            free(opts->contents->list[i]);
    }

    if (opts->rdr_uri)
        free(opts->rdr_uri);

    if (opts->request)
        free(opts->request);

    if (opts->query)
        free(opts->query);

    if (opts->app_info)
        free(opts->app_info);

    pcutils_array_destroy(opts->urls, true);
    pcutils_array_destroy(opts->body_ids, true);
    pcutils_array_destroy(opts->contents, true);

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
    struct purc_broken_down_url broken_down;

    memset(&broken_down, 0, sizeof(broken_down));
    if (pcutils_url_break_down(&broken_down, url)) {
        char *my_url = strdup(url);
        pcutils_array_push(opts->urls, my_url);

        char* my_body_id;
        if (broken_down.fragment) {
            my_body_id = strdup(broken_down.fragment);
        }
        else
            my_body_id = NULL;

        pcutils_array_push(opts->body_ids, my_body_id);

        pcutils_broken_down_url_clear(&broken_down);
    }
    else {
        /* try if it is a path name */
        FILE *fp = fopen(url, "r");
        if (fp == NULL) {
            return false;
        }

        fclose(fp);

        char *path = NULL;
        if (url[0] != '/') {
            path = getcwd(NULL, 0);
            if (path == NULL) {
                return false;
            }
        }

        char *my_url = calloc(1, strlen("file://") +
                (path ? strlen(path) : 0) + strlen(url) + 2);
        strcpy(my_url, "file://");
        if (path) {
            strcat(my_url, path);
            strcat(my_url, "/");
            free(path);
        }
        strcat(my_url, url);

        pcutils_array_push(opts->urls, my_url);
        pcutils_array_push(opts->body_ids, NULL);
    }

    return true;
}

static int read_option_args(struct my_opts *opts, int argc, char **argv)
{
    static const char short_options[] = "a:r:d:c:u:j:q:P:L:T:A:s:R:U:G:lvCVh";
    static const struct option long_opts[] = {
        { "app"                  , required_argument , NULL , 'a' },
        { "runner"               , required_argument , NULL , 'r' },
        { "data-fetcher"         , required_argument , NULL , 'd' },
        { "rdr-comm"             , required_argument , NULL , 'c' },
        { "rdr-uri"              , required_argument , NULL , 'u' },
        { "request"              , required_argument , NULL , 'j' },
        { "query"                , required_argument , NULL , 'q' },
        { "pageid"               , required_argument , NULL , 'P' },
        { "layout-style"         , required_argument , NULL , 'L' },
        { "toolkit-style"        , required_argument , NULL , 'T' },
        { "transition-style"     , required_argument , NULL , 'A' },
        { "allow-switching-rdr"  , required_argument , NULL , 's' },
        { "chroot"               , required_argument , NULL , 'R' },
        { "setuser"              , required_argument , NULL , 'U' },
        { "setgroup"             , required_argument , NULL , 'G' },
        { "parallel"             , no_argument       , NULL , 'l' },
        { "verbose"              , no_argument       , NULL , 'v' },
        { "copying"              , no_argument       , NULL , 'C' },
        { "version"              , no_argument       , NULL , 'V' },
        { "help"                 , no_argument       , NULL , 'h' },
        { 0, 0, 0, 0 }
    };

    if (argc == 1) {
        goto bad_arg;
    }

    int o, idx = 0;
    while ((o = getopt_long(argc, argv, short_options, long_opts, &idx)) >= 0) {
        if (-1 == o || EOF == o)
            break;

        switch (o) {
        case 'h':
            print_usage(stdout);
            return 1;

        case 'V':
            print_version(stdout);
            return 1;

        case 'C':
            print_version(stdout);
            print_long_copying(stdout);
            return 1;

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

        case 'c':
            if (strcmp(optarg, "headless") == 0) {
                opts->rdr_prot = "headless";
            }
            else if (strcmp(optarg, "thread") == 0) {
                opts->rdr_prot = "thread";
            }
            else if (strcmp(optarg, "socket") == 0) {
                opts->rdr_prot = "socket";
            }
            else if (strcmp(optarg, "websocket") == 0) {
                opts->rdr_prot = "websocket";
            }
            else {
                goto bad_arg;
            }
            break;

        case 'u':
            if (strcmp(opts->rdr_prot, "thread") == 0) {
                opts->rdr_uri = strdup(optarg);
            }
            else if (pcutils_url_is_valid(optarg)) {
                opts->rdr_uri = strdup(optarg);
            }
            else {
                goto bad_arg;
            }

            break;

        case 'j':
            if (strcmp(optarg, "-") == 0 ||
                    is_json_or_ejson_file(optarg)) {
                opts->request = strdup(optarg);
            }
            else {
                goto bad_arg;
            }
            break;

        case 'q':
            opts->query = strdup(optarg);
            break;

        case 'P':
            if (purc_split_page_identifier(optarg, NULL, NULL, NULL, NULL) < 0)
                goto bad_arg;
            opts->pageid = optarg;
            break;

        case 'L':
            opts->layout_style = optarg;
            break;

        case 'T':
            opts->toolkit_style = optarg;
            break;

        case 'A':
            opts->transition_style = optarg;
            break;

        case 's':
            opts->allow_switching_rdr = optarg;
            break;

        case 'R':
            opts->chroot = optarg;
            break;

        case 'U':
            opts->setuser = optarg;
            break;

        case 'G':
            opts->setuser = optarg;
            break;

        case 'l':
            opts->parallel = true;
            break;

        case 'v':
            opts->verbose = true;
            break;

        case '?':
            fprintf(stderr, "Run with `-h` option for usage.\n");
            return -1;

        default:
            goto bad_arg;
        }
    }

    if (optind < argc) {
        if (is_json_or_ejson_file(argv[optind])) {
            opts->app_info = strdup(argv[optind]);
        }
        else {
            for (int i = optind; i < argc; i++) {
                if (!validate_url(opts, argv[i])) {
                    fprintf(stderr, "%s: bad file or URL: %s\n",
                            argv[0], argv[i]);
                    return -1;
                }
            }
        }
    }

    return 0;

bad_arg:
    fprintf(stderr,
            "%s: bad option or argument; run with `-h` option for usage.\n",
            argv[0]);
    return -1;
}

static void
transfer_opts_to_variant(struct my_opts *opts, purc_variant_t request)
{
    run_info.opts = purc_variant_make_object_0();
    purc_variant_t tmp;

    // set default options
    if (opts->app) {
        tmp = purc_variant_make_string_reuse_buff(opts->app,
                strlen(opts->app) + 1, false);
        opts->app = NULL;
    }
    else {
        tmp = purc_variant_make_string_static(DEF_APP_NAME, false);
    }
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_APP_NAME, tmp);
    purc_variant_unref(tmp);

    //assert(opts->run);
    if (opts->run) {
        tmp = purc_variant_make_string_static(opts->run, false);
        purc_variant_object_set_by_static_ckey(run_info.opts,
                KEY_RUN_NAME, tmp);
        purc_variant_unref(tmp);
    }

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
    opts->rdr_uri = NULL;   // the ownership transfered
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_RDR_URI, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_array_0();
    for (size_t i = 0; i < opts->urls->length; i++) {
        char *url = opts->urls->list[i];
        purc_variant_t url_vrt = purc_variant_make_string_reuse_buff(url,
                strlen(url) + 1, false);
        opts->urls->list[i] = NULL;
        purc_variant_array_append(tmp, url_vrt);
        purc_variant_unref(url_vrt);
    }
    purc_variant_object_set_by_static_ckey(run_info.opts, KEY_URLS, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_array_0();
    for (size_t i = 0; i < opts->body_ids->length; i++) {
        char *body_id = opts->body_ids->list[i];
        purc_variant_t body_id_vrt;
        if (body_id) {
            body_id_vrt = purc_variant_make_string_reuse_buff(body_id,
                    strlen(body_id) + 1, false);
            opts->body_ids->list[i] = NULL;
        }
        else {
            body_id_vrt = purc_variant_make_null();
        }
        purc_variant_array_append(tmp, body_id_vrt);
        purc_variant_unref(body_id_vrt);
    }
    purc_variant_object_set_by_static_ckey(run_info.opts, KEY_BODYIDS, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_boolean(opts->parallel);
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_FLAG_PARALLEL, tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_boolean(opts->verbose);
    purc_variant_object_set_by_static_ckey(run_info.opts,
            KEY_FLAG_VERBOSE, tmp);
    purc_variant_unref(tmp);

    if (request) {
        purc_variant_object_set_by_static_ckey(run_info.opts,
                KEY_FLAG_REQUEST, request);
    }
}

static ssize_t cb_stdio_write(void *ctxt, const void *buf, size_t count)
{
    FILE *fp = ctxt;
    return fwrite(buf, 1, count, fp);
}

static ssize_t cb_stdio_read(void *ctxt, void *buf, size_t count)
{
    FILE *fp = ctxt;
    return fread(buf, 1, count, fp);
}

static purc_variant_t get_request_data(struct my_opts *opts)
{
    purc_variant_t v = PURC_VARIANT_INVALID;
    purc_rwstream_t stm;

    if (strcmp(opts->request, "-") == 0) {
        // read from stdin stream
        stm = purc_rwstream_new_for_read(stdin, cb_stdio_read);
    }
    else {
        stm = purc_rwstream_new_from_file(opts->request, "r");
    }

    if (stm) {
        v = purc_variant_load_from_json_stream(stm);
        purc_rwstream_destroy(stm);
    }

    return v;
}

static purc_variant_t parse_query_string(struct my_opts *opts)
{
    /* we use rfc 3986 */
    return purc_make_object_from_query_string(opts->query, false);
}

static purc_variant_t get_dvobj(void* ctxt, const char* name)
{
    struct run_info *run_info = ctxt;
    if (strcmp(name, "OPTS") == 0)
        return run_info->opts;

    return purc_get_runner_variable(name);
}

static char *
load_file_contents(struct my_opts *opts, const char *file, size_t *length)
{
    char *buf = NULL;
    FILE *f = fopen(file, "r");

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

        if (length)
            *length = (size_t)len;
failed:
        fclose(f);
    }
    else {
        return NULL;
    }

    pcutils_array_push(opts->contents, buf);
    return buf;
}


static bool evalute_app_info(struct my_opts *opts, const char *app_info)
{
    const char *ejson = app_info;
    size_t nr_ejson = strlen(app_info);

    if (ejson[0] != '{') {
        ejson = load_file_contents(opts, app_info, &nr_ejson);
    }

    struct purc_ejson_parsing_tree *ptree;

    ptree = purc_variant_ejson_parse_string(ejson, nr_ejson);
    if (ptree) {
        run_info.app_info = purc_ejson_parsing_tree_evalute(ptree,
                get_dvobj, &run_info, true);
        purc_ejson_parsing_tree_destroy(ptree);
        return true;
    }

    return false;
}


static purc_vdom_t load_hvml(const char *url)
{
    struct purc_broken_down_url broken_down;

    memset(&broken_down, 0, sizeof(broken_down));
    pcutils_url_break_down(&broken_down, url);

    purc_vdom_t vdom;
    if (strcasecmp(broken_down.schema, "file") == 0) {
        vdom = purc_load_hvml_from_file(broken_down.path);
    }
    else {
        vdom = purc_load_hvml_from_url(url);
    }

    pcutils_broken_down_url_clear(&broken_down);

    return vdom;
}

static pcrdr_page_type_k get_page_type(purc_variant_t rdr)
{
    pcrdr_page_type_k page_type = PCRDR_PAGE_TYPE_NULL;

    const char *str = NULL;

    purc_variant_t tmp = purc_variant_object_get_by_ckey(rdr, "pageType");
    if (tmp)
        str = purc_variant_get_string_const(tmp);

    if (str) {
        if (strcmp(str, PCRDR_PAGE_TYPE_NAME_NULL) == 0)
            page_type = PCRDR_PAGE_TYPE_NULL;
        else if (strcmp(str, PCRDR_PAGE_TYPE_NAME_PLAINWIN) == 0)
            page_type = PCRDR_PAGE_TYPE_PLAINWIN;
        else if (strcmp(str, PCRDR_PAGE_TYPE_NAME_WIDGET) == 0)
            page_type = PCRDR_PAGE_TYPE_WIDGET;
    }

    return page_type;
}

static const char *get_workspace(purc_variant_t rdr)
{
    const char *workspace = NULL;

    purc_variant_t tmp = purc_variant_object_get_by_ckey(rdr, "workspace");
    if (tmp)
        workspace = purc_variant_get_string_const(tmp);

    return workspace;
}

static const char *get_page_group(purc_variant_t rdr)
{
    const char *page_group = NULL;

    purc_variant_t tmp = purc_variant_object_get_by_ckey(rdr, "pageGroupId");
    if (tmp)
        page_group = purc_variant_get_string_const(tmp);

    return page_group;
}

static const char *get_page_name(purc_variant_t rdr)
{
    const char *page_name = NULL;

    purc_variant_t tmp = purc_variant_object_get_by_ckey(rdr, "pageName");
    if (tmp)
        page_name = purc_variant_get_string_const(tmp);

    return page_name;
}

static void
fill_cor_rdr_info(struct my_opts *opts,
        purc_renderer_extra_info *rdr_info, purc_variant_t rdr)
{
    purc_variant_t tmp;

    tmp = purc_variant_object_get_by_ckey(rdr, "class");
    if (tmp)
        rdr_info->klass = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "title");
    if (tmp)
        rdr_info->title = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "layoutStyle");
    if (tmp)
        rdr_info->layout_style = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "transitionStyle");
    if (tmp)
        rdr_info->transition_style = purc_variant_get_string_const(tmp);

    rdr_info->toolkit_style = purc_variant_object_get_by_ckey(rdr,
            "toolkitStyle");

    tmp = purc_variant_object_get_by_ckey(rdr, "pageGroups");
    if (tmp) {
        const char *file = purc_variant_get_string_const(tmp);
        if (file) {
            rdr_info->page_groups = load_file_contents(opts, file, NULL);
        }
    }
}

static void
fill_run_rdr_info(struct my_opts *opts,
        purc_instance_extra_info *rdr_info, purc_variant_t rdr)
{
    purc_variant_t tmp;

    tmp = purc_variant_object_get_by_ckey(rdr, "commMethod");
    if (tmp) {
        const char *str = purc_variant_get_string_const(tmp);
        if (str) {
            if (strcmp(str, "headless") == 0)
                rdr_info->renderer_comm = PURC_RDRCOMM_HEADLESS;
            else if (strcmp(str, "thread") == 0)
                rdr_info->renderer_comm = PURC_RDRCOMM_THREAD;
            else if (strcmp(str, "socket") == 0)
                rdr_info->renderer_comm = PURC_RDRCOMM_SOCKET;
            else if (strcmp(str, "websocket") == 0)
                rdr_info->renderer_comm = PURC_RDRCOMM_WEBSOCKET;
        }
    }

    tmp = purc_variant_object_get_by_ckey(rdr, "uri");
    if (tmp)
        rdr_info->renderer_uri = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "sslCert");
    if (tmp)
        rdr_info->ssl_cert = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "sslKey");
    if (tmp)
        rdr_info->ssl_key = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "workspaceName");
    if (tmp)
        rdr_info->workspace_name = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "workspaceTitle");
    if (tmp)
        rdr_info->workspace_title = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "workspaceLayout");
    if (tmp) {
        const char *file = purc_variant_get_string_const(tmp);
        if (file) {
            rdr_info->workspace_layout = load_file_contents(opts, file, NULL);
        }
    }
}

static int start_thread_renderer(const char *uri);
static size_t
schedule_coroutines_for_runner(struct my_opts *opts,
        purc_variant_t app, purc_variant_t runner, purc_variant_t coroutines)
{
    const char *endpoint = purc_get_endpoint(NULL);
    char curr_app_name[PURC_LEN_APP_NAME + 1];
    purc_extract_app_name(endpoint, curr_app_name);

    char curr_run_name[PURC_LEN_RUNNER_NAME + 1];
    purc_extract_runner_name(endpoint, curr_run_name);

    purc_variant_t tmp;
    const char *app_name = NULL;
    const char *run_name = NULL;

    if (app) {
        app_name = purc_variant_get_string_const(app);
    }
    if (app_name == NULL) {
        app_name = curr_app_name;
    }

    tmp = purc_variant_object_get_by_ckey(runner, "runner");
    if (tmp) {
        run_name = purc_variant_get_string_const(tmp);
    }
    if (run_name == NULL) {
        run_name = curr_run_name;
    }

    size_t n = 0;
    purc_atom_t rid = 0;

    /* create new runner if app_name or run_name differ */
    if (strcmp(app_name, curr_app_name) || strcmp(run_name, curr_run_name)) {
        purc_instance_extra_info inst_info = {};

        inst_info.allow_switching_rdr = 1;
        if (opts->allow_switching_rdr) {
            if (strcmp(opts->allow_switching_rdr, "true") == 0) {
                inst_info.allow_switching_rdr = 1;
            }
            else {
                inst_info.allow_switching_rdr = 0;
            }
        }

        tmp = purc_variant_object_get_by_ckey(runner, "allowSwitchingRdr");
        if (tmp) {
            inst_info.allow_switching_rdr = purc_variant_booleanize(tmp);
        }

        tmp = purc_variant_object_get_by_ckey(runner, "allowScalingByDensity");
        if (tmp) {
            inst_info.allow_scaling_by_density = purc_variant_booleanize(tmp);
        }

        tmp = purc_variant_object_get_by_ckey(runner, "renderer");
        if (tmp)
            fill_run_rdr_info(opts, &inst_info, tmp);

        if (inst_info.renderer_comm == PURC_RDRCOMM_THREAD) {
            start_thread_renderer(inst_info.renderer_uri);
        }

        rid = purc_inst_create_or_get(app_name, run_name,
            NULL, &inst_info);
        if (rid == 0) {
            fprintf(stderr, "Failed to create PurC instance for %s/%s\n",
                    app_name, run_name);
            return n;
        }
    }

    size_t nr_coroutines = 0;
    purc_variant_array_size(coroutines, &nr_coroutines);
    assert(nr_coroutines > 0);

    for (size_t i = 0; i < nr_coroutines; i++) {
        purc_variant_t crtn = purc_variant_array_get(coroutines, i);
        if (!purc_variant_is_object(crtn)) {
            fprintf(stderr, "Not an object for crtn[%u]\n", (unsigned)i);
            continue;
        }

        tmp = purc_variant_object_get_by_ckey(crtn, "url");
        const char *url = NULL;
        if (tmp) {
            url = purc_variant_get_string_const(tmp);
        }

        if (url == NULL) {
            fprintf(stderr, "No valid URL given for crtn[%u]\n", (unsigned)i);
            continue;
        }

        purc_vdom_t vdom = load_hvml(url);
        if (vdom == NULL) {
            fprintf(stderr, "Failed to load HVML from %s for crtn[%u]: %s\n",
                    url, (unsigned)i,
                    purc_get_error_message(purc_get_last_error()));

            if (opts->verbose) {
                struct purc_parse_error_info *parse_error = NULL;
                purc_get_local_data(PURC_LDNAME_PARSE_ERROR,
                        (uintptr_t *)(void *)&parse_error, NULL);
                if (parse_error) {
                    fprintf(stdout,
                            "Parse %s failed : line=%d, column=%d, character=0x%x\n",
                            url, parse_error->line, parse_error->column,
                            parse_error->character);
                }
            }
            continue;
        }

        purc_variant_t request =
            purc_variant_object_get_by_ckey(crtn, "request");

        const char *body_id = NULL;
        tmp = purc_variant_object_get_by_ckey(crtn, "bodyId");
        if (tmp) {
            body_id = purc_variant_get_string_const(tmp);
        }

        pcrdr_page_type_k page_type = PCRDR_PAGE_TYPE_NULL;
        const char *target_workspace = NULL;
        const char *target_group = NULL;
        const char *page_name = NULL;
        purc_renderer_extra_info rdr_info = {};

        purc_variant_t rdr =
            purc_variant_object_get_by_ckey(crtn, "renderer");
        if (purc_variant_is_object(rdr)) {
            page_type = get_page_type(rdr);
            target_workspace = get_workspace(rdr);
            target_group = get_page_group(rdr);
            page_name = get_page_name(rdr);
            fill_cor_rdr_info(opts, &rdr_info, rdr);
        }

        purc_atom_t cid = 0;
        if (rid == 0) {
            purc_coroutine_t cor = purc_schedule_vdom(vdom,
                    0, request, page_type, target_workspace,
                    target_group, page_name, &rdr_info, body_id, NULL);
            if (cor) {
                cid = purc_coroutine_identifier(cor);
            }
        }
        else {
            cid = purc_inst_schedule_vdom(rid, vdom,
                    0, request, page_type, target_workspace,
                    target_group, page_name, &rdr_info, body_id);
        }

        if (cid) {
            n++;
        }
        else {
            fprintf(stderr, "Failed to schedule coroutine from %s for #%u\n",
                    url, (unsigned)i);
        }
    }

    return n;
}

#define MY_VRT_OPTS \
    (PCVRNT_SERIALIZE_OPT_SPACED | PCVRNT_SERIALIZE_OPT_NOSLASHESCAPE)

static int app_cond_handler(purc_cond_k event, void *arg, void *data)
{
    (void)arg;
    (void)data;

    if (event == PURC_COND_SHUTDOWN_ASKED) {
        return 0;
    }

    return 0;
}

static bool run_app(struct my_opts *opts)
{
#ifndef NDEBUG
    fprintf(stdout, "The options: ");

    if (run_info.opts) {
        purc_variant_serialize(run_info.opts,
                run_info.dump_stm, 0, MY_VRT_OPTS, NULL);
    }
    else {
        fprintf(stdout, "INVALID VALUE");
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "The app info: ");

    if (run_info.app_info) {
        purc_variant_serialize(run_info.app_info,
                run_info.dump_stm, 0,  MY_VRT_OPTS, NULL);
    }
    else {
        fprintf(stdout, "INVALID VALUE");
    }
    fprintf(stdout, "\n");
#endif

    purc_variant_t app =
        purc_variant_object_get_by_ckey(run_info.app_info, "app");

    purc_variant_t runners =
        purc_variant_object_get_by_ckey(run_info.app_info, "runners");
    size_t nr_runners = 0;
    if (!purc_variant_array_size(runners, &nr_runners) || nr_runners == 0) {
        fprintf(stderr, "No runner defined.\n");
        return false;
    }

    size_t nr_live_runners = 0;
    size_t nr_live_coroutines = 0;
    for (size_t i = 0; i < nr_runners; i++) {
        purc_variant_t runner = purc_variant_array_get(runners, i);

        purc_variant_t coroutines =
            purc_variant_object_get_by_ckey(runner, "coroutines");

        if (!coroutines) {
            fprintf(stderr, "No coroutines for runner #%u\n",
                    (unsigned)i);
            continue;
        }

        size_t nr_coroutines = 0;
        if (!purc_variant_array_size(coroutines, &nr_coroutines) ||
                nr_coroutines == 0) {
            fprintf(stderr, "Invalid coroutines for runner #%u\n",
                    (unsigned)i);
            continue;
        }

        size_t n;
        n = schedule_coroutines_for_runner(opts, app, runner, coroutines);
        if (n == 0) {
            fprintf(stderr, "No coroutine schedule for runner #%u\n",
                    (unsigned)i);
            continue;
        }

        nr_live_runners += 1;
        nr_live_coroutines += n;
    }

    if (opts->verbose) {
        fprintf(stdout, "Totally %u runners and %u coroutines scheduled.\n",
                (unsigned)nr_live_runners, (unsigned)nr_live_coroutines);
    }

    if (nr_live_coroutines > 0)
        purc_run(app_cond_handler);

    return nr_live_coroutines > 0;
}

#define RUNR_INFO_NAME    "runr-data"

struct runr_info {
    struct my_opts *opts;
    struct run_info *run_info;
};

struct crtn_info {
    const char *url;
};

static int prog_cond_handler(purc_cond_k event, purc_coroutine_t cor,
        void *data)
{
    if (event == PURC_COND_COR_EXITED) {
        struct runr_info *runr_info = NULL;
        purc_get_local_data(RUNR_INFO_NAME,
                (uintptr_t *)(void *)&runr_info, NULL);
        assert(runr_info);

        if (runr_info->opts->verbose) {
            struct crtn_info *crtn_info = purc_coroutine_get_user_data(cor);
            if (crtn_info) {
                fprintf(stdout, "\nThe main coroutine exited.\n");
            }
            else {
                fprintf(stdout, "\nA child coroutine exited.\n");
            }

            struct purc_cor_exit_info *exit_info = data;

            unsigned opt = 0;

            opt |= PCDOC_SERIALIZE_OPT_UNDEF;
            opt |= PCDOC_SERIALIZE_OPT_FULL_DOCTYPE;
#ifndef NDEBUG
            opt |= PCDOC_SERIALIZE_OPT_READABLE_C0CTRLS;
#else
            opt |= PCDOC_SERIALIZE_OPT_IGNORE_C0CTRLS;
#endif

            fprintf(stdout, ">> The document generated:\n");
            purc_document_serialize_contents_to_stream(exit_info->doc,
                    opt, runr_info->run_info->dump_stm);
            fprintf(stdout, "\n");

            fprintf(stdout, ">> The executed result:\n");
            if (exit_info->result) {
                purc_variant_serialize(exit_info->result,
                        runr_info->run_info->dump_stm, 0, MY_VRT_OPTS, NULL);
            }
            else {
                fprintf(stdout, "<INVALID VALUE>");
            }
            fprintf(stdout, "\n");
        }
    }
    else if (event == PURC_COND_COR_TERMINATED) {
        struct runr_info *runr_info = NULL;
        purc_get_local_data(RUNR_INFO_NAME,
                (uintptr_t *)(void *)&runr_info, NULL);
        assert(runr_info);

        if (runr_info->opts->verbose) {
            struct purc_cor_term_info *term_info = data;

            unsigned opt = 0;
            opt |= PCDOC_SERIALIZE_OPT_UNDEF;
            opt |= PCDOC_SERIALIZE_OPT_FULL_DOCTYPE;
#ifndef NDEBUG
            opt |= PCDOC_SERIALIZE_OPT_READABLE_C0CTRLS;
#else
            opt |= PCDOC_SERIALIZE_OPT_IGNORE_C0CTRLS;
#endif

            fprintf(stdout, ">> The document generated:\n");
            purc_document_serialize_contents_to_stream(term_info->doc,
                    opt, runr_info->run_info->dump_stm);
            fprintf(stdout, "\n");

            struct crtn_info *crtn_info = purc_coroutine_get_user_data(cor);
            if (crtn_info) {
                fprintf(stdout,
                        "\nThe main coroutine terminated due to an uncaught exception: %s.\n",
                        purc_atom_to_string(term_info->except));
            }
            else {
                fprintf(stdout,
                        "\nA child coroutine terminated due to an uncaught exception: %s.\n",
                        purc_atom_to_string(term_info->except));
            }

            fprintf(stdout, ">> The executing stack frame(s):\n");
            purc_coroutine_dump_stack(cor, runr_info->run_info->dump_stm);
            fprintf(stdout, "\n");
        }
    }

    return 0;
}

static bool
run_programs_sequentially(struct my_opts *opts, purc_variant_t request)
{
    size_t nr_executed = 0;
    char name[PURC_LEN_IDENTIFIER + 1] = "";
    char workspace[PURC_LEN_IDENTIFIER + 1] = "";
    char group[PURC_LEN_IDENTIFIER + 1] = "";
    int page_type;

    if (opts->pageid) {
        page_type = purc_split_page_identifier(opts->pageid,
                NULL, name, workspace, group);
    }
    else
        page_type = PCRDR_PAGE_TYPE_PLAINWIN;

    /* we have validated the pageid */
    assert(page_type >= 0);

    purc_variant_t toolkit_style = PURC_VARIANT_INVALID;
    if (opts->toolkit_style) {
        toolkit_style = purc_variant_make_from_json_string(
                opts->toolkit_style,
                strlen(opts->toolkit_style));
        if (!toolkit_style && opts->verbose) {
            fprintf(stdout, "Bad toolkit style `%s`, ignored\n",
                    opts->toolkit_style);
        }
    }

    struct runr_info runr_info = { opts, &run_info };
    purc_set_local_data(RUNR_INFO_NAME, (uintptr_t)&runr_info, NULL);

    for (size_t i = 0; i < opts->urls->length; i++) {
        const char *url = opts->urls->list[i];
        purc_vdom_t vdom = load_hvml(url);
        if (vdom) {
            if (opts->verbose)
                fprintf(stdout, "\nExecuting HVML program from `%s`...\n", url);

            purc_renderer_extra_info ex_rdr_info = {};
            if (page_type == PCRDR_PAGE_TYPE_PLAINWIN ||
                    page_type == PCRDR_PAGE_TYPE_WIDGET) {
                ex_rdr_info.layout_style = opts->layout_style;
                ex_rdr_info.toolkit_style = toolkit_style;
                ex_rdr_info.transition_style = opts->transition_style;
            }

            struct crtn_info crtn_info = { url };
            purc_schedule_vdom(vdom, 0, request,
                (pcrdr_page_type_k)page_type,
                workspace[0] ? workspace : NULL,
                group[0] ? group : NULL,
                name[0] ? name : NULL,
                (ex_rdr_info.layout_style || ex_rdr_info.toolkit_style ||
                 ex_rdr_info.transition_style) ? &ex_rdr_info : NULL,
                opts->body_ids->list[i], &crtn_info);
            purc_run((purc_cond_handler)prog_cond_handler);

            nr_executed++;
        }
        else {
            fprintf(stderr, "Failed to load HVML from %s: %s\n", url,
                    purc_get_error_message(purc_get_last_error()));

            if (opts->verbose) {
                struct purc_parse_error_info *parse_error = NULL;
                purc_get_local_data(PURC_LDNAME_PARSE_ERROR,
                        (uintptr_t *)(void *)&parse_error, NULL);
                if (parse_error) {
                    fprintf(stderr,
                            "Parse %s failed : line=%d, column=%d, character=0x%x\n",
                            url, parse_error->line, parse_error->column,
                            parse_error->character);
                }
            }
        }
    }

    if (toolkit_style)
        purc_variant_unref(toolkit_style);

    purc_remove_local_data(RUNR_INFO_NAME);
    return nr_executed > 0;
}

#ifndef NDEBUG
#include "util/unistring.h"

static void test_unistring(void)
{
    const char *str = "0123456789\0abcdef";

    foil_unistr *unistr;

    unistr = foil_unistr_new_len(str, -1);
    assert(unistr->len == 10);
    assert(unistr->sz >= 10);

    uint32_t *ucs = foil_unistr_free(unistr, false);
    assert(ucs != NULL);

    free(ucs);

    unistr = foil_unistr_new_len(str, 17);
    assert(unistr->len == 17);
    assert(unistr->sz >= 17);
    assert(unistr->ucs[10] == 0);

    foil_unistr_append_unichar(unistr, 0);
    assert(unistr->len == 18);
    assert(unistr->sz == 21);
    assert(unistr->ucs[10] == 0);
    assert(unistr->ucs[17] == 0);

    foil_unistr_prepend_unichar(unistr, 0);
    assert(unistr->len == 19);
    assert(unistr->sz == 21);
    assert(unistr->ucs[0] == 0);
    assert(unistr->ucs[11] == 0);
    assert(unistr->ucs[18] == 0);

    foil_unistr_set_size(unistr, 20);
    assert(unistr->sz >= 20);
    assert(unistr->ucs[0] == 0);
    assert(unistr->ucs[11] == 0);
    assert(unistr->ucs[18] == 0);

    foil_unistr_delete(unistr);

    unistr = foil_unistr_new(str);
    assert(unistr->len == 10);
    assert(unistr->sz >= 10);
    assert(unistr->ucs[0] == '0');
    assert(unistr->ucs[9] == '9');

    const char *str2 = "abcdef";
    unistr = foil_unistr_insert_len(unistr, 2, str2, 2);
    assert(unistr->len == 12);
    assert(unistr->sz >= 12);
    assert(unistr->ucs[2] == 'a');
    assert(unistr->ucs[3] == 'b');

    unistr = foil_unistr_insert(unistr, -1, str2);
    assert(unistr->len == 18);
    assert(unistr->sz >= 18);
    assert(unistr->ucs[2] == 'a');
    assert(unistr->ucs[3] == 'b');
    assert(unistr->ucs[12] == 'a');
    assert(unistr->ucs[13] == 'b');
    assert(unistr->ucs[17] == 'f');

    unistr = foil_unistr_truncate(unistr, 19);
    assert(unistr->len == 18);

    unistr = foil_unistr_truncate(unistr, 10);
    assert(unistr->len == 10);
    assert(unistr->sz >= 10);
    assert(unistr->ucs[2] == 'a');
    assert(unistr->ucs[3] == 'b');

    foil_unistr_delete(unistr);

    unistr = foil_unistr_new(str);
    foil_unistr_erase(unistr, 0, 2);
    assert(unistr->len == 8);
    assert(unistr->sz >= 8);
    assert(unistr->ucs[0] == '2');
    assert(unistr->ucs[1] == '3');
    assert(unistr->ucs[7] == '9');
    foil_unistr_delete(unistr);

    unistr = foil_unistr_new(str);
    foil_unistr_erase(unistr, 8, 1);
    assert(unistr->len == 9);
    assert(unistr->sz >= 9);
    assert(unistr->ucs[0] == '0');
    assert(unistr->ucs[1] == '1');
    assert(unistr->ucs[8] == '9');

    foil_unistr_erase(unistr, 6, 3);
    assert(unistr->len == 6);
    assert(unistr->sz >= 6);
    assert(unistr->ucs[0] == '0');
    assert(unistr->ucs[1] == '1');
    assert(unistr->ucs[5] == '5');

    foil_unistr_assign_len(unistr, str, 17);
    assert(unistr->len == 17);
    assert(unistr->sz >= 17);
    assert(unistr->ucs[10] == 0);

    foil_unistr_delete(unistr);
}
#endif

static struct thread_renderer {
    purc_atom_t atom;

    const char *shortname;
    const char *uri;
    purc_atom_t (*start)(const char *rdr_uri);
    void (*exit)(void);
} thrdrs[] = {
#if ENABLE(RENDERER_FOIL)
    { 0, FOIL_RUN_NAME, FOIL_RDR_URI,
        foil_start, foil_sync_exit },
#endif
    { 0, SEEKER_RUN_NAME, SEEKER_RDR_URI,
        seeker_start, seeker_sync_exit },
};

int start_thread_renderer(const char *uri)
{
    ssize_t thrdr_idx = -1;
    if (uri == NULL) {
        uri = thrdrs[0].uri;
        thrdr_idx = 0;
    }
    else {
        for (size_t i = 0; i < PCA_TABLESIZE(thrdrs); i++) {
            if (strncasecmp(uri, PURC_EDPT_SCHEMA,
                        sizeof(PURC_EDPT_SCHEMA) - 1) == 0) {
                if (strcasecmp(thrdrs[i].uri, uri) == 0) {
                    thrdr_idx = i;
                    break;
                }
            }
            else if (strcasecmp(thrdrs[i].shortname, uri) == 0) {
                uri = thrdrs[i].uri;
                thrdr_idx = i;
                break;
            }
        }
    }

    if (thrdr_idx < 0) {
        fprintf(stdout,
                "Not found given thread renderer (%s), use %s instead.\n",
                uri, thrdrs[0].uri);

        uri = thrdrs[0].uri;
    }

    if (thrdrs[thrdr_idx].atom != 0) {
        return 0;
    }

    thrdrs[thrdr_idx].atom = thrdrs[thrdr_idx].start(thrdrs[thrdr_idx].uri);
    if (thrdrs[thrdr_idx].atom == 0) {
        fprintf(stderr,
                "Failed to initialize the built-in thread renderer: %s\n",
                thrdrs[thrdr_idx].uri);
        return -1;
    }

    return 0;
}

static int find_user_group(const char *user, const char *group,
        uid_t *uid, gid_t *gid, const char **username)
{
    uid_t my_uid = 0;
    gid_t my_gid = 0;
    struct passwd *my_pwd = NULL;
    struct group *my_grp = NULL;
    char *endptr = NULL;
    *uid = 0; *gid = 0;

    if (username)
        *username = NULL;

    if (user) {
        my_uid = strtol(user, &endptr, 10);

        if (my_uid <= 0 || *endptr) {
            if (NULL == (my_pwd = getpwnam(user))) {
                fprintf(stderr, "hvml-fpm: can't find user name %s\n", user);
                return -1;
            }
            my_uid = my_pwd->pw_uid;

            if (my_uid == 0) {
                fprintf(stderr, "hvml-fpm: I will not set uid to 0\n");
                return -1;
            }

            if (username) *username = user;
        }
        else {
            my_pwd = getpwuid(my_uid);
            if (username && my_pwd)
                *username = my_pwd->pw_name;
        }
    }

    if (group) {
        my_gid = strtol(group, &endptr, 10);

        if (my_gid <= 0 || *endptr) {
            if (NULL == (my_grp = getgrnam(group))) {
                fprintf(stderr, "hvml-fpm: can't find group name %s\n", group);
                return -1;
            }
            my_gid = my_grp->gr_gid;

            if (my_gid == 0) {
                fprintf(stderr, "hvml-fpm: I will not set gid to 0\n");
                return -1;
            }
        }
    }
    else if (my_pwd) {
        my_gid = my_pwd->pw_gid;

        if (my_gid == 0) {
            fprintf(stderr, "hvml-fpm: I will not set gid to 0\n");
            return -1;
        }
    }

    *uid = my_uid;
    *gid = my_gid;
    return 0;
}

static int drop_root_privilege(const struct my_opts *opts)
{
    const char *username;
    const char *groupname;

    if (opts->setuser) {
        username = opts->setuser;
    }
    else if (purc_is_feature_enabled(PURC_FEATURE_APP_AUTH)) {
        username = opts->app;
    }
    else {
        username = NULL;
    }

    if (opts->setgroup) {
        groupname = opts->setgroup;
    }
    else if (purc_is_feature_enabled(PURC_FEATURE_APP_AUTH)) {
        groupname = opts->app;
    }
    else {
        groupname = NULL;
    }

    uid_t uid;
    gid_t gid;
    const char* real_username;

    if (find_user_group(username, groupname, &uid, &gid, &real_username) < 0)
        return -1;

    if (uid != 0 && gid == 0) {
        fprintf(stderr, "Failed to find the user for uid %d and no group was "
                "specified, so only the user privileges will be dropped\n",
                (int)uid);
    }

    /* Change group before chroot, when we have access to /etc/group */
    if (gid != 0) {
        if (-1 == setgid(gid)) {
            fprintf(stderr, "Failed setgid(%d): %s\n", (int)gid, strerror(errno));
            return -1;
        }

        if (-1 == setgroups(0, NULL)) {
            fprintf(stderr, "Failed setgroups(0, NULL): %s\n", strerror(errno));
            return -1;
        }

        if (real_username) {
            if (-1 == initgroups(real_username, gid)) {
                fprintf(stderr, "Failed initgroups('%s', %d): %s\n",
                        real_username, (int)gid, strerror(errno));
                return -1;
            }
        }
    }

    const char *changeroot;
    char app_top_dir[sizeof(PURC_HVML_APP_PREFIX) + PURC_LEN_APP_NAME] =
        PURC_HVML_APP_PREFIX;

    if (opts->chroot) {
        changeroot = opts->chroot;
    }
    else if (purc_is_feature_enabled(PURC_FEATURE_APP_AUTH)) {
        strcat(app_top_dir, opts->app);
        changeroot = app_top_dir;
    }
    else {
        changeroot = NULL;
    }

    if (changeroot) {
        if (-1 == chroot(changeroot)) {
            fprintf(stderr, "Failed chroot('%s'): %s\n",
                    changeroot, strerror(errno));
            return -1;
        }

        if (-1 == chdir("/")) {
            fprintf(stderr, "Failed: chdir('/'): %s\n", strerror(errno));
            return -1;
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    int ret;
    bool success = true;

#ifndef NDEBUG
    test_unistring();
#endif

    struct my_opts *opts = my_opts_new();
    ret = read_option_args(opts, argc, argv);
    if (ret) {
        my_opts_delete(opts);
        if (ret > 0)
            return EXIT_SUCCESS;
        return EXIT_FAILURE;
    }

    if (opts->app_info == NULL &&
            (opts->urls == NULL || opts->urls->length == 0)) {
        fprintf(stderr, "%s: no valid HVML program specified.\n", argv[0]);
        if (opts->verbose) {
            print_usage(stdout);
        }

        my_opts_delete(opts);
        return EXIT_FAILURE;
    }

    /* Since 0.9.17: use md5sum of the first URL of HVML programs
       as the runner name if not specified. */
    if (opts->run == NULL && opts->urls->length) {
        unsigned char digest[PCUTILS_MD5_DIGEST_SIZE];
        char md5sum[PCUTILS_MD5_DIGEST_SIZE * 2 + 1];

        pcutils_md5digest(opts->urls->list[0], digest);
        pcutils_bin2hex(digest, PCUTILS_MD5_DIGEST_SIZE, md5sum, false);
        /* R + md5sum[0-5](we use the first 6 characters only)*/
        opts->run = calloc(1, 8);
        opts->run[0] = 'R';
        memcpy(opts->run + 1, md5sum, 6);
        if (opts->run == NULL) {
            fprintf(stderr, "%s: failed to allocate space for "
                    "the default runner name.\n", argv[0]);
            my_opts_delete(opts);
            return EXIT_FAILURE;
        }
    }

    if (geteuid() == 0 && drop_root_privilege(opts) != 0) {
        fprintf(stderr, "%s: failed to drop root privilege", argv[0]);
        my_opts_delete(opts);
        return EXIT_FAILURE;
    }

    if (opts->verbose) {
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

    if (opts->allow_switching_rdr) {
        if (strcmp(opts->allow_switching_rdr, "true") == 0) {
            extra_info.allow_switching_rdr = 1;
        }
        else {
            extra_info.allow_switching_rdr = 0;
        }
    }
    else {
        extra_info.allow_switching_rdr = 1;
    }

    if (opts->rdr_prot == NULL || strcmp(opts->rdr_prot, "headless") == 0) {
        opts->rdr_prot = "headless";

        extra_info.renderer_comm = PURC_RDRCOMM_HEADLESS;
        if (opts->rdr_uri == NULL) {
            opts->rdr_uri = strdup(DEF_RDR_URI_HEADLESS);
        }

    }
    else if (strcmp(opts->rdr_prot, "thread") == 0) {
        opts->rdr_prot = "thread";
        extra_info.renderer_comm = PURC_RDRCOMM_THREAD;

        ssize_t thrdr_idx = -1;
        if (opts->rdr_uri == NULL) {
            opts->rdr_uri = strdup(thrdrs[0].uri);
            thrdr_idx = 0;
        }
        else {
            for (size_t i = 0; i < PCA_TABLESIZE(thrdrs); i++) {
                if (strncasecmp(opts->rdr_uri, PURC_EDPT_SCHEMA,
                            sizeof(PURC_EDPT_SCHEMA) - 1) == 0) {
                    if (strcasecmp(thrdrs[i].uri, opts->rdr_uri) == 0) {
                        thrdr_idx = i;
                        break;
                    }
                }
                else if (strcasecmp(thrdrs[i].shortname, opts->rdr_uri) == 0) {
                    free(opts->rdr_uri);
                    opts->rdr_uri = strdup(thrdrs[i].uri);
                    thrdr_idx = i;
                    break;
                }
            }
        }

        if (thrdr_idx < 0) {
            fprintf(stdout,
                    "Not found given thread renderer (%s), use %s instead.\n",
                    opts->rdr_uri, thrdrs[0].uri);

            thrdr_idx = 0;
            free(opts->rdr_uri);
            opts->rdr_uri = strdup(thrdrs[0].uri);
        }

        thrdrs[thrdr_idx].atom = thrdrs[thrdr_idx].start(thrdrs[thrdr_idx].uri);
        if (thrdrs[thrdr_idx].atom == 0) {
            fprintf(stderr,
                    "Failed to initialize the built-in thread renderer: %s\n",
                    thrdrs[thrdr_idx].uri);
            return EXIT_FAILURE;
        }
    }
    else if (strcmp(opts->rdr_prot, "websocket") == 0) {
        extra_info.renderer_comm = PURC_RDRCOMM_WEBSOCKET;
        if (opts->rdr_uri == NULL) {
            opts->rdr_uri = strdup(DEF_RDR_URI_WEBSOCKET);
        }
    }
    else {
        if (strcmp(opts->rdr_prot, "socket")) {
            fprintf(stderr, "Unknown renderer communication method: %s\n",
                    opts->rdr_prot);
            if (opts->verbose) {
                print_usage(stdout);
            }

            my_opts_delete(opts);
            return EXIT_FAILURE;
        }

        extra_info.renderer_comm = PURC_RDRCOMM_SOCKET;
        if (opts->rdr_uri == NULL) {
            opts->rdr_uri = strdup(DEF_RDR_URI_SOCKET);
        }
    }

    extra_info.renderer_uri = opts->rdr_uri;


    ret = purc_init_ex(modules, opts->app ? opts->app : DEF_APP_NAME,
            opts->run, &extra_info);
    if (ret != PURC_ERROR_OK) {
        fprintf(stderr, "Failed to initialize the PurC instance: %s\n",
            purc_get_error_message(ret));
        my_opts_delete(opts);
        return EXIT_FAILURE;
    }

    if (opts->verbose) {
        purc_enable_log_ex(PURC_LOG_MASK_DEFAULT | PURC_LOG_MASK_INFO,
                PURC_LOG_FACILITY_FILE);
    }
    else {
        purc_enable_log_ex(PURC_LOG_MASK_DEFAULT, PURC_LOG_FACILITY_FILE);
    }

    purc_variant_t request = PURC_VARIANT_INVALID;
    if (opts->request) {
        if ((request = get_request_data(opts)) == PURC_VARIANT_INVALID) {
            fprintf(stderr, "Failed to get the request data from %s\n",
                opts->request);
            my_opts_delete(opts);
            goto failed;
        }
    }
    else if (opts->query) {
        if ((request = parse_query_string(opts)) == PURC_VARIANT_INVALID) {
            fprintf(stderr, "Failed to parse the query string: %s\n",
                opts->query);
            my_opts_delete(opts);
            goto failed;
        }
    }
    else {
        if ((request = purc_variant_make_object_0()) == PURC_VARIANT_INVALID) {
            fprintf(stderr, "Failed to make an empty object as the request\n");
            my_opts_delete(opts);
            goto failed;
        }
    }

    run_info.dump_stm = purc_rwstream_new_for_dump(stdout, cb_stdio_write);

    if (opts->app_info == NULL && opts->parallel) {
        if (!construct_app_info(opts)) {
            my_opts_delete(opts);
            goto failed;
        }
    }

    if (opts->app_info) {
        transfer_opts_to_variant(opts, request);
        if (!evalute_app_info(opts, opts->app_info)) {
            fprintf(stderr, "Failed to evalute the app info from %s\n",
                    opts->app_info);
            my_opts_delete(opts);
            goto failed;
        }

        if (!run_app(opts)) {
            success = false;
        }

    }
    else {
        assert(!opts->parallel);
        success = run_programs_sequentially(opts, request);
    }

failed:
    if (request) {
        purc_variant_unref(request);
    }
    if (run_info.opts)
        purc_variant_unref(run_info.opts);
    if (run_info.app_info)
        purc_variant_unref(run_info.app_info);
    if (run_info.dump_stm)
        purc_rwstream_destroy(run_info.dump_stm);

    purc_cleanup();

    my_opts_delete(opts);

    for (size_t i = 0; i < PCA_TABLESIZE(thrdrs); i++) {
        if (thrdrs[i].atom) {
            thrdrs[i].exit();
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

