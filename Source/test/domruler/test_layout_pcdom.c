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
#include <string.h>

#include "purc/purc.h"
#include "domruler.h"
#include "node.h"
#include "hldom_node_ops.h"
#include "pcdom_node_ops.h"
/*
 
   <div id="root">
        <div id="title"></div>
        <div id="description"></div>
        <div id="page">
            <hiweb></hiweb>
            <hijs></hijs>
        </div>
        <div id="indicator"></div>
   </div>


 */
 
#define FPCT_OF_INT_TOINT(a, b) (FIXTOINT(FDIV((a * b), F_100)))

int hl_element_node_set_inner_attr(HLDomElement* node, const char* attr_name, const char* attr_value);
const char* hl_element_node_get_inner_attr(HLDomElement* node, const char* attr_name);

char* readCSS(char* filename)
{
    char* text;
    FILE* fp = fopen(filename,"r");
    fseek(fp,0,SEEK_END);

    size_t size = (size_t)ftell(fp);
    text = (char*)malloc(size+1);
    rewind(fp);
    size_t got = fread(text, sizeof(char), size, fp);
    if (got < size) {
        fprintf(stderr, "failed to load content from :%s\n", filename);
        exit(1);
    }

    text[size] = '\0';
    return text;
}

void destory_user_data(void* data)
{
    fprintf(stderr, "................................user data is callback\n");
    fprintf(stderr, "data is %s\n", (char*)data);
    free(data);

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
        fprintf(stderr, "node|name=%s|id=%s|(x,y,w,h)=(%f,%f,%f,%f)\n", name, id,
            box->x, box->y, box->w, box->h);
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

int main(int argc, char **argv)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex (PURC_MODULE_HTML, "cn.fmsoft.hybridos.test",
            "test_layout", &info);
    if (ret != PURC_ERROR_OK) {
        fprintf(stderr, "failed purc_init_ex\n");
        exit(1);
    }

    const char html[] = " \
           <div id=\"root\"> \n\
                <div id=\"title\"></div> \n\
                <div id=\"description\"></div>\n\
                <div id=\"page\"> \n\
                </div> \n\
                <div id=\"indicator\"></div>\n\
           </div> \
        ";
    const char data[] = "h1 { color: red } \n"
        "html { display: block; } \n"
        "head, link, meta, script, style, title { display: none; } \n"
        "body { display: block;  height: 100%; } \n"
        "address, article, aside, div, footer, header, hgroup, layer, main, nav, section {\n"
        "        display: block;"
        "}\n"
        "#root { display: block; height:100%} \n"
        "#title { position: relative; left:20%; width: 100%; height: 10%; color: #123; } \n"
        "#page { position: relative; width: 100%; height: 80%; color: #125; } \n"
        "#indicator { position: relative; width: 100%; height: 10%; color: #126; } \n"
        "#description { position: relative; width: 100%; height: 0%; color: #124; } \n";

    fprintf(stderr, "####################################### html ###########################\n");
    fprintf(stderr, "%s\n", html);

    fprintf(stderr, "####################################### css  ###########################\n");
    const char* css_data = data;
    if (argc > 1) {
        css_data = readCSS(argv[1]);
    }
    fprintf(stderr, "%s\n", css_data);

    struct DOMRulerCtxt *ctxt = domruler_create(1280, 720, 72, 27);
    if (ctxt == NULL) {
        HL_LOGE("create DOMRulerCtxt failed.\n");
        return DOMRULER_INVALID;
    }

    domruler_append_css(ctxt, css_data, strlen(css_data));

    pchtml_html_document_t *doc = pchtml_html_document_create();
    ret = pchtml_html_document_parse_with_buf(doc,
            (const unsigned char *)html, strlen(html));

    pcdom_document_t *document = pcdom_interface_document(doc);
    pcdom_element_t *root = document->element;
    fprintf(stderr, "####################################### layout ###########################\n");
    ret = domruler_layout_pcdom_elements(ctxt, root);

    print_layout_result(ctxt, root);

    pchtml_html_document_destroy(doc);
    domruler_destroy(ctxt);

    purc_cleanup ();

    return 0;
}

