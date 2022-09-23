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

#include <csseng/csseng.h>

#include "node.h"
#include "select.h"
#include "hl_dom_element_node.h"

int main(void)
{
//  const char data[] = "h1 {text-shadow: 5px 10px 100px red; filter:none; fill: url(#MyHatch); stroke: url(#stroke);} ";
    const char data[] = "h1 {fill: #123; stroke: red; } ";
//  const char data[] = "h1 {stroke-dasharray: none;text-shadow: none } ";
    //const char data[] = "h1 {text-shadow: 1px 5px 10px red } ";
//  const char data[] = "h1 {text-shadow: 5px 10px 100px red; filter:none;} ";
//  const char data[] = "h1 {stroke-dasharray: 10,5,10,9,1; } ";
#if 0
    const char data[] = "h1 {stroke-dasharray: 10 5 10 1; } "
        "h2 { stroke-dasharray: 1, 20%,3,4,5;}"
        "h3 { filter: none url(http://www.baidu.com);}"
        "h3 { fill: none;}"
        "h3 { fill: currentColor;}"
        "h3 { fill: url(http://www.baidu.com);}"
        "h3 { stroke: Inherited;}"
        "h3 { stroke: none;}"
        "h3 { stroke: currentColor;}"
        "h3 { stroke: url(http://www.baidu.com);}"
        ;
#endif
    unsigned int hh;
    css_media media = {
        .type = CSS_MEDIA_SCREEN,
    };

    HLCSS* css = domruler_css_create();
    if (css == NULL)
    {
        HL_LOGE("create HLCSS failed.\n");
        return DOMRULER_INVALID;
    }

    domruler_css_append_data(css, data, strlen(data));

    struct DOMRulerCtxt *ctxt = domruler_create(1080, 720, 72, 27);
    ctxt->origin_op = hl_dom_element_node_get_op();

    /* select style for each of h1 to h6 */
    for (hh = 1; hh < 2; hh++) {
        css_select_results *style;
        uint8_t color_type;
        css_color color_shade;


        HLDomElement* domNode = domruler_element_node_create("h1");
        HiLayoutNode *layout_node = hi_layout_node_from_origin_node(ctxt, domNode);
        style = hl_css_select_style(css, layout_node, &media, NULL, NULL);

        color_type = css_computed_color(
                style->styles[CSS_PSEUDO_ELEMENT_NONE],
                &color_shade);
        if (color_type == CSS_COLOR_INHERIT)
            HL_LOGW("color of h%i is 'inherit'\n", hh);
        else
            HL_LOGW("color of h%i is %x\n", hh, color_shade);

        css_fixed h, v, blur;
        css_unit h_unit, v_unit, blur_unit;
        css_color color;

        uint8_t shadow_type = css_computed_text_shadow(
                style->styles[CSS_PSEUDO_ELEMENT_NONE],
                &h, &h_unit, &v, &v_unit, &blur, &blur_unit, &color);

        HL_LOGW("text_shadow type=0x%x\n", shadow_type);
        HL_LOGW("text_shadow h=%d|h_unit=%d\n", h, h_unit);
        HL_LOGW("text_shadow v=%d|v_unit=%d\n", v, v_unit);
        HL_LOGW("text_shadow blur=%d|blur_unit=%d\n", blur, blur_unit);
        HL_LOGW("text_shadow color=0x%x\n", color);


        lwc_string *filter = NULL;
        css_computed_filter(
                style->styles[CSS_PSEUDO_ELEMENT_NONE],
                &filter);
        HL_LOGW("text_shadow filter=%s\n", filter ? lwc_string_data(filter) : "");

        lwc_string *fill = NULL;
        css_color fill_color;
        uint8_t fill_type = css_computed_fill(
                style->styles[CSS_PSEUDO_ELEMENT_NONE],
                &fill, &fill_color);
        HL_LOGW("text_shadow fill_type=%d|fill=%s|color=0x%x\n", fill_type, fill ? lwc_string_data(fill) : "", fill_color);

        lwc_string *stroke = NULL;
        css_color stroke_color;
        uint8_t stroke_type = css_computed_stroke(
                style->styles[CSS_PSEUDO_ELEMENT_NONE],
                &stroke, &stroke_color);
        HL_LOGW("text_shadow stroke_type=%d|stroke=%s|color=0x%x\n", stroke_type, stroke ? lwc_string_data(stroke) : "", stroke_color);


        int32_t n_values = 0;
        css_fixed* values = NULL;
        css_unit* units = NULL;
        uint8_t dasharray_t = css_computed_stroke_dasharray(
                style->styles[CSS_PSEUDO_ELEMENT_NONE],
                &n_values, &values, &units);
        HL_LOGW("stroke_dasharray type=0x%x\n", dasharray_t);
        HL_LOGW("stroke_dasharray count=%d\n", n_values);
        for (int i=0; i < n_values; i++)
        {
            fprintf(stderr, "index=%d|values=%d\n", i, values[i]);
        }

        hl_css_select_result_destroy(style);
        domruler_element_node_destroy(domNode);
    }


    domruler_css_destroy(css);
    domruler_destroy(ctxt);

    return 0;
}

