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

#include "node.h"
#include "utils.h"
#include "internal.h"
#include "hl_dom_element_node.h"
#include "csseng/csseng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/ghash.h>

#define MAX_ATTACH_DATA_SIZE 10

#define HL_UNKNOWN INT_MAX
#define UNKNOWN_MAX_WIDTH INT_MAX
#define UNKNOWN_MAX_HEIGHT INT_MAX
#define AUTO INT_MIN

HLDomElement *domruler_element_node_create(const char *tag)
{
    if (tag == NULL || tag[0] == 0) {
        HL_LOGE("create HLDomElement failed, tag is NULL\n");
        return NULL;
    }

    HLDomElement *node = (HLDomElement*)calloc(1, sizeof(HLDomElement));
    if (node == NULL) {
        HL_LOGE("create HLDomElement failed. %d\n", DOMRULER_NOMEM);
        return NULL;
    }

    node->tag = strdup(tag);
    return node;
}

const char *domruler_element_node_get_tag_name(HLDomElement *node)
{
    return node ? node->tag : NULL;
}

void hl_destroy_class_list_item (gpointer data)
{
    free(data);
}

static const char _DOMRULER_WHITESPACE[] = " ";
void hl_fill_inner_classes(HLDomElement *node, const char *classes)
{
    if (node == NULL || classes == NULL || strlen(classes) == 0) {
        return;
    }

    g_list_free_full(node->class_list, hl_destroy_class_list_item);
    node->class_list = NULL;

    char *value = strdup(classes);
    char *c = strtok(value, _DOMRULER_WHITESPACE);
    while (c != NULL) {
        node->class_list = g_list_append(node->class_list, strdup(c));
        c = strtok(NULL, _DOMRULER_WHITESPACE);
    }
    free(value);
}

void domruler_element_node_destroy(HLDomElement *node)
{
    if (node == NULL) {
        return;
    }

    if (node->tag) {
        free(node->tag);
    }

    if (node->common_attrs) {
        g_hash_table_destroy(node->common_attrs);
    }

    if (node->general_attrs) {
        g_hash_table_destroy(node->general_attrs);
    }

    if (node->user_data) {
        g_hash_table_destroy(node->user_data);
    }

    if (node->inner_attrs) {
        g_hash_table_destroy(node->inner_attrs);
    }

    if (node->inner_data) {
        g_hash_table_destroy(node->inner_data);
    }

    if (node->attach_data) {
        for (int i = 0; i < MAX_ATTACH_DATA_SIZE; i++) {
            HLAttachData *attach = node->attach_data + i;
            if (attach->data && attach->callback) {
                attach->callback(attach->data);
            }
        }
        free(node->attach_data);
    }

    if (node->class_list) {
        g_list_free_full(node->class_list, hl_destroy_class_list_item);
    }

    free(node);
}

const HLBox *
domruler_element_node_get_used_box_value(struct DOMRulerCtxt *ctxt,
        HLDomElement *node)
{
    if (!ctxt || !node) {
        return NULL;
    }
    HLLayoutNode *layout = (HLLayoutNode*)g_hash_table_lookup(ctxt->node_map,
            (gpointer)node);
    if (layout) {
        return &layout->box_values;
    }
    return NULL;
}

const HLUsedBackgroundValues *
domruler_element_node_get_used_background_value(struct DOMRulerCtxt *ctxt,
        HLDomElement *node)
{
    if (!ctxt || !node) {
        return NULL;
    }
    HLLayoutNode *layout = (HLLayoutNode*)g_hash_table_lookup(ctxt->node_map,
            (gpointer)node);
    if (layout) {
        return &layout->background_values;
    }
    return NULL;
}

const HLUsedTextValues *
domruler_element_node_get_used_text_value(struct DOMRulerCtxt *ctxt,
        HLDomElement *node)
{
    if (!ctxt || !node) {
        return NULL;
    }
    HLLayoutNode *layout = (HLLayoutNode*)g_hash_table_lookup(ctxt->node_map,
            (gpointer)node);
    if (layout) {
        return &layout->text_values;
    }
    return NULL;
}


HLUsedSvgValues *
domruler_element_node_get_used_svg_value(struct DOMRulerCtxt *ctxt,
        HLDomElement *node)
{
    if (!ctxt || !node) {
        return NULL;
    }
    HLLayoutNode *layout = (HLLayoutNode*)g_hash_table_lookup(ctxt->node_map,
            (gpointer)node);
    css_computed_style *style = NULL;
    if (layout) {
        style = layout->computed_style;
    }

    if (style == NULL) {
        return NULL;
    }

    hl_destroy_svg_values(layout->svg_values);
    HLUsedSvgValues *svg = (HLUsedSvgValues*)calloc(1, sizeof(HLUsedSvgValues));
    layout->svg_values = svg;

    // baseline_shift
    svg->baseline_shift = css_computed_baseline_shift(style);

    // clip-path
    lwc_string *clip_path = NULL;
    css_computed_clip_path(style, &clip_path);
    if (clip_path) {
        svg->clip_path = strdup(lwc_string_data(clip_path));
        lwc_string_unref(clip_path);
    }

    // clip-rule
    svg->clip_rule = css_computed_clip_rule(style);
    // color
    svg->color_type = css_computed_color(style, &svg->color);
    // direction
    svg->direction = css_computed_direction(style);
    // display
    svg->display = css_computed_display_static(style);
    // enable-background
    svg->enable_background = css_computed_enable_background(style);
    // comp-op
    svg->comp_op = css_computed_comp_op(style);

    // fill
    lwc_string *fill_string = NULL;
    svg->fill_type = css_computed_fill(style, &fill_string, &svg->fill_color);
    if (fill_string) {
        svg->fill_string = strdup(lwc_string_data(fill_string));
        lwc_string_unref(fill_string);
    }

    // fill-opacity
    css_fixed fill_opacity;
    svg->fill_opacity_type = css_computed_fill_opacity(style, &fill_opacity);
    if (svg->fill_opacity_type ==  CSS_FILL_OPACITY_SET) {
        svg->fill_opacity = FIXTOFLT(fill_opacity);
    }

    // fill-rule
    svg->fill_rule = css_computed_fill_rule(style);
    // filter
    lwc_string *filter = NULL;
    css_computed_filter(style, &filter);
    if (filter) {
        svg->filter = strdup(lwc_string_data(filter));
        lwc_string_unref(filter);
    }

    // flood-color
    svg->flood_color_type = css_computed_flood_color(style, &svg->flood_color);
    // flood-opacity
    css_fixed flood_opacity;
    svg->flood_opacity_type = css_computed_flood_opacity(style, &flood_opacity);
    if (svg->flood_opacity_type == CSS_FLOOD_OPACITY_SET) {
        svg->flood_opacity = FIXTOFLT(flood_opacity);
    }

    // font-family
    lwc_string* *font_family_list = NULL;
    svg->font_family_type = css_computed_font_family(style, &font_family_list);
    svg->font_family = NULL;
    if (font_family_list != NULL) {
        while (*font_family_list != NULL) {
            const char *name = lwc_string_data(*font_family_list);
            size_t last_size =  svg->font_family ? strlen(svg->font_family) : 0;
            size_t size = last_size + strlen(name) + 1;
            svg->font_family = (char*)realloc(svg->font_family, size);
            if (last_size) {
                strcat(svg->font_family, ",");
                strcat(svg->font_family, name);
            }
            else {
                strcpy(svg->font_family, name);
            }
            font_family_list++;
        }
    }

    // font-size
    css_fixed font_size_length;
    css_unit font_size_unit;
    svg->font_size_type = css_computed_font_size(style, &font_size_length,
            &font_size_unit);
    svg->font_size_unit = font_size_unit;
    svg->font_size = FIXTOFLT(font_size_length);
    // font-stretch
    svg->font_stretch = css_computed_font_stretch(style);
    // font-style
    svg->font_style = css_computed_font_style(style);
    // font-variant
    svg->font_variant = css_computed_font_variant(style);
    // font-weight
    svg->font_weight = css_computed_font_weight(style);
    // marker-end
    lwc_string *marker_end = NULL;
    css_computed_marker_end(style, &marker_end);
    if (marker_end) {
        svg->marker_end = strdup(lwc_string_data(marker_end));
        lwc_string_unref(marker_end);
    }
    // mask
    lwc_string *mask = NULL;
    css_computed_mask(style, &mask);
    if (mask) {
        svg->mask = strdup(lwc_string_data(mask));
        lwc_string_unref(mask);
    }
    // marker-mid
    lwc_string *marker_mid = NULL;
    css_computed_marker_mid(style, &marker_mid);
    if (marker_mid) {
        svg->marker_mid = strdup(lwc_string_data(marker_mid));
        lwc_string_unref(marker_mid);
    }

    // marker-start
    lwc_string *marker_start = NULL;
    css_computed_marker_start(style, &marker_start);
    if (marker_start) {
        svg->marker_start = strdup(lwc_string_data(marker_start));
        lwc_string_unref(marker_start);
    }
    // opacity
    css_fixed opacity;
    svg->opacity_type = css_computed_opacity(style, &opacity);
    if (svg->opacity_type ==  CSS_OPACITY_SET) {
        svg->opacity = FIXTOFLT(opacity);
    }

    // overflow
    svg->overflow = css_computed_overflow_x(style);
    // shape-rendering
    svg->shape_rendering = css_computed_shape_rendering(style); 
    // text-rendering
    svg->text_rendering = css_computed_text_rendering(style);
    // stop-color
    svg->stop_color_type = css_computed_stop_color(style, &svg->stop_color);
    // stop-opacity
    css_fixed stop_opacity;
    svg->stop_opacity_type = css_computed_stop_opacity(style, &stop_opacity);
    if (svg->stop_opacity_type ==  CSS_STOP_OPACITY_SET) {
        svg->stop_opacity = FIXTOFLT(stop_opacity);
    }

    // stroke
    lwc_string *stroke_string = NULL;
    svg->stroke_type = css_computed_stroke(style, &stroke_string,
            &svg->stroke_color);
    if (stroke_string) {
        svg->stroke_string = strdup(lwc_string_data(stroke_string));
        lwc_string_unref(stroke_string);
    }
    // stroke-dasharray
    int32_t count = 0;
    css_fixed *dasharray_values = NULL;
    css_unit *dasharray_units = NULL;
    svg->stroke_dasharray_type = css_computed_stroke_dasharray(style, &count,
            &dasharray_values, &dasharray_units);
    svg->stroke_dasharray_count = count;
    if (svg->stroke_dasharray_count > 0) {
        svg->stroke_dasharray = (hl_real_t*)calloc(svg->stroke_dasharray_count,
                sizeof(hl_real_t));
        for (size_t i=0; i < svg->stroke_dasharray_count; i++) {
            svg->stroke_dasharray[i] = FIXTOFLT(dasharray_values[i]);
        }
    }
    // stroke-dashoffset
    css_fixed stroke_dashoffset_length;
    css_unit stroke_dashoffset_unit;
    svg->stroke_dashoffset_type = css_computed_stroke_dashoffset(style,
            &stroke_dashoffset_length, &stroke_dashoffset_unit);
    svg->stroke_dashoffset_unit = stroke_dashoffset_unit;
    svg->stroke_dashoffset = FIXTOFLT(stroke_dashoffset_length);
    // stroke-linecap
    svg->stroke_linecap = css_computed_stroke_linecap(style);
    // stroke-linejoin
    svg->stroke_linejoin = css_computed_stroke_linejoin(style);
    // stroke-miterlimit
    css_fixed stroke_miterlimit;
    svg->stroke_miterlimit_type = css_computed_stroke_miterlimit(style,
            &stroke_miterlimit);
    if (svg->stroke_miterlimit_type == CSS_STROKE_MITERLIMIT_SET) {
        svg->stroke_miterlimit = FIXTOFLT(stroke_miterlimit);
    }
    // stroke-opacity
    css_fixed stroke_opacity;
    svg->stroke_opacity_type = css_computed_stroke_opacity(style,
            &stroke_opacity);
    if (svg->stroke_opacity_type ==  CSS_STROKE_OPACITY_SET) {
        svg->stroke_opacity = FIXTOFLT(stroke_opacity);
    }
    // stroke-width
    css_fixed stroke_width_length;
    css_unit stroke_width_unit;
    svg->stroke_width_type = css_computed_stroke_width(style,
            &stroke_width_length, &stroke_width_unit);
    svg->stroke_width_unit = stroke_width_unit;
    svg->stroke_width = FIXTOFLT(stroke_width_length);
    // text-anchor
    svg->text_anchor = css_computed_text_anchor(style);
    // text-decoration
    svg->text_decoration = css_computed_text_decoration(style);
    // unicode-bidi
    svg->unicode_bidi = css_computed_unicode_bidi(style);
    // letter-spacing
    css_fixed letter_spacing_length;
    css_unit letter_spacing_unit;
    svg->letter_spacing_type = css_computed_letter_spacing(style,
            &letter_spacing_length, &letter_spacing_unit);
    svg->letter_spacing_unit = letter_spacing_unit;
    svg->letter_spacing = FIXTOFLT(letter_spacing_length);
    // visibility
    svg->visibility = css_computed_visibility(style);
    // writing-mode
    svg->writing_mode = css_computed_writing_mode(style);
    return svg;
}

int domruler_element_node_append_as_last_child(HLDomElement *node,
        HLDomElement *parent)
{
    if (node == NULL || parent == NULL) {
        return DOMRULER_BADPARM;
    }

    parent->n_children++;
    node->parent = parent;
    if (parent->first_child == NULL) {
        parent->first_child = node;
        parent->last_child = node;
        node->previous = NULL;
        node->next = NULL;
        return DOMRULER_OK;
    }

    HLDomElement *last = parent->last_child;
    last->next = node;
    parent->last_child = node;

    node->previous = last;
    node->next = NULL;

    return DOMRULER_OK;
}

HLDomElement *domruler_element_node_get_parent(HLDomElement *node)
{
    return node ? node->parent : NULL;
}

HLDomElement *domruler_element_node_get_first_child(HLDomElement *node)
{
    return node ? node->first_child: NULL;
}

HLDomElement *domruler_element_node_get_last_child(HLDomElement *node)
{
    return node ? node->last_child : NULL;
}

HLDomElement *domruler_element_node_get_prev(HLDomElement *node)
{
    return node ? node->previous : NULL;
}

HLDomElement *domruler_element_node_get_next(HLDomElement *node)
{
    return node ? node->next : NULL;
}

uint32_t domruler_element_node_get_children_count(HLDomElement *node)
{
    return node ? node->n_children: 0;
}

void hl_destroy_common_attr_value (gpointer data)
{
    free(data);
}

int hl_verify_common_attr_id(const HLDomElement *node,
        HLCommonAttribute attr_id)
{
    (void)node;
    if (attr_id >= 0 && attr_id < HL_COMMON_ATTR_COUNT) {
        return DOMRULER_OK;
    }
    return DOMRULER_BADPARM;
}

int domruler_element_node_set_common_attr(HLDomElement *node,
        HLCommonAttribute attr_id, const char *attr_value)
{
    if (node == NULL || attr_value == NULL) {
        return DOMRULER_OK;
    }

    if (DOMRULER_OK != hl_verify_common_attr_id(node, attr_id)) {
        return DOMRULER_BADPARM;
    }

    if (node->common_attrs == NULL) {
        node->common_attrs = g_hash_table_new_full(g_direct_hash,
                g_direct_equal, NULL, hl_destroy_common_attr_value);
    }

    if (attr_id == HL_COMMON_ATTR_CLASS_NAME) {
        hl_fill_inner_classes(node, attr_value);
    }

    return g_hash_table_insert(node->common_attrs, (gpointer)attr_id,
            (gpointer)strdup(attr_value));
}

const char *domruler_element_node_get_common_attr(const HLDomElement *node,
        HLCommonAttribute attr_id)
{
    if (node == NULL || node->common_attrs == NULL) {
        return NULL;
    }

    if (DOMRULER_OK != hl_verify_common_attr_id(node, attr_id))
    {
        return NULL;
    }

    return g_hash_table_lookup(node->common_attrs, (gpointer)attr_id);
}


void hl_destroy_general_attr_key (gpointer data)
{
    free(data);
}

void hl_destroy_general_attr_value (gpointer data)
{
    free(data);
}

int domruler_element_node_set_general_attr(HLDomElement *node,
        const char *attr_name, const char *attr_value)
{
    if (node == NULL || attr_name == NULL || attr_value == NULL) {
        return DOMRULER_OK;
    }

    if (node->general_attrs == NULL) {
        node->general_attrs = g_hash_table_new_full(g_str_hash, g_str_equal,
                hl_destroy_general_attr_key, hl_destroy_general_attr_value);
    }

    return g_hash_table_insert(node->general_attrs,
            (gpointer)strdup(attr_name), (gpointer)strdup(attr_value));
}

const char *domruler_element_node_get_general_attr(const HLDomElement *node,
        const char *attr_name)
{
    if (node == NULL || attr_name == NULL || node->general_attrs == NULL) {
        return NULL;
    }
    return g_hash_table_lookup(node->general_attrs, (gpointer)attr_name);
}

void hl_destroy_inner_attr_key (gpointer data)
{
    free(data);
}

void hl_destroy_inner_attr_value (gpointer data)
{
    free(data);
}

int hl_element_node_set_inner_attr(HLDomElement *node,
        const char *attr_name, const char *attr_value)
{
    if (node == NULL || attr_name == NULL || attr_value == NULL) {
        return DOMRULER_OK;
    }

    if (node->inner_attrs == NULL) {
        node->inner_attrs = g_hash_table_new_full(g_str_hash, g_str_equal,
                hl_destroy_inner_attr_key, hl_destroy_inner_attr_value);
    }

    return g_hash_table_insert(node->inner_attrs, (gpointer)strdup(attr_name),
            (gpointer)strdup(attr_value));
}

const char *hl_element_node_get_inner_attr(HLDomElement *node,
        const char *attr_name)
{
    if (node == NULL || attr_name == NULL || node->inner_attrs == NULL) {
        return NULL;
    }
    return g_hash_table_lookup(node->inner_attrs, (gpointer)attr_name);
}

void hl_destroy_attach_data_key (gpointer data)
{
    free(data);
}

void hl_destroy_attach_data_value (gpointer data)
{
    HLAttachData *attach = (HLAttachData*)data;
    if (attach->callback)
    {
        attach->callback(attach->data);
    }
    free(attach);
}

int domruler_element_node_set_user_data(HLDomElement *node,
        const char *key, void *data, HlDestroyCallback destroy_callback)
{
    if (node == NULL || key == NULL) {
        return DOMRULER_OK;
    }

    if (node->user_data == NULL) {
        node->user_data = g_hash_table_new_full(g_str_hash, g_str_equal,
                hl_destroy_attach_data_key, hl_destroy_attach_data_value);
    }

    if (data == NULL) {
        return g_hash_table_remove(node->user_data, key);
    }

    HLAttachData *attach = (HLAttachData*)calloc(1, sizeof(HLAttachData));
    attach->data = data;
    attach->callback = destroy_callback;
    return g_hash_table_insert(node->user_data, (gpointer)strdup(key),
            (gpointer)attach);
}

void *domruler_element_node_get_user_data(const HLDomElement *node,
        const char *key)
{
    if (node == NULL || key == NULL || node->user_data == NULL) {
        return NULL;
    }
    HLAttachData *attach = (HLAttachData*) g_hash_table_lookup(node->user_data,
            (gpointer)key);
    return attach ? attach->data : NULL;
}

int hl_element_node_set_inner_data(HLDomElement *node, const char *key,
        void *data, HlDestroyCallback destroy_callback)
{
    if (node == NULL || key == NULL) {
        return DOMRULER_OK;
    }

    if (node->inner_data == NULL) {
        node->inner_data = g_hash_table_new_full(g_str_hash, g_str_equal,
                hl_destroy_attach_data_key, hl_destroy_attach_data_value);
    }

    if (data == NULL) {
        return g_hash_table_remove(node->inner_data, key);
    }

    HLAttachData *attach = (HLAttachData*)calloc(1, sizeof(HLAttachData));
    attach->data = data;
    attach->callback = destroy_callback;
    return g_hash_table_insert(node->inner_data, (gpointer)strdup(key),
            (gpointer)attach);
}

void *hl_element_node_get_inner_data(HLDomElement *node, const char *key)
{
    if (node == NULL || key == NULL) {
        return NULL;
    }
    HLAttachData *attach = (HLAttachData*) g_hash_table_lookup(
            node->inner_data, (gpointer)key);
    return attach ? attach->data : NULL;
}

gint hl_comp_class_name (gconstpointer a, gconstpointer b)
{
    return strcmp((const char*)a, (const char*)b);
}

int domruler_element_node_has_class (HLDomElement *node,
        const char *class_name)
{
    GList *ret = g_list_find_custom(node->class_list, class_name,
            hl_comp_class_name);
    return ret ? 1 : 0;
}

int domruler_element_node_include_class (HLDomElement *node,
        const char *class_name)
{
    if (node == NULL || class_name == NULL) {
        return DOMRULER_BADPARM;
    }

    if (domruler_element_node_has_class(node, class_name)) {
        return 0;
    }

    const char *classes = domruler_element_node_get_class (node);
    size_t len = (classes ? strlen(classes) : 0)  + strlen(class_name) + 2;
    char *buf = (char*) malloc(len);
    if (classes) {
        strcpy(buf, classes);
        strcat(buf, _DOMRULER_WHITESPACE);
        strcat(buf, class_name);
    }
    else {
        strcpy(buf, class_name);
    }
    domruler_element_node_set_class(node, buf);
    free(buf);
    return 0;
}

int domruler_element_node_exclude_class(HLDomElement *node,
        const char *class_name)
{
    GList *ret = g_list_find_custom(node->class_list,
            class_name, hl_comp_class_name);
    if (!ret) {
        return 0;
    }

    const char *classes = domruler_element_node_get_class(node);
    if (classes == NULL) {
        return 0;
    }

    char *buf = (char*) malloc(strlen(classes) + 1);

    GList *it = NULL;
    for (it = node->class_list; it; it = it->next) {
        if (it == ret) {
            continue;
        }
        strcat(buf, (const char*)it->data);
        strcat(buf, _DOMRULER_WHITESPACE);
    }
    domruler_element_node_set_class(node, buf);
    free(buf);
    return 0;
}

void domruler_element_node_for_each_child(HLDomElement *node,
        NodeCallback callback, void *user_data)
{
    if (node == NULL || callback == NULL) {
        return;
    }

    HLDomElement *child = node->first_child;
    while(child) {
        callback(child, user_data);
        child = child->next;
    }
}

void domruler_element_node_depth_first_search_tree(HLDomElement *node,
        NodeCallback callback, void *user_data)
{
    if (node == NULL || callback == NULL) {
        return;
    }
    callback(node, user_data);

    HLDomElement *child = node->first_child;
    while(child) {
        domruler_element_node_depth_first_search_tree(child,
                callback, user_data);
        child = child->next;
    }
}

int domruler_element_node_set_attach_data(HLDomElement *node,
        uint32_t index, void *data, HlDestroyCallback destroy_callback)
{
    if (node == NULL || index >= MAX_ATTACH_DATA_SIZE) {
        return DOMRULER_BADPARM;
    }

    if (node->attach_data == NULL) {
        node->attach_data = (HLAttachData*)calloc(MAX_ATTACH_DATA_SIZE,
                sizeof(HLAttachData));
    }

    HLAttachData *attach = node->attach_data + index;
    if (attach->data != NULL && attach->callback != NULL) {
        attach->callback(attach->data);
    }

    attach->data = data;
    attach->callback = destroy_callback;
    return DOMRULER_OK;
}

void *domruler_element_node_get_attach_data(const HLDomElement *node,
        uint32_t index)
{
    if (node == NULL || index >= MAX_ATTACH_DATA_SIZE) {
        return NULL;
    }
    HLAttachData *attach = node->attach_data + index;
    return attach->data;
}

// Begin HLDomElelementNode op
HLNodeType hl_dom_element_node_get_type(void *node)
{
    return ((HLDomElement*)node)->inner_dom_type;
}

const char *hl_dom_element_node_get_name(void *node)
{
    return ((HLDomElement*)node)->tag;
}

const char *hl_dom_element_node_get_id(void *node)
{
    return domruler_element_node_get_common_attr((HLDomElement*)node,
            HL_COMMON_ATTR_ID);
}

int hl_dom_element_node_get_classes(void *n, char ***classes)
{
    HLDomElement *node = (HLDomElement*)n;
    int size = g_list_length(node->class_list);

    char **cls = (char**)calloc(size, sizeof(char*));
    GList *it = NULL;
    int i = 0;
    for (it = node->class_list; it; it = it->next) {
        cls[i++]= strdup((const char*)it->data);
    }
    *classes = cls;
    return size;
}

const char *hl_dom_element_node_get_attr(void *n, const char *name)
{
    HLDomElement *node = (HLDomElement*)n;
    if (strcmp(name, ATTR_ID) == 0) {
        return domruler_element_node_get_common_attr(node,
            HL_COMMON_ATTR_ID);
    }
    else if (strcmp(name, ATTR_NAME) == 0) {
        return domruler_element_node_get_common_attr(node,
            HL_COMMON_ATTR_NAME);
    }
    else if (strcmp(name, ATTR_CLASS) == 0) {
        return domruler_element_node_get_common_attr(node,
            HL_COMMON_ATTR_CLASS_NAME);
    }
    return NULL;
}

void hl_dom_element_node_set_parent(void *node, void *parent)
{
    ((HLDomElement*)node)->parent = (HLDomElement*)parent;
}

void *hl_dom_element_node_get_parent(void *node)
{
    return ((HLDomElement*)node)->parent;
}

void *hl_dom_element_node_first_child(void *node)
{
    return ((HLDomElement*)node)->first_child;
}

void *hl_dom_element_node_next(void *node)
{
    return ((HLDomElement*)node)->next;
}

void *hl_dom_element_node_previous(void *node)
{
    return ((HLDomElement*)node)->previous;
}

bool hl_dom_element_node_is_root(void *node)
{
    HLDomElement *parent = ((HLDomElement*)node)->parent;
    if (parent != NULL) {
        return false;
    }
    return true;
}

DOMRulerNodeOp hl_dom_element_node_op = {
    .get_type = hl_dom_element_node_get_type,
    .get_name = hl_dom_element_node_get_name,
    .get_id = hl_dom_element_node_get_id,
    .get_classes = hl_dom_element_node_get_classes,
    .get_attr = hl_dom_element_node_get_attr,
    .set_parent = hl_dom_element_node_set_parent,
    .get_parent = hl_dom_element_node_get_parent,
    .first_child = hl_dom_element_node_first_child,
    .next = hl_dom_element_node_next,
    .previous = hl_dom_element_node_previous,
    .is_root = hl_dom_element_node_is_root
};

DOMRulerNodeOp *hl_dom_element_node_get_op()
{
    return &hl_dom_element_node_op;
}

// End HLDomElelementNode op

