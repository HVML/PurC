/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright (C) 2021 ~ 2023 Beijing FMSoft Technologies Co., Ltd.
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_grid_column_start(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_length_auto(opv, style, state, set_grid_column_start);
}

css_error css__set_grid_column_start_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_grid_column_start(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error css__initial_grid_column_start(css_select_state *state)
{
	return set_grid_column_start(state->computed, CSS_WIDTH_AUTO, 0, CSS_UNIT_PX);
}

css_error css__compose_grid_column_start(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_grid_column_start(child, &length, &unit);

	if (type == CSS_WIDTH_INHERIT) {
		type = get_grid_column_start(parent, &length, &unit);
	}

	return set_grid_column_start(result, type, length, unit);
}
