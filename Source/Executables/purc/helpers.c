/*
 * @file helpers.c
 * @author Vincent Wei
 * @date 2022/11/13
 * @brief The imlementation of some helpers.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#include "config.h"
#include "foil.h"

#include <purc/purc-document.h>
#include <glib.h>
#include <assert.h>

int foil_doc_get_element_lang(purc_document_t doc, pcdoc_element_t ele,
        const char **lang, size_t *len)
{
    int ret = pcdoc_element_get_attribute(doc, ele, "lang", lang, len);
    if (ret == 0)
        return 0;

    pcdoc_node node;
    node.type = PCDOC_NODE_ELEMENT;
    node.elem = ele;
    pcdoc_element_t parent = pcdoc_node_get_parent(doc, node);
    if (parent) {
        return foil_doc_get_element_lang(doc, parent, lang, len);
    }

    return -1;
}

int foil_ucs_calc_width_nowrap(const uint32_t *ucs, size_t nr_ucs)
{
    int w = 0;

    for (size_t i = 0; i < nr_ucs; i++) {
        if (g_unichar_isprint(ucs[i])) {
            if (g_unichar_iswide(ucs[i])) {
                w += FOIL_PX_GRID_CELL_W * 2;
            }
            else {
                w += FOIL_PX_GRID_CELL_W;
            }
        }
    }

    return w;
}

uint8_t foil_map_xrgb_to_256c(uint32_t xrgb)
{
    uint8_t r, g, b;

    // a = ((uint8_t)((uint32_t)xrgb >> 24));
    r = ((uint8_t)((uint32_t)xrgb >> 16));
    g = ((uint8_t)((uint32_t)xrgb >> 8));
    b = ((uint8_t)((uint32_t)xrgb));

    // R3G2B3
    return ((r & 0x07) << 5) | ((g & 0x03) << 3) | (b & 0x07);
}

uint8_t foil_map_xrgb_to_16c(uint32_t xrgb)
{
    uint8_t c = 0xFF;
    uint8_t r, g, b;

    // a = ((uint8_t)((uint32_t)xrgb >> 24));
    r = ((uint8_t)((uint32_t)xrgb >> 16));
    g = ((uint8_t)((uint32_t)xrgb >> 8));
    b = ((uint8_t)((uint32_t)xrgb));

    if (r > 0x80 && r <= 0xC0 &&
            g > 0x80 && g <= 0xC0 &&
            b > 0x80 && b <= 0xC0) {
        // {0xC0, 0xC0, 0xC0},     // gray    --7
        c = FOIL_STD_COLOR_GRAY;
    }
    else if (r > 0xC0) {
        if (g > 0xC0) {
            if (b > 0xC0) {
                // {0xFF, 0xFF, 0xFF},     // light white   --15
                c = FOIL_STD_COLOR_WHITE;
            }
            else {
                // {0xFF, 0xFF, 0x00},     // yellow        --11
                c = FOIL_STD_COLOR_YELLOW;
            }
        }
        else {
            if (b > 0xC0) {
                // {0xFF, 0x00, 0xFF},     // magenta       --13
                c = FOIL_STD_COLOR_MAGENTA;
            }
            else {
                // {0xFF, 0x00, 0x00},     // red           --9
                c = FOIL_STD_COLOR_RED;
            }
        }
    }
    else {
        if (g > 0xC0) {
            if (b > 0xC0) {
                // {0x00, 0xFF, 0xFF},     // cyan          --14
                c = FOIL_STD_COLOR_CYAN;
            }
            else {
                // {0x00, 0xFF, 0x00},     // green         --10
                c = FOIL_STD_COLOR_GREEN;
            }
        }
        else {
            if (b > 0xC0) {
                // {0x00, 0x00, 0xFF},     // blue          --12
                c = FOIL_STD_COLOR_BLUE;
            }
        }
    }

    if (c != 0xFF)
        goto done;

    if (r <= 0x40 && g <= 0x40 && b <= 0x40) {
        // {0x00, 0x00, 0x00},     // black         --0
        c = FOIL_STD_COLOR_BLACK;
    }
    else if (r > 0x40) {
        if (g > 0x40) {
            if (b > 0x40) {
                // {0x80, 0x80, 0x80},     // dark gray     --8
                c = FOIL_STD_COLOR_DARK_GRAY;
            }
            else {
                // {0x80, 0x80, 0x00},     // dark yellow   --3
                c = FOIL_STD_COLOR_DARK_YELLOW;
            }
        }
        else {
            if (b > 0x40) {
                // {0x80, 0x00, 0x80},     // dark magenta  --5
                c = FOIL_STD_COLOR_DARK_MAGENTA;
            }
            else {
                // {0x80, 0x00, 0x00},     // dark red      --1
                c = FOIL_STD_COLOR_DARK_RED;
            }
        }
    }
    else {
        if (g > 0x40) {
            if (b > 0x40) {
                // {0x00, 0x80, 0x80},     // dark cyan     --6
                c = FOIL_STD_COLOR_DARK_CYAN;
            }
            else {
                // {0x00, 0x80, 0x00},     // dark green    --2
                c = FOIL_STD_COLOR_DARK_GREEN;
            }
        }
        else {
            if (b > 0x40) {
                // {0x00, 0x00, 0x80},     // dark blue     --4
                c = FOIL_STD_COLOR_DARK_BLUE;
            }
        }
    }

    assert(c != 0xFF);

done:
    return c;
}

