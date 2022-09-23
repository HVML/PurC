/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Michael Drake <tlsa@netsurf-browser.org>
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_column_fill(uint32_t opv, css_style *style,
		css_select_state *state)
{
	uint16_t value = CSS_COLUMN_FILL_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case COLUMN_FILL_BALANCE:
			value = CSS_COLUMN_FILL_BALANCE;
			break;
		case COLUMN_FILL_AUTO:
			value = CSS_COLUMN_FILL_AUTO;
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_column_fill(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_column_fill_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_column_fill(style, hint->status);
}

css_error css__initial_column_fill(css_select_state *state)
{
	return set_column_fill(state->computed, CSS_COLUMN_FILL_BALANCE);
}

css_error css__compose_column_fill(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_column_fill(child);

	if (type == CSS_COLUMN_FILL_INHERIT) {
		type = get_column_fill(parent);
	}

	return set_column_fill(result, type);
}

