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

css_error css__cascade_stroke_opacity(uint32_t opv, css_style *style,
		css_select_state *state)
{
	uint16_t value = CSS_STROKE_OPACITY_INHERIT;
	css_fixed opacity = 0;

	if (isInherit(opv) == false) {
		value = CSS_STROKE_OPACITY_SET;

		opacity = *((css_fixed *) style->bytecode);
		advance_bytecode(style, sizeof(opacity));
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_stroke_opacity(state->computed, value, opacity);
	}

	return CSS_OK;
}

css_error css__set_stroke_opacity_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_stroke_opacity(style, hint->status, hint->data.fixed);
}

css_error css__initial_stroke_opacity(css_select_state *state)
{
	return set_stroke_opacity(state->computed, CSS_STROKE_OPACITY_SET, INTTOFIX(1));
}

css_error css__compose_stroke_opacity(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed opacity = 0;
	uint8_t type = get_stroke_opacity(child, &opacity);

	if (type == CSS_STROKE_OPACITY_INHERIT) {
		type = get_stroke_opacity(parent, &opacity);
	}

	return set_stroke_opacity(result, type, opacity);
}

