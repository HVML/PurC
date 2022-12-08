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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/ghash.h>


#define MAX_ATTACH_DATA_SIZE        10

HLLayoutNode *hl_layout_node_create(void)
{
    HLLayoutNode *node = (HLLayoutNode*) calloc(1, sizeof(HLLayoutNode));
    if (!node) {
        return NULL;
    }
    node->box_values.w = HL_UNKNOWN;
    node->box_values.display = HL_DISPLAY_BLOCK;
    node->box_values.position = HL_POSITION_RELATIVE;
    node->box_values.visibility = HL_VISIBILITY_VISIBLE;
    node->box_values.opacity = 1.0f;
    return node;
}

void hl_layout_node_destroy(HLLayoutNode *node)
{
    if (!node) {
        return;
    }

    hl_destroy_svg_values(node->svg_values);

    if (node->text_values.font_family) {
        free(node->text_values.font_family);
    }

    if (node->select_styles) {
        css_select_results_destroy(node->select_styles);
    }

    if (node->inner_data) {
        g_hash_table_destroy(node->inner_data);
    }

    if (node->attach_data) {
        for (int i = 0; i < MAX_ATTACH_DATA_SIZE; i++) {
            HLAttachData* attach = node->attach_data + i;
            if (attach->data && attach->callback) {
                attach->callback(attach->data);
            }
        }
        free(node->attach_data);
    }

    if (node->inner_tag) {
        lwc_string_unref(node->inner_tag);
    }
    if (node->inner_id) {
        lwc_string_unref(node->inner_id);
    }

    if (node->inner_classes) {
        for (int i = 0; i < node->nr_inner_classes; i++) {
            lwc_string_unref(node->inner_classes[i]);
        }
        free(node->inner_classes);
    }
    free(node);
}

int hl_layout_node_set_attach_data(HLLayoutNode *node,
        uint32_t index, void *data, HlDestroyCallback destroy_callback)
{
    if (node == NULL || index >= MAX_ATTACH_DATA_SIZE) {
        return DOMRULER_BADPARM;
    }

    if (node->attach_data == NULL) {
        node->attach_data = (HLAttachData*)calloc(MAX_ATTACH_DATA_SIZE,
                sizeof(HLAttachData));
    }

    HLAttachData* attach = node->attach_data + index;
    if (attach->data != NULL && attach->callback != NULL)
    {
        attach->callback(attach->data);
    }

    attach->data = data;
    attach->callback = destroy_callback;
    return DOMRULER_OK;
}

void *hl_layout_node_get_attach_data(const HLLayoutNode *node,
        uint32_t index)
{
    if (node == NULL || index >= MAX_ATTACH_DATA_SIZE) {
        return NULL;
    }
    HLAttachData* attach = node->attach_data + index;
    return attach->data;
}

void hl_layout_node_destroy_attach_data_key(gpointer data)
{
    free(data);
}

void hl_layout_node_destroy_attach_data_value(gpointer data)
{
    HLAttachData *attach = (HLAttachData *)data;
    if (attach->callback) {
        attach->callback(attach->data);
    }
    free(attach);
}


int hl_layout_node_set_inner_data(HLLayoutNode *node, const char *key,
        void *data, HlDestroyCallback destroy_callback)
{
    if (node == NULL || key == NULL) {
        return DOMRULER_OK;
    }

    if (node->inner_data == NULL) {
        node->inner_data = g_hash_table_new_full(g_str_hash, g_str_equal,
                hl_layout_node_destroy_attach_data_key,
                hl_layout_node_destroy_attach_data_value);
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

void *hl_layout_node_get_inner_data(HLLayoutNode *node, const char *key)
{
    if (node == NULL || key == NULL) {
        return NULL;
    }
    HLAttachData *attach = (HLAttachData*) g_hash_table_lookup(node->inner_data,
            (gpointer)key);
    return attach ? attach->data : NULL;
}

void cb_hl_layout_node_destroy(void *n)
{
    hl_layout_node_destroy((HLLayoutNode *)n);
}

int hl_find_background(HLLayoutNode *node)
{
    css_color color;
    css_computed_background_color(node->computed_style, &color);
    node->background_values.color = color;
    return DOMRULER_OK;
}

int hl_find_font(struct DOMRulerCtxt *ctx, HLLayoutNode *node)
{
    lwc_string **families;
    css_fixed length = 0;
    css_unit unit = CSS_UNIT_PX;

    HLLayoutNode* parent = hl_layout_node_get_parent(node);

    uint8_t val = css_computed_font_family(node->computed_style, &families);
    if (val == CSS_FONT_FAMILY_INHERIT) {
        HL_LOGD("node node|tag=%s|id=%s|font inherit\n", node->tag, node->id);
        if (parent && parent->text_values.font_family) {
            free (node->text_values.font_family);
            node->text_values.font_family =
                strdup(parent->text_values.font_family);
        }
    }
    else {
        char* buf[1024] = {0};
        int index = 0;
        int len = 0;
        if (families != NULL) {
            while (*families != NULL) {
                buf[index] = strdup(lwc_string_data(*families));
                len += strlen(buf[index]) + 1;
                index++;
                families++;
            }
        }

        switch (val) {
            case CSS_FONT_FAMILY_SERIF:
                buf[index] = strdup("serif");
                len += strlen(buf[index]) + 1;
                index++;
                break;
            case CSS_FONT_FAMILY_SANS_SERIF:
                buf[index] = strdup("sans-serif");
                len += strlen(buf[index]) + 1;
                index++;
                break;
            case CSS_FONT_FAMILY_CURSIVE:
                buf[index] = strdup("cursive");
                len += strlen(buf[index]) + 1;
                index++;
                break;
            case CSS_FONT_FAMILY_FANTASY:
                buf[index] = strdup("fantasy");
                len += strlen(buf[index]) + 1;
                index++;
                break;
            case CSS_FONT_FAMILY_MONOSPACE:
                buf[index] = strdup("monospace");
                len += strlen(buf[index]) + 1;
                index++;
                break;
        }

        char* result = (char*)calloc(len + 1, 1);
        for (int i=0; i<index; i++) {
            strcat(result, buf[i]);
            strcat(result, ",");
            free(buf[i]);
        }
        result[strlen(result) - 1 ] = 0;
        free (node->text_values.font_family);
        node->text_values.font_family = result;
    }

    css_computed_font_size(node->computed_style, &length, &unit);
    int text_height = hl_css_len2px(ctx, length, unit, node->computed_style);
    node->text_values.font_size = FIXTOINT(text_height * 3 / 4);

    css_color color;
    val = css_computed_color(node->computed_style, &color);
    if (val == CSS_COLOR_INHERIT) {
        if (parent) {
            node->text_values.color = parent->text_values.color;
        }
    } else if (val == CSS_COLOR_COLOR) {
        node->text_values.color = color;
    }

    val = css_computed_font_weight(node->computed_style);
    switch (val) {
    case CSS_FONT_WEIGHT_100:
        node->text_values.font_weight = HLFONT_WEIGHT_THIN;
        break;
    case CSS_FONT_WEIGHT_200:
        node->text_values.font_weight = HLFONT_WEIGHT_EXTRA_LIGHT;
        break;
    case CSS_FONT_WEIGHT_300:
        node->text_values.font_weight = HLFONT_WEIGHT_LIGHT;
        break;
    case CSS_FONT_WEIGHT_400:
    case CSS_FONT_WEIGHT_NORMAL:
    default:
        node->text_values.font_weight = HLFONT_WEIGHT_NORMAL;
        break;
    case CSS_FONT_WEIGHT_500:
        node->text_values.font_weight = HLFONT_WEIGHT_MEDIUM;
        break;
    case CSS_FONT_WEIGHT_600:
        node->text_values.font_weight = HLFONT_WEIGHT_DEMIBOLD;
        break;
    case CSS_FONT_WEIGHT_700:
    case CSS_FONT_WEIGHT_BOLD:
        node->text_values.font_weight = HLFONT_WEIGHT_BOLD;
        break;
    case CSS_FONT_WEIGHT_800:
        node->text_values.font_weight = HLFONT_WEIGHT_EXTRA_BOLD;
        break;
    case CSS_FONT_WEIGHT_900:
        node->text_values.font_weight = HLFONT_WEIGHT_BLACK;
        break;
    }

    return 0;
}

HLGridItem *hl_grid_item_create(HLLayoutNode *node)
{
    if (node == NULL) {
        return NULL;
    }

    css_fixed value = 0;
    css_unit unit = CSS_UNIT_PX;

    HLGridItem *item = calloc(1, sizeof(HLGridItem));

    int8_t type = css_computed_grid_column_start(node->computed_style,
            &value, &unit);
    if (type == CSS_GRID_COLUMN_START_SET) {
        item->rc_set = item->rc_set | HL_GRID_ITEM_RC_COLUMN_START;
        item->column_start = FIXTOINT(value);
    }

    type = css_computed_grid_column_end(node->computed_style, &value, &unit);
    if (type == CSS_GRID_COLUMN_END_SET) {
        item->rc_set = item->rc_set | HL_GRID_ITEM_RC_COLUMN_END;
        item->column_end = FIXTOINT(value);
    }

    type = css_computed_grid_row_start(node->computed_style, &value, &unit);
    if (type == CSS_GRID_ROW_START_SET) {
        item->rc_set = item->rc_set | HL_GRID_ITEM_RC_ROW_START;
        item->row_start = FIXTOINT(value);
    }

    type = css_computed_grid_row_end(node->computed_style, &value, &unit);
    if (type == CSS_GRID_ROW_END_SET) {
        item->rc_set = item->rc_set | HL_GRID_ITEM_RC_ROW_END;
        item->row_end = FIXTOINT(value);
    }

    hl_layout_node_set_inner_data(node, HL_INNER_LAYOUT_ATTACH, item, NULL);
    return item;
}

void hl_grid_item_destroy(HLGridItem *p)
{
    if (p) {
        free(p);
    }
}

HLGridTemplate *hl_grid_template_create(const struct DOMRulerCtxt *ctx,
        HLLayoutNode *node)
{
    if (node == NULL) {
        return NULL;
    }

    int row_size = 0;
    css_fixed *row_values = NULL;
    css_unit *row_units = NULL;

    int column_size = 0;
    css_fixed *column_values = NULL;
    css_unit *column_units = NULL;

    uint8_t ret = 0;

    ret = css_computed_grid_template_rows(node->computed_style,
            &row_size, &row_values, &row_units);
    if (ret != CSS_GRID_TEMPLATE_ROWS_SET) {
        return NULL;
    }

    ret = css_computed_grid_template_columns(node->computed_style,
            &column_size, &column_values, &column_units);
    if (ret != CSS_GRID_TEMPLATE_COLUMNS_SET) {
        return NULL;
    }

    HLGridTemplate *gt = (HLGridTemplate*)calloc(1, sizeof(HLGridTemplate));
    gt->x = node->box_values.x;
    gt->y = node->box_values.y;
    gt->w = node->box_values.w;
    gt->h = node->box_values.h;

    gt->n_row = row_size;
    gt->n_column = column_size;
    gt->mask = (uint8_t**)calloc(gt->n_row, sizeof(uint8_t*));
    for (int i = 0; i < gt->n_row; i++) {
        gt->mask[i] = (uint8_t*)calloc(gt->n_column, sizeof(uint8_t));
    }

    gt->rows = (int32_t*)malloc(gt->n_row * sizeof(int32_t));
    gt->columns = (int32_t*)malloc(gt->n_column * sizeof(int32_t));

    for (int i = 0; i < row_size; i++) {
        if (row_units[i] == CSS_UNIT_PCT) {
            gt->rows[i] = HL_FPCT_OF_INT_TOINT(row_values[i], gt->h);
        } else {
            gt->rows[i] = FIXTOINT(hl_css_len2px(ctx, row_values[i],
                        row_units[i], node->computed_style));
        }
    }

    for (int i = 0; i < column_size; i++)
    {
        if (column_units[i] == CSS_UNIT_PCT) {
            gt->columns[i] = HL_FPCT_OF_INT_TOINT(column_values[i], gt->w);
        } else {
            gt->columns[i] = FIXTOINT(hl_css_len2px(ctx, column_values[i],
                        column_units[i], node->computed_style));
        }
    }

    free(row_values);
    free(row_units);
    free(column_values);
    free(column_units);

    return gt;
}

void hl_grid_template_destroy(HLGridTemplate *p)
{
    if (p == NULL) {
        return;
    }

    if (p->rows != NULL) {
        free(p->rows);
    }

    if (p->columns != NULL) {
        free(p->columns);
    }

    if (p->mask != NULL) {
        for (int i = 0; i < p->n_row; i++) {
            free(p->mask[i]);
        }
    }

    free(p->mask);
    free(p);
}

void hl_for_each_child(struct DOMRulerCtxt *ctx, HLLayoutNode *node,
        each_child_callback callback, void *user_data)
{
    if (ctx == NULL || node == NULL || callback == NULL) {
        return;
    }

    HLLayoutNode *child = hl_layout_node_first_child(node);
    while(child) {
        callback(ctx, child, user_data);
        child = hl_layout_node_next(child);
    }
}

// BEGIN: HLLayoutNode  < ----- > Origin Node
HLLayoutNode *hl_layout_node_from_origin_node(struct DOMRulerCtxt *ctxt,
        void *origin)
{
    if (!ctxt || !origin) {
        return NULL;
    }

    HLLayoutNode *layout = (HLLayoutNode*)g_hash_table_lookup(ctxt->node_map,
            (gpointer)origin);
    if (layout) {
        return layout;
    }

    layout = hl_layout_node_create();
    if (!layout) {
        return NULL;
    }
    layout->ctxt = ctxt;

    // inner_id
    const char *id = ctxt->origin_op->get_id(origin);
    if (id) {
        layout->inner_id = hl_lwc_string_dup(id);
    }

    // inner_tag
    const char *name = ctxt->origin_op->get_name(origin);
    if (name) {
        layout->inner_tag = hl_lwc_string_dup(name);
    }
    // inner_classes
    char **classes = NULL;
    int nr_classes = ctxt->origin_op->get_classes(origin, &classes);
    if (nr_classes > 0) {
        layout->inner_classes = (lwc_string**)calloc(nr_classes,
                sizeof(lwc_string*));
        for (int i = 0; i < nr_classes; i++) {
            // VM FIXED
            // layout->inner_classes[i++]= hl_lwc_string_dup(classes[i]);
            layout->inner_classes[i]= hl_lwc_string_dup(classes[i]);
            free(classes[i]);
        }
        layout->nr_inner_classes = nr_classes;
        free(classes);
    }
    else if (classes) {
        free(classes);
    }


    layout->origin = origin;
    layout->ctxt = ctxt;
    g_hash_table_insert(ctxt->node_map, (gpointer)origin, (gpointer)layout);
    return layout;
}

void *hl_layout_node_to_origin_node(HLLayoutNode *layout,
        DOMRulerNodeOp **op)
{
    if (!layout->origin) {
        return NULL;
    }
    if (op) {
        *op = layout->ctxt->origin_op;
    }
    return layout->origin;
}

HLNodeType hl_layout_node_get_type(HLLayoutNode *node)
{
    return node->ctxt->origin_op->get_type(node->origin);
}

const char *hl_layout_node_get_name(HLLayoutNode *node)
{
    return node->ctxt->origin_op->get_name(node->origin);
}

const char *hl_layout_node_get_id(HLLayoutNode *node)
{
    return node->ctxt->origin_op->get_id(node->origin);
}

int hl_layout_node_get_classes(HLLayoutNode *node, char ***classes)
{
    return node->ctxt->origin_op->get_classes(node->origin, classes);
}

const char *hl_layout_node_get_attr(HLLayoutNode *node, const char *attr)
{
    return node->ctxt->origin_op->get_attr(node->origin, attr);
}

HLLayoutNode *hl_layout_node_get_parent(HLLayoutNode *node)
{
    void *origin = node->ctxt->origin_op->get_parent(node->origin);
    return hl_layout_node_from_origin_node(node->ctxt, origin);
}

void hl_layout_node_set_parent(HLLayoutNode *node, HLLayoutNode *parent)
{
    node->ctxt->origin_op->set_parent(node->origin, parent->origin);
}

HLLayoutNode *hl_layout_node_first_child(HLLayoutNode *node)
{
    void *origin = node->ctxt->origin_op->first_child(node->origin);
    return hl_layout_node_from_origin_node(node->ctxt,  origin);
}

HLLayoutNode *hl_layout_node_next(HLLayoutNode *node)
{
    void *origin = node->ctxt->origin_op->next(node->origin);
    return hl_layout_node_from_origin_node(node->ctxt, origin);
}

HLLayoutNode *hl_layout_node_previous(HLLayoutNode *node)
{
    void *origin = node->ctxt->origin_op->previous(node->origin);
    return hl_layout_node_from_origin_node(node->ctxt, origin);
}

bool hl_layout_node_is_root(HLLayoutNode *node)
{
    return node->ctxt->origin_op->is_root(node->origin);
}

// END: HLLayoutNode  < ----- > Origin Node
