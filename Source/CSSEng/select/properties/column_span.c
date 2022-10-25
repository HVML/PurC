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

css_error css__cascade_column_span(uint32_t opv, css_style *style,
		css_select_state *state)
{
	uint16_t value = CSS_COLUMN_SPAN_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case COLUMN_SPAN_NONE:
			value = CSS_COLUMN_SPAN_NONE;
			break;
		case COLUMN_SPAN_ALL:
			value = CSS_COLUMN_SPAN_ALL;
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_column_span(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_column_span_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_column_span(style, hint->status);
}

css_error css__initial_column_span(css_select_state *state)
{
	return set_column_span(state->computed, CSS_COLUMN_SPAN_NONE);
}

css_error css__compose_column_span(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_column_span(child);

	if (type == CSS_COLUMN_SPAN_INHERIT) {
		type = get_column_span(parent);
	}

	return set_column_span(result, type);
}

