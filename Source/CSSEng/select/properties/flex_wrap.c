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

css_error css__cascade_flex_wrap(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FLEX_WRAP_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case FLEX_WRAP_NOWRAP:
			value = CSS_FLEX_WRAP_NOWRAP;
			break;
		case FLEX_WRAP_WRAP:
			value = CSS_FLEX_WRAP_WRAP;
			break;
		case FLEX_WRAP_WRAP_REVERSE:
			value = CSS_FLEX_WRAP_WRAP_REVERSE;
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_flex_wrap(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_flex_wrap_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_flex_wrap(style, hint->status);
}

css_error css__initial_flex_wrap(css_select_state *state)
{
	return set_flex_wrap(state->computed, CSS_FLEX_WRAP_NOWRAP);
}

css_error css__compose_flex_wrap(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_flex_wrap(child);

	if (type == CSS_FLEX_WRAP_INHERIT) {
		type = get_flex_wrap(parent);
	}

	return set_flex_wrap(result, type);
}

