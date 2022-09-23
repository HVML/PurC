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

int main(void)
{

    HLDomElement* root = domruler_element_node_create("div");

    // test get tag name
    const char* tag_name = domruler_element_node_get_tag_name(root);
    HL_LOGW("test|get tag name|tag=%s\n", tag_name);

    // test set attribute
    domruler_element_node_set_id(root, "idAttribute");
    domruler_element_node_set_class(root, "classA classB classC");
    domruler_element_node_set_style(root, "color=#33333; width=10px;");


    // test get attribute
    const char* value = domruler_element_node_get_id(root);
    HL_LOGW("test|get attribute|attr=id|value=%s\n", value);

    value = domruler_element_node_get_class(root);
    HL_LOGW("test|get attribute|attr=class|value=%s\n", value);

    value = domruler_element_node_get_style(root);
    HL_LOGW("test|get attribute|attr=style|value=%s\n", value);

    // test append as last child
    int index = 0;
    for (index = 0; index < 10; index++)
    {
        HL_LOGW("\n");
        HL_LOGW("test|add node|index=%d\n", index);
        HLDomElement* div = domruler_element_node_create("div");
        domruler_element_node_append_as_last_child(div, root);
    }


    domruler_element_node_destroy(root);
	return 0;
}

