/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 * Copyright (C) 2021 Beijing FMSoft Technologies Co., Ltd.
 */

#ifndef libcss_computed_h_
#define libcss_computed_h_

#include "csseng-wapcaplet.h"
#include "csseng-errors.h"
#include "csseng-functypes.h"
#include "csseng-properties.h"
#include "csseng-types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct css_hint;
struct css_select_handler;

typedef struct css_computed_counter {
	lwc_string *name;
	css_fixed value;
} css_computed_counter;

typedef struct css_computed_clip_rect {
	css_fixed top;
	css_fixed right;
	css_fixed bottom;
	css_fixed left;

	css_unit tunit;
	css_unit runit;
	css_unit bunit;
	css_unit lunit;

	bool top_auto;
	bool right_auto;
	bool bottom_auto;
	bool left_auto;
} css_computed_clip_rect;

enum css_computed_content_type {
	CSS_COMPUTED_CONTENT_NONE		= 0,
	CSS_COMPUTED_CONTENT_STRING		= 1,
	CSS_COMPUTED_CONTENT_URI		= 2,
	CSS_COMPUTED_CONTENT_COUNTER		= 3,
	CSS_COMPUTED_CONTENT_COUNTERS		= 4,
	CSS_COMPUTED_CONTENT_ATTR		= 5,
	CSS_COMPUTED_CONTENT_OPEN_QUOTE		= 6,
	CSS_COMPUTED_CONTENT_CLOSE_QUOTE	= 7,
	CSS_COMPUTED_CONTENT_NO_OPEN_QUOTE	= 8,
	CSS_COMPUTED_CONTENT_NO_CLOSE_QUOTE	= 9
};

typedef struct css_computed_content_item {
	uint8_t type;
	union {
		lwc_string *string;
		lwc_string *uri;
		lwc_string *attr;
		struct {
			lwc_string *name;
			uint8_t style;
		} counter;
		struct {
			lwc_string *name;
			lwc_string *sep;
			uint8_t style;
		} counters;
	} data;
} css_computed_content_item;

css_error css_computed_style_destroy(css_computed_style *style);

css_error css_computed_style_compose(
		const css_computed_style *restrict parent,
		const css_computed_style *restrict child,
		css_error (*compute_font_size)(void *pw,
				const struct css_hint *parent,
				struct css_hint *size),
		void *pw,
		css_computed_style **restrict result);

/******************************************************************************
 * Property accessors below here                                              *
 ******************************************************************************/

uint8_t css_computed_letter_spacing(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_outline_color(
		const css_computed_style *style, css_color *color);

uint8_t css_computed_outline_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_border_spacing(
		const css_computed_style *style,
		css_fixed *hlength, css_unit *hunit,
		css_fixed *vlength, css_unit *vunit);

uint8_t css_computed_word_spacing(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_counter_increment(
		const css_computed_style *style,
		const css_computed_counter **counters);

uint8_t css_computed_counter_reset(
		const css_computed_style *style,
		const css_computed_counter **counters);

uint8_t css_computed_cursor(
		const css_computed_style *style,
		lwc_string ***urls);

uint8_t css_computed_clip(
		const css_computed_style *style,
		css_computed_clip_rect *rect);

uint8_t css_computed_content(
		const css_computed_style *style,
		const css_computed_content_item **content);

uint8_t css_computed_vertical_align(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_font_size(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_border_top_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_border_right_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_border_bottom_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_border_left_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_background_image(
		const css_computed_style *style,
		lwc_string **url);

uint8_t css_computed_color(
		const css_computed_style *style,
		css_color *color);

uint8_t css_computed_list_style_image(
		const css_computed_style *style,
		lwc_string **url);

uint8_t css_computed_quotes(
		const css_computed_style *style,
		lwc_string ***quotes);

uint8_t css_computed_top(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_right(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_bottom(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_left(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_border_top_color(
		const css_computed_style *style,
		css_color *color);

uint8_t css_computed_border_right_color(
		const css_computed_style *style,
		css_color *color);

uint8_t css_computed_border_bottom_color(
		const css_computed_style *style,
		css_color *color);

uint8_t css_computed_border_left_color(
		const css_computed_style *style,
		css_color *color);

uint8_t css_computed_box_sizing(
		const css_computed_style *style);

uint8_t css_computed_height(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_line_height(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_background_color(
		const css_computed_style *style,
		css_color *color);

uint8_t css_computed_z_index(
		const css_computed_style *style,
		int32_t *z_index);

uint8_t css_computed_margin_top(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_margin_right(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_margin_bottom(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_margin_left(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_background_attachment(
		const css_computed_style *style);

uint8_t css_computed_border_collapse(
		const css_computed_style *style);

uint8_t css_computed_caption_side(
		const css_computed_style *style);

uint8_t css_computed_direction(
		const css_computed_style *style);

uint8_t css_computed_max_height(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_max_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_empty_cells(
		const css_computed_style *style);

uint8_t css_computed_float(
		const css_computed_style *style);

uint8_t css_computed_writing_mode(
		const css_computed_style *style);

uint8_t css_computed_font_style(
		const css_computed_style *style);

uint8_t css_computed_min_height(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_min_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_background_repeat(
		const css_computed_style *style);

uint8_t css_computed_clear(
		const css_computed_style *style);

uint8_t css_computed_padding_top(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_padding_right(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_padding_bottom(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_padding_left(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_overflow_x(
		const css_computed_style *style);

uint8_t css_computed_overflow_y(
		const css_computed_style *style);

uint8_t css_computed_position(
		const css_computed_style *style);

uint8_t css_computed_opacity(
		const css_computed_style *style,
		css_fixed *opacity);

uint8_t css_computed_text_transform(
		const css_computed_style *style);

uint8_t css_computed_text_indent(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_text_overflow(
		const css_computed_style *style,
		lwc_string **string);

uint8_t css_computed_white_space(
		const css_computed_style *style);

uint8_t css_computed_background_position(
		const css_computed_style *style,
		css_fixed *hlength, css_unit *hunit,
		css_fixed *vlength, css_unit *vunit);

uint8_t css_computed_break_after(
		const css_computed_style *style);

uint8_t css_computed_break_before(
		const css_computed_style *style);

uint8_t css_computed_break_inside(
		const css_computed_style *style);

uint8_t css_computed_column_count(
		const css_computed_style *style,
		int32_t *column_count);

uint8_t css_computed_column_fill(
		const css_computed_style *style);

uint8_t css_computed_column_gap(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_column_rule_color(
		const css_computed_style *style,
		css_color *color);

uint8_t css_computed_column_rule_style(
		const css_computed_style *style);

uint8_t css_computed_column_rule_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_column_span(
		const css_computed_style *style);

uint8_t css_computed_column_width(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_display(
		const css_computed_style *style, bool root);

uint8_t css_computed_display_static(
		const css_computed_style *style);

uint8_t css_computed_font_variant(
		const css_computed_style *style);

uint8_t css_computed_text_decoration(
		const css_computed_style *style);

uint8_t css_computed_font_family(
		const css_computed_style *style,
		lwc_string ***names);

uint8_t css_computed_border_top_style(
		const css_computed_style *style);

uint8_t css_computed_border_right_style(
		const css_computed_style *style);

uint8_t css_computed_border_bottom_style(
		const css_computed_style *style);

uint8_t css_computed_border_left_style(
		const css_computed_style *style);

uint8_t css_computed_font_weight(
		const css_computed_style *style);

uint8_t css_computed_list_style_type(
		const css_computed_style *style);

uint8_t css_computed_outline_style(
		const css_computed_style *style);

uint8_t css_computed_table_layout(
		const css_computed_style *style);

uint8_t css_computed_unicode_bidi(
		const css_computed_style *style);

uint8_t css_computed_visibility(
		const css_computed_style *style);

uint8_t css_computed_list_style_position(
		const css_computed_style *style);

uint8_t css_computed_text_align(
		const css_computed_style *style);

uint8_t css_computed_page_break_after(
		const css_computed_style *style);

uint8_t css_computed_page_break_before(
		const css_computed_style *style);

uint8_t css_computed_page_break_inside(
		const css_computed_style *style);

uint8_t css_computed_orphans(
		const css_computed_style *style,
		int32_t *orphans);

uint8_t css_computed_widows(
		const css_computed_style *style,
		int32_t *widows);

uint8_t css_computed_align_content(
		const css_computed_style *style);

uint8_t css_computed_align_items(
		const css_computed_style *style);

uint8_t css_computed_align_self(
		const css_computed_style *style);

uint8_t css_computed_flex_basis(
		const css_computed_style *style,
		css_fixed *length,
		css_unit *unit);

uint8_t css_computed_flex_direction(
		const css_computed_style *style);

uint8_t css_computed_flex_grow(
		const css_computed_style *style,
		css_fixed *number);

uint8_t css_computed_flex_shrink(
		const css_computed_style *style,
		css_fixed *number);

uint8_t css_computed_flex_wrap(
		const css_computed_style *style);

uint8_t css_computed_justify_content(
		const css_computed_style *style);

uint8_t css_computed_order(
		const css_computed_style *style,
		int32_t *order);

uint8_t css_computed_grid_column_start(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_grid_column_end(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_grid_row_start(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_grid_row_end(
		const css_computed_style *style,
		css_fixed *length, css_unit *unit);

uint8_t css_computed_grid_template_columns(
		const css_computed_style *style,
        int32_t* n_values,
		css_fixed** values, css_unit** units);

uint8_t css_computed_grid_template_rows(
		const css_computed_style *style,
        int32_t* n_values,
		css_fixed** values, css_unit** units);

uint8_t css_computed_border_top_left_radius(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);

uint8_t css_computed_border_top_right_radius(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);

uint8_t css_computed_border_bottom_left_radius(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);

uint8_t css_computed_border_bottom_right_radius(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);

uint8_t css_computed_text_align_last(
        const css_computed_style *style);

uint8_t css_computed_text_justify(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);

uint8_t css_computed_text_shadow(
        const css_computed_style *style,
        css_fixed* text_shadow_h, css_unit* text_shadow_h_unit, 
        css_fixed* text_shadow_v, css_unit* text_shadow_v_unit,
        css_fixed* text_shadow_blur, css_unit* text_shadow_blur_unit ,
        css_color* text_shadow_color);

uint8_t css_computed_word_break(
        const css_computed_style *style);

uint8_t css_computed_line_break(
        const css_computed_style *style);

uint8_t css_computed_word_wrap(
        const css_computed_style *style);

uint8_t css_computed_baseline_shift(
        const css_computed_style *style);

uint8_t css_computed_clip_path(const css_computed_style *style,
        lwc_string **string);

uint8_t css_computed_clip_rule(
        const css_computed_style *style);

uint8_t css_computed_comp_op(
        const css_computed_style *style);

uint8_t css_computed_enable_background(
        const css_computed_style *style);

uint8_t css_computed_fill(
        const css_computed_style *style,
        lwc_string **string, css_color* color);

uint8_t css_computed_fill_opacity(
        const css_computed_style *style,
        css_fixed *length);

uint8_t css_computed_fill_rule(
        const css_computed_style *style);

uint8_t css_computed_filter(
        const css_computed_style *style,
        lwc_string **string);

uint8_t css_computed_flood_color(
        const css_computed_style *style,
        css_color *color);

uint8_t css_computed_flood_opacity(
        const css_computed_style *style,
        css_fixed *length);

uint8_t css_computed_font_stretch(
        const css_computed_style *style);

uint8_t css_computed_marker_start(
        const css_computed_style *style,
        lwc_string **string);

uint8_t css_computed_marker_mid(
        const css_computed_style *style,
        lwc_string **string);

uint8_t css_computed_marker_end(
        const css_computed_style *style,
        lwc_string **string);

uint8_t css_computed_mask(
        const css_computed_style *style,
        lwc_string **string);

uint8_t css_computed_shape_rendering(
        const css_computed_style *style);

uint8_t css_computed_stop_color(
        const css_computed_style *style,
        css_color *color);

uint8_t css_computed_stop_opacity(
        const css_computed_style *style,
        css_fixed *length);

uint8_t css_computed_stroke(
        const css_computed_style *style,
        lwc_string **string, css_color* color);

uint8_t css_computed_stroke_width(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);

uint8_t css_computed_stroke_opacity(
        const css_computed_style *style,
        css_fixed *length);

uint8_t css_computed_stroke_dasharray(
		const css_computed_style *style,
        int32_t* n_values,
		css_fixed** values, css_unit** units);

uint8_t css_computed_stroke_dashoffset(
        const css_computed_style *style,
        css_fixed *length, css_unit *unit);

uint8_t css_computed_stroke_linecap(
        const css_computed_style *style);

uint8_t css_computed_stroke_linejoin(
        const css_computed_style *style);

uint8_t css_computed_stroke_miterlimit(
        const css_computed_style *style,
        css_fixed *length);

uint8_t css_computed_text_anchor(
        const css_computed_style *style);

uint8_t css_computed_text_rendering(const css_computed_style *style);

#ifdef __cplusplus
}
#endif

#endif
