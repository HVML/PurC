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

css_error css__cascade_flex_grow(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FLEX_GROW_INHERIT;
	css_fixed flex_grow = 0;

	if (isInherit(opv) == false) {
		value = CSS_FLEX_GROW_SET;

		flex_grow = *((css_fixed *) style->bytecode);
		advance_bytecode(style, sizeof(flex_grow));
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_flex_grow(state->computed, value, flex_grow);
	}

	return CSS_OK;
}

css_error css__set_flex_grow_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_flex_grow(style, hint->status, hint->data.fixed);
}

css_error css__initial_flex_grow(css_select_state *state)
{
	return set_flex_grow(state->computed, CSS_FLEX_GROW_SET, INTTOFIX(0));
}

css_error css__compose_flex_grow(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed flex_grow = 0;
	uint8_t type = get_flex_grow(child, &flex_grow);

	if (type == CSS_FLEX_GROW_INHERIT) {
		type = get_flex_grow(parent, &flex_grow);
	}

	return set_flex_grow(result, type, flex_grow);
}

