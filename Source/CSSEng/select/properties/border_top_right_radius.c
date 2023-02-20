/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright (C) 2021 ~ 2023 Beijing FMSoft Technologies Co., Ltd.
 */


#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_border_top_right_radius(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_length_auto(opv, style, state, set_border_top_right_radius);
}

css_error css__set_border_top_right_radius_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_border_top_right_radius(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error css__initial_border_top_right_radius(css_select_state *state)
{
	return set_border_top_right_radius(state->computed, CSS_BORDER_TOP_RIGHT_RADIUS_AUTO, 0, CSS_UNIT_PX);
}

css_error css__compose_border_top_right_radius(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_border_top_right_radius(child, &length, &unit);

	if (type == CSS_BORDER_TOP_RIGHT_RADIUS_INHERIT) {
		type = get_border_top_right_radius(parent, &length, &unit);
	}

	return set_border_top_right_radius(result, type, length, unit);
}

