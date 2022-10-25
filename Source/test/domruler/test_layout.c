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

#include "domruler.h"
#include "node.h"
#include "hldom_node_ops.h"
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


void print_node_info(HLDomElement* node, void* user_data)
{
    (void)user_data;
    fprintf(stderr, "................................node=%s|id=%s\n", domruler_element_node_get_tag_name(node), domruler_element_node_get_id(node));
}

int main(int argc, char *argv[])
{
    const char html[] = " \
           <div id=\"root\"> \n\
                <div id=\"title\"></div> \n\
                <div id=\"description\"></div>\n\
                <div id=\"page\"> \n\
                    <hiweb></hiweb> \n\
                    <hijs></hijs>\n \
                </div> \n\
                <div id=\"indicator\"></div>\n\
           </div> \
        ";
    const char data[] = "h1 { color: red } \n"
        "#root { display: block; } \n"
        "#title { position: relative; left:20%; width: 100%; height: 10%; color: #123; } \n"
        "#page { position: relative; width: 100%; height: 80%; color: #125; } \n"
        "#indicator { position: relative; width: 100%; height: 10%; color: #126; } \n"
        "#description { position: relative; width: 100%; height: 0%; color: #124; } \n"
        "hiweb { position: relative; width: 50%; height: 50%; color: #127; font-family: \"Times New Roman\", Times, serif; font-size:10;} \n"
        "hiweb2 { position: relative; width: 50%; height: 50%; color: #127; font-family: \"Times New Roman\", Times, serif; font-size:10;} \n"
        "hijs { position: relative; width: 50%; height: 50%; color: #127; } \n"
        "hijs2 { position: relative; width: 50%; height: 50%; color: #128; background:red;} \n";

    fprintf(stderr, "####################################### html ###########################\n");
    fprintf(stderr, "%s\n", html);


    fprintf(stderr, "####################################### css  ###########################\n");
    const char* css_data = data;
    if (argc > 1)
    {
        css_data = readCSS(argv[1]);
    }
    fprintf(stderr, "%s\n", css_data);

    struct DOMRulerCtxt *ctxt = domruler_create(1280, 720, 72, 27);
    if (ctxt == NULL) {
        HL_LOGE("create DOMRulerCtxt failed.\n");
        return DOMRULER_INVALID;
    }

    domruler_append_css(ctxt, css_data, strlen(css_data));

    HLDomElement* root = domruler_element_node_create("div");
    domruler_element_node_set_id(root, "root");

    HLDomElement* title = domruler_element_node_create("div");
    domruler_element_node_set_id(title, "title");

    HLDomElement* description = domruler_element_node_create("div");
    domruler_element_node_set_id(description, "description");

    HLDomElement* page = domruler_element_node_create("div");
    domruler_element_node_set_id(page, "page");
//    char page_inline_style[] = "display:grid;";
//    domruler_element_node_set_style(page, page_inline_style);

    HLDomElement* indicator = domruler_element_node_create("div");
    domruler_element_node_set_id(indicator, "indicator");


    HLDomElement* hiweb = domruler_element_node_create("hiweb");
    domruler_element_node_set_id(hiweb, "hiweb");

    HLDomElement* hiweb2 = domruler_element_node_create("hiweb");
    domruler_element_node_set_id(hiweb2, "hiweb2");

    HLDomElement* hijs = domruler_element_node_create("hijs");
    domruler_element_node_set_id(hijs, "hijs");

    HLDomElement* hijs2 = domruler_element_node_create("hijs");
    domruler_element_node_set_id(hijs2, "hijs2");

    domruler_element_node_append_as_last_child(title, root);
    domruler_element_node_append_as_last_child(description, root);
    domruler_element_node_append_as_last_child(page, root);
    domruler_element_node_append_as_last_child(indicator, root);

    domruler_element_node_append_as_last_child(hiweb, page);
    domruler_element_node_append_as_last_child(hiweb2, page);
    domruler_element_node_append_as_last_child(hijs, page);
    domruler_element_node_append_as_last_child(hijs2, page);

    fprintf(stderr, "####################################### layout ###########################\n");
    domruler_layout_hldom_elements(ctxt, root);

    const HLUsedTextValues* txtVaule = domruler_element_node_get_used_text_value(ctxt, hijs);
    fprintf(stderr, "############### txtVaule=%p|txt->family=%s\n", txtVaule, txtVaule->font_family);

    domruler_element_node_set_general_attr(hijs, "xsmKey", "xsmValue");
    fprintf(stderr, "############### test get attr =%s\n", domruler_element_node_get_general_attr(hijs, "xsmKey"));

    domruler_element_node_set_general_attr(hijs, "xsmKey", "xsmValue2222222");
    fprintf(stderr, "############### test get attr =%s\n", domruler_element_node_get_general_attr(hijs, "xsmKey"));

    fprintf(stderr, ".......................HL_PROP_CATEGORY_BOX=%d\n", HL_PROP_CATEGORY_BOX);
    domruler_element_node_set_common_attr(hijs, HL_PROP_ID_WIDTH, "privateValue1111");
    fprintf(stderr, "############### test get attr id=%d | value =%s\n", HL_PROP_ID_WIDTH, domruler_element_node_get_common_attr(hijs, HL_PROP_ID_WIDTH));

    fprintf(stderr, "############### test get attr id=%d | value =%s\n", HL_PROP_ID_BACKGROUND_COLOR, domruler_element_node_get_common_attr(hijs, HL_PROP_ID_BACKGROUND_COLOR));

    hl_element_node_set_inner_attr(hijs, "innerKey", "innerValue2222");
    fprintf(stderr, "############### test get attr id=%d | value =%s\n", HL_PROP_ID_WIDTH, hl_element_node_get_inner_attr(hijs, "innerKey2"));

    char* buf = (char*)malloc(100);
    strcpy(buf, "this is test buf for userdata.\n");
    domruler_element_node_set_user_data(hijs, "userData", buf, destory_user_data);
    void* udata = domruler_element_node_get_user_data(hijs, "userData");
    fprintf(stderr, "############### test get user data key=userData | value =%s\n",  (char*)udata);

    buf = (char*)malloc(100);
    strcpy(buf, "this is test buf for inner data.\n");
    hl_element_node_set_inner_data(hijs, "innerData", buf, destory_user_data);
    udata = hl_element_node_get_inner_data(hijs, "innerData");
    fprintf(stderr, "############### test get inner data key=innerData | value =%s\n",  (char*)udata);

    const char* class_name = "   aa bb cc dd ee ff   ";
    domruler_element_node_set_class(hijs, class_name);
    const char* get_name = domruler_element_node_get_class(hijs);
    fprintf(stderr, ".....................set class = %s\n", class_name);
    fprintf(stderr, ".....................get class = %s\n", get_name);

    fprintf(stderr, " domruler_element_node_has_class xsm=%d\n", domruler_element_node_has_class(hijs, "xsm"));
    fprintf(stderr, " domruler_element_node_has_class aa=%d\n", domruler_element_node_has_class(hijs, "aa"));
    fprintf(stderr, " domruler_element_node_has_class bb=%d\n", domruler_element_node_has_class(hijs, "bb"));
    fprintf(stderr, " domruler_element_node_has_class cc=%d\n", domruler_element_node_has_class(hijs, "cc"));
    fprintf(stderr, " domruler_element_node_has_class dd=%d\n", domruler_element_node_has_class(hijs, "dd"));
    fprintf(stderr, " domruler_element_node_has_class ee=%d\n", domruler_element_node_has_class(hijs, "ee"));
    fprintf(stderr, " domruler_element_node_has_class ff=%d\n", domruler_element_node_has_class(hijs, "ff"));

    fprintf(stderr, " domruler_element_node_include_class xsm=%d\n", domruler_element_node_include_class(hijs, "xsm"));
    fprintf(stderr, ".....................get class = %s\n", domruler_element_node_get_class(hijs));

    fprintf(stderr, " domruler_element_node_exclude_class zxx=%d\n", domruler_element_node_exclude_class(hijs, "zxx"));
    fprintf(stderr, ".....................get class = %s\n", domruler_element_node_get_class(hijs));

    fprintf(stderr, " domruler_element_node_exclude_class ff=%d\n", domruler_element_node_exclude_class(hijs, "ff"));
    fprintf(stderr, ".....................get class = %s\n", domruler_element_node_get_class(hijs));


    domruler_element_node_depth_first_search_tree(root, print_node_info, NULL);


    domruler_element_node_destroy(root);
    domruler_element_node_destroy(title);
    domruler_element_node_destroy(page);
    domruler_element_node_destroy(description);
    domruler_element_node_destroy(indicator);
    domruler_element_node_destroy(hiweb);
    domruler_element_node_destroy(hiweb2);
    domruler_element_node_destroy(hijs);
    domruler_element_node_destroy(hijs2);

    return 0;
}

