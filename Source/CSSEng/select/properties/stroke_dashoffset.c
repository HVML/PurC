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

css_error css__cascade_stroke_dashoffset(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_length_auto(opv, style, state, set_stroke_dashoffset);
}

css_error css__set_stroke_dashoffset_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_stroke_dashoffset(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error css__initial_stroke_dashoffset(css_select_state *state)
{
	return set_stroke_dashoffset(state->computed, CSS_STROKE_DASHOFFSET_AUTO, 0, CSS_UNIT_PX);
}

css_error css__compose_stroke_dashoffset(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_stroke_dashoffset(child, &length, &unit);

	if (type == CSS_STROKE_DASHOFFSET_INHERIT) {
		type = get_stroke_dashoffset(parent, &length, &unit);
	}

	return set_stroke_dashoffset(result, type, length, unit);
}

