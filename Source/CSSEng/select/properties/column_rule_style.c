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

css_error css__cascade_column_rule_style(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_border_style(opv, style, state,
			set_column_rule_style);
}

css_error css__set_column_rule_style_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_column_rule_style(style, hint->status);
}

css_error css__initial_column_rule_style(css_select_state *state)
{
	return set_column_rule_style(state->computed,
			CSS_COLUMN_RULE_STYLE_NONE);
}

css_error css__compose_column_rule_style(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_column_rule_style(child);

	if (type == CSS_COLUMN_RULE_STYLE_INHERIT) {
		type = get_column_rule_style(parent);
	}

	return set_column_rule_style(result, type);
}

