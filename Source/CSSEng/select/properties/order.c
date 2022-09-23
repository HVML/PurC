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

css_error css__cascade_order(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_ORDER_INHERIT;
	css_fixed order = 0;

	if (isInherit(opv) == false) {
		value = CSS_ORDER_SET;

		order = FIXTOINT(*((css_fixed *) style->bytecode));
		advance_bytecode(style, sizeof(order));
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_order(state->computed, value, order);
	}

	return CSS_OK;
}

css_error css__set_order_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_order(style, hint->status, hint->data.integer);
}

css_error css__initial_order(css_select_state *state)
{
	return set_order(state->computed, CSS_ORDER_SET, 0);
}

css_error css__compose_order(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	int32_t order = 0;
	uint8_t type = get_order(child, &order);

	if (type == CSS_ORDER_INHERIT) {
		type = get_order(parent, &order);
	}

	return set_order(result, type, order);
}

