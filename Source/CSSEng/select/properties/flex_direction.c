/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2017 Lucas Neves <lcneves@gmail.com>
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_flex_direction(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FLEX_DIRECTION_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case FLEX_DIRECTION_ROW:
			value = CSS_FLEX_DIRECTION_ROW;
			break;
		case FLEX_DIRECTION_ROW_REVERSE:
			value = CSS_FLEX_DIRECTION_ROW_REVERSE;
			break;
		case FLEX_DIRECTION_COLUMN:
			value = CSS_FLEX_DIRECTION_COLUMN;
			break;
		case FLEX_DIRECTION_COLUMN_REVERSE:
			value = CSS_FLEX_DIRECTION_COLUMN_REVERSE;
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_flex_direction(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_flex_direction_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_flex_direction(style, hint->status);
}

css_error css__initial_flex_direction(css_select_state *state)
{
	return set_flex_direction(state->computed, CSS_FLEX_DIRECTION_ROW);
}

css_error css__compose_flex_direction(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_flex_direction(child);

	if (type == CSS_FLEX_DIRECTION_INHERIT) {
		type = get_flex_direction(parent);
	}

	return set_flex_direction(result, type);
}

