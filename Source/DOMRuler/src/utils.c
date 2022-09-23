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
#include "utils.h"
#include "node.h"
#include "hl_dom_element_node.h"
#include <string.h>

/** Media DPI in fixed point units: defaults to 96, same as nscss_baseline_pixel_density */
//css_fixed default_hl_css_media_dpi = F_96;
int hl_default_media_dpi = 96;

/** Medium screen density for device viewing distance. */
//css_fixed default_hl_css_baseline_pixel_density = F_96;
int hl_default_css_baseline_pixel_density = 96;

/**
 * Convert css pixels to physical pixels.
 *
 * \param[in] css_pixels  Length in css pixels.
 * \return length in physical pixels
 */
css_fixed hl_css_pixels_css_to_physical(const struct DOMRulerCtxt* ctx,
        css_fixed css_pixels)
{
    return FDIV(FMUL(css_pixels, ctx->hl_css_media_dpi),
            ctx->hl_css_baseline_pixel_density);
}

/**
 * Convert physical pixels to css pixels.
 *
 * \param[in] physical_pixels  Length in physical pixels.
 * \return length in css pixels
 */
css_fixed hl_css_pixels_physical_to_css(const struct DOMRulerCtxt* ctx,
        css_fixed physical_pixels)
{
    return FDIV(FMUL(physical_pixels, ctx->hl_css_baseline_pixel_density),
            ctx->hl_css_media_dpi);
}

int hl_set_media_dpi(struct DOMRulerCtxt *ctx, int dpi)
{
    if (dpi <= 0)
    {
        dpi = hl_default_media_dpi;
    }

    if (dpi < 72 || dpi > 250) {
        dpi = min(max(dpi, 72), 250);
    }
    ctx->hl_css_media_dpi = INTTOFIX(dpi);
    return DOMRULER_OK;
}

int hl_set_baseline_pixel_density(struct DOMRulerCtxt* ctx, int density)
{
    if (density <= 0) {
        density = hl_default_css_baseline_pixel_density;
    }

    if (density < 72 || density > 250) {
        density = min(max(density, 72), 250);
    }
    ctx->hl_css_baseline_pixel_density = INTTOFIX(density);
    return DOMRULER_OK;
}

css_unit hl_css_utils_fudge_viewport_units(const struct DOMRulerCtxt *ctx, css_unit unit)
{
    switch (unit) {
        case CSS_UNIT_VI:
            assert(ctx->root_style != NULL);
            if (css_computed_writing_mode(ctx->root_style) ==
                    CSS_WRITING_MODE_HORIZONTAL_TB) {
                unit = CSS_UNIT_VW;
            } else {
                unit = CSS_UNIT_VH;
            }
            break;
        case CSS_UNIT_VB:
            assert(ctx->root_style != NULL);
            if (css_computed_writing_mode(ctx->root_style) ==
                    CSS_WRITING_MODE_HORIZONTAL_TB) {
                unit = CSS_UNIT_VH;
            } else {
                unit = CSS_UNIT_VW;
            }
            break;
        case CSS_UNIT_VMIN:
            if (ctx->vh < ctx->vw) {
                unit = CSS_UNIT_VH;
            } else {
                unit = CSS_UNIT_VW;
            }
            break;
        case CSS_UNIT_VMAX:
            if (ctx->vh > ctx->vw) {
                unit = CSS_UNIT_VH;
            } else {
                unit = CSS_UNIT_VW;
            }
            break;
        default: break;
    }

    return unit;
}

css_fixed hl_css_len2pt(const struct DOMRulerCtxt *ctx, css_fixed length, css_unit unit)
{
    /* Length must not be relative */
    assert(unit != CSS_UNIT_EM &&
            unit != CSS_UNIT_EX &&
            unit != CSS_UNIT_CAP &&
            unit != CSS_UNIT_CH &&
            unit != CSS_UNIT_IC &&
            unit != CSS_UNIT_REM &&
            unit != CSS_UNIT_RLH);

    unit = hl_css_utils_fudge_viewport_units(ctx, unit);

    switch (unit) {
        /* We assume the screen and any other output has the same dpi */
        /* 1in = DPIpx => 1px = (72/DPI)pt */
        case CSS_UNIT_PX: return FDIV(FMUL(length, F_72), F_96);
                          /* 1in = 72pt */
        case CSS_UNIT_IN: return FMUL(length, F_72);
                          /* 1in = 2.54cm => 1cm = (72/2.54)pt */
        case CSS_UNIT_CM: return FMUL(length,
                                  FDIV(F_72, FLTTOFIX(2.54)));
                          /* 1in = 25.4mm => 1mm = (72/25.4)pt */
        case CSS_UNIT_MM: return FMUL(length,
                                  FDIV(F_72, FLTTOFIX(25.4)));
                          /* 1in = 101.6q => 1mm = (72/101.6)pt */
        case CSS_UNIT_Q: return FMUL(length,
                                 FDIV(F_72, FLTTOFIX(101.6)));
        case CSS_UNIT_PT: return length;
                          /* 1pc = 12pt */
        case CSS_UNIT_PC: return FMUL(length, INTTOFIX(12));
        case CSS_UNIT_VH: return FDIV(FMUL(FDIV(FMUL(length, ctx->vh), F_100), F_72), F_96);
        case CSS_UNIT_VW: return FDIV(FMUL(FDIV(FMUL(length,ctx->vw), F_100), F_72), F_96);
        default: break;
    }

    return 0;
}

css_fixed hl_css_len2px(const struct DOMRulerCtxt *ctx,
        css_fixed length, css_unit unit, const css_computed_style *style)
{
    (void)style;
    /* We assume the screen and any other output has the same dpi */
    css_fixed px_per_unit;

    unit = hl_css_utils_fudge_viewport_units(ctx, unit);

    switch (unit) {
        case CSS_UNIT_PX:
            px_per_unit = F_1;
            break;
            /* 1in = 96 CSS pixels */
        case CSS_UNIT_IN:
            px_per_unit = F_96;
            break;
            /* 1in = 2.54cm => 1cm = (DPI/2.54)px */
        case CSS_UNIT_CM:
            px_per_unit = FDIV(F_96, FLTTOFIX(2.54));
            break;
            /* 1in = 25.4mm => 1mm = (DPI/25.4)px */
        case CSS_UNIT_MM:
            px_per_unit = FDIV(F_96, FLTTOFIX(25.4));
            break;
            /* 1in = 101.6q => 1q = (DPI/101.6)px */
        case CSS_UNIT_Q:
            px_per_unit = FDIV(F_96, FLTTOFIX(101.6));
            break;
            /* 1in = 72pt => 1pt = (DPI/72)px */
        case CSS_UNIT_PT:
            px_per_unit = FDIV(F_96, F_72);
            break;
            /* 1pc = 12pt => 1in = 6pc => 1pc = (DPI/6)px */
        case CSS_UNIT_PC:
            px_per_unit = FDIV(F_96, INTTOFIX(6));
            break;
        case CSS_UNIT_VH:
            px_per_unit = FDIV(ctx->vh, F_100);
            break;
        case CSS_UNIT_VW:
            px_per_unit = FDIV(ctx->vw, F_100);
            break;
        default:
            px_per_unit = 0;
            break;
    }

    px_per_unit = hl_css_pixels_css_to_physical(ctx, px_per_unit);

    /* Ensure we round px_per_unit to the nearest whole number of pixels:
     * the use of FIXTOINT() below will truncate. */
    px_per_unit += F_0_5;

    /* Calculate total number of pixels */
    return FMUL(length, TRUNCATEFIX(px_per_unit));
}


uint8_t hl_computed_min_height(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit)
{
    uint8_t value = css_computed_min_height(style, length, unit);

    if (value == CSS_MIN_HEIGHT_AUTO) {
        value = CSS_MIN_HEIGHT_SET;
        *length = 0;
        *unit = CSS_UNIT_PX;
    }

    return value;
}

uint8_t hl_computed_min_width(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit)
{
    uint8_t value = css_computed_min_width(style, length, unit);

    if (value == CSS_MIN_WIDTH_AUTO) {
        value = CSS_MIN_WIDTH_SET;
        *length = 0;
        *unit = CSS_UNIT_PX;
    }

    return value;
}


void hl_destroy_svg_values(HLUsedSvgValues* svg)
{
    if (svg == NULL)
    {
        return;
    }
    // baseline_shift
    // clip-path
    free(svg->clip_path);
    // clip-rule
    // color
    // direction
    // display
    // enable-background
    // comp-op
    // fill
    free(svg->fill_string);
    // fill-opacity
    // fill-rule
    // filter
    free(svg->filter);
    // flood-color
    // flood-opacity
    // font-family
    free(svg->font_family);
    // font-size
    // font-stretch
    // font-style
    // font-variant
    // font-weight
    // marker-end
    free(svg->marker_end);
    // mask
    free(svg->mask);
    // marker-mid
    free(svg->marker_mid);
    // marker-start
    free(svg->marker_start);
    // opacity
    // overflow
    // shape-rendering
    // text-rendering
    // stop-color
    // stop-opacity
    // stroke
    free(svg->stroke_string);
    // stroke-dasharray
    free(svg->stroke_dasharray);
    // stroke-dashoffset
    // stroke-linecap
    // stroke-linejoin
    // stroke-miterlimit
    // stroke-opacity
    // stroke-width
    // text-anchor
    // text-decoration
    // unicode-bidi
    // letter-spacing
    // visibility
    // writing-mode
    free(svg);
}

lwc_string *hl_lwc_string_dup(const char *str)
{
    if (str == NULL) {
        return NULL;
    }

    lwc_string *result = NULL;
    lwc_intern_string(str, strlen(str), &result);
    return result;
}

void hl_lwc_string_destroy(lwc_string *str)
{
    if (str) {
        lwc_string_unref(str);
    }
}
