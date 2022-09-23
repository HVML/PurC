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

css_error css__cascade_align_self(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_ALIGN_SELF_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case ALIGN_SELF_STRETCH:
			value = CSS_ALIGN_SELF_STRETCH;
			break;
		case ALIGN_SELF_FLEX_START:
			value = CSS_ALIGN_SELF_FLEX_START;
			break;
		case ALIGN_SELF_FLEX_END:
			value = CSS_ALIGN_SELF_FLEX_END;
			break;
		case ALIGN_SELF_CENTER:
			value = CSS_ALIGN_SELF_CENTER;
			break;
		case ALIGN_SELF_BASELINE:
			value = CSS_ALIGN_SELF_BASELINE;
			break;
		case ALIGN_SELF_AUTO:
			value = CSS_ALIGN_SELF_AUTO;
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_align_self(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_align_self_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_align_self(style, hint->status);
}

css_error css__initial_align_self(css_select_state *state)
{
	return set_align_self(state->computed, CSS_ALIGN_SELF_AUTO);
}

css_error css__compose_align_self(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_align_self(child);

	if (type == CSS_ALIGN_SELF_INHERIT) {
		type = get_align_self(parent);
	}

	return set_align_self(result, type);
}

