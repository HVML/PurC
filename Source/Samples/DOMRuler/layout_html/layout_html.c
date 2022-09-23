/*
** This file is part of DOM Ruler. DOM Ruler is a library to
** maintain a DOM tree, lay out and stylize the DOM nodes by
** using CSS (Cascaded Style Sheets).
**
** Copyright (C) 2022 Beijing FMSoft Technologies Co., Ltd.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General License for more details.
**
** You should have received a copy of the GNU Lesser General License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>

#include "purc/purc.h"
#include "domruler/domruler.h"

#define  LAYOUT_HTML_VERSION        "1.2.0"

// get path from env or __FILE__/../<rel> otherwise
#define getpath_from_env_or_rel(_path, _len, _env, _rel) do {  \
    const char *p = getenv(_env);                                      \
    if (p) {                                                           \
        snprintf(_path, _len, "%s", p);                                \
    } else {                                                           \
        char tmp[PATH_MAX+1];                                          \
        snprintf(tmp, sizeof(tmp), __FILE__);                          \
        const char *folder = dirname(tmp);                             \
        snprintf(_path, _len, "%s/%s", folder, _rel);                  \
    }                                                                  \
} while (0)

struct layout_info {
    char *default_css;
    char *html_content;
    char *css_content;
};

struct layout_info run_info;

static void print_copying(void)
{
    fprintf (stdout,
        "\n"
        "layout_html - A standalone program based-on HiDomLahyout for lay out\n"
        "and stylizer the DOM nodes by using CSS (Cascaded Style Sheets).\n"
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
    printf("load_html (%s) - "
           "layout_html - A standalone program based-on HiDomLahyout for lay out\n"
           "and stylizer the DOM nodes by using CSS (Cascaded Style Sheets).\n\n",
           LAYOUT_HTML_VERSION);

    printf(
           "Usage: "
           "layout_html [ options ... ]\n\n"
           ""
           "The following options can be supplied to the command:\n\n"
           ""
           "  -f --file=<html_file>        - The initial HTML file to load.\n"
           "  -c --css=<css_file>          - The initial CSS file to load.\n"
           "  -v --version                 - Display version information and exit.\n"
           "  -h --help                    - This help.\n"
           "\n"
          );
}

static char *load_file(const char *file)
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

static char short_options[] = "f:c:vh";
static struct option long_opts[] = {
    {"file"           , required_argument , NULL , 'f' } ,
    {"css"            , required_argument , NULL , 'c' } ,
    {"version"        , no_argument       , NULL , 'v' } ,
    {"help"           , no_argument       , NULL , 'h' } ,
    {0, 0, 0, 0}
};

static int read_option_args(int argc, char **argv)
{
    int o, idx = 0;
    if (argc == 1) {
        print_usage ();
        return -1;
    }

    while ((o = getopt_long(argc, argv, short_options, long_opts, &idx)) >= 0) {
        if (-1 == o || EOF == o)
            break;
        switch (o) {
        case 'h':
            print_usage ();
            return -1;
        case 'v':
            fprintf (stdout, "layout_html: %s\n", PURC_VERSION_STRING);
            return -1;
        case 'f':
            run_info.html_content = load_file(optarg);
            if (run_info.html_content == NULL
                    || strlen(run_info.html_content) == 0) {
                fprintf(stderr, "layout_html: load %s failed.\n", optarg);
                return -1;
            }
            break;
        case 'c':
            run_info.css_content = load_file(optarg);
            if (run_info.css_content == NULL
                    || strlen(run_info.css_content) == 0) {
                fprintf(stderr, "layout_html: load %s failed.\n", optarg);
                return -1;
            }
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

void print_layout_info(struct DOMRulerCtxt *ctxt, pcdom_element_t *node)
{
    if (node->node.type == PCDOM_NODE_TYPE_TEXT
            || node->node.type == PCDOM_NODE_TYPE_UNDEF) {
        return;
    }

    const char *name = (const char *)pcdom_element_tag_name(node, NULL);
    const char *id = (const char *)pcdom_element_get_attribute(node,
            (const unsigned char *)"id", 2, NULL);
    const HLBox *box = domruler_get_node_bounding_box(ctxt, node);

    if (box) {
        fprintf(stderr, "node|name=%s|id=%s", name, id);
        if (box->display == HL_DISPLAY_NONE) {
            fprintf(stderr, "|display=none\n");
        }
        else {
            fprintf(stderr, "|display=%d|(x,y,w,h)=(", box->display);
            fprintf(stderr, "%d, %d", (int)box->x, (int)box->y);
            if (box->w == HL_AUTO) {
                fprintf(stderr, "0");
            }
            else {
                fprintf(stderr, ", %d", (int)box->w);
            }
            if (box->h == HL_AUTO) {
                fprintf(stderr, ", 0)\n");
            }
            else {
                fprintf(stderr, ", %d)\n", (int)box->h);
            }
        }
    }
}

void print_layout_result(struct DOMRulerCtxt *ctxt, pcdom_element_t *elem)
{
    print_layout_info(ctxt, elem);
    pcdom_element_t *child = (pcdom_element_t *)elem->node.first_child;
    while(child) {
        print_layout_result(ctxt, child);
        child = (pcdom_element_t *)child->node.next;
    }
}

void load_default_css()
{
    const char* env = "LAYOUT_HTML_DFAULT_CSS";
    char css_path[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(css_path, sizeof(css_path), env, "html.css");
    run_info.default_css = load_file(css_path);
}

int main(int argc, char **argv)
{
    int ret;
    print_copying();
    if (read_option_args (argc, argv)) {
        return EXIT_FAILURE;
    }

    purc_instance_extra_info info = {};
    ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "layout_html", &info);

    const char* css_data = run_info.css_content;

    struct DOMRulerCtxt *ctxt = domruler_create(1280, 720, 72, 27);
    if (ctxt == NULL) {
        return DOMRULER_INVALID;
    }

    load_default_css();
    if (run_info.default_css && strlen(run_info.default_css)) {
        domruler_append_css(ctxt, run_info.default_css,
                strlen(run_info.default_css));
    }

    domruler_append_css(ctxt, css_data, strlen(css_data));

    pchtml_html_document_t *doc = pchtml_html_document_create();
    ret = pchtml_html_document_parse_with_buf(doc,
            (const unsigned char *)run_info.html_content,
            strlen(run_info.html_content));
    if (ret) {
        fprintf(stderr, "Failed to parse html.");
        goto failed;
    }

    pcdom_document_t *document = pcdom_interface_document(doc);
    pcdom_element_t *root = document->element;

    ret = domruler_layout_pcdom_elements(ctxt, root);
    if (ret) {
        fprintf(stderr, "Failed to layout html.");
        goto failed;
    }

    print_layout_result(ctxt, root);

failed:
    pchtml_html_document_destroy(doc);
    domruler_destroy(ctxt);

    if (run_info.default_css) {
        free (run_info.default_css);
    }

    if (run_info.css_content) {
        free (run_info.css_content);
    }

    if (run_info.html_content) {
        free (run_info.html_content);
    }

    purc_cleanup ();

    return 0;
}

