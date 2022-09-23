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

css_error css__cascade_flex_basis(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FLEX_BASIS_INHERIT;
	css_fixed length = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case FLEX_BASIS_AUTO:
			value = CSS_FLEX_BASIS_AUTO;
			break;
		case FLEX_BASIS_CONTENT:
			value = CSS_FLEX_BASIS_CONTENT;
			break;
		case FLEX_BASIS_SET:
			value = CSS_FLEX_BASIS_SET;
			length = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(length));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		}
	}

	unit = css__to_css_unit(unit);

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_flex_basis(state->computed, value, length, unit);
	}

	return CSS_OK;
}

css_error css__set_flex_basis_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_flex_basis(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error css__initial_flex_basis(css_select_state *state)
{
	return set_flex_basis(state->computed, CSS_FLEX_BASIS_AUTO, 0,
			CSS_UNIT_PX);
}

css_error css__compose_flex_basis(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_flex_basis(child, &length, &unit);

	if (type == CSS_FLEX_BASIS_INHERIT) {
		type = get_flex_basis(parent, &length, &unit);
	}

	return set_flex_basis(result, type, length, unit);
}

