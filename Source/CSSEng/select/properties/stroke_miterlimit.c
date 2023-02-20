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

css_error css__cascade_stroke_miterlimit(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_STROKE_MITERLIMIT_INHERIT;
	css_fixed stroke_miterlimit = 0;

	if (isInherit(opv) == false) {
		value = CSS_STROKE_MITERLIMIT_SET;

		stroke_miterlimit = *((css_fixed *) style->bytecode);
		advance_bytecode(style, sizeof(stroke_miterlimit));
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_stroke_miterlimit(state->computed, value, stroke_miterlimit);
	}

	return CSS_OK;
}

css_error css__set_stroke_miterlimit_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_stroke_miterlimit(style, hint->status, hint->data.fixed);
}

css_error css__initial_stroke_miterlimit(css_select_state *state)
{
	return set_stroke_miterlimit(state->computed, CSS_STROKE_MITERLIMIT_SET, INTTOFIX(4));
}

css_error css__compose_stroke_miterlimit(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed stroke_miterlimit = 0;
	uint8_t type = get_stroke_miterlimit(child, &stroke_miterlimit);

	if (type == CSS_STROKE_MITERLIMIT_INHERIT) {
		type = get_stroke_miterlimit(parent, &stroke_miterlimit);
	}

	return set_stroke_miterlimit(result, type, stroke_miterlimit);
}

