/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_overflow_y(uint32_t opv, css_style *style,
		css_select_state *state)
{
	uint16_t value = CSS_OVERFLOW_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case OVERFLOW_VISIBLE:
			value = CSS_OVERFLOW_VISIBLE;
			break;
		case OVERFLOW_HIDDEN:
			value = CSS_OVERFLOW_HIDDEN;
			break;
		case OVERFLOW_SCROLL:
			value = CSS_OVERFLOW_SCROLL;
			break;
		case OVERFLOW_AUTO:
			value = CSS_OVERFLOW_AUTO;
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_overflow_y(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_overflow_y_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_overflow_y(style, hint->status);
}

css_error css__initial_overflow_y(css_select_state *state)
{
	return set_overflow_y(state->computed, CSS_OVERFLOW_VISIBLE);
}

css_error css__compose_overflow_y(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_overflow_y(child);

	if (type == CSS_OVERFLOW_INHERIT) {
		type = get_overflow_y(parent);
	}

	return set_overflow_y(result, type);
}

