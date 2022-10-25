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

#ifndef _HL_UTILS_H_
#define _HL_UTILS_H_

#include "domruler.h"
#include "internal.h"
#include <csseng/csseng.h>

// top, right, bottom, left
#define HL_TOP          0
#define HL_RIGHT        1
#define HL_BOTTOM       2
#define HL_LEFT         3

/* Fixed point percentage (a) of an integer (b), to an integer */
#define HL_FPCT_OF_INT_TOINT(a, b) (FIXTOINT(FDIV((a * b), F_100)))

/** 22:10 fixed point */
#define HL_PLOT_STYLE_RADIX (10)

/** Scaling factor for plot styles */
#define HL_PLOT_STYLE_SCALE (1 << HL_PLOT_STYLE_RADIX)

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

struct HLLayoutNode;

#ifdef __cplusplus
extern "C" {
#endif

css_fixed hl_css_pixels_css_to_physical(const struct DOMRulerCtxt *ctx,
        css_fixed css_pixels);
css_fixed hl_css_pixels_physical_to_css(const struct DOMRulerCtxt *ctx,
        css_fixed physical_pixels);
int hl_set_media_dpi(struct DOMRulerCtxt *ctx, int dpi);
int hl_set_baseline_pixel_density(struct DOMRulerCtxt *ctx, int density);

css_unit hl_css_utils_fudge_viewport_units(const struct DOMRulerCtxt *ctx, css_unit unit);
css_fixed hl_css_len2pt(const struct DOMRulerCtxt *ctx, css_fixed length, css_unit unit);
css_fixed hl_css_len2px(const struct DOMRulerCtxt *ctx,
        css_fixed length, css_unit unit, const css_computed_style *style);

uint8_t hl_computed_min_height(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);
uint8_t hl_computed_min_width(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);


void hl_destroy_svg_values(HLUsedSvgValues *svg);
lwc_string *hl_lwc_string_dup(const char *str);
void hl_lwc_string_destroy(lwc_string *str);

#ifdef __cplusplus
}
#endif

#endif // _HL_UTILS_H_

