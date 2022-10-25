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

css_error css__cascade_justify_content(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_JUSTIFY_CONTENT_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case JUSTIFY_CONTENT_FLEX_START:
			value = CSS_JUSTIFY_CONTENT_FLEX_START;
			break;
		case JUSTIFY_CONTENT_FLEX_END:
			value = CSS_JUSTIFY_CONTENT_FLEX_END;
			break;
		case JUSTIFY_CONTENT_CENTER:
			value = CSS_JUSTIFY_CONTENT_CENTER;
			break;
		case JUSTIFY_CONTENT_SPACE_BETWEEN:
			value = CSS_JUSTIFY_CONTENT_SPACE_BETWEEN;
			break;
		case JUSTIFY_CONTENT_SPACE_AROUND:
			value = CSS_JUSTIFY_CONTENT_SPACE_AROUND;
			break;
		case JUSTIFY_CONTENT_SPACE_EVENLY:
			value = CSS_JUSTIFY_CONTENT_SPACE_EVENLY;
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_justify_content(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_justify_content_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_justify_content(style, hint->status);
}

css_error css__initial_justify_content(css_select_state *state)
{
	return set_justify_content(state->computed,
			CSS_JUSTIFY_CONTENT_FLEX_START);
}

css_error css__compose_justify_content(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_justify_content(child);

	if (type == CSS_JUSTIFY_CONTENT_INHERIT) {
		type = get_justify_content(parent);
	}

	return set_justify_content(result, type);
}

