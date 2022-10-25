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

css_error css__cascade_break_before(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_break_after_before_inside(opv, style, state,
			set_break_before);
}

css_error css__set_break_before_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_break_before(style, hint->status);
}

css_error css__initial_break_before(css_select_state *state)
{
	return set_break_before(state->computed, CSS_BREAK_BEFORE_AUTO);
}

css_error css__compose_break_before(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_break_before(child);

	if (type == CSS_BREAK_BEFORE_INHERIT) {
		type = get_break_before(parent);
	}

	return set_break_before(result, type);
}

