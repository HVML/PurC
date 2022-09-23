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

css_error css__cascade_column_rule_color(uint32_t opv, css_style *style,
		css_select_state *state)
{
	bool inherit = isInherit(opv);
	uint16_t value = CSS_COLUMN_RULE_COLOR_INHERIT;
	css_color color = 0;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case COLUMN_RULE_COLOR_TRANSPARENT:
			value = CSS_COLUMN_RULE_COLOR_COLOR;
			break;
		case COLUMN_RULE_COLOR_CURRENT_COLOR:
			value = CSS_COLUMN_RULE_COLOR_CURRENT_COLOR;
			break;
		case COLUMN_RULE_COLOR_SET:
			value = CSS_COLUMN_RULE_COLOR_COLOR;
			color = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(color));
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			inherit)) {
		return set_column_rule_color(state->computed, value, color);
	}

	return CSS_OK;
}

css_error css__set_column_rule_color_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_column_rule_color(style, hint->status, hint->data.color);
}

css_error css__initial_column_rule_color(css_select_state *state)
{
	return set_column_rule_color(state->computed,
			CSS_COLUMN_RULE_COLOR_CURRENT_COLOR, 0);
}

css_error css__compose_column_rule_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color;
	uint8_t type = get_column_rule_color(child, &color);

	if (type == CSS_COLUMN_RULE_COLOR_INHERIT) {
		type = get_column_rule_color(parent, &color);
	}

	return set_column_rule_color(result, type, color);
}

