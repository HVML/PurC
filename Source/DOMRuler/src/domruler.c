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

#include "domruler.h"
#include "internal.h"
#include "layout.h"

#include "utils.h"
#include "select.h"

#include "hldom_node_ops.h"
#include "pcdom_node_ops.h"

#include <stdio.h>
#include <stdlib.h>

#if 0
static inline
bool hl_verify_handler(DOMRulerNodeOp *op)
{
    if (!op || !op->get_type || !op->get_name
            || !op->get_id || !op->get_classes
            || !op->get_attr
            || !op->set_parent || !op->get_parent
            || !op->first_child || !op->next
            || !op->previous
            || !op->is_root) {
        return false;
    }
    return true;
}
#endif

struct DOMRulerCtxt *domruler_create(uint32_t width, uint32_t height,
        uint32_t dpi, uint32_t density)
{
    struct DOMRulerCtxt *ctxt = (struct DOMRulerCtxt *)calloc(1,
            sizeof(struct DOMRulerCtxt));
    if (ctxt == NULL) {
        return NULL;
    }
    ctxt->width = width;
    ctxt->height = height;
    ctxt->dpi = dpi;
    ctxt->density = density;

    ctxt->node_map = g_hash_table_new_full(g_direct_hash,
            g_direct_equal, NULL, cb_hl_layout_node_destroy);
    if (ctxt->node_map == NULL) {
        domruler_destroy(ctxt);
        return NULL;
    }
    return ctxt;
}

int domruler_append_css(struct DOMRulerCtxt *ctxt, const char *css,
        size_t nr_css)
{
    if (!ctxt || !css) {
        return DOMRULER_BADPARM;
    }
    if (!ctxt->css) {
        ctxt->css = domruler_css_create();
        if (!ctxt->css) {
            return DOMRULER_NOMEM;
        }
    }
    return domruler_css_append_data(ctxt->css, css, nr_css);
}

int domruler_layout(struct DOMRulerCtxt *ctxt, void *root_node,
        DOMRulerNodeOp *op)
{
    ctxt->origin_root = root_node;
    ctxt->origin_op = op;
    HLLayoutNode *layout_node = hl_layout_node_from_origin_node(ctxt, root_node);
    return hl_layout_do_layout(ctxt, layout_node);
}

const HLBox *domruler_get_node_bounding_box(struct DOMRulerCtxt *ctxt,
        void *node)
{
    HLLayoutNode *layout = (HLLayoutNode*)g_hash_table_lookup(ctxt->node_map,
            (gpointer)node);
    return layout ? &layout->box_values : NULL;
}

void domruler_destroy(struct DOMRulerCtxt *ctxt)
{
    if (!ctxt) {
        return;
    }

    if (ctxt->css) {
        domruler_css_destroy(ctxt->css);
    }

    if (ctxt->node_map) {
        g_hash_table_destroy(ctxt->node_map);
    }
    free(ctxt);
}

/* VM: reset elements for re-layout when DOM changed */
void domruler_reset_nodes(struct DOMRulerCtxt *ctxt)
{
    if (ctxt && ctxt->node_map) {
        g_hash_table_remove_all(ctxt->node_map);
    }
}

int domruler_layout_hldom_elements(struct DOMRulerCtxt *ctxt,
        HLDomElement *root_node)
{
    return domruler_layout(ctxt, root_node, hldom_node_get_op());
}

int domruler_layout_pcdom_elements(struct DOMRulerCtxt *ctxt,
        pcdom_element_t *root_node)
{
    return domruler_layout(ctxt, root_node, pcdom_node_get_op());
}


